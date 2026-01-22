/*
 * Arena-Redirected Malloc Implementation
 *
 * See runtime_malloc_redirect.h for full documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "runtime_malloc_redirect.h"
#include "runtime_arena.h"

#ifdef SN_MALLOC_REDIRECT

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

/* Thread-local redirect state stack (NULL = not redirecting) */
static __thread RtRedirectState *tls_redirect_state = NULL;

/* Thread-local guard to prevent recursive hook calls */
static __thread int tls_hook_guard = 0;

/* Original function pointers - populated by hooking libraries */
static void *(*orig_malloc)(size_t) = NULL;
static void (*orig_free)(void *) = NULL;
static void *(*orig_calloc)(size_t, size_t) = NULL;
static void *(*orig_realloc)(void *, size_t) = NULL;

/* Hook installation state */
static bool hooks_installed = false;

/* ============================================================================
 * Hash Set Implementation
 * ============================================================================ */

/* Hash function for pointers */
static size_t hash_ptr(void *ptr, size_t bucket_count)
{
    uintptr_t addr = (uintptr_t)ptr;
    /* Mix bits for better distribution */
    addr ^= addr >> 17;
    addr *= 0xed5ad4bbU;
    addr ^= addr >> 11;
    addr *= 0xac4c1b51U;
    addr ^= addr >> 15;
    return addr % bucket_count;
}

RtAllocHashSet *rt_alloc_hash_set_create(size_t initial_buckets)
{
    if (initial_buckets == 0) {
        initial_buckets = 256;
    }

    /* Use system malloc for hash set internals (not redirected) */
    RtAllocHashSet *set;
    if (orig_malloc) {
        set = orig_malloc(sizeof(RtAllocHashSet));
    } else {
        set = malloc(sizeof(RtAllocHashSet));
    }
    if (!set) return NULL;

    if (orig_calloc) {
        set->buckets = orig_calloc(initial_buckets, sizeof(RtAllocHashEntry *));
    } else {
        set->buckets = calloc(initial_buckets, sizeof(RtAllocHashEntry *));
    }
    if (!set->buckets) {
        if (orig_free) orig_free(set);
        else free(set);
        return NULL;
    }

    set->bucket_count = initial_buckets;
    set->entry_count = 0;
    set->grow_threshold = initial_buckets * 3 / 4;  /* 75% load factor */

    return set;
}

void rt_alloc_hash_set_destroy(RtAllocHashSet *set)
{
    if (!set) return;

    /* Free all entries */
    for (size_t i = 0; i < set->bucket_count; i++) {
        RtAllocHashEntry *entry = set->buckets[i];
        while (entry) {
            RtAllocHashEntry *next = entry->next;
            if (orig_free) orig_free(entry);
            else free(entry);
            entry = next;
        }
    }

    if (orig_free) {
        orig_free(set->buckets);
        orig_free(set);
    } else {
        free(set->buckets);
        free(set);
    }
}

/* Rehash to larger size */
static bool hash_set_grow(RtAllocHashSet *set)
{
    size_t new_bucket_count = set->bucket_count * 2;
    RtAllocHashEntry **new_buckets;

    if (orig_calloc) {
        new_buckets = orig_calloc(new_bucket_count, sizeof(RtAllocHashEntry *));
    } else {
        new_buckets = calloc(new_bucket_count, sizeof(RtAllocHashEntry *));
    }
    if (!new_buckets) return false;

    /* Rehash all entries */
    for (size_t i = 0; i < set->bucket_count; i++) {
        RtAllocHashEntry *entry = set->buckets[i];
        while (entry) {
            RtAllocHashEntry *next = entry->next;
            size_t new_idx = hash_ptr(entry->ptr, new_bucket_count);
            entry->next = new_buckets[new_idx];
            new_buckets[new_idx] = entry;
            entry = next;
        }
    }

    if (orig_free) orig_free(set->buckets);
    else free(set->buckets);

    set->buckets = new_buckets;
    set->bucket_count = new_bucket_count;
    set->grow_threshold = new_bucket_count * 3 / 4;

    return true;
}

bool rt_alloc_hash_set_insert(RtAllocHashSet *set, void *ptr, size_t size)
{
    if (!set || !ptr) return false;

    /* Grow if needed */
    if (set->entry_count >= set->grow_threshold) {
        hash_set_grow(set);  /* Best effort - continue even if grow fails */
    }

    size_t idx = hash_ptr(ptr, set->bucket_count);

    /* Check if already exists */
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) {
            e->size = size;  /* Update size */
            return true;
        }
    }

    /* Create new entry */
    RtAllocHashEntry *entry;
    if (orig_malloc) {
        entry = orig_malloc(sizeof(RtAllocHashEntry));
    } else {
        entry = malloc(sizeof(RtAllocHashEntry));
    }
    if (!entry) return false;

    entry->ptr = ptr;
    entry->size = size;
    entry->next = set->buckets[idx];
    set->buckets[idx] = entry;
    set->entry_count++;

    return true;
}

bool rt_alloc_hash_set_remove(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return false;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    RtAllocHashEntry **prev = &set->buckets[idx];

    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) {
            *prev = e->next;
            if (orig_free) orig_free(e);
            else free(e);
            set->entry_count--;
            return true;
        }
        prev = &e->next;
    }

    return false;
}

bool rt_alloc_hash_set_contains(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return false;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) return true;
    }

    return false;
}

size_t rt_alloc_hash_set_get_size(RtAllocHashSet *set, void *ptr)
{
    if (!set || !ptr) return 0;

    size_t idx = hash_ptr(ptr, set->bucket_count);
    for (RtAllocHashEntry *e = set->buckets[idx]; e; e = e->next) {
        if (e->ptr == ptr) return e->size;
    }

    return 0;
}

/* ============================================================================
 * Redirect State Management
 * ============================================================================ */

bool rt_malloc_redirect_push(RtArena *arena, const RtRedirectConfig *config)
{
    if (!arena) return false;

    /* Allocate new state using system malloc */
    RtRedirectState *state;
    if (orig_malloc) {
        state = orig_malloc(sizeof(RtRedirectState));
    } else {
        state = malloc(sizeof(RtRedirectState));
    }
    if (!state) return false;

    memset(state, 0, sizeof(RtRedirectState));

    state->active = true;
    state->arena = arena;
    state->prev = tls_redirect_state;

    /* Apply config */
    if (config) {
        state->config = *config;
    } else {
        RtRedirectConfig default_config = RT_REDIRECT_CONFIG_DEFAULT;
        state->config = default_config;
    }

    /* Create hash set for allocation tracking */
    state->alloc_set = rt_alloc_hash_set_create(256);
    if (!state->alloc_set) {
        if (orig_free) orig_free(state);
        else free(state);
        return false;
    }

    /* Create mutex if thread-safe mode requested */
    if (state->config.thread_safe) {
        if (orig_malloc) {
            state->mutex = orig_malloc(sizeof(pthread_mutex_t));
        } else {
            state->mutex = malloc(sizeof(pthread_mutex_t));
        }
        if (state->mutex) {
            pthread_mutex_init(state->mutex, NULL);
        }
    }

    /* Push onto stack */
    tls_redirect_state = state;

    return true;
}

bool rt_malloc_redirect_pop(void)
{
    RtRedirectState *state = tls_redirect_state;
    if (!state) return false;

    /* Pop from stack */
    tls_redirect_state = state->prev;

    /* Clean up */
    if (state->alloc_set) {
        rt_alloc_hash_set_destroy(state->alloc_set);
    }

    if (state->mutex) {
        pthread_mutex_destroy(state->mutex);
        if (orig_free) orig_free(state->mutex);
        else free(state->mutex);
    }

    /* Free tracking entries */
    RtAllocTrackEntry *track = state->track_head;
    while (track) {
        RtAllocTrackEntry *next = track->next;
        if (orig_free) orig_free(track);
        else free(track);
        track = next;
    }

    if (orig_free) orig_free(state);
    else free(state);

    return true;
}

bool rt_malloc_redirect_is_active(void)
{
    return tls_redirect_state != NULL && tls_redirect_state->active;
}

RtArena *rt_malloc_redirect_arena(void)
{
    if (!tls_redirect_state) return NULL;
    return tls_redirect_state->arena;
}

int rt_malloc_redirect_depth(void)
{
    int depth = 0;
    for (RtRedirectState *s = tls_redirect_state; s; s = s->prev) {
        depth++;
    }
    return depth;
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

bool rt_malloc_redirect_get_stats(RtRedirectStats *stats)
{
    if (!stats || !tls_redirect_state) return false;

    RtRedirectState *state = tls_redirect_state;
    stats->alloc_count = state->alloc_count;
    stats->free_count = state->free_count;
    stats->realloc_count = state->realloc_count;
    stats->total_requested = state->total_requested;
    stats->total_allocated = state->total_allocated;
    stats->fallback_count = state->fallback_count;
    stats->current_live = state->current_live;
    stats->peak_live = state->peak_live;
    stats->hash_set_entries = state->alloc_set ? state->alloc_set->entry_count : 0;

    /* Count tracking entries */
    stats->track_entries = 0;
    for (RtAllocTrackEntry *e = state->track_head; e; e = e->next) {
        stats->track_entries++;
    }

    return true;
}

void rt_malloc_redirect_reset_stats(void)
{
    if (!tls_redirect_state) return;

    RtRedirectState *state = tls_redirect_state;
    state->alloc_count = 0;
    state->free_count = 0;
    state->realloc_count = 0;
    state->total_requested = 0;
    state->total_allocated = 0;
    state->fallback_count = 0;
    /* Don't reset current_live or peak_live - they track actual state */
}

void rt_malloc_redirect_print_stats(void)
{
    RtRedirectStats stats;
    if (!rt_malloc_redirect_get_stats(&stats)) {
        fprintf(stderr, "[REDIRECT] Not active\n");
        return;
    }

    fprintf(stderr, "[REDIRECT] Statistics:\n");
    fprintf(stderr, "  Allocations:   %zu\n", stats.alloc_count);
    fprintf(stderr, "  Frees:         %zu\n", stats.free_count);
    fprintf(stderr, "  Reallocs:      %zu\n", stats.realloc_count);
    fprintf(stderr, "  Requested:     %zu bytes\n", stats.total_requested);
    fprintf(stderr, "  Allocated:     %zu bytes (with headers)\n", stats.total_allocated);
    fprintf(stderr, "  Fallbacks:     %zu\n", stats.fallback_count);
    fprintf(stderr, "  Current live:  %zu\n", stats.current_live);
    fprintf(stderr, "  Peak live:     %zu\n", stats.peak_live);
    fprintf(stderr, "  Hash entries:  %zu\n", stats.hash_set_entries);
    if (stats.track_entries > 0) {
        fprintf(stderr, "  Track entries: %zu\n", stats.track_entries);
    }
}

/* ============================================================================
 * Pointer Queries
 * ============================================================================ */

bool rt_malloc_redirect_is_arena_ptr(void *ptr)
{
    if (!ptr || !tls_redirect_state) return false;
    return rt_alloc_hash_set_contains(tls_redirect_state->alloc_set, ptr);
}

size_t rt_malloc_redirect_ptr_size(void *ptr)
{
    if (!ptr || !tls_redirect_state) return 0;
    return rt_alloc_hash_set_get_size(tls_redirect_state->alloc_set, ptr);
}

/* ============================================================================
 * Allocation Tracking
 * ============================================================================ */

/* Prevent compiler from reordering or eliminating tracking code */
#if defined(__GNUC__) || defined(__clang__)
#define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
#define NOINLINE __attribute__((noinline))
#else
#define COMPILER_BARRIER() do {} while(0)
#define NOINLINE
#endif

static NOINLINE void track_allocation(RtRedirectState *state, void *ptr, size_t size, void *caller)
{
    /* Force memory read - prevent compiler from optimizing away config check */
    COMPILER_BARRIER();
    bool should_track = state->config.track_allocations;
    COMPILER_BARRIER();
    if (!should_track) return;

    RtAllocTrackEntry *entry;
    if (orig_malloc) {
        entry = orig_malloc(sizeof(RtAllocTrackEntry));
    } else {
        entry = malloc(sizeof(RtAllocTrackEntry));
    }
    if (!entry) return;

    entry->ptr = ptr;
    entry->size = size;
    entry->caller = caller;
    entry->freed = false;

    /* Memory barrier to ensure proper ordering of linked list update */
    COMPILER_BARRIER();
    entry->next = state->track_head;
    COMPILER_BARRIER();
    state->track_head = entry;
    COMPILER_BARRIER();
}

static NOINLINE void track_free(RtRedirectState *state, void *ptr)
{
    if (!state->config.track_allocations) return;

    for (RtAllocTrackEntry *e = state->track_head; e; e = e->next) {
        if (e->ptr == ptr && !e->freed) {
            e->freed = true;
            return;
        }
    }
}

size_t rt_malloc_redirect_track_iterate(RtAllocTrackCallback callback, void *user_data)
{
    if (!callback || !tls_redirect_state) return 0;

    size_t count = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        callback(e->ptr, e->size, e->freed, e->caller, user_data);
        count++;
    }

    return count;
}

size_t rt_malloc_redirect_track_leaks(void **ptrs, size_t *sizes, size_t max_count)
{
    if (!tls_redirect_state) return 0;

    size_t count = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        if (!e->freed) {
            if (count < max_count) {
                if (ptrs) ptrs[count] = e->ptr;
                if (sizes) sizes[count] = e->size;
            }
            count++;
        }
    }

    return count;
}

void rt_malloc_redirect_track_print(void)
{
    if (!tls_redirect_state || !tls_redirect_state->config.track_allocations) {
        fprintf(stderr, "[REDIRECT] Tracking not enabled\n");
        return;
    }

    fprintf(stderr, "[REDIRECT] Tracked allocations:\n");
    size_t live = 0, freed = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        fprintf(stderr, "  %p: %zu bytes %s", e->ptr, e->size,
                e->freed ? "[freed]" : "[live]");
        if (e->caller) {
            fprintf(stderr, " (caller: %p)", e->caller);
        }
        fprintf(stderr, "\n");

        if (e->freed) freed++;
        else live++;
    }
    fprintf(stderr, "  Total: %zu live, %zu freed\n", live, freed);
}

/* ============================================================================
 * Hooked Functions Implementation
 * ============================================================================ */

static NOINLINE void *redirected_malloc(size_t size)
{
    RtRedirectState *state = tls_redirect_state;

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_malloc) return orig_malloc(size);
        return malloc(size);
    }

    /* Check max size limit */
    if (state->config.max_arena_size > 0) {
        size_t current = rt_arena_total_allocated(state->arena);
        if (current + size + sizeof(RtAllocHeader) > state->config.max_arena_size) {
            switch (state->config.overflow_policy) {
            case RT_REDIRECT_OVERFLOW_GROW:
                /* Continue anyway */
                break;
            case RT_REDIRECT_OVERFLOW_FALLBACK:
                state->fallback_count++;
                if (orig_malloc) return orig_malloc(size);
                return malloc(size);
            case RT_REDIRECT_OVERFLOW_FAIL:
                return NULL;
            case RT_REDIRECT_OVERFLOW_PANIC:
                if (state->config.on_overflow) {
                    state->config.on_overflow(state->arena, size,
                                               state->config.callback_user_data);
                }
                fprintf(stderr, "[REDIRECT] Arena overflow: requested %zu, "
                        "current %zu, max %zu\n",
                        size, current, state->config.max_arena_size);
                abort();
            }
        }
    }

    /* IMPORTANT: Set hook guard BEFORE any operations that might call malloc,
     * including pthread_mutex_lock which may allocate internally.
     * This prevents infinite recursion when pthread or arena calls malloc. */
    tls_hook_guard = 1;

    /* Lock if thread-safe */
    if (state->mutex) {
        pthread_mutex_lock(state->mutex);
    }

    /* Allocate with header */
    size_t total_size = sizeof(RtAllocHeader) + size;
    void *raw = rt_arena_alloc(state->arena, total_size);

    if (state->mutex) {
        pthread_mutex_unlock(state->mutex);
    }

    tls_hook_guard = 0;

    if (!raw) {
        /* Arena allocation failed */
        if (state->config.overflow_policy == RT_REDIRECT_OVERFLOW_FALLBACK) {
            state->fallback_count++;
            if (orig_malloc) return orig_malloc(size);
            return malloc(size);
        }
        return NULL;
    }

    /* Fill in header */
    RtAllocHeader *header = (RtAllocHeader *)raw;
    header->size = size;
    header->magic = RT_ALLOC_MAGIC;
    header->flags = 0;

    void *user_ptr = header + 1;  /* Pointer to user data */

    /* Add to hash set */
    rt_alloc_hash_set_insert(state->alloc_set, user_ptr, size);

    /* Update stats */
    state->alloc_count++;
    state->total_requested += size;
    state->total_allocated += total_size;
    state->current_live++;
    if (state->current_live > state->peak_live) {
        state->peak_live = state->current_live;
    }

    /* Track allocation */
    void *caller = NULL;
#if defined(__GNUC__) || defined(__clang__)
    caller = __builtin_return_address(0);
#endif
    track_allocation(state, user_ptr, size, caller);

    /* Callback */
    if (state->config.on_alloc) {
        tls_hook_guard = 1;
        state->config.on_alloc(user_ptr, size, state->config.callback_user_data);
        tls_hook_guard = 0;
    }

    return user_ptr;
}

static void redirected_free(void *ptr)
{
    if (!ptr) return;

    RtRedirectState *state = tls_redirect_state;

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_free) orig_free(ptr);
        else free(ptr);
        return;
    }

    /* Check if this is an arena pointer */
    if (!rt_alloc_hash_set_contains(state->alloc_set, ptr)) {
        /* Not our pointer - pass through to real free */
        if (orig_free) orig_free(ptr);
        else free(ptr);
        return;
    }

    /* Get size from header for stats/callbacks */
    RtAllocHeader *header = ((RtAllocHeader *)ptr) - 1;
    size_t size = 0;
    if (header->magic == RT_ALLOC_MAGIC) {
        size = header->size;
    }

    /* Apply free policy */
    switch (state->config.free_policy) {
    case RT_REDIRECT_FREE_IGNORE:
        /* Do nothing */
        break;

    case RT_REDIRECT_FREE_TRACK:
        track_free(state, ptr);
        break;

    case RT_REDIRECT_FREE_WARN:
        tls_hook_guard = 1;
        fprintf(stderr, "[REDIRECT] Warning: free(%p) called on arena memory "
                "(size=%zu)\n", ptr, size);
        tls_hook_guard = 0;
        break;

    case RT_REDIRECT_FREE_ERROR:
        tls_hook_guard = 1;
        fprintf(stderr, "[REDIRECT] Error: free(%p) called on arena memory\n", ptr);
        tls_hook_guard = 0;
        abort();
    }

    /* Zero memory if requested */
    if (state->config.zero_on_free && size > 0) {
        memset(ptr, 0, size);
    }

    /* Remove from hash set */
    rt_alloc_hash_set_remove(state->alloc_set, ptr);

    /* Update stats */
    state->free_count++;
    if (state->current_live > 0) {
        state->current_live--;
    }

    /* Callback */
    if (state->config.on_free) {
        tls_hook_guard = 1;
        state->config.on_free(ptr, size, state->config.callback_user_data);
        tls_hook_guard = 0;
    }

    /* Note: Memory is NOT actually freed - it remains in the arena
     * and will be freed when the arena is destroyed */
}

static void *redirected_calloc(size_t count, size_t size)
{
    size_t total = count * size;

    /* Check for overflow */
    if (size != 0 && total / size != count) {
        return NULL;
    }

    void *ptr = redirected_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

static void *redirected_realloc(void *ptr, size_t new_size)
{
    RtRedirectState *state = tls_redirect_state;

    /* Case 1: ptr is NULL - equivalent to malloc */
    if (ptr == NULL) {
        return redirected_malloc(new_size);
    }

    /* Case 2: new_size is 0 - equivalent to free */
    if (new_size == 0) {
        redirected_free(ptr);
        return NULL;
    }

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_realloc) return orig_realloc(ptr, new_size);
        return realloc(ptr, new_size);
    }

    /* Case 3: Check if ptr is from arena */
    if (!rt_alloc_hash_set_contains(state->alloc_set, ptr)) {
        /* Not our pointer - use real realloc */
        if (orig_realloc) return orig_realloc(ptr, new_size);
        return realloc(ptr, new_size);
    }

    state->realloc_count++;

    /* Get original size from header */
    RtAllocHeader *header = ((RtAllocHeader *)ptr) - 1;
    size_t old_size = 0;
    if (header->magic == RT_ALLOC_MAGIC) {
        old_size = header->size;
    } else {
        /* Fallback: try hash set */
        old_size = rt_alloc_hash_set_get_size(state->alloc_set, ptr);
    }

    /* Case 4: Shrinking - just update size metadata */
    if (new_size <= old_size) {
        header->size = new_size;
        rt_alloc_hash_set_insert(state->alloc_set, ptr, new_size);
        return ptr;
    }

    /* Case 5: Growing - allocate new and copy */
    void *new_ptr = redirected_malloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* Copy old data */
    memcpy(new_ptr, ptr, old_size);

    /* Zero old memory if requested */
    if (state->config.zero_on_free) {
        memset(ptr, 0, old_size);
    }

    /* Remove old from hash set (don't update stats - not a real free) */
    rt_alloc_hash_set_remove(state->alloc_set, ptr);

    /* Track the "free" of old pointer */
    track_free(state, ptr);

    return new_ptr;
}

/* ============================================================================
 * Wrapper functions that call the redirected implementations
 * ============================================================================ */

static void *hooked_malloc(size_t size)
{
    return redirected_malloc(size);
}

static void hooked_free(void *ptr)
{
    redirected_free(ptr);
}

static void *hooked_calloc(size_t count, size_t size)
{
    return redirected_calloc(count, size);
}

static void *hooked_realloc(void *ptr, size_t size)
{
    return redirected_realloc(ptr, size);
}

/* ============================================================================
 * Platform-specific hook installation
 * ============================================================================ */

bool rt_malloc_redirect_hooks_installed(void)
{
    return hooks_installed;
}

/* ============================================================================
 * macOS: fishhook-based hooking
 * ============================================================================ */

#ifdef __APPLE__

#include "fishhook.h"

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    struct rebinding rebindings[] = {
        {"malloc", (void *)hooked_malloc, (void **)&orig_malloc},
        {"free", (void *)hooked_free, (void **)&orig_free},
        {"calloc", (void *)hooked_calloc, (void **)&orig_calloc},
        {"realloc", (void *)hooked_realloc, (void **)&orig_realloc},
    };
    rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    /* fishhook doesn't support unhooking */
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

/* ============================================================================
 * Linux: plthook-based hooking
 * ============================================================================ */

#elif defined(__linux__)

#include "plthook.h"
#include <link.h>

#define MAX_HOOKED_LIBS 64
static plthook_t *plthooks[MAX_HOOKED_LIBS];
static int plthook_count = 0;

static void hook_library(plthook_t *ph)
{
    void *tmp_orig = NULL;
    int rv;

    rv = plthook_replace(ph, "malloc", (void *)hooked_malloc, &tmp_orig);
    if (rv == 0 && orig_malloc == NULL && tmp_orig != NULL) {
        orig_malloc = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "free", (void *)hooked_free, &tmp_orig);
    if (rv == 0 && orig_free == NULL && tmp_orig != NULL) {
        orig_free = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "calloc", (void *)hooked_calloc, &tmp_orig);
    if (rv == 0 && orig_calloc == NULL && tmp_orig != NULL) {
        orig_calloc = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "realloc", (void *)hooked_realloc, &tmp_orig);
    if (rv == 0 && orig_realloc == NULL && tmp_orig != NULL) {
        orig_realloc = tmp_orig;
    }
}

static int hook_library_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;

    if (plthook_count >= MAX_HOOKED_LIBS) return 0;

    plthook_t *ph = NULL;
    const char *name = info->dlpi_name;
    int rv;

    if (name == NULL || name[0] == '\0') {
        rv = plthook_open(&ph, NULL);
    } else {
        rv = plthook_open(&ph, name);
    }

    if (rv != 0) return 0;

    hook_library(ph);
    plthooks[plthook_count++] = ph;

    return 0;
}

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    dl_iterate_phdr(hook_library_callback, NULL);
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    for (int i = 0; i < plthook_count; i++) {
        if (plthooks[i] != NULL) {
            plthook_close(plthooks[i]);
            plthooks[i] = NULL;
        }
    }
    plthook_count = 0;
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

__attribute__((destructor))
static void auto_uninstall_hooks(void)
{
    rt_malloc_redirect_uninstall_hooks();
}

/* ============================================================================
 * Windows: MinHook-based hooking
 * ============================================================================ */

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "minhook/MinHook.h"

static void *get_crt_function(const char *name)
{
    HMODULE hCRT = GetModuleHandleA("ucrtbase.dll");
    if (!hCRT) hCRT = GetModuleHandleA("msvcrt.dll");
    if (!hCRT) hCRT = GetModuleHandleA("api-ms-win-crt-heap-l1-1-0.dll");
    if (hCRT) return (void *)GetProcAddress(hCRT, name);
    return NULL;
}

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return;

    void *crt_malloc = get_crt_function("malloc");
    void *crt_free = get_crt_function("free");
    void *crt_calloc = get_crt_function("calloc");
    void *crt_realloc = get_crt_function("realloc");

    if (crt_malloc) {
        MH_CreateHook(crt_malloc, hooked_malloc, (void **)&orig_malloc);
    }
    if (crt_free) {
        MH_CreateHook(crt_free, hooked_free, (void **)&orig_free);
    }
    if (crt_calloc) {
        MH_CreateHook(crt_calloc, hooked_calloc, (void **)&orig_calloc);
    }
    if (crt_realloc) {
        MH_CreateHook(crt_realloc, hooked_realloc, (void **)&orig_realloc);
    }

    MH_EnableHook(MH_ALL_HOOKS);
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

__attribute__((destructor))
static void auto_uninstall_hooks(void)
{
    rt_malloc_redirect_uninstall_hooks();
}

#else
#error "Unsupported platform for malloc redirect"
#endif

#endif /* SN_MALLOC_REDIRECT */
