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

/* ============================================================================
 * Copy Callback - Deep Copy Support
 * ============================================================================
 * When a handle is promoted/cloned, the shallow copy happens first, then
 * the copy callback (if set) is invoked to handle deep copying of nested
 * handles. The callback receives the destination arena and a pointer to
 * the already-copied data in the new handle.
 *
 * Example: A struct with handle fields
 *   void __copy_MyStruct__(RtArenaV2 *dest, RtHandleV2 *new_handle) {
 *       MyStruct *s = (MyStruct *)new_handle->ptr;
 *       s->name = rt_arena_v2_promote(dest, s->name);
 *       s->data = rt_arena_v2_promote(dest, s->data);
 *   }
 *
 * The callback is inherited during promote/clone, so nested structures
 * with their own callbacks work automatically.
 * ============================================================================ */

typedef void (*RtHandleV2CopyCallback)(RtArenaV2 *dest, RtHandleV2 *new_handle);

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
 * Handles live in contiguous blocks (slabs) within their owning arena.
 * ============================================================================ */

struct RtHandleV2 {
    /* Core data */
    void *ptr;                  /* Direct pointer to allocated data */
    size_t size;                /* Size of allocation */

    /* Ownership */
    RtArenaV2 *arena;           /* Owning arena (NULL for free/recycled slots) */

    /* Callbacks */
    RtHandleV2CopyCallback copy_callback;     /* Called after shallow copy (NULL for simple types) */
    void (*cleanup_fn)(RtHandleV2 *);         /* Called on GC collect or arena destroy (NULL if none) */

    /* GC metadata */
    uint16_t flags;             /* RtHandleFlags */
    uint16_t _reserved;         /* Future use */
    uint32_t _pad;              /* Alignment padding */
};  /* 48 bytes */

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
 * Handle Block Management
 * ============================================================================
 * Handles live in contiguous blocks (slabs) within arenas. Block iteration
 * replaces linked list traversal for GC and stats.
 * ============================================================================ */

/* ============================================================================
 * Handle Transactions
 * ============================================================================
 * All access to handle->ptr must occur within a transaction. Transactions
 * lock the arena's recursive mutex for safe concurrent access.
 *
 * Example:
 *   rt_handle_begin_transaction(handle);
 *   char *data = (char *)handle->ptr;
 *   process(data);
 *   rt_handle_end_transaction(handle);
 *
 * ============================================================================ */

/* Get monotonic time in nanoseconds (used by runtime) */
uint64_t rt_get_monotonic_ns(void);

/* Begin a transaction. Locks the arena's recursive mutex. */
void rt_handle_begin_transaction(RtHandleV2 *handle);

/* Begin a transaction (timeout parameter ignored, for API compatibility). */
void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns);

/* End a transaction. Unlocks the arena's recursive mutex. */
void rt_handle_end_transaction(RtHandleV2 *handle);

/* Renew a transaction (no-op, kept for API compatibility). */
void rt_handle_renew_transaction(RtHandleV2 *handle);

/* ============================================================================
 * Copy Callback Management
 * ============================================================================ */

/* Set copy callback for deep copy support during promote/clone. */
void rt_handle_set_copy_callback(RtHandleV2 *handle, RtHandleV2CopyCallback callback);

/* Get copy callback (NULL if none set). */
RtHandleV2CopyCallback rt_handle_get_copy_callback(RtHandleV2 *handle);

/* ============================================================================
 * Cleanup Callback Management
 * ============================================================================
 * Per-handle cleanup for external resources (json-c objects, file descriptors,
 * sockets, etc.). Stored directly on the handle for O(1) registration and
 * lookup, replacing the arena's O(n) linked list for per-handle cleanups.
 * Called when GC collects the handle OR when the arena is destroyed.
 * ============================================================================ */

/* Set cleanup callback for external resource management. */
void rt_handle_set_cleanup(RtHandleV2 *handle, void (*fn)(RtHandleV2 *));

#endif /* ARENA_HANDLE_H */
