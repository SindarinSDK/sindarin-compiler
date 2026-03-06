/*
 * Arena Handle - Handle Operations and Transactions Implementation
 * =================================================================
 */

#include "arena_handle.h"
#include "arena_v2.h"
#include "arena_id.h"

#include <stdio.h>
#include <stdlib.h>
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

/* Handle list management removed — handles now live in contiguous blocks.
 * See arena_v2.h RtHandleBlock. */

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

void rt_handle_begin_transaction(RtHandleV2 *handle)
{
    if (handle == NULL) return;
    pthread_mutex_lock(&handle->arena->mutex);
}

void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns)
{
    (void)timeout_ns;
    rt_handle_begin_transaction(handle);
}

void rt_handle_end_transaction(RtHandleV2 *handle)
{
    if (handle == NULL) return;
    pthread_mutex_unlock(&handle->arena->mutex);
}

void rt_handle_renew_transaction(RtHandleV2 *handle)
{
    /* No-op: arena recursive mutex has no timeout to renew */
    (void)handle;
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

/* ============================================================================
 * Cleanup Callback Management
 * ============================================================================ */

void rt_handle_set_cleanup(RtHandleV2 *handle, void (*fn)(RtHandleV2 *))
{
    if (handle == NULL) return;
    handle->cleanup_fn = fn;
}

