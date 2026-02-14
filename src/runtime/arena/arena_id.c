/*
 * Arena ID - Thread Identification Implementation
 * ================================================
 * Provides unique thread identification for the arena transaction system.
 * Each thread gets a unique ID used for block-level locking during
 * handle access and GC synchronization.
 *
 * Main thread gets ID lazily on first rt_arena_get_thread_id() call.
 * Worker threads get ID assigned at creation time in the thread module.
 */

#include "arena_id.h"
#include <stdatomic.h>

/* ============================================================================
 * Thread ID System
 * ============================================================================
 * Global atomic counter ensures unique IDs. Starts at 1 so ID 0 means "no holder".
 * TLS variable stores the current thread's ID, lazy-initialized on first access.
 * ============================================================================ */

static _Atomic uint64_t g_thread_id_counter = 1;

/* Thread-local storage compatibility */
#ifdef __TINYC__
    #define RT_THREAD_LOCAL
#elif defined(_MSC_VER)
    #define RT_THREAD_LOCAL __declspec(thread)
#else
    #define RT_THREAD_LOCAL __thread
#endif

RT_THREAD_LOCAL uint64_t rt_current_thread_id = 0;

uint64_t rt_arena_get_thread_id(void)
{
    if (rt_current_thread_id == 0) {
        /* First call on this thread (main thread case) - lazy init */
        rt_current_thread_id = atomic_fetch_add(&g_thread_id_counter, 1);
    }
    return rt_current_thread_id;
}

/* Allocate next thread ID - used by thread module when creating worker threads */
uint64_t rt_arena_alloc_thread_id(void)
{
    return atomic_fetch_add(&g_thread_id_counter, 1);
}

/* Set current thread's ID - used by thread module for worker threads */
void rt_arena_set_thread_id(uint64_t id)
{
    rt_current_thread_id = id;
}
