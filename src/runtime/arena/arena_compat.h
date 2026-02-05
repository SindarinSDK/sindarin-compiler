#ifndef ARENA_COMPAT_H
#define ARENA_COMPAT_H

/*
 * Platform compatibility layer for the managed arena.
 * Self-contained — no dependencies on the rest of the project.
 *
 * Provides:
 *   - pthread (mutex, thread create/join) across all platforms
 *   - Portable rt_arena_sleep_ms() for timed waits
 *   - C11 atomics (required; checked at compile time)
 */

/* ============================================================================
 * C11 Atomics
 * ============================================================================
 * Required. Available in GCC 4.9+, Clang 3.1+, MSVC 2022 17.5+ (/std:c17).
 * ============================================================================ */
#include <stdatomic.h>

/* ============================================================================
 * Threading: pthread
 * ============================================================================ */
#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        /* MinGW provides native pthreads */
        #include <pthread.h>
    #elif defined(SN_COMPAT_PTHREAD_H)
        /* compat_pthread.h already included - use its definitions */
    #else
        /* MSVC / clang-cl: minimal pthread compat using Windows API */
        #define ARENA_COMPAT_PTHREAD_DEFINED 1
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #ifndef NOGDI
            #define NOGDI
        #endif
        #include <windows.h>
        #include <process.h>
        #include <stdlib.h>

        typedef HANDLE pthread_t;
        typedef CRITICAL_SECTION pthread_mutex_t;
        typedef CONDITION_VARIABLE pthread_cond_t;

        /* Thread start wrapper */
        typedef void *(*_arena_thread_fn)(void *);
        typedef struct { _arena_thread_fn fn; void *arg; } _arena_thread_data;

        static inline unsigned __stdcall _arena_thread_wrapper(void *data) {
            _arena_thread_data *td = (_arena_thread_data *)data;
            _arena_thread_fn fn = td->fn;
            void *arg = td->arg;
            free(td);
            fn(arg);
            return 0;
        }

        static inline int pthread_create(pthread_t *thread, const void *attr,
                                          void *(*start)(void *), void *arg) {
            (void)attr;
            _arena_thread_data *td = (_arena_thread_data *)malloc(sizeof(_arena_thread_data));
            if (!td) return -1;
            td->fn = start;
            td->arg = arg;
            uintptr_t h = _beginthreadex(NULL, 0, _arena_thread_wrapper, td, 0, NULL);
            if (h == 0) { free(td); return -1; }
            *thread = (HANDLE)h;
            return 0;
        }

        static inline int pthread_join(pthread_t thread, void **retval) {
            (void)retval;
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
            return 0;
        }

        static inline int pthread_mutex_init(pthread_mutex_t *m, const void *attr) {
            (void)attr;
            InitializeCriticalSection(m);
            return 0;
        }

        static inline int pthread_mutex_destroy(pthread_mutex_t *m) {
            DeleteCriticalSection(m);
            return 0;
        }

        static inline int pthread_mutex_lock(pthread_mutex_t *m) {
            EnterCriticalSection(m);
            return 0;
        }

        static inline int pthread_mutex_unlock(pthread_mutex_t *m) {
            LeaveCriticalSection(m);
            return 0;
        }

        static inline int pthread_cond_init(pthread_cond_t *cond, const void *attr) {
            (void)attr;
            InitializeConditionVariable(cond);
            return 0;
        }

        static inline int pthread_cond_destroy(pthread_cond_t *cond) {
            (void)cond;  /* Windows condition variables don't need explicit cleanup */
            return 0;
        }

        static inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
            SleepConditionVariableCS(cond, mutex, INFINITE);
            return 0;
        }

        static inline int pthread_cond_broadcast(pthread_cond_t *cond) {
            WakeAllConditionVariable(cond);
            return 0;
        }

        static inline int pthread_detach(pthread_t thread) {
            /* On Windows, just close the handle - the thread continues running */
            CloseHandle(thread);
            return 0;
        }
    #endif /* MinGW vs MSVC */
#else
    /* POSIX (Linux, macOS) */
    #include <pthread.h>
#endif /* _WIN32 */

/* ============================================================================
 * Portable Sleep
 * ============================================================================ */
#include <time.h>

static inline void rt_arena_sleep_ms(int ms)
{
#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        /* MinGW has nanosleep */
        struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
        nanosleep(&ts, NULL);
    #else
        Sleep((DWORD)ms);
    #endif
#else
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
#endif
}

/* ============================================================================
 * Portable Memory Mapping
 * ============================================================================
 * Uses mmap/munmap on POSIX, VirtualAlloc/VirtualFree on Windows.
 * Provides page-aligned memory without malloc heap fragmentation.
 * ============================================================================ */

#ifdef _WIN32
    #if !(defined(__MINGW32__) || defined(__MINGW64__)) || defined(SN_USE_WIN32_THREADS)
    /* Windows API already included above for threading */
    #else
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #endif

static inline void *rt_arena_mmap(size_t size)
{
    void *ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return ptr;
}

static inline void rt_arena_munmap(void *ptr, size_t size)
{
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#else /* POSIX */
#include <sys/mman.h>

static inline void *rt_arena_mmap(size_t size)
{
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
}

static inline void rt_arena_munmap(void *ptr, size_t size)
{
    munmap(ptr, size);
}

#endif /* _WIN32 */

#endif /* ARENA_COMPAT_H */
