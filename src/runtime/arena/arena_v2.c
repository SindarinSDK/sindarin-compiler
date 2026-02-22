/*
 * Arena V2 - Core Implementation
 * ===============================
 */

#include "arena_v2.h"
#include "arena_handle.h"
#include "arena_id.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "arena_compat.h"

/* ============================================================================
 * Debug Logging (enabled via RT_ARENA_DEBUG=1 environment variable)
 * ============================================================================ */

int arena_debug_initialized = 0;
int arena_debug_enabled = 0;

void arena_debug_init(void)
{
    if (arena_debug_initialized) return;
    arena_debug_initialized = 1;
    const char *debug_env = getenv("RT_ARENA_DEBUG");
    arena_debug_enabled = (debug_env != NULL && debug_env[0] == '1');
}

#define ARENA_DEBUG_LOG(fmt, ...) do { \
    if (!arena_debug_initialized) arena_debug_init(); \
    if (arena_debug_enabled) { \
        fprintf(stderr, "[ARENA:T%lu] " fmt "\n", (unsigned long)(uintptr_t)pthread_self(), ##__VA_ARGS__); \
        fflush(stderr); \
    } \
} while(0)

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

static __thread RtArenaV2 *tls_current_arena = NULL;

/* ============================================================================
 * Internal: Handle Management
 * ============================================================================ */

static RtHandleV2 *handle_create(RtArenaV2 *arena, void *ptr, size_t size)
{
    RtHandleV2 *handle = malloc(sizeof(RtHandleV2));
    if (handle == NULL) {
        fprintf(stderr, "arena_v2: handle allocation failed\n");
        return NULL;
    }

    handle->ptr = ptr;
    handle->size = size;
    handle->arena = arena;
    handle->flags = RT_HANDLE_FLAG_NONE;
    handle->copy_callback = NULL;
    handle->free_callback = NULL;
    handle->next = NULL;
    handle->prev = NULL;

    ARENA_DEBUG_LOG("handle_create: h=%p ptr=%p size=%zu arena=%p(%s)",
                    (void *)handle, ptr, size, (void *)arena,
                    arena && arena->name ? arena->name : "?");

    return handle;
}

/* Handle link/unlink/destroy functions moved to arena_handle.c */

/* ============================================================================
 * Arena Lifecycle
 * ============================================================================ */

RtArenaV2 *rt_arena_v2_create(RtArenaV2 *parent, RtArenaMode mode, const char *name)
{
    RtArenaV2 *arena = calloc(1, sizeof(RtArenaV2));
    if (arena == NULL) {
        fprintf(stderr, "arena_v2: arena allocation failed\n");
        return NULL;
    }

    arena->name = name;
    arena->mode = mode;
    arena->parent = parent;
    arena->first_child = NULL;
    arena->next_sibling = NULL;

    arena->handles_head = NULL;

    arena->gc_running = false;
    arena->flags = RT_ARENA_FLAG_NONE;

    /* Use recursive mutex to allow re-entrant locking during nested promotes.
     * When promoting a struct with handle fields, we lock the source arena,
     * then the copy callback promotes nested handles which may be in the same arena. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&arena->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    arena->cleanups = NULL;
    memset(&arena->stats, 0, sizeof(arena->stats));

    /* Condemned queue fields */
    arena->condemned_head = NULL;
    arena->condemned_next = NULL;

    /* Set root arena reference - walk up to find the root, or self if no parent */
    if (parent != NULL) {
        arena->root = parent->root;  /* Inherit root from parent */
        arena->gc_log_enabled = parent->gc_log_enabled;  /* Inherit GC logging */

        if (parent->gc_log_enabled) {
            fprintf(stderr, "[ARENA] created '%s' parent='%s'\n",
                    name ? name : "(unnamed)", parent->name ? parent->name : "(unnamed)");
        }

        /* Link into parent's child list */
        pthread_mutex_lock(&parent->mutex);
        arena->next_sibling = parent->first_child;
        parent->first_child = arena;
        pthread_mutex_unlock(&parent->mutex);
    } else {
        arena->root = arena;  /* This is a root arena */
    }

    return arena;
}

void rt_arena_v2_condemn(RtArenaV2 *arena)
{
    if (arena == NULL) return;
    if (arena->gc_log_enabled) {
        fprintf(stderr, "[ARENA] condemn '%s'\n", arena->name ? arena->name : "(unnamed)");
    }
    arena->flags |= RT_ARENA_FLAG_DEAD;

    /* Unlink from parent's child list */
    RtArenaV2 *parent = arena->parent;
    if (parent != NULL) {
        pthread_mutex_lock(&parent->mutex);
        RtArenaV2 **pp = &parent->first_child;
        while (*pp != NULL) {
            if (*pp == arena) {
                *pp = arena->next_sibling;
                break;
            }
            pp = &(*pp)->next_sibling;
        }
        pthread_mutex_unlock(&parent->mutex);
        arena->parent = NULL;
        arena->next_sibling = NULL;
    }

    /* Push onto root's condemned queue (lock-free CAS) */
    RtArenaV2 *root = arena->root;
    if (root != NULL) {
        RtArenaV2 *old_head;
        do {
            old_head = root->condemned_head;
            arena->condemned_next = old_head;
        } while (!__sync_bool_compare_and_swap(&root->condemned_head, old_head, arena));
    }
}


void rt_arena_v2_on_cleanup(RtArenaV2 *arena, RtHandleV2 *data, RtCleanupFnV2 fn, int priority)
{
    if (arena == NULL || fn == NULL) return;

    RtCleanupNodeV2 *node = malloc(sizeof(RtCleanupNodeV2));
    if (node == NULL) return;

    node->data = data;
    node->fn = fn;
    node->priority = priority;

    pthread_mutex_lock(&arena->mutex);

    /* Insert in priority order (lower first) */
    RtCleanupNodeV2 **pp = &arena->cleanups;
    while (*pp != NULL && (*pp)->priority <= priority) {
        pp = &(*pp)->next;
    }
    node->next = *pp;
    *pp = node;

    pthread_mutex_unlock(&arena->mutex);
}

void rt_arena_v2_remove_cleanup(RtArenaV2 *arena, RtHandleV2 *data)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtCleanupNodeV2 **pp = &arena->cleanups;
    while (*pp != NULL) {
        if ((*pp)->data == data) {
            RtCleanupNodeV2 *node = *pp;
            *pp = node->next;
            free(node);
            break;
        }
        pp = &(*pp)->next;
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* ============================================================================
 * Allocation
 * ============================================================================ */

RtHandleV2 *rt_arena_v2_alloc(RtArenaV2 *arena, size_t size)
{
    if (arena == NULL || size == 0) return NULL;

    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "arena_v2: data allocation failed (size=%zu)\n", size);
        return NULL;
    }

    pthread_mutex_lock(&arena->mutex);

    RtHandleV2 *handle = handle_create(arena, ptr, size);
    if (handle == NULL) {
        free(ptr);
        pthread_mutex_unlock(&arena->mutex);
        return NULL;
    }

    rt_handle_v2_link(arena, handle);

    pthread_mutex_unlock(&arena->mutex);

    return handle;
}

RtHandleV2 *rt_arena_v2_calloc(RtArenaV2 *arena, size_t count, size_t size)
{
    size_t total = count * size;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total);
    if (handle != NULL) {
        memset(handle->ptr, 0, total);
    }
    return handle;
}

RtHandleV2 *rt_arena_v2_realloc(RtArenaV2 *arena, RtHandleV2 *old, size_t new_size)
{
    if (arena == NULL) return NULL;
    if (old == NULL) return rt_arena_v2_alloc(arena, new_size);
    if (new_size == 0) {
        rt_arena_v2_free(old);
        return NULL;
    }

    /* Allocate new */
    RtHandleV2 *new_handle = rt_arena_v2_alloc(arena, new_size);
    if (new_handle == NULL) return NULL;

    /* Copy old data */
    size_t copy_size = old->size < new_size ? old->size : new_size;
    memcpy(new_handle->ptr, old->ptr, copy_size);

    /* Mark old as dead */
    rt_arena_v2_free(old);

    return new_handle;
}

RtHandleV2 *rt_arena_v2_strdup(RtArenaV2 *arena, const char *str)
{
    if (arena == NULL || str == NULL) return NULL;

    size_t len = strlen(str) + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, len);
    if (handle != NULL) {
        memcpy(handle->ptr, str, len);
    }
    return handle;
}

void rt_arena_v2_free(RtHandleV2 *handle)
{
    if (handle == NULL) return;

    ARENA_DEBUG_LOG("rt_arena_v2_free: h=%p ptr=%p flags=0x%x arena=%p(%s) %s",
                    (void *)handle, handle->ptr, handle->flags,
                    (void *)handle->arena,
                    handle->arena && handle->arena->name ? handle->arena->name : "?",
                    (handle->flags & RT_HANDLE_FLAG_DEAD) ? "(ALREADY DEAD!)" : "");

    handle->flags |= RT_HANDLE_FLAG_DEAD;
}

/* ============================================================================
 * Promotion
 * ============================================================================ */

RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;
    if (handle->arena == dest) return handle;  /* Already in dest */

    /* Hold source arena mutex during entire promote operation.
     * This prevents GC from collecting handles from this arena while we're
     * in the middle of promoting. Without this lock, the copy callback can
     * mark child handles dead, GC can collect and free them, and then the
     * parent's free callback later tries to access freed memory. */
    RtArenaV2 *source_arena = handle->arena;
    pthread_mutex_lock(&source_arena->mutex);

    /* Clone to dest (handles both shallow copy and deep copy via callback) */
    RtHandleV2 *new_handle = rt_arena_v2_clone(dest, handle);

    /* Mark old as dead */
    if (new_handle != NULL) {
        rt_arena_v2_free(handle);
    }

    pthread_mutex_unlock(&source_arena->mutex);

    return new_handle;
}

RtHandleV2 *rt_arena_v2_clone(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;

    RtHandleV2 *new_handle = rt_arena_v2_alloc(dest, handle->size);
    if (new_handle != NULL) {
        /* Shallow copy */
        memcpy(new_handle->ptr, handle->ptr, handle->size);

        /* Inherit callbacks */
        new_handle->copy_callback = handle->copy_callback;
        new_handle->free_callback = handle->free_callback;

        /* Deep copy if callback registered */
        if (new_handle->copy_callback != NULL) {
            rt_handle_begin_transaction(new_handle);
            new_handle->copy_callback(dest, new_handle->ptr);
            rt_handle_end_transaction(new_handle);
        }
    }
    return new_handle;
}

/* ============================================================================
 * Thread Support
 * ============================================================================ */

RtArenaV2 *rt_tls_arena_get(void)
{
    return tls_current_arena;
}

void rt_tls_arena_set(RtArenaV2 *arena)
{
    tls_current_arena = arena;
}
