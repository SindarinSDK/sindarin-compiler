#ifndef SN_THREAD_H
#define SN_THREAD_H

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

/* ---- Thread type ---- */

typedef struct {
    pthread_t thread;
    void *result;
    size_t result_size;
    int joined;
} SnThread;

static inline void sn_cleanup_thread(SnThread **p)
{
    if (*p) {
        if (!(*p)->joined) {
            pthread_join((*p)->thread, NULL);
        }
        free((*p)->result);
        free(*p);
    }
}

#define sn_auto_thread __attribute__((cleanup(sn_cleanup_thread)))

static inline SnThread *sn_thread_create(void)
{
    SnThread *t = sn_calloc(1, sizeof(SnThread));
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
