/*
 * Arena GC - Garbage Collection Implementation
 * =============================================
 * Stop-the-world garbage collection for arena memory management.
 * Two-pass algorithm:
 * 1. Dead arena sweep - remove arenas marked with RT_ARENA_FLAG_DEAD
 * 2. Handle collection - collect dead handles, free data via free()
 */

#include "arena_gc.h"
#include "arena_v2.h"
#include "arena_handle.h"
#include "arena_id.h"
#include "arena_stats.h"

#include <stdio.h>
#include <stdlib.h>
#include "arena_compat.h"

/* Debug logging (shared with arena_v2.c) */
extern int arena_debug_initialized;
extern int arena_debug_enabled;
void arena_debug_init(void);

#define GC_DEBUG_LOG(fmt, ...) do { \
    if (!arena_debug_initialized) arena_debug_init(); \
    if (arena_debug_enabled) { \
        fprintf(stderr, "[GC:T%lu] " fmt "\n", (unsigned long)(uintptr_t)pthread_self(), ##__VA_ARGS__); \
        fflush(stderr); \
    } \
} while(0)

/* ============================================================================
 * Core Helpers
 * ============================================================================ */

/* Call all handle free_callbacks in an arena (handles remain allocated) */
static void gc_call_arena_handle_callbacks(RtArenaV2 *arena)
{
    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        if (handle->free_callback != NULL) {
            handle->free_callback(handle);
            handle->free_callback = NULL;
        }
        handle = handle->next;
    }
}

/* Free all handles in an arena (callbacks must already be called) */
static void gc_free_arena_handles(RtArenaV2 *arena)
{
    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        if (!(handle->flags & RT_HANDLE_FLAG_EXTERN)) {
            free(handle->ptr);
        }
        free(handle);
        handle = next;
    }
    arena->handles_head = NULL;
}

/* Run cleanup callbacks for an arena */
static void gc_run_cleanup_callbacks(RtArenaV2 *arena)
{
    RtCleanupNodeV2 *cleanup = arena->cleanups;
    arena->cleanups = NULL;
    while (cleanup != NULL) {
        RtCleanupNodeV2 *next = cleanup->next;
        if (cleanup->fn) {
            cleanup->fn(cleanup->data);
        }
        free(cleanup);
        cleanup = next;
    }
}

/* Destroy arena struct (mutex and memory) */
static void gc_destroy_arena_struct(RtArenaV2 *arena)
{
    pthread_mutex_destroy(&arena->mutex);
    free(arena);
}

/* Forward declaration for recursive call */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);

/* Destroy an arena and all its handles */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent)
{
    if (arena == NULL) return;

    /* Run cleanup callbacks FIRST (before destroying children).
     * This is critical because thread cleanup callbacks need to join threads
     * that may still be using their child arenas. */
    gc_run_cleanup_callbacks(arena);

    pthread_mutex_lock(&arena->mutex);

    /* Destroy children (recursive) - children don't need to unlink */
    RtArenaV2 *child = arena->first_child;
    arena->first_child = NULL;
    while (child != NULL) {
        RtArenaV2 *next = child->next_sibling;
        child->parent = NULL;
        rt_arena_v2_destroy(child, false);
        child = next;
    }

    /* Two passes: callbacks first, then free (avoids use-after-free) */
    gc_call_arena_handle_callbacks(arena);
    gc_free_arena_handles(arena);

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
    gc_destroy_arena_struct(arena);
}

/* ============================================================================
 * Pass 1: Dead Arena Sweep (Two-Phase Collection)
 * ============================================================================
 * To avoid use-after-free when arenas reference handles in other arenas,
 * we collect all dead arenas first, then destroy them in two passes:
 * 1. Call all free_callbacks (handles still valid across all arenas)
 * 2. Free all handles and arena structs
 * ============================================================================ */

/* Simple arena list for collection */
typedef struct ArenaListNode {
    RtArenaV2 *arena;
    struct ArenaListNode *next;
} ArenaListNode;

/* Collect dead arenas recursively into a list.
 * Also collects all children of dead arenas (they must be destroyed too). */
static void gc_collect_dead_arenas(RtArenaV2 *arena, ArenaListNode **list)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtArenaV2 **pp = &arena->first_child;
    while (*pp != NULL) {
        RtArenaV2 *child = *pp;

        if (child->flags & RT_ARENA_FLAG_DEAD) {
            /* Unlink from sibling list */
            *pp = child->next_sibling;
            child->parent = NULL;
            child->next_sibling = NULL;

            /* Add to collection list */
            ArenaListNode *node = malloc(sizeof(ArenaListNode));
            node->arena = child;
            node->next = *list;
            *list = node;

            /* Collect all children of this dead arena too (they're implicitly dead) */
            pthread_mutex_unlock(&arena->mutex);
            RtArenaV2 *grandchild = child->first_child;
            while (grandchild != NULL) {
                RtArenaV2 *next_grandchild = grandchild->next_sibling;
                grandchild->parent = NULL;
                grandchild->next_sibling = NULL;
                ArenaListNode *gc_node = malloc(sizeof(ArenaListNode));
                gc_node->arena = grandchild;
                gc_node->next = *list;
                *list = gc_node;
                grandchild = next_grandchild;
            }
            child->first_child = NULL;
            pthread_mutex_lock(&arena->mutex);
            /* pp already points to the next child after unlinking */
        } else {
            pthread_mutex_unlock(&arena->mutex);
            gc_collect_dead_arenas(child, list);  /* Recurse into live children */
            pthread_mutex_lock(&arena->mutex);
            pp = &child->next_sibling;
        }
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* Call all callbacks in an arena (cleanup + handle callbacks) */
static void gc_call_all_arena_callbacks(RtArenaV2 *arena)
{
    gc_run_cleanup_callbacks(arena);
    gc_call_arena_handle_callbacks(arena);
}

/* Free all handles and destroy arena struct */
static void gc_destroy_arena_fully(RtArenaV2 *arena)
{
    gc_free_arena_handles(arena);
    gc_destroy_arena_struct(arena);
}

/* Sweep dead arenas - phase 1: collect and call callbacks.
 * Returns the list of dead arenas (handle structs still valid).
 * Caller must pass the returned list to gc_sweep_finalize() to free memory. */
static ArenaListNode *gc_sweep_dead_arenas_callbacks(RtArenaV2 *arena)
{
    if (arena == NULL) return NULL;

    /* Collect all dead arenas */
    ArenaListNode *dead_list = NULL;
    gc_collect_dead_arenas(arena, &dead_list);

    if (dead_list == NULL) return NULL;

    /* Call all callbacks on all dead arenas (handle structs still valid) */
    for (ArenaListNode *node = dead_list; node != NULL; node = node->next) {
        gc_call_all_arena_callbacks(node->arena);
    }

    return dead_list;
}

/* Sweep dead arenas - phase 2: free all handle structs and arena structs.
 * Must be called AFTER gc_compact_all so that compact's free_callbacks
 * can still read handle structs from dead arenas. */
static void gc_sweep_finalize(ArenaListNode *dead_list)
{
    ArenaListNode *node = dead_list;
    while (node != NULL) {
        ArenaListNode *next = node->next;
        gc_destroy_arena_fully(node->arena);
        free(node);
        node = next;
    }
}

/* ============================================================================
 * Pass 2: Handle Collection (Two-Phase)
 * ============================================================================
 * Similar to dead arena sweep, we collect all dead handles first, then
 * destroy them in two passes to avoid use-after-free when callbacks
 * reference other dead handles.
 * ============================================================================ */

/* Simple handle list for collection */
typedef struct HandleListNode {
    RtHandleV2 *handle;
    RtArenaV2 *arena;
    struct HandleListNode *next;
} HandleListNode;

/* Collect dead handles from a single arena */
static void gc_collect_arena_handles(RtArenaV2 *arena, HandleListNode **list, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    if (pthread_mutex_trylock(&arena->mutex) != 0) return;  /* skip if locked */

    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        if (handle->flags & RT_HANDLE_FLAG_DEAD) {
            rt_handle_v2_unlink(arena, handle);
            result->bytes_freed += handle->size;
            result->handles_freed++;

            HandleListNode *node = malloc(sizeof(HandleListNode));
            node->handle = handle;
            node->arena = arena;
            node->next = *list;
            *list = node;
        }
        handle = next;
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* Recursively collect dead handles from all arenas */
static void gc_collect_all_handles(RtArenaV2 *arena, HandleListNode **list, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    gc_collect_arena_handles(arena, list, result);

    /* Snapshot children while holding mutex, then recurse without holding it.
     * This avoids parent->child lock ordering that deadlocks with promote's
     * child->parent ordering. */
    pthread_mutex_lock(&arena->mutex);
    size_t child_count = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        child_count++;

    if (child_count == 0) {
        pthread_mutex_unlock(&arena->mutex);
        return;
    }

    RtArenaV2 **children = malloc(child_count * sizeof(RtArenaV2 *));
    size_t i = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        children[i++] = c;
    pthread_mutex_unlock(&arena->mutex);

    for (i = 0; i < child_count; i++)
        gc_collect_all_handles(children[i], list, result);

    free(children);
}

/* ============================================================================
 * Reference Count Hash Table
 * ============================================================================
 * Simple open-addressing hash table mapping handle addresses to reference
 * counts. Used during GC to determine if a dead handle's children are
 * shared with other handles (live or dead).
 * ============================================================================ */

#define RC_TABLE_INITIAL_CAP 256

typedef struct {
    void *key;          /* Handle address */
    size_t count;       /* Number of handles referencing this address */
} RcEntry;

typedef struct {
    RcEntry *entries;
    size_t capacity;
    size_t size;
} RcTable;

static void rc_table_init(RcTable *t)
{
    t->capacity = RC_TABLE_INITIAL_CAP;
    t->size = 0;
    t->entries = calloc(t->capacity, sizeof(RcEntry));
}

static void rc_table_free(RcTable *t)
{
    free(t->entries);
    t->entries = NULL;
    t->capacity = 0;
    t->size = 0;
}

static void rc_table_grow(RcTable *t)
{
    size_t old_cap = t->capacity;
    RcEntry *old = t->entries;
    t->capacity *= 2;
    t->entries = calloc(t->capacity, sizeof(RcEntry));
    t->size = 0;

    for (size_t i = 0; i < old_cap; i++) {
        if (old[i].key != NULL) {
            /* Re-insert */
            size_t idx = ((uintptr_t)old[i].key >> 3) % t->capacity;
            while (t->entries[idx].key != NULL) {
                idx = (idx + 1) % t->capacity;
            }
            t->entries[idx] = old[i];
            t->size++;
        }
    }
    free(old);
}

static void rc_table_inc(RcTable *t, void *key)
{
    if (key == NULL) return;
    if (t->size * 2 >= t->capacity) rc_table_grow(t);

    size_t idx = ((uintptr_t)key >> 3) % t->capacity;
    while (t->entries[idx].key != NULL) {
        if (t->entries[idx].key == key) {
            t->entries[idx].count++;
            return;
        }
        idx = (idx + 1) % t->capacity;
    }
    t->entries[idx].key = key;
    t->entries[idx].count = 1;
    t->size++;
}

static size_t rc_table_get(RcTable *t, void *key)
{
    if (key == NULL || t->size == 0) return 0;

    size_t idx = ((uintptr_t)key >> 3) % t->capacity;
    while (t->entries[idx].key != NULL) {
        if (t->entries[idx].key == key) {
            return t->entries[idx].count;
        }
        idx = (idx + 1) % t->capacity;
    }
    return 0;
}

/* Build a set of all known handle addresses across all arenas.
 * Used to filter conservative pointer scanning to avoid false positives. */
static void gc_build_handle_set(RtArenaV2 *arena, RcTable *handle_set)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        rc_table_inc(handle_set, (void *)handle);
        handle = handle->next;
    }

    /* Snapshot children, release mutex, then recurse */
    size_t child_count = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        child_count++;

    if (child_count == 0) {
        pthread_mutex_unlock(&arena->mutex);
        return;
    }

    RtArenaV2 **children = malloc(child_count * sizeof(RtArenaV2 *));
    size_t i = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        children[i++] = c;
    pthread_mutex_unlock(&arena->mutex);

    for (i = 0; i < child_count; i++)
        gc_build_handle_set(children[i], handle_set);

    free(children);
}

/* Build reference count table by scanning all LIVE handles across all arenas.
 * Only counts references to known handle addresses (avoids false positives
 * from non-handle pointers like arena metadata). */
static void gc_build_refcount_table(RtArenaV2 *arena, RcTable *t, RcTable *handle_set)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        if (handle->ptr != NULL) {
            void **scan = (void **)handle->ptr;
            size_t slots = handle->size / sizeof(void *);
            for (size_t i = 0; i < slots; i++) {
                /* Only count if value is a known handle address */
                if (scan[i] != NULL && rc_table_get(handle_set, scan[i]) > 0) {
                    rc_table_inc(t, scan[i]);
                }
            }
        }
        handle = handle->next;
    }

    /* Snapshot children, release mutex, then recurse */
    size_t child_count = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        child_count++;

    if (child_count == 0) {
        pthread_mutex_unlock(&arena->mutex);
        return;
    }

    RtArenaV2 **children = malloc(child_count * sizeof(RtArenaV2 *));
    size_t i = 0;
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling)
        children[i++] = c;
    pthread_mutex_unlock(&arena->mutex);

    for (i = 0; i < child_count; i++)
        gc_build_refcount_table(children[i], t, handle_set);

    free(children);
}

/* Free children of a dead handle based on reference counts.
 * Scans the handle's data for pointer-sized values that match known handle
 * addresses. For each child handle found:
 *   - ref count == 0 (no live handle references it): exclusively owned by this
 *     dead handle, mark it DEAD so it's collected next GC pass.
 *   - ref count >= 1 (referenced by a live handle): shared, leave it alone.
 * Returns true if all children were exclusively owned (ref count 0). */
static bool gc_free_dead_handle_children(RcTable *rctable, RcTable *handle_set, RtHandleV2 *handle)
{
    if (handle->ptr == NULL) return true;
    void **scan = (void **)handle->ptr;
    size_t slots = handle->size / sizeof(void *);
    bool all_exclusive = true;
    for (size_t i = 0; i < slots; i++) {
        if (scan[i] != NULL && rc_table_get(handle_set, scan[i]) > 0) {
            if (rc_table_get(rctable, scan[i]) == 0) {
                /* ref count 0: only dead handles reference this child -- free it */
                RtHandleV2 *child = (RtHandleV2 *)scan[i];
                if (!(child->flags & RT_HANDLE_FLAG_DEAD)) {
                    GC_DEBUG_LOG("    free exclusive child: h=%p ptr=%p", (void *)child, child->ptr);
                    rt_arena_v2_free(child);
                }
            } else {
                /* ref count >= 1: shared with a live handle -- leave it */
                all_exclusive = false;
            }
        }
    }
    return all_exclusive;
}

/* GC all arenas using two-phase approach with reference counting:
 * Phase 1: Build ref counts, free exclusively-owned children per ref count
 * Phase 2: Free all dead handle data + structs */
static void gc_compact_all(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    /* Collect all dead handles */
    HandleListNode *dead_handles = NULL;
    gc_collect_all_handles(arena, &dead_handles, result);

    if (dead_handles == NULL) return;

    /* Build set of all known handle addresses (live + dead) to filter
     * the conservative pointer scan against. This prevents false positives
     * from non-handle pointers (e.g., arena pointers in array metadata). */
    RcTable handle_set;
    rc_table_init(&handle_set);
    gc_build_handle_set(arena, &handle_set);
    for (HandleListNode *node = dead_handles; node != NULL; node = node->next) {
        rc_table_inc(&handle_set, (void *)node->handle);
    }

    /* Build reference count table from LIVE handles only.
     * Dead handles were already unlinked, so only live handles are scanned.
     * Only counts references to known handle addresses (no false positives). */
    RcTable rctable;
    rc_table_init(&rctable);
    gc_build_refcount_table(arena, &rctable, &handle_set);

    /* Rescue pass: Dead handles still referenced by live handles are rescued.
     * This prevents use-after-free when scope cleanup marks handles as DEAD
     * but they're still stored in live data structures (e.g., struct fields
     * pushed into arrays). Re-link rescued handles into their arenas and
     * clear the DEAD flag so they stay alive. */
    HandleListNode *truly_dead = NULL;
    HandleListNode *temp_node = dead_handles;
    while (temp_node != NULL) {
        HandleListNode *next = temp_node->next;
        if (rc_table_get(&rctable, (void *)temp_node->handle) > 0) {
            /* Handle is referenced by a live handle -- rescue it */
            GC_DEBUG_LOG("  rescue: h=%p ptr=%p (refcount > 0)", (void *)temp_node->handle, temp_node->handle->ptr);
            temp_node->handle->flags &= ~RT_HANDLE_FLAG_DEAD;
            rt_handle_v2_link(temp_node->arena, temp_node->handle);
            result->handles_freed--;  /* Undo the count from collection */
            result->bytes_freed -= temp_node->handle->size;
            free(temp_node);
        } else {
            /* Truly dead -- keep in list for freeing */
            temp_node->next = truly_dead;
            truly_dead = temp_node;
        }
        temp_node = next;
    }
    dead_handles = truly_dead;

    GC_DEBUG_LOG("gc_compact_all: Pass 1 - free children by ref count");

    /* Pass 1: For each dead handle, free exclusively-owned children using
     * ref counts. A child with ref count 0 (no live handle references it) is
     * only reachable through dead handles -- mark it DEAD. A child with ref
     * count >= 1 is shared with a live handle -- leave it alone.
     *
     * If ALL children were exclusively owned, also run the free callback for
     * any non-pointer cleanup. Otherwise skip the callback since it would
     * attempt to free shared children. */
    for (HandleListNode *node = dead_handles; node != NULL; node = node->next) {
        if (node->handle->free_callback != NULL) {
            bool all_exclusive = gc_free_dead_handle_children(&rctable, &handle_set, node->handle);
            if (all_exclusive) {
                GC_DEBUG_LOG("  all exclusive (deep free): h=%p ptr=%p arena=%p(%s)",
                             (void *)node->handle, node->handle->ptr,
                             (void *)node->handle->arena,
                             node->handle->arena && node->handle->arena->name ? node->handle->arena->name : "?");
                node->handle->free_callback(node->handle);
            } else {
                GC_DEBUG_LOG("  mixed ownership (children freed by ref count): h=%p ptr=%p",
                             (void *)node->handle, node->handle->ptr);
            }
            node->handle->free_callback = NULL;
        }
    }

    GC_DEBUG_LOG("gc_compact_all: Pass 2 - freeing handle data + structs");

    /* Pass 2: Free all dead handle data + structs */
    HandleListNode *node = dead_handles;
    while (node != NULL) {
        HandleListNode *next = node->next;
        GC_DEBUG_LOG("  free: h=%p ptr=%p", (void *)node->handle, node->handle->ptr);
        if (!(node->handle->flags & RT_HANDLE_FLAG_EXTERN)) {
            free(node->handle->ptr);
        }
        free(node->handle);
        free(node);
        node = next;
    }

    rc_table_free(&rctable);
    rc_table_free(&handle_set);
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/* Main GC entry point - three-phase collection.
 * Phase 1: Collect dead arenas, call their callbacks (handle structs stay valid)
 * Phase 2: Collect dead handles in live arenas, free data via free()
 * Phase 3: Free dead arena handle structs and arena structs */
size_t rt_arena_v2_gc(RtArenaV2 *arena)
{
    /* First check (lock-free fast path) */
    if (arena->gc_running) {
        return 0;
    }

    /* Lock briefly to safely set gc_running */
    pthread_mutex_lock(&arena->mutex);

    if (arena->gc_running) {
        pthread_mutex_unlock(&arena->mutex);
        return 0;
    }

    arena->gc_running = true;
    pthread_mutex_unlock(&arena->mutex);  /* Release before calling helpers */

    /* Initialize result */
    RtArenaGCResult result = {0};

    /* Phase 1: Sweep dead arenas - call callbacks but keep handle structs alive.
     * Handle structs must remain valid so that compact's free_callbacks
     * (which may reference handles in dead arenas) don't hit use-after-free. */
    ArenaListNode *dead_arenas = gc_sweep_dead_arenas_callbacks(arena);

    /* Phase 2: Collect dead handles in all live arenas */
    gc_compact_all(arena, &result);

    /* Phase 3: Now safe to free dead arena handle structs and arena structs.
     * All callbacks (both sweep and compact) have completed. */
    gc_sweep_finalize(dead_arenas);

    /* Record GC results */
    rt_arena_stats_record_gc(arena, &result);

    /* Lock briefly to clear gc_running */
    pthread_mutex_lock(&arena->mutex);
    arena->gc_running = false;
    pthread_mutex_unlock(&arena->mutex);

    return result.handles_freed;
}
