#include "arena_safepoint.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

_Atomic bool rt_gc_safepoint_requested = false;

/* Race detection counter: incremented when a thread registers during active STW */
volatile int rt_safepoint_race_count = 0;

/* Per-thread flag: is this thread registered with the safepoint subsystem? */
static _Thread_local bool g_sp_thread_registered = false;

static struct {
    pthread_mutex_t mutex;
    pthread_cond_t  all_parked;   /* GC waits on this */
    pthread_cond_t  gc_done;      /* Parked threads wait on this */
    int thread_count;             /* Registered threads (protected by mutex) */
    int parked_count;             /* Currently parked (protected by mutex) */
    int gc_epoch;                 /* Incremented each GC cycle */
} g_sp;

void rt_safepoint_init(void) {
    pthread_mutex_init(&g_sp.mutex, NULL);
    pthread_cond_init(&g_sp.all_parked, NULL);
    pthread_cond_init(&g_sp.gc_done, NULL);
    g_sp.thread_count = 0;
    g_sp.parked_count = 0;
    g_sp.gc_epoch = 0;
}

void rt_safepoint_thread_register(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.thread_count++;
    g_sp_thread_registered = true;
    /* Race detection: if STW is active when a new thread registers,
     * it means this thread will run unparked during GC — a data race.
     * The GC already counted threads and started; this new thread wasn't
     * included and won't be waited on. */
    if (atomic_load_explicit(&rt_gc_safepoint_requested, memory_order_acquire)) {
        __sync_add_and_fetch(&rt_safepoint_race_count, 1);
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_pre_register_thread(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.thread_count++;
    g_sp.parked_count++;
    /* If GC is waiting for all threads to park, signal it —
     * this new "thread" is already parked from GC's perspective. */
    if (rt_gc_safepoint_requested &&
        g_sp.parked_count >= g_sp.thread_count - (g_sp_thread_registered ? 1 : 0)) {
        pthread_cond_signal(&g_sp.all_parked);
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_adopt_registration(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp_thread_registered = true;
    g_sp.parked_count--;  /* Undo the implicit park from pre_register */
    /* If STW is active, we must wait for GC to finish before running */
    if (rt_gc_safepoint_requested) {
        g_sp.parked_count++;
        int my_epoch = g_sp.gc_epoch;
        while (g_sp.gc_epoch == my_epoch && rt_gc_safepoint_requested) {
            pthread_cond_wait(&g_sp.gc_done, &g_sp.mutex);
        }
        g_sp.parked_count--;
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_thread_deregister(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.thread_count--;
    g_sp_thread_registered = false;
    /* If GC is waiting and this was the last thread it needed,
     * re-check using the same logic as request_stw. The GC caller
     * may or may not be registered, so we conservatively signal. */
    if (rt_gc_safepoint_requested) {
        pthread_cond_signal(&g_sp.all_parked);
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_park(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.parked_count++;
    /* Signal GC if all mutator threads are now parked */
    if (g_sp.parked_count >= g_sp.thread_count - 1) {
        pthread_cond_signal(&g_sp.all_parked);
    }
    /* Wait until GC completes (epoch changes) */
    int my_epoch = g_sp.gc_epoch;
    while (g_sp.gc_epoch == my_epoch && rt_gc_safepoint_requested) {
        pthread_cond_wait(&g_sp.gc_done, &g_sp.mutex);
    }
    g_sp.parked_count--;
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_poll(void) {
    if (__builtin_expect(!atomic_load_explicit(&rt_gc_safepoint_requested, memory_order_relaxed), 1))
        return;
    rt_safepoint_park();
}

void rt_safepoint_request_stw(void) {
    pthread_mutex_lock(&g_sp.mutex);
    atomic_store_explicit(&rt_gc_safepoint_requested, true, memory_order_release);
    /* Wait for all other registered threads to reach safepoints.
     * If the calling thread is registered (worker), exclude it (-1).
     * If the calling thread is NOT registered (main), wait for all. */
    int exclude_self = g_sp_thread_registered ? 1 : 0;
    while (g_sp.parked_count < g_sp.thread_count - exclude_self) {
        pthread_cond_wait(&g_sp.all_parked, &g_sp.mutex);
    }
    pthread_mutex_unlock(&g_sp.mutex);
    /* All mutators parked — safe to collect */
}

void rt_safepoint_release_stw(void) {
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.gc_epoch++;
    atomic_store_explicit(&rt_gc_safepoint_requested, false, memory_order_release);
    pthread_cond_broadcast(&g_sp.gc_done);
    pthread_mutex_unlock(&g_sp.mutex);
}

int rt_safepoint_thread_count(void) {
    pthread_mutex_lock(&g_sp.mutex);
    int count = g_sp.thread_count;
    pthread_mutex_unlock(&g_sp.mutex);
    return count;
}

void rt_safepoint_enter_native(void) {
    /* Treat this thread as parked from GC's perspective.
     * If STW is already requested, this immediately helps satisfy
     * the park count. If not, it pre-parks so future STW won't wait. */
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.parked_count++;
    if (rt_gc_safepoint_requested &&
        g_sp.parked_count >= g_sp.thread_count - 1) {
        pthread_cond_signal(&g_sp.all_parked);
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

void rt_safepoint_leave_native(void) {
    /* Returning from native code — unpark and wait if GC is active. */
    pthread_mutex_lock(&g_sp.mutex);
    g_sp.parked_count--;
    /* If STW is active, we must wait for GC to finish before resuming */
    if (rt_gc_safepoint_requested) {
        g_sp.parked_count++;
        int my_epoch = g_sp.gc_epoch;
        while (g_sp.gc_epoch == my_epoch && rt_gc_safepoint_requested) {
            pthread_cond_wait(&g_sp.gc_done, &g_sp.mutex);
        }
        g_sp.parked_count--;
    }
    pthread_mutex_unlock(&g_sp.mutex);
}

int rt_safepoint_get_race_count(void) {
    return __sync_add_and_fetch(&rt_safepoint_race_count, 0);
}
