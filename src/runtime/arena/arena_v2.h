/*
 * Arena V2 - Simplified Memory Management for Sindarin
 * =====================================================
 *
 * Design principles:
 * 1. Fat handles - RtHandleV2 contains everything (ptr, arena, flags)
 * 2. Simple GC - Lock-based mark-sweep, no lock-free complexity
 * 3. Clear ownership - Every handle knows its arena
 * 4. Interop-friendly - Native code can get arena from any handle
 * 5. No compaction - Trade memory for simplicity
 *
 * Mental model:
 * - Allocate: create handle, link into arena's list
 * - Use: dereference handle->ptr within a transaction
 * - Free/GC: unlink from list, free data and handle
 * - Arena destroy: walk list, free everything
 *
 * Arena modes:
 * - Default: own arena, data promoted on return
 * - Shared: use caller's arena
 * - Private: isolated arena, destroyed on exit
 *
 * This header provides the complete arena API by including:
 * - arena_handle.h  - Handle operations and transactions
 * - arena_gc.h      - Garbage collection
 * - arena_stats.h   - Statistics and observability
 */

#ifndef ARENA_V2_H
#define ARENA_V2_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* Use arena_compat.h for pthread types/functions to avoid conflicts on Windows */
#include "runtime/arena/arena_compat.h"

/* Include handle definition (provides RtHandleV2, RtHandleFlags, forward decls) */
#include "runtime/arena/arena_handle.h"

/* Forward declarations for types defined in this header */
typedef struct RtArenaV2 RtArenaV2;

/* Include stats types before RtArenaV2 definition (stats are embedded in arena) */
#include "runtime/arena/arena_stats.h"

/* Null handle constant */
#define RT_HANDLE_V2_NULL ((RtHandleV2 *)NULL)

/* ============================================================================
 * Arena V2 - Memory Region with Scoped Lifetime
 * ============================================================================
 * Arenas own handles. Simple mutex-based synchronization.
 * Handle data is allocated via malloc and freed when GC collects.
 * ============================================================================ */

typedef enum {
    RT_ARENA_MODE_DEFAULT,      /* Own arena, promote on return */
    RT_ARENA_MODE_SHARED,       /* Reuse caller's arena */
    RT_ARENA_MODE_PRIVATE,      /* Isolated, destroyed on exit */
} RtArenaMode;

/* Arena flags - mirrors handle flag pattern */
typedef enum {
    RT_ARENA_FLAG_NONE = 0,
    RT_ARENA_FLAG_DEAD = (1 << 0),  /* Marked for GC destruction */
} RtArenaFlags;

/* Cleanup callback for external resources */
typedef void (*RtCleanupFnV2)(RtHandleV2 *data);

typedef struct RtCleanupNodeV2 {
    RtHandleV2 *data;
    RtCleanupFnV2 fn;
    int priority;               /* Lower = called first */
    struct RtCleanupNodeV2 *next;
} RtCleanupNodeV2;

struct RtArenaV2 {
    /* Identity */
    const char *name;           /* Optional debug name */
    RtArenaMode mode;           /* Arena mode */

    /* Hierarchy */
    RtArenaV2 *parent;          /* Parent arena (for promotion) */
    RtArenaV2 *first_child;     /* First child arena */
    RtArenaV2 *next_sibling;    /* Next sibling arena */

    /* Handle storage */
    RtHandleV2 *handles_head;   /* Head of arena handle linked list */

    /* GC state */
    volatile bool gc_running;   /* Is GC currently running on this arena? */
    volatile uint16_t flags;    /* RtArenaFlags */

    /* Synchronization */
    pthread_mutex_t mutex;

    /* Cleanup callbacks */
    RtCleanupNodeV2 *cleanups;

    /* Statistics - updated by GC cycle only */
    RtArenaV2Stats stats;

    /* GC logging */
    bool gc_log_enabled;        /* Print one-line report per GC pass */

    /* Root arena reference - for quick access to tree root */
    RtArenaV2 *root;            /* Root ancestor (self if this is root) */

    /* Condemned queue (root arena only) — lock-free LIFO stack.
     * Condemned arenas are pushed here by rt_arena_v2_condemn and
     * drained by GC Phase 1, avoiding slow tree walks. */
    RtArenaV2 *condemned_head;  /* Head of condemned list (root only) */

    /* Per-arena field for condemned queue linkage */
    RtArenaV2 *condemned_next;  /* Next in condemned list */
};

/* ============================================================================
 * Arena Lifecycle
 * ============================================================================ */

/* Create a new arena. Parent can be NULL for root arenas. */
RtArenaV2 *rt_arena_v2_create(RtArenaV2 *parent, RtArenaMode mode, const char *name);

/* Mark an arena as dead for GC collection. */
void rt_arena_v2_condemn(RtArenaV2 *arena);

/* Cleanup priority constants - lower values are called first */
#define RT_CLEANUP_PRIORITY_HIGH   0
#define RT_CLEANUP_PRIORITY_NORMAL 100
#define RT_CLEANUP_PRIORITY_LOW    200

/* Register a cleanup callback. */
void rt_arena_v2_on_cleanup(RtArenaV2 *arena, RtHandleV2 *data, RtCleanupFnV2 fn, int priority);

/* Remove a cleanup callback. */
void rt_arena_v2_remove_cleanup(RtArenaV2 *arena, RtHandleV2 *data);

/* ============================================================================
 * Allocation
 * ============================================================================ */

/* Allocate memory and return a handle. */
RtHandleV2 *rt_arena_v2_alloc(RtArenaV2 *arena, size_t size);

/* Allocate zeroed memory. */
RtHandleV2 *rt_arena_v2_calloc(RtArenaV2 *arena, size_t count, size_t size);

/* Reallocate - returns new handle, old handle marked dead. */
RtHandleV2 *rt_arena_v2_realloc(RtArenaV2 *arena, RtHandleV2 *old, size_t new_size);

/* Duplicate a string. */
RtHandleV2 *rt_arena_v2_strdup(RtArenaV2 *arena, const char *str);

/* Mark a handle as dead (eligible for GC). */
void rt_arena_v2_free(RtHandleV2 *handle);

/* ============================================================================
 * Promotion (moving data between arenas)
 * ============================================================================ */

/* Promote a handle from source arena to destination arena.
 * Returns new handle in dest, marks old handle dead. */
RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle);

/* Clone a handle to another arena (source remains valid). */
RtHandleV2 *rt_arena_v2_clone(RtArenaV2 *dest, RtHandleV2 *handle);

/* Reparent an arena from its current parent to a new parent.
 * Unlinks from old parent's child list, links into new parent's child list.
 * Used when returning structs: the struct's arena must move to the caller's
 * hierarchy so it survives the function's local arena destruction. */
void rt_arena_v2_reparent(RtArenaV2 *arena, RtArenaV2 *new_parent);

/* ============================================================================
 * Thread Support
 * ============================================================================ */

/* Get/set the current thread's arena (TLS). */
RtArenaV2 *rt_tls_arena_get(void);
void rt_tls_arena_set(RtArenaV2 *arena);

/* ============================================================================
 * Convenience Wrappers
 * ============================================================================ */

/* Simple arena creation with default mode */
static inline RtArenaV2 *rt_arena_create(RtArenaV2 *parent) {
    return rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, NULL);
}

/* ============================================================================
 * Include Split Headers for Complete API
 * ============================================================================ */

#include "runtime/arena/arena_handle.h"
#include "runtime/arena/arena_gc.h"
#include "runtime/arena/arena_redirect.h"
/* arena_stats.h already included above for RtArenaV2Stats type */

#endif /* ARENA_V2_H */
