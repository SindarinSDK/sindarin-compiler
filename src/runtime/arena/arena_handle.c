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

/* Claim a block: set start time, timeout, and recurse count */
static void tx_claim(RtBlockV2 *block, uint64_t timeout_ns)
{
    atomic_store(&block->tx_start_ns, rt_get_monotonic_ns());
    atomic_store(&block->tx_timeout_ns, timeout_ns);
    atomic_store(&block->tx_recurse_count, 1);
}

/* Try to force-acquire a block whose holder's lease has expired.
 * Returns true if we successfully took over. */
static bool tx_try_force_acquire(RtBlockV2 *block, uint64_t my_id, uint64_t timeout_ns)
{
    uint64_t start   = atomic_load(&block->tx_start_ns);
    uint64_t timeout = atomic_load(&block->tx_timeout_ns);
    if (start == 0 || timeout == 0) return false;

    uint64_t now = rt_get_monotonic_ns();
    if (now - start <= timeout) return false;

    uint64_t stale = atomic_load(&block->tx_holder);
    if (stale == 0 || stale == my_id) return false;
    if (!atomic_compare_exchange_strong(&block->tx_holder, &stale, my_id)) return false;

    fprintf(stderr, "WARNING: force-acquired expired tx on block %p "
            "(stale=%lu, recurse=%u, acquirer=%lu, held %llu ms)\n",
            (void *)block, (unsigned long)stale,
            (unsigned)atomic_load(&block->tx_recurse_count),
            (unsigned long)my_id,
            (unsigned long long)((now - start) / 1000000ULL));

    tx_claim(block, timeout_ns);
    return true;
}

void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns)
{
    if (handle == NULL || handle->block == NULL) return;

    RtBlockV2 *block = handle->block;
    uint64_t my_id = rt_arena_get_thread_id();

    /* Re-entrant: same thread already holds this block */
    if (atomic_load(&block->tx_holder) == my_id) {
        atomic_fetch_add(&block->tx_recurse_count, 1);
        return;
    }

    /* Spin to acquire */
    int spins = 0;
    while (1) {
        uint64_t expected = 0;
        if (atomic_compare_exchange_weak(&block->tx_holder, &expected, my_id)) {
            tx_claim(block, timeout_ns);
            return;
        }
        if (++spins >= 1000) {
            spins = 0;
            if (tx_try_force_acquire(block, my_id, timeout_ns)) return;
        }
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

/* ============================================================================
 * Copy Callback Management
 * ============================================================================ */

void rt_handle_set_copy_callback(RtHandleV2 *handle, RtHandleV2CopyCallback callback)
{
    if (handle == NULL) return;
    handle->copy_callback = callback;
}

RtHandleV2CopyCallback rt_handle_get_copy_callback(RtHandleV2 *handle)
{
    if (handle == NULL) return NULL;
    return handle->copy_callback;
}

void rt_handle_set_free_callback(RtHandleV2 *handle, RtHandleV2FreeCallback callback)
{
    if (handle == NULL) return;
    handle->free_callback = callback;
}

RtHandleV2FreeCallback rt_handle_get_free_callback(RtHandleV2 *handle)
{
    if (handle == NULL) return NULL;
    return handle->free_callback;
}
