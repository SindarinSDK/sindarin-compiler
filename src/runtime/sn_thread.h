#ifndef SN_THREAD_H
#define SN_THREAD_H

#include <stdatomic.h>
#include "sn_core.h"

/* ---- Platform-specific pthread include ---- */

#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        #include <pthread.h>
    #else
        #include "compat_pthread.h"
    #endif
#else
    #include <pthread.h>
#endif

/* ---- Thread type ----
 *
 * Ownership:
 *   SnThread is co-owned by the caller (the scope that spawned it) and by
 *   the thread wrapper function. Refcount starts at 2. Both sides drop
 *   exactly one ref; whoever reaches 0 frees the struct and its result.
 *
 *   This eliminates the TOCTOU race on the `detached` flag: previously the
 *   wrapper read `detached` at exit and self-freed, while the caller wrote
 *   `detached` in a separate statement after pthread_create. If the wrapper
 *   finished before the caller's write, the struct was orphaned.
 */

typedef struct {
    pthread_t thread;
    void *result;
    size_t result_size;
    int joined;
    int detached;
    _Atomic int refcount;
} SnThread;

static inline void sn_thread_release(SnThread *t)
{
    if (!t) return;
    if (atomic_fetch_sub(&t->refcount, 1) == 1) {
        free(t->result);
        free(t);
    }
}

static inline void sn_cleanup_thread(SnThread **p)
{
    if (*p) {
        /* Only join if this ref still owns a joinable thread. Detached
         * threads are reaped by pthread; joined threads are already done. */
        if (!(*p)->joined && !(*p)->detached) {
            pthread_join((*p)->thread, NULL);
            (*p)->joined = 1;
        }
        sn_thread_release(*p);
    }
}

#define sn_auto_thread __attribute__((cleanup(sn_cleanup_thread)))

static inline SnThread *sn_thread_create(void)
{
    SnThread *t = sn_calloc(1, sizeof(SnThread));
    atomic_init(&t->refcount, 2);
    return t;
}

static inline void sn_thread_join(SnThread *t)
{
    if (t && !t->joined) {
        pthread_join(t->thread, NULL);
        t->joined = 1;
    }
}

#endif
