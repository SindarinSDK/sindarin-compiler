/*
 * POSIX threads compatibility layer for Windows
 * Provides pthread-like API using Windows threading primitives
 */

#ifndef SN_COMPAT_PTHREAD_H
#define SN_COMPAT_PTHREAD_H

/* Only needed for MSVC/clang-cl on Windows, not MinGW (which provides pthreads)
 * However, SN_USE_WIN32_THREADS forces use of Windows API even with MinGW
 * for creating standalone packages that don't require pthread DLLs */
#if defined(_WIN32) && (!defined(__MINGW32__) && !defined(__MINGW64__) || defined(SN_USE_WIN32_THREADS))

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
    #define NOGDI  /* Exclude GDI to avoid conflicts with user-defined Rectangle, etc. */
#endif
#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <stdlib.h>  /* For malloc/free used in pthread_create */
#include <errno.h>
#include <time.h>

/* ============================================================================
 * struct timespec compatibility
 * MSVC 2015+ (VS2015 = _MSC_VER 1900) defines struct timespec in <time.h>
 * MinGW also provides these, so skip when __MINGW32__/__MINGW64__ is defined
 * ============================================================================ */
#if defined(_MSC_VER) && _MSC_VER < 1900
#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
    time_t tv_sec;   /* seconds */
    long   tv_nsec;  /* nanoseconds */
};
#endif
#endif

/* TIME_UTC for timespec_get - MSVC 2015+ and MinGW define this */
#ifndef TIME_UTC
#define TIME_UTC 1
#endif

/* timespec_get implementation for older MSVC versions
 * MinGW provides this in its libc, so skip when __MINGW32__/__MINGW64__ */
#if defined(_MSC_VER) && _MSC_VER < 1900
static inline int timespec_get(struct timespec *ts, int base) {
    if (base != TIME_UTC || ts == NULL) return 0;
    /* Get time as 100-nanosecond intervals since January 1, 1601 */
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    /* Convert to Unix epoch */
    static const uint64_t EPOCH_DIFF = 116444736000000000ULL;
    time -= EPOCH_DIFF;
    ts->tv_sec = (time_t)(time / 10000000ULL);
    ts->tv_nsec = (long)((time % 10000000ULL) * 100);
    return base;
}
#endif

/* Define POSIX error codes that may be missing on Windows */
#ifndef ENOMEM
    #define ENOMEM 12
#endif
#ifndef EINVAL
    #define EINVAL 22
#endif
#ifndef ESRCH
    #define ESRCH 3
#endif
#ifndef EBUSY
    #define EBUSY 16
#endif
#ifndef EAGAIN
    #define EAGAIN 11
#endif
#ifndef ETIMEDOUT
    #define ETIMEDOUT 110
#endif

/* ============================================================================
 * Thread types
 * ============================================================================ */

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

/* Thread attributes (simplified - not fully implemented) */
typedef struct {
    int detach_state;
} pthread_attr_t;

/* Mutex attributes (simplified) */
typedef struct {
    int type;
} pthread_mutexattr_t;

/* Condition variable attributes (simplified) */
typedef struct {
    int dummy;
} pthread_condattr_t;

/* ============================================================================
 * Constants
 * ============================================================================ */

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_COND_INITIALIZER {0}

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_RECURSIVE  1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

/* ============================================================================
 * Thread functions
 * ============================================================================ */

/* Thread start routine wrapper */
typedef void *(*pthread_start_routine)(void *);

/* Internal wrapper structure for thread start */
typedef struct {
    pthread_start_routine start_routine;
    void *arg;
} _pthread_start_data;

/* Internal thread start wrapper */
static inline unsigned __stdcall _pthread_start_wrapper(void *data) {
    _pthread_start_data *start_data = (_pthread_start_data *)data;
    pthread_start_routine routine = start_data->start_routine;
    void *arg = start_data->arg;
    free(start_data);
    routine(arg);
    return 0;
}

/* pthread_create - create a new thread */
static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                  void *(*start_routine)(void *), void *arg) {
    (void)attr;  /* Attributes not fully supported */

    _pthread_start_data *data = (_pthread_start_data *)malloc(sizeof(_pthread_start_data));
    if (data == NULL) return ENOMEM;

    data->start_routine = start_routine;
    data->arg = arg;

    uintptr_t handle = _beginthreadex(NULL, 0, _pthread_start_wrapper, data, 0, NULL);
    if (handle == 0) {
        free(data);
        return errno;
    }

    *thread = (HANDLE)handle;
    return 0;
}

/* pthread_join - wait for thread termination */
static inline int pthread_join(pthread_t thread, void **retval) {
    (void)retval;  /* Return value not supported in this simple implementation */
    DWORD result = WaitForSingleObject(thread, INFINITE);
    if (result == WAIT_OBJECT_0) {
        CloseHandle(thread);
        return 0;
    }
    return EINVAL;
}

/* pthread_detach - detach a thread */
static inline int pthread_detach(pthread_t thread) {
    return CloseHandle(thread) ? 0 : EINVAL;
}

/* pthread_self - get current thread ID */
static inline pthread_t pthread_self(void) {
    return GetCurrentThread();
}

/* pthread_equal - compare thread IDs */
static inline int pthread_equal(pthread_t t1, pthread_t t2) {
    return GetThreadId(t1) == GetThreadId(t2);
}

/* pthread_exit - terminate calling thread */
static inline void pthread_exit(void *retval) {
    (void)retval;
    _endthreadex(0);
}

/* pthread_cancel - request cancellation of a thread
 * Note: Windows doesn't have a direct equivalent to pthread_cancel.
 * TerminateThread is dangerous and not recommended.
 * This is a best-effort implementation.
 */
static inline int pthread_cancel(pthread_t thread) {
    /* TerminateThread is dangerous but is the only option on Windows */
    return TerminateThread(thread, (DWORD)-1) ? 0 : ESRCH;
}

/* ============================================================================
 * Thread attributes
 * ============================================================================ */

static inline int pthread_attr_init(pthread_attr_t *attr) {
    attr->detach_state = PTHREAD_CREATE_JOINABLE;
    return 0;
}

static inline int pthread_attr_destroy(pthread_attr_t *attr) {
    (void)attr;
    return 0;
}

static inline int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    attr->detach_state = detachstate;
    return 0;
}

static inline int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    *detachstate = attr->detach_state;
    return 0;
}

/* ============================================================================
 * Mutex functions
 * ============================================================================
 * Note: Windows CRITICAL_SECTION cannot be statically initialized like POSIX
 * mutexes. We use INIT_ONCE to handle lazy initialization for mutexes that
 * use PTHREAD_MUTEX_INITIALIZER.
 */

/* Helper for lazy initialization of statically initialized mutexes */
static inline void _pthread_mutex_ensure_init(pthread_mutex_t *mutex) {
    /* Check if the CRITICAL_SECTION is initialized by checking DebugInfo
     * An uninitialized critical section will have all zeros. */
    if (mutex->DebugInfo == NULL) {
        InitializeCriticalSection(mutex);
    }
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (mutex->DebugInfo != NULL) {
        DeleteCriticalSection(mutex);
    }
    return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex) {
    _pthread_mutex_ensure_init(mutex);
    EnterCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    _pthread_mutex_ensure_init(mutex);
    return TryEnterCriticalSection(mutex) ? 0 : EBUSY;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

/* ============================================================================
 * Mutex attributes
 * ============================================================================ */

static inline int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    attr->type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

static inline int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    (void)attr;
    return 0;
}

static inline int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    attr->type = type;
    return 0;
}

static inline int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
    *type = attr->type;
    return 0;
}

/* ============================================================================
 * Condition variable functions
 * ============================================================================ */

static inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    (void)attr;
    InitializeConditionVariable(cond);
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *cond) {
    (void)cond;
    /* Windows condition variables don't need destruction */
    return 0;
}

static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    return SleepConditionVariableCS(cond, mutex, INFINITE) ? 0 : EINVAL;
}

static inline int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                          const struct timespec *abstime) {
    /* Calculate timeout in milliseconds from absolute time */
    struct timespec now;
    timespec_get(&now, TIME_UTC);

    int64_t timeout_ms = (abstime->tv_sec - now.tv_sec) * 1000 +
                         (abstime->tv_nsec - now.tv_nsec) / 1000000;
    if (timeout_ms < 0) timeout_ms = 0;

    return SleepConditionVariableCS(cond, mutex, (DWORD)timeout_ms) ? 0 : ETIMEDOUT;
}

static inline int pthread_cond_signal(pthread_cond_t *cond) {
    WakeConditionVariable(cond);
    return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t *cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

/* ============================================================================
 * Thread-local storage
 * ============================================================================ */

typedef DWORD pthread_key_t;

static inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    (void)destructor;  /* Destructors not supported in this simple implementation */
    DWORD tls_index = TlsAlloc();
    if (tls_index == TLS_OUT_OF_INDEXES) {
        return EAGAIN;
    }
    *key = tls_index;
    return 0;
}

static inline int pthread_key_delete(pthread_key_t key) {
    return TlsFree(key) ? 0 : EINVAL;
}

static inline void *pthread_getspecific(pthread_key_t key) {
    return TlsGetValue(key);
}

static inline int pthread_setspecific(pthread_key_t key, const void *value) {
    return TlsSetValue(key, (LPVOID)value) ? 0 : EINVAL;
}

/* ============================================================================
 * Once control
 * ============================================================================ */

typedef INIT_ONCE pthread_once_t;
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

typedef struct {
    void (*init_routine)(void);
} _pthread_once_data;

static inline BOOL CALLBACK _pthread_once_callback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
    (void)InitOnce;
    (void)Context;
    _pthread_once_data *data = (_pthread_once_data *)Parameter;
    data->init_routine();
    return TRUE;
}

static inline int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    _pthread_once_data data = { init_routine };
    return InitOnceExecuteOnce(once_control, _pthread_once_callback, &data, NULL) ? 0 : EINVAL;
}

#endif /* _WIN32 */

#endif /* SN_COMPAT_PTHREAD_H */
