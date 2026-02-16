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

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

static __thread RtArenaV2 *tls_current_arena = NULL;

/* ============================================================================
 * Internal: Block Management
 * ============================================================================ */

static RtBlockV2 *block_create(RtArenaV2 *arena, size_t min_size)
{
    size_t capacity = RT_BLOCK_V2_SIZE;
    if (min_size > capacity) {
        capacity = min_size;
    }

    RtBlockV2 *block = malloc(sizeof(RtBlockV2) + capacity);
    if (block == NULL) {
        fprintf(stderr, "arena_v2: block allocation failed\n");
        return NULL;
    }

    block->next = NULL;
    block->arena = arena;
    block->capacity = capacity;
    block->used = 0;
    block->handles_head = NULL;

    /* Initialize transaction state */
    atomic_store(&block->tx_holder, 0);
    atomic_store(&block->tx_recurse_count, 0);
    atomic_store(&block->tx_start_ns, 0);
    atomic_store(&block->tx_timeout_ns, 0);

    return block;
}

static void *block_alloc(RtBlockV2 *block, size_t size)
{
    /* Align to 8 bytes */
    size_t aligned = (size + 7) & ~7;

    if (block->used + aligned > block->capacity) {
        return NULL;  /* Block full */
    }

    void *ptr = block->data + block->used;
    block->used += aligned;
    return ptr;
}

/* ============================================================================
 * Internal: Handle Management
 * ============================================================================ */

static RtHandleV2 *handle_create(RtArenaV2 *arena, void *ptr, size_t size, RtBlockV2 *block)
{
    /* Allocate handle struct itself via malloc (not in arena block) */
    RtHandleV2 *handle = malloc(sizeof(RtHandleV2));
    if (handle == NULL) {
        fprintf(stderr, "arena_v2: handle allocation failed\n");
        return NULL;
    }

    handle->ptr = ptr;
    handle->size = size;
    handle->arena = arena;
    handle->block = block;
    handle->flags = RT_HANDLE_FLAG_NONE;
    handle->copy_callback = NULL;     /* No deep copy by default */
    handle->free_callback = NULL;  /* No cleanup by default */
    handle->next = NULL;
    handle->prev = NULL;

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

    arena->blocks_head = NULL;
    arena->current_block = NULL;

    arena->gc_running = false;
    arena->flags = RT_ARENA_FLAG_NONE;

    pthread_mutex_init(&arena->mutex, NULL);
    pthread_mutex_init(&arena->gc_mutex, NULL);

    arena->cleanups = NULL;
    memset(&arena->stats, 0, sizeof(arena->stats));

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

    pthread_mutex_lock(&arena->mutex);

    /* Try current block first */
    void *ptr = NULL;
    if (arena->current_block != NULL) {
        ptr = block_alloc(arena->current_block, size);
    }

    /* Need a new block */
    if (ptr == NULL) {
        RtBlockV2 *block = block_create(arena, size);
        if (block == NULL) {
            pthread_mutex_unlock(&arena->mutex);
            return NULL;
        }

        /* Link new block */
        block->next = arena->blocks_head;
        arena->blocks_head = block;
        arena->current_block = block;

        ptr = block_alloc(block, size);
    }

    /* Create handle */
    RtHandleV2 *handle = handle_create(arena, ptr, size, arena->current_block);
    if (handle == NULL) {
        pthread_mutex_unlock(&arena->mutex);
        return NULL;
    }

    rt_handle_v2_link(arena->current_block, handle);

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
    handle->flags |= RT_HANDLE_FLAG_DEAD;
}

/* ============================================================================
 * Promotion
 * ============================================================================ */

RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;
    if (handle->arena == dest) return handle;  /* Already in dest */

    /* Clone to dest (handles both shallow copy and deep copy via callback) */
    RtHandleV2 *new_handle = rt_arena_v2_clone(dest, handle);

    /* Mark old as dead */
    if (new_handle != NULL) {
        rt_arena_v2_free(handle);
    }

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
