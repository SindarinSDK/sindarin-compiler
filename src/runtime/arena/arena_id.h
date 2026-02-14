/*
 * Arena ID - Thread Identification for Arena Transactions
 * ========================================================
 * Provides unique thread identification for the arena transaction system.
 * Each thread gets a unique ID used for block-level locking during
 * handle access and GC synchronization.
 */

#ifndef ARENA_ID_H
#define ARENA_ID_H

#include <stdint.h>

/* Get unique thread ID for current thread.
 * Lazily initialized on first call for main thread.
 * Worker threads get ID assigned at creation time. */
uint64_t rt_arena_get_thread_id(void);

/* Allocate next thread ID - used by thread module when creating worker threads */
uint64_t rt_arena_alloc_thread_id(void);

/* Set current thread's ID - used by thread module for worker threads */
void rt_arena_set_thread_id(uint64_t id);

#endif /* ARENA_ID_H */
