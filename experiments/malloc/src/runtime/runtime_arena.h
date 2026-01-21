#ifndef RUNTIME_ARENA_H
#define RUNTIME_ARENA_H

#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
    /* MinGW provides pthreads - but SN_USE_WIN32_THREADS forces native Win32 API */
    #include <pthread.h>
    #else
    /* Use Windows API compatibility layer for MSVC/clang-cl and packaged builds */
    #include "../platform/compat_pthread.h"
    #endif
#else
#include <pthread.h>
#endif

/* ============================================================================
 * Arena Memory Management
 * ============================================================================
 * Arenas provide block-scoped memory allocation. All allocations within an
 * arena are freed together when the arena is destroyed. This eliminates
 * individual free() calls and prevents memory leaks.
 * ============================================================================ */

/* Default block size for arena allocations (64KB) */
#define RT_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)

/* Arena block - linked list of memory blocks */
typedef struct RtArenaBlock {
    struct RtArenaBlock *next;  /* Next block in chain */
    size_t size;                /* Size of this block's data area */
    size_t used;                /* Bytes used in this block */
    char data[];                /* Flexible array member for actual data */
} RtArenaBlock;

/* ============================================================================
 * Generic Cleanup Callback System
 * ============================================================================
 * Arenas support registering cleanup callbacks that are invoked when the arena
 * is destroyed or reset. This allows any module to register resources for
 * automatic cleanup without the arena needing type-specific knowledge.
 * ============================================================================ */

/* Cleanup callback function type */
typedef void (*RtCleanupFn)(void *data);

/* Cleanup priority - lower values are cleaned up first */
typedef enum {
    RT_CLEANUP_PRIORITY_HIGH = 0,     /* Threads synced first (highest priority) */
    RT_CLEANUP_PRIORITY_MEDIUM = 10,      /* Files closed after threads */
    RT_CLEANUP_PRIORITY_DEFAULT = 50     /* Default priority for user resources */
} RtCleanupPriority;

/* Cleanup node - linked list node for tracking cleanup callbacks */
typedef struct RtCleanupNode {
    void *data;                     /* Data to pass to cleanup function */
    RtCleanupFn cleanup_fn;         /* Cleanup function to call */
    int priority;                   /* Cleanup priority (lower = earlier) */
    struct RtCleanupNode *next;     /* Next node in chain */
} RtCleanupNode;

/* Arena - manages a collection of memory blocks */
typedef struct RtArena {
    struct RtArena *parent;         /* Parent arena for hierarchy */
    RtArenaBlock *first;            /* First block in chain */
    RtArenaBlock *current;          /* Current block for allocations */
    size_t default_block_size;      /* Size for new blocks */
    size_t total_allocated;         /* Total bytes allocated (for stats) */
    RtCleanupNode *cleanup_list;    /* Head of cleanup callback list (sorted by priority) */
    bool frozen;                    /* True if arena is frozen (shared thread executing) */
    pthread_t frozen_owner;         /* Thread ID that owns frozen arena (can still allocate) */
} RtArena;

/* ============================================================================
 * Arena Function Declarations
 * ============================================================================ */

/* Create a new arena, optionally with a parent */
RtArena *rt_arena_create(RtArena *parent);

/* Create arena with custom block size */
RtArena *rt_arena_create_sized(RtArena *parent, size_t block_size);

/* Allocate memory from arena (uninitialized) */
void *rt_arena_alloc(RtArena *arena, size_t size);

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size);

/* Allocate aligned memory from arena */
void *rt_arena_alloc_aligned(RtArena *arena, size_t size, size_t alignment);

/* Duplicate a string into arena */
char *rt_arena_strdup(RtArena *arena, const char *str);

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n);

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena);

/* Reset arena for reuse (keeps first block, frees rest) */
void rt_arena_reset(RtArena *arena);

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size);

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src);

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena);

/* ============================================================================
 * Cleanup Callback Registration
 * ============================================================================
 * Functions for registering and removing cleanup callbacks. Callbacks are
 * invoked in priority order (lower priority values first) when the arena
 * is destroyed or reset.
 * ============================================================================ */

/* Register a cleanup callback for arena destruction/reset.
 * The callback will be invoked with 'data' when the arena is destroyed or reset.
 * Callbacks are invoked in priority order (lower values first).
 * Returns the cleanup node (can be used for removal), or NULL on failure. */
RtCleanupNode *rt_arena_on_cleanup(RtArena *arena, void *data,
                                    RtCleanupFn cleanup_fn, int priority);

/* Remove a cleanup callback by data pointer.
 * Finds and removes the first cleanup node matching 'data'.
 * The callback will NOT be invoked when removed this way. */
void rt_arena_remove_cleanup(RtArena *arena, void *data);

/* ============================================================================
 * Arena Freezing (for shared thread mode)
 * ============================================================================
 * When a shared thread is spawned, the caller's arena is frozen to prevent
 * allocations from the main thread while the shared thread is executing.
 * This prevents data races on arena state.
 * ============================================================================ */

/* Freeze an arena to prevent allocations (used during shared thread execution) */
void rt_arena_freeze(RtArena *arena);

/* Unfreeze an arena to allow allocations again (called after thread sync) */
void rt_arena_unfreeze(RtArena *arena);

/* Check if an arena is frozen */
bool rt_arena_is_frozen(RtArena *arena);

#endif /* RUNTIME_ARENA_H */
