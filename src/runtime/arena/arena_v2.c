/*
 * Arena V2 - Core Implementation
 * ===============================
 */

#include "arena_v2.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include hooks only when building with SN_MALLOC_HOOKS defined */
#ifdef SN_MALLOC_HOOKS
#include "../malloc/runtime_malloc_hooks.h"
#define HOOKS_AVAILABLE 1
#else
#define HOOKS_AVAILABLE 0
#endif

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

static __thread RtArenaV2 *tls_current_arena = NULL;
static __thread RtArenaV2 *tls_redirect_stack[16];
static __thread int tls_redirect_depth = 0;

/* ============================================================================
 * Background GC Thread
 * ============================================================================
 * Simple GC thread that periodically runs rt_arena_v2_gc_recursive on the
 * root arena. Uses the existing arena tree structure (parent/child/sibling).
 */

static pthread_t gc_thread;
static volatile bool gc_thread_running = false;
static RtArenaV2 *gc_thread_root = NULL;
static int gc_thread_interval_ms = 100;

static void *gc_thread_func(void *arg)
{
    (void)arg;

    while (gc_thread_running) {
        /* Sleep for interval */
        struct timespec ts;
        ts.tv_sec = gc_thread_interval_ms / 1000;
        ts.tv_nsec = (gc_thread_interval_ms % 1000) * 1000000;
        nanosleep(&ts, NULL);

        if (!gc_thread_running) break;

        /* Run GC recursively on the entire arena tree */
        if (gc_thread_root != NULL) {
            rt_arena_v2_gc_recursive(gc_thread_root);
        }
    }

    return NULL;
}

void rt_arena_v2_gc_thread_start(RtArenaV2 *root, int interval_ms)
{
    if (gc_thread_running) return;
    if (root == NULL) return;

    gc_thread_root = root;
    gc_thread_interval_ms = (interval_ms > 0) ? interval_ms : 100;
    gc_thread_running = true;
    pthread_create(&gc_thread, NULL, gc_thread_func, NULL);
}

void rt_arena_v2_gc_thread_stop(void)
{
    if (!gc_thread_running) return;

    /* Final GC pass to destroy any condemned arenas before stopping */
    if (gc_thread_root != NULL) {
        rt_arena_v2_gc_recursive(gc_thread_root);
    }

    gc_thread_running = false;
    pthread_join(gc_thread, NULL);
    gc_thread_root = NULL;
}

bool rt_arena_v2_gc_thread_running(void)
{
    return gc_thread_running;
}

/* ============================================================================
 * Ptr → Handle Hash Map for Redirect Tracking (only when hooks available)
 * ============================================================================
 * When malloc is redirected to arena, we need to find the handle from the
 * raw pointer for free/realloc. This hash map provides O(1) lookup.
 * Thread-local since each thread has its own redirect stack.
 *
 * Thread cleanup: When a thread exits, the ptr_map entries would leak.
 * We use pthread_key with a destructor to clean up on thread exit.
 */

#if HOOKS_AVAILABLE

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

#endif /* HOOKS_AVAILABLE */

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

    arena->blocks_created++;

    return block;
}

static void block_destroy(RtBlockV2 *block)
{
    free(block);
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
    handle->pin_count = 0;
    handle->next = NULL;
    handle->prev = NULL;

    arena->handles_created++;

    return handle;
}

static void handle_link(RtBlockV2 *block, RtHandleV2 *handle)
{
    /* Add to head of block's handle list */
    handle->next = block->handles_head;
    handle->prev = NULL;
    if (block->handles_head) {
        block->handles_head->prev = handle;
    }
    block->handles_head = handle;
}

static void handle_unlink(RtBlockV2 *block, RtHandleV2 *handle)
{
    /* Remove from block's handle list */
    if (handle->prev) {
        handle->prev->next = handle->next;
    } else {
        block->handles_head = handle->next;
    }
    if (handle->next) {
        handle->next->prev = handle->prev;
    }
    handle->prev = NULL;
    handle->next = NULL;
}

static void handle_destroy(RtHandleV2 *handle)
{
    free(handle);
}

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

    arena->cleanups = NULL;
    arena->total_allocated = 0;
    arena->total_freed = 0;
    arena->gc_runs = 0;

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

/* Internal destroy - called with parent already handling unlinking */
static void arena_v2_destroy_internal(RtArenaV2 *arena, bool unlink_from_parent)
{
    if (arena == NULL) return;

    /* Run cleanup callbacks FIRST (before destroying children).
     * This is critical because thread cleanup callbacks need to join threads
     * that may still be using their child arenas. If we destroy children first,
     * threads would crash trying to use destroyed arenas. */
    RtCleanupNodeV2 *cleanup = arena->cleanups;
    arena->cleanups = NULL;  /* Clear list before invoking to prevent re-entry issues */
    while (cleanup != NULL) {
        RtCleanupNodeV2 *next = cleanup->next;
        if (cleanup->fn) {
            cleanup->fn(cleanup->data);
        }
        free(cleanup);
        cleanup = next;
    }

    pthread_mutex_lock(&arena->mutex);

    /* Destroy children (recursive) - children don't need to unlink,
     * we're clearing the whole child list anyway.
     * This is safe now because cleanup callbacks (e.g., thread joins) have completed. */
    RtArenaV2 *child = arena->first_child;
    arena->first_child = NULL;  /* Clear list before destroying */
    while (child != NULL) {
        RtArenaV2 *next = child->next_sibling;
        child->parent = NULL;  /* Prevent child from trying to unlink */
        arena_v2_destroy_internal(child, false);
        child = next;
    }

    /* Destroy all blocks and their handles */
    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        RtBlockV2 *next_block = block->next;
        /* Free all handles in this block */
        RtHandleV2 *handle = block->handles_head;
        while (handle != NULL) {
            RtHandleV2 *next_handle = handle->next;
            handle_destroy(handle);
            handle = next_handle;
        }
        block_destroy(block);
        block = next_block;
    }

    /* Unlink from parent (only if requested and parent exists) */
    if (unlink_from_parent && arena->parent != NULL) {
        pthread_mutex_lock(&arena->parent->mutex);
        RtArenaV2 **pp = &arena->parent->first_child;
        while (*pp != NULL) {
            if (*pp == arena) {
                *pp = arena->next_sibling;
                break;
            }
            pp = &(*pp)->next_sibling;
        }
        pthread_mutex_unlock(&arena->parent->mutex);
    }

    pthread_mutex_unlock(&arena->mutex);
    pthread_mutex_destroy(&arena->mutex);

    free(arena);
}

void rt_arena_v2_condemn(RtArenaV2 *arena)
{
    if (arena == NULL) return;
    if (arena->gc_log_enabled) {
        fprintf(stderr, "[ARENA] condemn '%s'\n", arena->name ? arena->name : "(unnamed)");
    }
    arena->flags |= RT_ARENA_FLAG_DEAD;
}

void rt_arena_v2_destroy(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    /* When GC thread is running, defer destruction to GC */
    if (gc_thread_running) {
        rt_arena_v2_condemn(arena);
        return;
    }

    /* GC not running - safe to destroy directly */
    arena_v2_destroy_internal(arena, true);
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

    handle_link(arena->current_block, handle);
    arena->total_allocated += size;

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
 * Garbage Collection
 * ============================================================================ */

size_t rt_arena_v2_gc(RtArenaV2 *arena)
{
    if (arena == NULL) return 0;

    pthread_mutex_lock(&arena->mutex);

    if (arena->gc_running) {
        pthread_mutex_unlock(&arena->mutex);
        return 0;
    }
    arena->gc_running = true;

    size_t handles_swept = 0;
    size_t handles_collected = 0;
    size_t blocks_swept = 0;
    size_t blocks_freed = 0;
    size_t bytes_collected = 0;

    RtBlockV2 **bp = &arena->blocks_head;

    while (*bp != NULL) {
        RtBlockV2 *block = *bp;
        blocks_swept++;

        /* Sweep handles in this block */
        RtHandleV2 *handle = block->handles_head;
        while (handle != NULL) {
            RtHandleV2 *next = handle->next;
            handles_swept++;
            if ((handle->flags & RT_HANDLE_FLAG_DEAD) && handle->pin_count == 0) {
                handle_unlink(block, handle);
                bytes_collected += handle->size;
                arena->total_freed += handle->size;
                handle_destroy(handle);
                handles_collected++;
            }
            handle = next;
        }

        /* If block is empty, free it */
        if (block->handles_head == NULL) {
            *bp = block->next;  /* Unlink from chain */
            if (arena->current_block == block) {
                arena->current_block = NULL;
            }
            block_destroy(block);
            blocks_freed++;
            arena->blocks_freed++;
        } else {
            bp = &block->next;
        }
    }

    arena->gc_runs++;
    arena->handles_collected += handles_collected;

    /* Store last GC report */
    arena->last_gc_handles_swept = handles_swept;
    arena->last_gc_handles_collected = handles_collected;
    arena->last_gc_blocks_swept = blocks_swept;
    arena->last_gc_blocks_freed = blocks_freed;
    arena->last_gc_bytes_collected = bytes_collected;

    /* Log if enabled */
    if (arena->gc_log_enabled && handles_swept > 0) {
        fprintf(stderr, "[GC] arena=%s blocks=%zu swept=%zu collected=%zu freed_blocks=%zu bytes=%zu\n",
                arena->name ? arena->name : "(unnamed)",
                blocks_swept - blocks_freed, handles_swept,
                handles_collected, blocks_freed, bytes_collected);
    }

    arena->gc_running = false;

    pthread_mutex_unlock(&arena->mutex);

    return handles_collected;
}

size_t rt_arena_v2_gc_recursive(RtArenaV2 *arena)
{
    if (arena == NULL) return 0;

    size_t total = rt_arena_v2_gc(arena);

    /* Traverse children under parent mutex.
     * Dead children are unlinked and destroyed here - safe because
     * only the GC thread performs destruction when GC is running. */
    pthread_mutex_lock(&arena->mutex);

    RtArenaV2 **pp = &arena->first_child;
    while (*pp != NULL) {
        RtArenaV2 *child = *pp;

        if (child->flags & RT_ARENA_FLAG_DEAD) {
            /* Unlink from sibling list */
            *pp = child->next_sibling;
            child->parent = NULL;

            pthread_mutex_unlock(&arena->mutex);
            arena_v2_destroy_internal(child, false);
            pthread_mutex_lock(&arena->mutex);
            /* pp already points to the next child after unlinking */
        } else {
            pthread_mutex_unlock(&arena->mutex);
            total += rt_arena_v2_gc_recursive(child);
            pthread_mutex_lock(&arena->mutex);
            pp = &child->next_sibling;
        }
    }

    pthread_mutex_unlock(&arena->mutex);

    return total;
}

void rt_arena_v2_gc_flush(RtArenaV2 *arena)
{
    /* Keep running GC until nothing more to collect */
    while (rt_arena_v2_gc(arena) > 0) {
        /* Continue */
    }
}

/* ============================================================================
 * Promotion
 * ============================================================================ */

RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;
    if (handle->arena == dest) return handle;  /* Already in dest */

    /* Clone to dest */
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
        memcpy(new_handle->ptr, handle->ptr, handle->size);
    }
    return new_handle;
}

/* ============================================================================
 * Malloc Redirect - Handler Functions (only when hooks available)
 * ============================================================================ */

#if HOOKS_AVAILABLE

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

#endif /* HOOKS_AVAILABLE */

/* ============================================================================
 * Malloc Redirect - Push/Pop API
 * ============================================================================ */

void rt_arena_v2_redirect_push(RtArenaV2 *arena)
{
    if (tls_redirect_depth < 16) {
        tls_redirect_stack[tls_redirect_depth++] = arena;

#if HOOKS_AVAILABLE
        /* Register handler with hooks on first push */
        if (tls_redirect_depth == 1) {
            /* Ensure thread cleanup is registered for when thread exits */
            tls_ensure_cleanup_registered();
            rt_malloc_hooks_set_handler(&tls_arena_v2_handler);
        }
#endif
    }
}

void rt_arena_v2_redirect_pop(void)
{
    if (tls_redirect_depth > 0) {
        tls_redirect_depth--;

#if HOOKS_AVAILABLE
        /* Unregister handler when stack empty */
        if (tls_redirect_depth == 0) {
            rt_malloc_hooks_clear_handler();
        }
#endif
    }
}

RtArenaV2 *rt_arena_v2_redirect_current(void)
{
    if (tls_redirect_depth > 0) {
        return tls_redirect_stack[tls_redirect_depth - 1];
    }
    return NULL;
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

/* ============================================================================
 * Statistics API (rt_arena_stats_*)
 * ============================================================================ */

void rt_arena_stats_get(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    if (arena == NULL || stats == NULL) return;

    memset(stats, 0, sizeof(RtArenaV2Stats));

    pthread_mutex_lock(&arena->mutex);

    /* Walk all blocks, counting handles and bytes */
    size_t handle_count = 0;
    size_t dead_handle_count = 0;
    size_t live_bytes = 0;
    size_t dead_bytes = 0;
    size_t block_count = 0;
    size_t block_capacity_total = 0;
    size_t block_used_total = 0;

    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        block_count++;
        block_capacity_total += block->capacity;
        block_used_total += block->used;

        RtHandleV2 *h = block->handles_head;
        while (h != NULL) {
            if (h->flags & RT_HANDLE_FLAG_DEAD) {
                dead_handle_count++;
                dead_bytes += h->size;
            } else {
                handle_count++;
                live_bytes += h->size;
            }
            h = h->next;
        }
        block = block->next;
    }

    stats->handle_count = handle_count;
    stats->dead_handle_count = dead_handle_count;
    stats->handles_created = arena->handles_created;
    stats->handles_collected = arena->handles_collected;
    stats->live_bytes = live_bytes;
    stats->dead_bytes = dead_bytes;
    stats->total_allocated = arena->total_allocated;
    stats->total_freed = arena->total_freed;
    stats->block_count = block_count;
    stats->block_capacity_total = block_capacity_total;
    stats->block_used_total = block_used_total;
    stats->blocks_created = arena->blocks_created;
    stats->blocks_freed = arena->blocks_freed;
    stats->gc_runs = arena->gc_runs;

    /* Fragmentation: how much block space is not live data
     * 0.0 = perfect (all used space is live), 1.0 = total waste */
    if (block_used_total > 0) {
        stats->fragmentation = 1.0 - ((double)live_bytes / (double)block_used_total);
    } else {
        stats->fragmentation = 0.0;
    }

    pthread_mutex_unlock(&arena->mutex);
}

void rt_arena_stats_print(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    RtArenaV2Stats s;
    rt_arena_stats_get(arena, &s);

    fprintf(stderr, "Arena '%s' stats:\n", arena->name ? arena->name : "(unnamed)");
    fprintf(stderr, "  Handles:       %zu live, %zu dead, %zu created, %zu collected\n",
            s.handle_count, s.dead_handle_count, s.handles_created, s.handles_collected);
    fprintf(stderr, "  Bytes:         %zu live, %zu dead, %zu allocated, %zu freed\n",
            s.live_bytes, s.dead_bytes, s.total_allocated, s.total_freed);
    fprintf(stderr, "  Blocks:        %zu current, %zu created, %zu freed\n",
            s.block_count, s.blocks_created, s.blocks_freed);
    fprintf(stderr, "  Block space:   %zu used / %zu capacity\n",
            s.block_used_total, s.block_capacity_total);
    fprintf(stderr, "  Fragmentation: %.1f%%\n", s.fragmentation * 100.0);
    fprintf(stderr, "  GC runs:       %zu\n", s.gc_runs);
}

void rt_arena_stats_last_gc(RtArenaV2 *arena, RtArenaV2GCReport *report)
{
    if (arena == NULL || report == NULL) return;

    pthread_mutex_lock(&arena->mutex);
    report->handles_swept = arena->last_gc_handles_swept;
    report->handles_collected = arena->last_gc_handles_collected;
    report->blocks_swept = arena->last_gc_blocks_swept;
    report->blocks_freed = arena->last_gc_blocks_freed;
    report->bytes_collected = arena->last_gc_bytes_collected;
    pthread_mutex_unlock(&arena->mutex);
}

void rt_arena_stats_snapshot(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    fprintf(stderr, "=== Arena Snapshot: '%s' ===\n",
            arena->name ? arena->name : "(unnamed)");

    size_t block_idx = 0;
    size_t total_live = 0;
    size_t total_dead = 0;

    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        size_t live = 0, dead = 0;
        size_t live_bytes = 0, dead_bytes = 0;

        RtHandleV2 *h = block->handles_head;
        while (h != NULL) {
            if (h->flags & RT_HANDLE_FLAG_DEAD) {
                dead++;
                dead_bytes += h->size;
            } else {
                live++;
                live_bytes += h->size;
            }
            h = h->next;
        }

        const char *marker = (block == arena->current_block) ? " [current]" : "";
        double occupancy = block->capacity > 0
            ? ((double)block->used / (double)block->capacity) * 100.0
            : 0.0;

        fprintf(stderr, "  block[%zu]: cap=%zu used=%zu (%.0f%%) handles=%zu live/%zu dead "
                "bytes=%zu live/%zu dead%s\n",
                block_idx, block->capacity, block->used, occupancy,
                live, dead, live_bytes, dead_bytes, marker);

        total_live += live;
        total_dead += dead;
        block_idx++;
        block = block->next;
    }

    fprintf(stderr, "  --- %zu blocks, %zu live handles, %zu dead handles ---\n",
            block_idx, total_live, total_dead);

    pthread_mutex_unlock(&arena->mutex);
}

void rt_arena_stats_enable_gc_log(RtArenaV2 *arena)
{
    if (arena == NULL) return;
    arena->gc_log_enabled = true;
}

void rt_arena_stats_disable_gc_log(RtArenaV2 *arena)
{
    if (arena == NULL) return;
    arena->gc_log_enabled = false;
}
