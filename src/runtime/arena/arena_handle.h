/*
 * Arena Handle - Handle Operations and Transactions
 * ==================================================
 * Handle access patterns and transaction-based locking for safe
 * concurrent access to handle data.
 */

#ifndef ARENA_HANDLE_H
#define ARENA_HANDLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Forward declarations */
typedef struct RtHandleV2 RtHandleV2;
typedef struct RtArenaV2 RtArenaV2;
typedef struct RtBlockV2 RtBlockV2;

/* ============================================================================
 * Copy Callback - Deep Copy Support
 * ============================================================================
 * When a handle is promoted/cloned, the shallow copy happens first, then
 * the copy callback (if set) is invoked to handle deep copying of nested
 * handles. The callback receives the destination arena and a pointer to
 * the already-copied data in the new handle.
 *
 * Example: A struct with handle fields
 *   void __copy_MyStruct__(RtArenaV2 *dest, void *ptr) {
 *       MyStruct *s = (MyStruct *)ptr;
 *       s->name = rt_arena_v2_promote(dest, s->name);
 *       s->data = rt_arena_v2_promote(dest, s->data);
 *   }
 *
 * The callback is inherited during promote/clone, so nested structures
 * with their own callbacks work automatically.
 * ============================================================================ */

typedef void (*RtHandleV2CopyCallback)(RtArenaV2 *dest, void *ptr);

/* ============================================================================
 * Destroy Callback - Cleanup Before Free
 * ============================================================================
 * When GC frees a handle, the destroy callback (if set) is invoked first.
 * Use this to release resources that can't be abandoned (pthread primitives,
 * file handles, etc.).
 *
 * Example: RtThread cleanup
 *   void __destroy_RtThread__(void *ptr) {
 *       RtThread *t = (RtThread *)ptr;
 *       pthread_mutex_destroy(&t->mutex);
 *       pthread_cond_destroy(&t->cond);
 *   }
 * ============================================================================ */

typedef void (*RtHandleV2FreeCallback)(RtHandleV2 *handle);

/* ============================================================================
 * Handle Flags
 * ============================================================================ */

typedef enum {
    RT_HANDLE_FLAG_NONE     = 0,
    RT_HANDLE_FLAG_DEAD     = 1 << 1,  /* Marked for collection */
    RT_HANDLE_FLAG_ROOT     = 1 << 2,  /* Is a GC root (global/static) */
    RT_HANDLE_FLAG_EXTERN   = 1 << 3,  /* Data allocated externally (don't free ptr) */
} RtHandleFlags;

/* ============================================================================
 * Handle V2 - The First-Class Citizen
 * ============================================================================
 * Fat handle containing everything needed. No separate entry table.
 * Handles form a doubly-linked list within their owning block.
 * ============================================================================ */

struct RtHandleV2 {
    /* Core data */
    void *ptr;                  /* Direct pointer to allocated data */
    size_t size;                /* Size of allocation */

    /* Ownership */
    RtArenaV2 *arena;           /* Owning arena (never NULL for valid handles) */
    RtBlockV2 *block;           /* Block containing the data */

    /* GC metadata */
    uint16_t flags;             /* RtHandleFlags */

    /* Callbacks */
    RtHandleV2CopyCallback copy_callback;     /* Called after shallow copy (NULL for simple types) */
    RtHandleV2FreeCallback free_callback;     /* Called before GC frees handle (NULL if no cleanup) */

    /* Intrusive linked list for block's handle tracking */
    RtHandleV2 *next;           /* Next handle in block */
    RtHandleV2 *prev;           /* Previous handle in block */
};

/* ============================================================================
 * Handle Operations
 * ============================================================================
 *
 * IMPORTANT: Never discard the RtHandleV2* reference after allocation.
 * All allocations return RtHandleV2* which MUST be retained. Losing the
 * handle reference means the GC cannot track, collect, or manage the
 * allocation — leading to silent memory leaks that are extremely difficult
 * to diagnose. Access data via handle->ptr within a transaction.
 *
 * Correct pattern:
 *   RtHandleV2 *h = rt_arena_v2_alloc(arena, sizeof(MyStruct));
 *   rt_handle_begin_transaction(h);
 *   MyStruct *s = (MyStruct *)h->ptr;
 *   // ... use s ...
 *   rt_handle_end_transaction(h);
 *   rt_arena_v2_free(h);
 *
 * ============================================================================ */

/* Get owning arena. */
RtArenaV2 *rt_handle_v2_arena(RtHandleV2 *handle);

/* Check if handle is valid (not null, not dead). */
bool rt_handle_v2_is_valid(RtHandleV2 *handle);

/* ============================================================================
 * Handle List Management
 * ============================================================================
 * Internal functions for managing the handle linked list within blocks.
 * Used by allocation (link) and GC (unlink).
 * ============================================================================ */

/* Link handle into block's handle list (at head) */
void rt_handle_v2_link(RtBlockV2 *block, RtHandleV2 *handle);

/* Unlink handle from block's handle list */
void rt_handle_v2_unlink(RtBlockV2 *block, RtHandleV2 *handle);

/* ============================================================================
 * Handle Transactions
 * ============================================================================
 * All access to handle->ptr must occur within a transaction. Transactions
 * provide block-level locking that allows GC to safely compact memory.
 *
 * Rules:
 * - Never hold handle->ptr outside a transaction
 * - Transactions have a timeout (default 2 seconds)
 * - Long-running operations must call rt_handle_renew_transaction() periodically
 * - GC can force-acquire expired leases
 *
 * Example:
 *   rt_handle_begin_transaction(handle);
 *   char *data = (char *)handle->ptr;
 *   process(data);
 *   rt_handle_end_transaction(handle);
 *
 * ============================================================================ */

/* Transaction constants */
#define TX_DEFAULT_TIMEOUT_NS (2ULL * 1000000000ULL)  /* 2 seconds */

/* Get monotonic time in nanoseconds (used for transaction timeouts) */
uint64_t rt_get_monotonic_ns(void);

/* Begin a transaction with default timeout (2 seconds).
 * Acquires a lease on the handle's block. Same thread can nest transactions. */
void rt_handle_begin_transaction(RtHandleV2 *handle);

/* Begin a transaction with custom timeout (in nanoseconds). */
void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns);

/* End a transaction. Decrements nesting count, releases when count reaches 0.
 * Panics if current thread does not hold the lease. */
void rt_handle_end_transaction(RtHandleV2 *handle);

/* Renew a transaction's timeout. Resets the start time to now.
 * Use this in long-running operations to prevent GC from force-acquiring.
 * Panics if current thread does not hold the lease. */
void rt_handle_renew_transaction(RtHandleV2 *handle);

/* ============================================================================
 * Copy Callback Management
 * ============================================================================ */

/* Set copy callback for deep copy support during promote/clone. */
void rt_handle_set_copy_callback(RtHandleV2 *handle, RtHandleV2CopyCallback callback);

/* Get copy callback (NULL if none set). */
RtHandleV2CopyCallback rt_handle_get_copy_callback(RtHandleV2 *handle);

/* Set destroy callback for cleanup before GC frees handle. */
void rt_handle_set_free_callback(RtHandleV2 *handle, RtHandleV2FreeCallback callback);

/* Get destroy callback (NULL if none set). */
RtHandleV2FreeCallback rt_handle_get_free_callback(RtHandleV2 *handle);

#endif /* ARENA_HANDLE_H */
