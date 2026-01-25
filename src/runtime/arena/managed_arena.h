#ifndef MANAGED_ARENA_H
#define MANAGED_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "arena_compat.h"

/* ============================================================================
 * Managed Arena — Handle-based memory manager with concurrent GC
 * ============================================================================
 * Wraps RtArena with a handle table for safe reassignment, pinning,
 * and background compaction. See README.md for full design.
 * ============================================================================ */

/* Handle type — opaque index into the handle table. 0 is the null handle. */
typedef uint32_t RtHandle;

#define RT_HANDLE_NULL 0

/* Moving sentinel — used by compactor to indicate entry is being relocated */
#define RT_LEASE_MOVING (-1)

/* ============================================================================
 * Arena Block (backing store)
 * ============================================================================ */
typedef struct RtManagedBlock {
    struct RtManagedBlock *next;  /* Next block in chain */
    size_t size;                  /* Block capacity */
    atomic_size_t used;           /* Bytes used (atomic for lock-free bump) */
    atomic_int lease_count;       /* Number of temporarily leased entries in this block */
    atomic_int pinned_count;      /* Number of permanently pinned entries in this block */
    bool retired;                 /* Marked for deallocation */
    char _padding[7];             /* Padding to ensure data[] is 8-byte aligned */
    char data[];                  /* Flexible array member (8-byte aligned) */
} RtManagedBlock;

/* ============================================================================
 * Handle Table Entry
 * ============================================================================ */
typedef struct {
    void *ptr;              /* Pointer to data in backing arena */
    RtManagedBlock *block;  /* Block containing this allocation */
    size_t size;            /* Size of the allocation */
    atomic_int leased;      /* Pin/lease counter (atomic) */
    bool dead;              /* Marked for reclamation */
    bool pinned;            /* Permanently pinned - compactor will never move this */
} RtHandleEntry;

/* Default block size: 64KB */
#define RT_MANAGED_BLOCK_SIZE (64 * 1024)

/* Maximum block size for geometric growth: 4MB */
#define RT_MANAGED_BLOCK_MAX_SIZE (4 * 1024 * 1024)

/* Handle table page size (entries per page) */
#define RT_HANDLE_PAGE_SIZE 256

/* Initial page directory capacity (number of page pointers) */
#define RT_HANDLE_DIR_INIT_CAP 16

/* Compaction threshold: trigger when fragmentation exceeds this ratio */
#define RT_MANAGED_COMPACT_THRESHOLD 0.5

/* Block utilization threshold: trigger compaction when live_bytes/block_capacity
 * falls below this ratio. This catches cases where the cleaner recycles handles
 * faster than dead_bytes accumulates, leaving blocks mostly empty. */
#define RT_MANAGED_UTILIZATION_THRESHOLD 0.25

/* Minimum block count before utilization-based compaction triggers */
#define RT_MANAGED_UTILIZATION_MIN_BLOCKS 2

/* Cleaner/compactor sleep interval in milliseconds */
#define RT_MANAGED_GC_INTERVAL_MS 10

/* ============================================================================
 * Cleanup Callback
 * ============================================================================ */

/* Cleanup callback function type */
typedef void (*RtManagedCleanupFn)(void *data);

/* Cleanup node — priority-ordered linked list */
typedef struct RtManagedCleanupNode {
    void *data;                          /* User data passed to callback */
    RtManagedCleanupFn fn;               /* Callback function */
    int priority;                        /* Lower = invoked first */
    struct RtManagedCleanupNode *next;   /* Next node in list */
} RtManagedCleanupNode;

/* ============================================================================
 * Managed Arena
 * ============================================================================
 * Arenas form a tree: root (main arena) → children (function scopes).
 * Only the root has GC threads; they walk the entire tree.
 *
 * Scope modes:
 *   default  — new child arena, destroyed on scope exit
 *   private  — new child arena, destroyed on scope exit (no escape allowed)
 *   shared   — reuse parent arena (caller passes its own arena)
 * ============================================================================ */
typedef struct RtManagedArena {
    /* Backing store */
    RtManagedBlock *first;        /* First block in active chain */
    RtManagedBlock *current;      /* Current block for allocations */
    size_t block_size;            /* Default block size */
    size_t total_allocated;       /* Total bytes allocated across all blocks */

    /* Retired arenas (pending free when pins drain) */
    RtManagedBlock *retired_list; /* Chain of retired blocks */

    /* Handle table (paged — no copying on growth) */
    RtHandleEntry **pages;        /* Array of page pointers */
    uint32_t pages_count;         /* Number of allocated pages */
    uint32_t pages_capacity;      /* Capacity of pages pointer array */
    uint32_t table_count;         /* Total entries allocated (across all pages) */

    /* Free list (recycled handle indices) */
    uint32_t *free_list;          /* Stack of recyclable handle indices */
    uint32_t free_count;          /* Number of free handles available */
    uint32_t free_capacity;       /* Free list capacity */

    /* Arena tree (parent-child linked list) */
    struct RtManagedArena *parent;        /* Parent arena (NULL for root) */
    struct RtManagedArena *first_child;   /* Head of children linked list */
    struct RtManagedArena *next_sibling;  /* Next sibling in parent's child list */
    bool is_root;                         /* Only root owns GC threads */
    pthread_mutex_t children_mutex;       /* Protects child list modifications */
    atomic_int gc_processing;             /* GC passes currently processing this arena */
    atomic_bool destroying;               /* Set before unlinking; GC skips if true */

    /* Background threads (root only) */
    pthread_t cleaner_thread;     /* Cleaner thread handle */
    pthread_t compactor_thread;   /* Compactor thread handle */
    atomic_bool running;          /* Signal threads to stop */
    atomic_uint gc_cleaner_epoch;   /* Incremented after each cleaner iteration */
    atomic_uint gc_compactor_epoch; /* Incremented after each compactor iteration */

    /* Synchronization */
    pthread_mutex_t alloc_mutex;  /* Protects table/block mutations */
    atomic_uint block_epoch;      /* Incremented when compactor swaps blocks */

    /* Stats */
    atomic_size_t live_bytes;     /* Bytes in live allocations */
    atomic_size_t dead_bytes;     /* Bytes in dead allocations (reclaimable) */

    /* Cleanup callbacks (invoked on destroy/reset) */
    RtManagedCleanupNode *cleanup_list;  /* Priority-sorted linked list */

    /* Retired arena list (root only): destroyed child structs awaiting final free */
    struct RtManagedArena *retired_arenas; /* Linked via next_sibling */
} RtManagedArena;

/* ============================================================================
 * Handle Table Access (paged)
 * ============================================================================ */

/* Get a pointer to the handle entry at the given index.
 * The returned pointer remains valid even after table growth. */
static inline RtHandleEntry *rt_handle_get(RtManagedArena *ma, uint32_t index)
{
    return &ma->pages[index / RT_HANDLE_PAGE_SIZE][index % RT_HANDLE_PAGE_SIZE];
}

/* ============================================================================
 * Public API: Lifecycle
 * ============================================================================ */

/* Create root managed arena. Starts cleaner and compactor threads. */
RtManagedArena *rt_managed_arena_create(void);

/* Create a child arena (default/private scope). No GC threads — root's
 * threads walk the tree. Links into parent's child list. */
RtManagedArena *rt_managed_arena_create_child(RtManagedArena *parent);

/* Destroy a child arena. Marks all live handles dead, unlinks from parent,
 * retires blocks to parent for GC cleanup. For scope exit. */
void rt_managed_arena_destroy_child(RtManagedArena *child);

/* Destroy the root managed arena. Stops threads, frees all memory
 * (including any remaining children). */
void rt_managed_arena_destroy(RtManagedArena *ma);

/* Get the root arena from any arena in the tree. */
RtManagedArena *rt_managed_arena_root(RtManagedArena *ma);

/* Allocate memory. If old != RT_HANDLE_NULL, marks old allocation as dead.
 * Returns a new handle. Thread-safe. */
RtHandle rt_managed_alloc(RtManagedArena *ma, RtHandle old, size_t size);

/* Allocate permanently pinned memory that will never be moved by the compactor.
 * Use for structures containing pthread_mutex_t, pthread_cond_t, or other
 * OS resources that cannot be relocated. Returns a new handle. Thread-safe. */
RtHandle rt_managed_alloc_pinned(RtManagedArena *ma, RtHandle old, size_t size);

/* Promote a handle from src arena to dest arena.
 * Copies the data, marks the source handle dead, returns new handle in dest.
 * Use for escaping allocations (child → parent on scope exit). */
RtHandle rt_managed_promote(RtManagedArena *dest, RtManagedArena *src, RtHandle h);

/* Clone a handle from one arena to another without marking source dead.
 * Unlike promote, the source entry remains valid after cloning.
 * Used for thread spawn arguments where multiple threads read the same source. */
RtHandle rt_managed_clone(RtManagedArena *dest, RtManagedArena *src, RtHandle h);

/* Pin a handle — returns raw pointer, increments lease counter.
 * The pointer is valid until rt_managed_unpin is called. */
void *rt_managed_pin(RtManagedArena *ma, RtHandle h);

/* Pin from any arena in the tree (self, parents, or root).
 * Tries the given arena first, then walks up to parent arenas.
 * Used when handles may come from parent scopes (e.g., globals in main arena). */
void *rt_managed_pin_any(RtManagedArena *ma, RtHandle h);

/* Unpin a handle — decrements lease counter.
 * After unpin, the previously returned pointer may become invalid. */
void rt_managed_unpin(RtManagedArena *ma, RtHandle h);

/* ============================================================================
 * Public API: Mark Dead (escape analysis support)
 * ============================================================================ */

/* Mark a handle as dead without allocating. GC will reclaim the memory.
 * Used by escape analysis to mark scope-local handles at scope exit. */
void rt_managed_mark_dead(RtManagedArena *ma, RtHandle h);

/* ============================================================================
 * Public API: Pin Helpers
 * ============================================================================ */

/* Array metadata — stored immediately before array data.
 * Forward-declared here for rt_managed_pin_array offset calculation. */
struct RtArrayMetadata_tag;

/* Pin a string handle — returns char* pointing to string data */
static inline char *rt_managed_pin_str(RtManagedArena *ma, RtHandle h) {
    return (char *)rt_managed_pin(ma, h);
}

/* Pin an array handle — returns pointer to array DATA (past metadata).
 * Array handle layout: [RtArrayMetadata][element data...]
 * This returns a pointer to element data, which is what existing
 * array access patterns (arr[i]) expect. */
static inline void *rt_managed_pin_array(RtManagedArena *ma, RtHandle h) {
    void *raw = rt_managed_pin(ma, h);
    if (!raw) return NULL;
    /* Skip past the RtArrayMetadata (3 pointer-sized fields: arena, size, capacity) */
    return (void *)((char *)raw + sizeof(void *) + sizeof(size_t) + sizeof(size_t));
}

/* Pin an array from any arena in tree (for parameters that may hold global handles) */
static inline void *rt_managed_pin_array_any(RtManagedArena *ma, RtHandle h) {
    void *raw = rt_managed_pin_any(ma, h);
    if (!raw) return NULL;
    return (void *)((char *)raw + sizeof(void *) + sizeof(size_t) + sizeof(size_t));
}

/* ============================================================================
 * Public API: String Helpers
 * ============================================================================ */

/* Duplicate a null-terminated string into the arena. Returns a handle. */
RtHandle rt_managed_strdup(RtManagedArena *ma, RtHandle old, const char *str);

/* Duplicate up to n bytes of a string into the arena. Returns a handle. */
RtHandle rt_managed_strndup(RtManagedArena *ma, RtHandle old, const char *str, size_t n);

/* Promote a string handle from src arena to dest arena (convenience). */
RtHandle rt_managed_promote_string(RtManagedArena *dest, RtManagedArena *src, RtHandle h);

/* ============================================================================
 * Public API: Cleanup Callbacks
 * ============================================================================ */

/* Register a cleanup callback invoked on arena destroy/reset.
 * Lower priority values are invoked first. Returns the node, or NULL on failure. */
RtManagedCleanupNode *rt_managed_on_cleanup(RtManagedArena *ma, void *data,
                                             RtManagedCleanupFn fn, int priority);

/* Remove a cleanup callback by data pointer. */
void rt_managed_remove_cleanup(RtManagedArena *ma, void *data);

/* ============================================================================
 * Public API: Reset
 * ============================================================================ */

/* Reset the arena: invokes cleanup callbacks, marks all entries dead.
 * GC threads will reclaim the memory. */
void rt_managed_arena_reset(RtManagedArena *ma);

/* ============================================================================
 * Diagnostics
 * ============================================================================ */

/* Total bytes allocated across all blocks (including overhead) */
size_t rt_managed_total_allocated(RtManagedArena *ma);

/* Number of live (non-dead) allocations */
size_t rt_managed_live_count(RtManagedArena *ma);

/* Number of dead (reclaimable) allocations */
size_t rt_managed_dead_count(RtManagedArena *ma);

/* Fragmentation ratio: dead_bytes / (live_bytes + dead_bytes) */
double rt_managed_fragmentation(RtManagedArena *ma);

/* Total bytes used by backing arena (including dead allocations) */
size_t rt_managed_arena_used(RtManagedArena *ma);

/* Force a compaction cycle (for testing) */
void rt_managed_compact(RtManagedArena *ma);

/* Block until both GC threads complete one full iteration.
 * Navigates to root if called on a child arena.
 * Max wait: 500ms safety bound. */
void rt_managed_gc_flush(RtManagedArena *ma);

#endif /* MANAGED_ARENA_H */
