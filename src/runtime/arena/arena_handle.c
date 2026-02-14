/*
 * Arena Handle - Handle Operations and Transactions Implementation
 * =================================================================
 */

#include "arena_handle.h"
#include "arena_v2.h"
#include "arena_id.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>

/* ============================================================================
 * Handle Operations
 * ============================================================================ */

RtArenaV2 *rt_handle_v2_arena(RtHandleV2 *handle)
{
    return handle ? handle->arena : NULL;
}

bool rt_handle_v2_is_valid(RtHandleV2 *handle)
{
    return handle != NULL && !(handle->flags & RT_HANDLE_FLAG_DEAD);
}

/* ============================================================================
 * Handle List Management
 * ============================================================================ */

void rt_handle_v2_link(RtBlockV2 *block, RtHandleV2 *handle)
{
    /* Add to head of block's handle list */
    handle->next = block->handles_head;
    handle->prev = NULL;
    if (block->handles_head) {
        block->handles_head->prev = handle;
    }
    block->handles_head = handle;
}

void rt_handle_v2_unlink(RtBlockV2 *block, RtHandleV2 *handle)
{
    /* Remove from block's handle list */
    if (handle->prev) {
        handle->prev->next = handle->next;
    } else {
        block->handles_head = handle->next;
    }
    if (handle->next) {
        handle->next->prev = handle->prev;
    }
    handle->prev = NULL;
    handle->next = NULL;
}

/* ============================================================================
 * Handle Transactions
 * ============================================================================ */

/* Get monotonic time in nanoseconds */
uint64_t rt_get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns)
{
    if (handle == NULL || handle->block == NULL) return;

    RtBlockV2 *block = handle->block;
    uint64_t my_id = rt_arena_get_thread_id();

    /* Check if we already hold this block */
    uint64_t current_holder = atomic_load(&block->tx_holder);
    if (current_holder == my_id) {
        /* Same thread - increment nesting count */
        atomic_fetch_add(&block->tx_recurse_count, 1);
        return;
    }

    /* Try to acquire the block */
    while (1) {
        uint64_t expected = 0;
        if (atomic_compare_exchange_weak(&block->tx_holder, &expected, my_id)) {
            /* Successfully acquired - set start time and timeout */
            atomic_store(&block->tx_start_ns, rt_get_monotonic_ns());
            atomic_store(&block->tx_timeout_ns, timeout_ns);
            atomic_store(&block->tx_recurse_count, 1);
            return;
        }

        /* Block is held by someone else - spin and retry
         * In the future, we could add backoff or yield here */
    }
}

void rt_handle_begin_transaction(RtHandleV2 *handle)
{
    rt_handle_begin_transaction_with_timeout(handle, TX_DEFAULT_TIMEOUT_NS);
}

void rt_handle_end_transaction(RtHandleV2 *handle)
{
    if (handle == NULL || handle->block == NULL) return;

    RtBlockV2 *block = handle->block;
    uint64_t my_id = rt_arena_get_thread_id();

    /* Verify we hold the lease */
    uint64_t holder = atomic_load(&block->tx_holder);
    if (holder != my_id) {
        fprintf(stderr, "PANIC: rt_handle_end_transaction called but thread %lu does not hold lease (holder=%lu)\n",
                (unsigned long)my_id, (unsigned long)holder);
        abort();
    }

    /* Decrement nesting count */
    uint32_t count = atomic_fetch_sub(&block->tx_recurse_count, 1);
    if (count == 1) {
        /* Last nested transaction - release the block */
        atomic_store(&block->tx_holder, 0);
    }
}

void rt_handle_renew_transaction(RtHandleV2 *handle)
{
    if (handle == NULL || handle->block == NULL) return;

    RtBlockV2 *block = handle->block;
    uint64_t my_id = rt_arena_get_thread_id();

    /* Verify we hold the lease */
    uint64_t holder = atomic_load(&block->tx_holder);
    if (holder != my_id) {
        fprintf(stderr, "PANIC: rt_handle_renew_transaction called but thread %lu does not hold lease (holder=%lu)\n",
                (unsigned long)my_id, (unsigned long)holder);
        abort();
    }

    /* Reset start time */
    atomic_store(&block->tx_start_ns, rt_get_monotonic_ns());
}
