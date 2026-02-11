/*
 * Arena V2 - Simplified Memory Management for Sindarin
 * =====================================================
 *
 * Design principles:
 * 1. Fat handles - RtHandleV2 contains everything (ptr, arena, epoch, flags)
 * 2. Simple GC - Lock-based with epoch waves, no lock-free complexity
 * 3. Clear ownership - Every handle knows its arena
 * 4. Interop-friendly - Native code can get arena from any handle
 * 5. No compaction - Trade memory for simplicity
 *
 * Mental model:
 * - Allocate: create handle, link into arena's list
 * - Use: dereference handle->ptr
 * - Free/GC: unlink from list, free data and handle
 * - Arena destroy: walk list, free everything
 *
 * Arena modes (unchanged from v1):
 * - Default: own arena, data promoted on return
 * - Shared: use caller's arena
 * - Private: isolated arena, destroyed on exit
 */

#ifndef ARENA_V2_H
#define ARENA_V2_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Use arena_compat.h for pthread types/functions to avoid conflicts on Windows */
#include "runtime/arena/arena_compat.h"

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

typedef struct RtHandleV2 RtHandleV2;
typedef struct RtArenaV2 RtArenaV2;
typedef struct RtBlockV2 RtBlockV2;

/* ============================================================================
 * Handle Flags
 * ============================================================================ */

typedef enum {
    RT_HANDLE_FLAG_NONE     = 0,
    RT_HANDLE_FLAG_PINNED   = 1 << 0,  /* Cannot be GC'd while pinned */
    RT_HANDLE_FLAG_DEAD     = 1 << 1,  /* Marked for collection */
    RT_HANDLE_FLAG_ROOT     = 1 << 2,  /* Is a GC root (global/static) */
    RT_HANDLE_FLAG_EXTERN   = 1 << 3,  /* Data allocated externally (don't free ptr) */
} RtHandleFlags;

/* ============================================================================
 * Handle V2 - The First-Class Citizen
 * ============================================================================
 * Fat handle containing everything needed. No separate entry table.
 * Handles form a doubly-linked list within their owning arena.
 * ============================================================================ */

struct RtHandleV2 {
    /* Core data */
    void *ptr;                  /* Direct pointer to allocated data */
    size_t size;                /* Size of allocation */

    /* Ownership */
    RtArenaV2 *arena;           /* Owning arena (never NULL for valid handles) */
    RtBlockV2 *block;           /* Block containing the data */

    /* GC metadata */
    uint32_t alloc_epoch;       /* Epoch when allocated */
    uint32_t last_seen_epoch;   /* Last GC epoch where handle was live */
    uint16_t flags;             /* RtHandleFlags */
    uint16_t pin_count;         /* Reference count for pinning */

    /* Intrusive linked list for arena's handle tracking */
    RtHandleV2 *next;           /* Next handle in arena */
    RtHandleV2 *prev;           /* Previous handle in arena */
};

/* Null handle constant */
#define RT_HANDLE_V2_NULL ((RtHandleV2 *)NULL)

/* ============================================================================
 * Block V2 - Backing Storage
 * ============================================================================
 * Simple block for bulk memory allocation. No lock-free complexity.
 * ============================================================================ */

struct RtBlockV2 {
    RtBlockV2 *next;            /* Next block in arena's chain */
    RtArenaV2 *arena;           /* Owning arena */
    size_t capacity;            /* Total block capacity */
    size_t used;                /* Bytes used */
    char data[];                /* Flexible array for actual data */
};

/* Default block size: 64KB */
#define RT_BLOCK_V2_SIZE (64 * 1024)

/* ============================================================================
 * Arena V2 - Memory Region with Scoped Lifetime
 * ============================================================================
 * Arenas own handles and blocks. Simple mutex-based synchronization.
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

    /* Handle tracking - simple linked list */
    RtHandleV2 *handles_head;   /* Head of handle list */
    RtHandleV2 *handles_tail;   /* Tail for O(1) append */
    size_t handle_count;        /* Number of live handles */

    /* Block storage */
    RtBlockV2 *blocks_head;     /* Head of block chain */
    RtBlockV2 *current_block;   /* Current allocation block */

    /* GC state */
    uint32_t current_epoch;     /* Current GC epoch */
    uint32_t gc_threshold;      /* Handles before triggering GC */
    bool gc_running;            /* Is GC currently running? */
    volatile uint16_t flags;    /* RtArenaFlags */

    /* Synchronization - simple mutex, no lock-free */
    pthread_mutex_t mutex;

    /* Cleanup callbacks */
    RtCleanupNodeV2 *cleanups;

    /* Statistics */
    size_t total_allocated;     /* Total bytes allocated */
    size_t total_freed;         /* Total bytes freed by GC */
    size_t gc_runs;             /* Number of GC runs */

    /* Root arena reference - for quick access to tree root */
    RtArenaV2 *root;            /* Root ancestor (self if this is root) */
};

/* ============================================================================
 * Arena Lifecycle
 * ============================================================================ */

/* Create a new arena. Parent can be NULL for root arenas. */
RtArenaV2 *rt_arena_v2_create(RtArenaV2 *parent, RtArenaMode mode, const char *name);

/* Destroy an arena and all its handles/blocks. */
void rt_arena_v2_destroy(RtArenaV2 *arena);

/* Mark an arena as dead for GC collection. The GC thread will
 * destroy the arena on its next pass. */
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
 * Handle Operations
 * ============================================================================
 *
 * IMPORTANT: Never discard the RtHandleV2* reference after allocation.
 * All allocations return RtHandleV2* which MUST be retained. Losing the
 * handle reference means the GC cannot track, collect, or manage the
 * allocation — leading to silent memory leaks that are extremely difficult
 * to diagnose. Access data via handle->ptr after pinning.
 *
 * Correct pattern:
 *   RtHandleV2 *h = rt_arena_v2_alloc(arena, sizeof(MyStruct));
 *   rt_handle_v2_pin(h);
 *   MyStruct *s = (MyStruct *)h->ptr;
 *   // ... use s ...
 *   rt_handle_v2_unpin(h);
 *   rt_arena_v2_free(h);
 *
 * ============================================================================ */

/* Pin a handle (increment pin count, prevents GC).
 * Access data via handle->ptr after pinning. */
static inline void rt_handle_v2_pin(RtHandleV2 *handle) {
    if (handle == NULL) return;
    handle->pin_count++;
}

/* Unpin a handle (decrement pin count). */
static inline void rt_handle_v2_unpin(RtHandleV2 *handle) {
    if (handle == NULL) return;
    if (handle->pin_count > 0) handle->pin_count--;
}

/* Get owning arena. */
static inline RtArenaV2 *rt_handle_v2_arena(RtHandleV2 *handle) {
    return handle ? handle->arena : NULL;
}

/* Check if handle is valid (not null, not dead). */
static inline bool rt_handle_v2_is_valid(RtHandleV2 *handle) {
    return handle != NULL && !(handle->flags & RT_HANDLE_FLAG_DEAD);
}

/* ============================================================================
 * Garbage Collection
 * ============================================================================ */

/* Run one GC pass on an arena. Returns number of handles collected. */
size_t rt_arena_v2_gc(RtArenaV2 *arena);

/* Run GC on arena and all descendants. */
size_t rt_arena_v2_gc_recursive(RtArenaV2 *arena);

/* Force immediate collection of all dead handles. */
void rt_arena_v2_gc_flush(RtArenaV2 *arena);

/* ============================================================================
 * Background GC Thread
 * ============================================================================ */

/* Start the background GC thread. Runs GC recursively on root arena tree
 * every interval_ms. Default interval is 100ms if interval_ms <= 0. */
void rt_arena_v2_gc_thread_start(RtArenaV2 *root, int interval_ms);

/* Stop the background GC thread. Blocks until thread exits. */
void rt_arena_v2_gc_thread_stop(void);

/* Check if GC thread is running. */
bool rt_arena_v2_gc_thread_running(void);

/* ============================================================================
 * Promotion (moving data between arenas)
 * ============================================================================ */

/* Promote a handle from source arena to destination arena.
 * Returns new handle in dest, marks old handle dead. */
RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle);

/* Clone a handle to another arena (source remains valid). */
RtHandleV2 *rt_arena_v2_clone(RtArenaV2 *dest, RtHandleV2 *handle);

/* ============================================================================
 * Malloc Redirect Integration
 * ============================================================================ */

/* Push arena for malloc redirection (thread-local). */
void rt_arena_v2_redirect_push(RtArenaV2 *arena);

/* Pop malloc redirection. */
void rt_arena_v2_redirect_pop(void);

/* Get current redirect arena for this thread. */
RtArenaV2 *rt_arena_v2_redirect_current(void);

/* ============================================================================
 * Thread Support
 * ============================================================================ */

/* Get/set the current thread's arena. */
RtArenaV2 *rt_arena_v2_thread_current(void);
void rt_arena_v2_thread_set(RtArenaV2 *arena);

/* Get thread arena or fallback if not set. */
static inline RtArenaV2 *rt_arena_v2_thread_or(RtArenaV2 *fallback) {
    RtArenaV2 *current = rt_arena_v2_thread_current();
    return current ? current : fallback;
}

/* ============================================================================
 * Debug / Statistics
 * ============================================================================ */

typedef struct {
    size_t handle_count;
    size_t total_allocated;
    size_t total_freed;
    size_t gc_runs;
    uint32_t current_epoch;
} RtArenaV2Stats;

void rt_arena_v2_get_stats(RtArenaV2 *arena, RtArenaV2Stats *stats);
void rt_arena_v2_print_stats(RtArenaV2 *arena);

/* Simple arena creation with default mode */
static inline RtArenaV2 *rt_arena_create(RtArenaV2 *parent) {
    return rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, NULL);
}

/* Arena destruction */
static inline void rt_arena_destroy(RtArenaV2 *arena) {
    rt_arena_v2_destroy(arena);
}

/* Get total allocated bytes in arena */
static inline size_t rt_arena_total_allocated(RtArenaV2 *arena) {
    return arena ? arena->total_allocated : 0;
}

#endif /* ARENA_V2_H */
