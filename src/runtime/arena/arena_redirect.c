/*
 * Arena Redirect - Malloc Redirection Implementation
 * ===================================================
 * Redirects malloc/free/realloc to arena allocation using
 * runtime malloc hooks.
 */

#include "arena_redirect.h"
#include "arena_v2.h"
#include "../malloc/runtime_malloc_hooks.h"

#include <stdlib.h>
#include <stdbool.h>
#include "arena_compat.h"

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

static __thread RtArenaV2 *tls_redirect_stack[16];
static __thread int tls_redirect_depth = 0;

/* ============================================================================
 * Ptr → Handle Hash Map for Redirect Tracking
 * ============================================================================
 * When malloc is redirected to arena, we need to find the handle from the
 * raw pointer for free/realloc. This hash map provides O(1) lookup.
 * Thread-local since each thread has its own redirect stack.
 */

#define PTR_MAP_BUCKETS 256

typedef struct PtrMapEntry {
    void *ptr;
    RtHandleV2 *handle;
    struct PtrMapEntry *next;
} PtrMapEntry;

static __thread PtrMapEntry *tls_ptr_map[PTR_MAP_BUCKETS];
static __thread bool tls_cleanup_registered = false;

/* pthread_key for thread exit cleanup */
static pthread_key_t tls_cleanup_key;
static pthread_once_t tls_cleanup_once = PTHREAD_ONCE_INIT;

/* Clear all entries from the ptr_map, marking handles as dead */
static void ptr_map_clear_all(void)
{
    for (size_t i = 0; i < PTR_MAP_BUCKETS; i++) {
        PtrMapEntry *entry = tls_ptr_map[i];
        while (entry) {
            PtrMapEntry *next = entry->next;
            /* Mark the handle as dead so GC can reclaim it */
            if (entry->handle) {
                rt_arena_v2_free(entry->handle);
            }
            free(entry);
            entry = next;
        }
        tls_ptr_map[i] = NULL;
    }
}

/* Thread exit destructor - called when thread terminates */
static void tls_thread_cleanup(void *arg)
{
    (void)arg;

    /* Clear the ptr_map entries */
    ptr_map_clear_all();

    /* Clear redirect state */
    tls_redirect_depth = 0;
    rt_malloc_hooks_clear_handler();
}

/* Initialize the pthread_key (called once per process) */
static void tls_cleanup_init(void)
{
    pthread_key_create(&tls_cleanup_key, tls_thread_cleanup);
}

/* Register cleanup for this thread (called on first redirect_push) */
static void tls_ensure_cleanup_registered(void)
{
    if (!tls_cleanup_registered) {
        pthread_once(&tls_cleanup_once, tls_cleanup_init);
        /* Set a non-NULL value to trigger destructor on thread exit */
        pthread_setspecific(tls_cleanup_key, (void *)1);
        tls_cleanup_registered = true;
    }
}

static inline size_t ptr_hash(void *ptr)
{
    return ((size_t)ptr >> 4) % PTR_MAP_BUCKETS;
}

static void ptr_map_insert(void *ptr, RtHandleV2 *handle)
{
    size_t bucket = ptr_hash(ptr);
    PtrMapEntry *entry = malloc(sizeof(PtrMapEntry));
    if (entry == NULL) return;
    entry->ptr = ptr;
    entry->handle = handle;
    entry->next = tls_ptr_map[bucket];
    tls_ptr_map[bucket] = entry;
}

static RtHandleV2 *ptr_map_find(void *ptr)
{
    size_t bucket = ptr_hash(ptr);
    PtrMapEntry *entry = tls_ptr_map[bucket];
    while (entry) {
        if (entry->ptr == ptr) return entry->handle;
        entry = entry->next;
    }
    return NULL;
}

static RtHandleV2 *ptr_map_remove(void *ptr)
{
    size_t bucket = ptr_hash(ptr);
    PtrMapEntry **pp = &tls_ptr_map[bucket];
    while (*pp) {
        if ((*pp)->ptr == ptr) {
            PtrMapEntry *entry = *pp;
            RtHandleV2 *handle = entry->handle;
            *pp = entry->next;
            free(entry);
            return handle;
        }
        pp = &(*pp)->next;
    }
    return NULL;
}

/* ============================================================================
 * Malloc Handler Functions
 * ============================================================================ */

static void *arena_v2_malloc_handler(size_t size, bool *handled, void *user_data)
{
    (void)user_data;

    RtArenaV2 *arena = rt_arena_v2_redirect_current();
    if (arena == NULL) {
        *handled = false;
        return NULL;
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, size);
    if (handle == NULL) {
        *handled = true;  /* We tried, allocation failed */
        return NULL;
    }

    /* Track ptr → handle for free/realloc */
    ptr_map_insert(handle->ptr, handle);

    *handled = true;
    return handle->ptr;
}

static void arena_v2_free_handler(void *ptr, bool *handled, void *user_data)
{
    (void)user_data;

    /* Check if this ptr came from our arena */
    RtHandleV2 *handle = ptr_map_find(ptr);
    if (handle == NULL) {
        *handled = false;  /* Not ours, pass through */
        return;
    }

    /* Remove from map and mark handle as dead */
    ptr_map_remove(ptr);
    rt_arena_v2_free(handle);

    *handled = true;
}

static void *arena_v2_realloc_handler(void *ptr, size_t size, bool *handled, void *user_data)
{
    (void)user_data;

    /* realloc(NULL, size) == malloc(size) */
    if (ptr == NULL) {
        return arena_v2_malloc_handler(size, handled, user_data);
    }

    /* realloc(ptr, 0) == free(ptr) */
    if (size == 0) {
        arena_v2_free_handler(ptr, handled, user_data);
        return NULL;
    }

    /* Check if this ptr came from our arena */
    RtHandleV2 *old_handle = ptr_map_find(ptr);
    if (old_handle == NULL) {
        *handled = false;  /* Not ours, pass through */
        return NULL;
    }

    RtArenaV2 *arena = old_handle->arena;

    /* Allocate new */
    RtHandleV2 *new_handle = rt_arena_v2_realloc(arena, old_handle, size);
    if (new_handle == NULL) {
        *handled = true;
        return NULL;
    }

    /* Update tracking - remove old, add new */
    ptr_map_remove(ptr);
    ptr_map_insert(new_handle->ptr, new_handle);

    *handled = true;
    return new_handle->ptr;
}

/* Thread-local handler struct */
static __thread RtMallocHandler tls_arena_v2_handler = {
    .malloc_fn = arena_v2_malloc_handler,
    .free_fn = arena_v2_free_handler,
    .realloc_fn = arena_v2_realloc_handler,
    .user_data = NULL
};

/* ============================================================================
 * Public API
 * ============================================================================ */

void rt_arena_v2_redirect_push(RtArenaV2 *arena)
{
    if (tls_redirect_depth < 16) {
        tls_redirect_stack[tls_redirect_depth++] = arena;

        /* Register handler with hooks on first push */
        if (tls_redirect_depth == 1) {
            /* Ensure thread cleanup is registered for when thread exits */
            tls_ensure_cleanup_registered();
            rt_malloc_hooks_set_handler(&tls_arena_v2_handler);
        }
    }
}

void rt_arena_v2_redirect_pop(void)
{
    if (tls_redirect_depth > 0) {
        tls_redirect_depth--;

        /* Unregister handler when stack empty */
        if (tls_redirect_depth == 0) {
            rt_malloc_hooks_clear_handler();
        }
    }
}

RtArenaV2 *rt_arena_v2_redirect_current(void)
{
    if (tls_redirect_depth > 0) {
        return tls_redirect_stack[tls_redirect_depth - 1];
    }
    return NULL;
}
