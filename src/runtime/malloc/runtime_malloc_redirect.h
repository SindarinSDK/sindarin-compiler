#ifndef RUNTIME_MALLOC_REDIRECT_H
#define RUNTIME_MALLOC_REDIRECT_H

/*
 * Arena-Redirected Malloc System
 *
 * Redirects malloc/free/realloc/calloc calls to use an arena allocator.
 * This allows native C libraries to allocate memory that is automatically
 * managed by Sindarin's arena system.
 *
 * Features:
 *   - Per-thread enable/disable with thread-local state
 *   - Nested redirect scopes (push/pop stack)
 *   - Configurable policies for free, realloc, and overflow behavior
 *   - Optional allocation tracking for debugging
 *   - Hash set for reliable arena pointer detection
 *   - Optional mutex for thread-safe arena access
 *
 * Usage:
 *   RtArena *arena = rt_arena_create(NULL);
 *   rt_malloc_redirect_push(arena, NULL);  // Use default config
 *
 *   char *str = malloc(100);  // Actually uses arena
 *   // ... use str ...
 *   // No need to free - arena handles it
 *
 *   rt_malloc_redirect_pop();
 *   rt_arena_destroy(arena);  // Frees all redirected allocations
 *
 * Build Notes:
 *   - Core redirect functionality works with all optimization levels
 *   - Allocation tracking (track_allocations=true) requires -O0 or -O1
 *     due to aggressive optimization interfering with tracking callbacks
 *   - For production use, tracking should be disabled anyway (default)
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
    #include <pthread.h>
    #else
    #include "../platform/compat_pthread.h"
    #endif
#else
#include <pthread.h>
#endif

/* Include arena header for RtArena type */
#include "runtime_arena.h"

/* ============================================================================
 * Policy Enums
 * ============================================================================ */

/* How free() behaves when called on arena-allocated memory */
typedef enum {
    RT_REDIRECT_FREE_IGNORE,      /* Silently ignore (memory freed with arena) */
    RT_REDIRECT_FREE_TRACK,       /* Track frees for leak detection/debugging */
    RT_REDIRECT_FREE_WARN,        /* Log warning to stderr */
    RT_REDIRECT_FREE_ERROR        /* Panic/abort (strict mode) */
} RtRedirectFreePolicy;

/* How realloc() behaves */
typedef enum {
    RT_REDIRECT_REALLOC_COPY,     /* Alloc new in arena, copy, abandon old */
    RT_REDIRECT_REALLOC_IN_PLACE  /* Try to extend if at end of block (optimization) */
} RtRedirectReallocPolicy;

/* What happens when arena exceeds max_size (if set) */
typedef enum {
    RT_REDIRECT_OVERFLOW_GROW,    /* Continue growing (ignore max_size) */
    RT_REDIRECT_OVERFLOW_FALLBACK,/* Fall back to system malloc */
    RT_REDIRECT_OVERFLOW_FAIL,    /* Return NULL */
    RT_REDIRECT_OVERFLOW_PANIC    /* Abort with error message */
} RtRedirectOverflowPolicy;

/* ============================================================================
 * Allocation Header
 * ============================================================================
 * Every redirected allocation includes a hidden header for metadata.
 * Layout: [RtAllocHeader][user data...]
 */

#define RT_ALLOC_MAGIC 0x41524E41  /* "ARNA" in little-endian */

typedef struct {
    size_t size;           /* User-requested size (for realloc) */
    uint32_t magic;        /* Magic number to identify arena allocations */
    uint32_t flags;        /* Reserved for future use */
} RtAllocHeader;

/* ============================================================================
 * Hash Set for Allocation Tracking
 * ============================================================================
 * Maintains a set of all pointers allocated from the redirect arena.
 * Used for reliable pointer detection (is this pointer from our arena?).
 */

typedef struct RtAllocHashEntry {
    void *ptr;                      /* Allocated pointer (user data, not header) */
    size_t size;                    /* Allocation size */
    struct RtAllocHashEntry *next;  /* Chain for collision handling */
} RtAllocHashEntry;

typedef struct {
    RtAllocHashEntry **buckets;     /* Hash table buckets */
    size_t bucket_count;            /* Number of buckets */
    size_t entry_count;             /* Number of entries */
    size_t grow_threshold;          /* Rehash when entry_count exceeds this */
} RtAllocHashSet;

/* ============================================================================
 * Configuration
 * ============================================================================ */

typedef struct {
    RtRedirectFreePolicy free_policy;
    RtRedirectReallocPolicy realloc_policy;
    RtRedirectOverflowPolicy overflow_policy;

    size_t max_arena_size;          /* 0 = unlimited */
    bool track_allocations;         /* Enable detailed allocation tracking */
    bool zero_on_free;              /* Zero memory on "free" (security) */
    bool thread_safe;               /* Use mutex for arena operations */

    /* Optional callbacks for custom behavior */
    void (*on_overflow)(RtArena *arena, size_t requested, void *user_data);
    void (*on_alloc)(void *ptr, size_t size, void *user_data);
    void (*on_free)(void *ptr, size_t size, void *user_data);
    void *callback_user_data;       /* Passed to callbacks */
} RtRedirectConfig;

/* Default configuration */
#define RT_REDIRECT_CONFIG_DEFAULT { \
    .free_policy = RT_REDIRECT_FREE_IGNORE, \
    .realloc_policy = RT_REDIRECT_REALLOC_COPY, \
    .overflow_policy = RT_REDIRECT_OVERFLOW_GROW, \
    .max_arena_size = 0, \
    .track_allocations = false, \
    .zero_on_free = false, \
    .thread_safe = false, \
    .on_overflow = NULL, \
    .on_alloc = NULL, \
    .on_free = NULL, \
    .callback_user_data = NULL \
}

/* ============================================================================
 * Redirect State
 * ============================================================================
 * Per-thread state for malloc redirection. Supports nesting via linked list.
 */

typedef struct RtRedirectState {
    bool active;                    /* Is redirection currently active? */
    RtArena *arena;                 /* Target arena for allocations */
    RtRedirectConfig config;        /* Current configuration */
    RtAllocHashSet *alloc_set;      /* Hash set of arena allocations */
    pthread_mutex_t *mutex;         /* Optional mutex for thread safety */

    /* Statistics */
    size_t alloc_count;             /* Number of allocations */
    size_t free_count;              /* Number of free calls */
    size_t realloc_count;           /* Number of realloc calls */
    size_t total_requested;         /* Total bytes requested */
    size_t total_allocated;         /* Total bytes allocated (including headers) */
    size_t fallback_count;          /* Number of fallbacks to system malloc */
    size_t current_live;            /* Current live allocations (alloc - free) */
    size_t peak_live;               /* Peak live allocations */

    /* Allocation tracking list (when track_allocations is true) */
    struct RtAllocTrackEntry *track_head;

    /* Saved state for nested scopes */
    struct RtRedirectState *prev;   /* Previous state (for nesting) */
} RtRedirectState;

/* Detailed allocation tracking entry */
typedef struct RtAllocTrackEntry {
    void *ptr;                      /* Allocated pointer */
    size_t size;                    /* Allocation size */
    void *caller;                   /* Return address of caller (if available) */
    bool freed;                     /* Has this been "freed"? */
    struct RtAllocTrackEntry *next;
} RtAllocTrackEntry;

/* ============================================================================
 * Statistics Structure
 * ============================================================================ */

typedef struct {
    size_t alloc_count;             /* Number of allocations */
    size_t free_count;              /* Number of free calls */
    size_t realloc_count;           /* Number of realloc calls */
    size_t total_requested;         /* Total bytes requested */
    size_t total_allocated;         /* Total bytes allocated (including headers) */
    size_t fallback_count;          /* Number of fallbacks to system malloc */
    size_t current_live;            /* Current live allocations */
    size_t peak_live;               /* Peak live allocations */
    size_t hash_set_entries;        /* Entries in allocation hash set */
    size_t track_entries;           /* Entries in tracking list (if enabled) */
} RtRedirectStats;

/* ============================================================================
 * API: Enable/Disable
 * ============================================================================ */

/* Push a new redirect scope. All subsequent malloc calls use this arena.
 * Supports nesting - each push saves previous state.
 * Config can be NULL for defaults.
 * Returns true on success, false on failure. */
bool rt_malloc_redirect_push(RtArena *arena, const RtRedirectConfig *config);

/* Pop current redirect scope. Restores previous state (or disables if top-level).
 * Returns true if there was a scope to pop, false if not redirecting. */
bool rt_malloc_redirect_pop(void);

/* Check if redirection is currently active for this thread */
bool rt_malloc_redirect_is_active(void);

/* Get the current redirect arena (NULL if not redirecting) */
RtArena *rt_malloc_redirect_arena(void);

/* Get current nesting depth (0 = not redirecting) */
int rt_malloc_redirect_depth(void);

/* ============================================================================
 * API: Statistics
 * ============================================================================ */

/* Get current statistics for the active redirect scope.
 * Returns false if not redirecting. */
bool rt_malloc_redirect_get_stats(RtRedirectStats *stats);

/* Reset statistics for current scope */
void rt_malloc_redirect_reset_stats(void);

/* Print statistics to stderr (for debugging) */
void rt_malloc_redirect_print_stats(void);

/* ============================================================================
 * API: Pointer Queries
 * ============================================================================ */

/* Check if a pointer was allocated from the current redirect arena.
 * Returns false if not redirecting or pointer not from arena. */
bool rt_malloc_redirect_is_arena_ptr(void *ptr);

/* Get allocation size for an arena pointer.
 * Returns 0 if not an arena pointer. */
size_t rt_malloc_redirect_ptr_size(void *ptr);

/* ============================================================================
 * API: Allocation Tracking (when track_allocations is enabled)
 * ============================================================================ */

/* Callback type for iterating tracked allocations */
typedef void (*RtAllocTrackCallback)(void *ptr, size_t size, bool freed,
                                      void *caller, void *user_data);

/* Iterate all tracked allocations in current scope.
 * Callback is called for each allocation.
 * Returns number of allocations visited. */
size_t rt_malloc_redirect_track_iterate(RtAllocTrackCallback callback,
                                         void *user_data);

/* Get list of "leaked" allocations (allocated but not freed).
 * Returns count, fills ptrs/sizes arrays up to max_count.
 * Only useful when track_allocations is enabled. */
size_t rt_malloc_redirect_track_leaks(void **ptrs, size_t *sizes,
                                       size_t max_count);

/* Print tracked allocations to stderr */
void rt_malloc_redirect_track_print(void);

/* ============================================================================
 * API: Hook Installation (called automatically via constructor)
 * ============================================================================ */

/* Install malloc hooks. Called automatically at program startup.
 * Can be called manually if constructor doesn't run. */
void rt_malloc_redirect_install_hooks(void);

/* Uninstall malloc hooks. Called automatically at program exit. */
void rt_malloc_redirect_uninstall_hooks(void);

/* Check if hooks are installed */
bool rt_malloc_redirect_hooks_installed(void);

/* ============================================================================
 * Internal: Hash Set Operations (exposed for testing)
 * ============================================================================ */

RtAllocHashSet *rt_alloc_hash_set_create(size_t initial_buckets);
void rt_alloc_hash_set_destroy(RtAllocHashSet *set);
bool rt_alloc_hash_set_insert(RtAllocHashSet *set, void *ptr, size_t size);
bool rt_alloc_hash_set_remove(RtAllocHashSet *set, void *ptr);
bool rt_alloc_hash_set_contains(RtAllocHashSet *set, void *ptr);
size_t rt_alloc_hash_set_get_size(RtAllocHashSet *set, void *ptr);

#endif /* RUNTIME_MALLOC_REDIRECT_H */
