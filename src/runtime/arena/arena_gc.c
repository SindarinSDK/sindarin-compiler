/*
 * Arena GC - Garbage Collection Implementation
 * =============================================
 * Stop-the-world garbage collection for arena memory management.
 * Two-pass algorithm:
 * 1. Dead arena sweep - remove arenas marked with RT_ARENA_FLAG_DEAD
 * 2. Block compaction - acquire blocks, compact live handles, free dead ones
 */

#include "arena_gc.h"
#include "arena_v2.h"
#include "arena_handle.h"
#include "arena_id.h"
#include "arena_stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

/* ============================================================================
 * Core Helpers
 * ============================================================================ */

/* Call all handle free_callbacks in a block (handles remain allocated) */
static void gc_call_block_callbacks(RtBlockV2 *block)
{
    RtHandleV2 *handle = block->handles_head;
    while (handle != NULL) {
        if (handle->free_callback != NULL) {
            handle->free_callback(handle);
            handle->free_callback = NULL;
        }
        handle = handle->next;
    }
}

/* Free all handles in a block (callbacks must already be called) */
static void gc_free_block_handles(RtBlockV2 *block)
{
    RtHandleV2 *handle = block->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        free(handle);
        handle = next;
    }
    block->handles_head = NULL;
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

/* Call all handle callbacks in an arena's blocks */
static void gc_call_arena_handle_callbacks(RtArenaV2 *arena)
{
    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        gc_call_block_callbacks(block);
        block = block->next;
    }
}

/* Free all handles and blocks in an arena */
static void gc_free_arena_blocks(RtArenaV2 *arena)
{
    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        RtBlockV2 *next = block->next;
        gc_free_block_handles(block);
        free(block);
        block = next;
    }
    arena->blocks_head = NULL;
    arena->current_block = NULL;
}

/* Destroy arena struct (mutex and memory) */
static void gc_destroy_arena_struct(RtArenaV2 *arena)
{
    pthread_mutex_destroy(&arena->mutex);
    free(arena);
}

/* Forward declaration for recursive call */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);

/* Destroy an arena and all its handles/blocks */
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
    gc_free_arena_blocks(arena);

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
 * Internal: Block Acquisition
 * ============================================================================ */

/* Try to acquire a block for GC.
 * Returns: 0 = acquired, 1 = skip (valid lease), -1 = force acquired (expired) */
static int gc_acquire_block(RtBlockV2 *block)
{
    uint64_t expected = 0;

    /* Try to acquire free block */
    if (atomic_compare_exchange_strong(&block->tx_holder, &expected, GC_OWNER_ID)) {
        atomic_store(&block->tx_recurse_count, 1);
        return 0;  /* Successfully acquired free block */
    }

    /* Block is held - check timeout */
    uint64_t start_ns = atomic_load(&block->tx_start_ns);
    uint64_t timeout_ns = atomic_load(&block->tx_timeout_ns);
    uint64_t now_ns = rt_get_monotonic_ns();

    if (timeout_ns > 0 && (now_ns - start_ns) > timeout_ns) {
        /* Lease expired - force acquire */
        atomic_store(&block->tx_holder, GC_OWNER_ID);
        atomic_store(&block->tx_recurse_count, 1);
        return -1;  /* Force acquired expired lease */
    }

    /* Valid lease - skip this block */
    return 1;
}

/* Release a block after GC processing */
static void gc_release_block(RtBlockV2 *block)
{
    atomic_store(&block->tx_recurse_count, 0);
    atomic_store(&block->tx_holder, 0);
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

/* Free all handles/blocks and destroy arena struct */
static void gc_destroy_arena_fully(RtArenaV2 *arena)
{
    gc_free_arena_blocks(arena);
    gc_destroy_arena_struct(arena);
}

/* Sweep dead arenas using two-phase collection.
 * Phase 1: Collect all dead arenas, call all callbacks
 * Phase 2: Free all handles and arena structs */
static void gc_sweep_dead_arenas(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    /* Collect all dead arenas */
    ArenaListNode *dead_list = NULL;
    gc_collect_dead_arenas(arena, &dead_list);

    if (dead_list == NULL) return;

    /* Pass 1: Call all callbacks on all dead arenas */
    for (ArenaListNode *node = dead_list; node != NULL; node = node->next) {
        gc_call_all_arena_callbacks(node->arena);
    }

    /* Pass 2: Free all handles and arena structs */
    ArenaListNode *node = dead_list;
    while (node != NULL) {
        ArenaListNode *next = node->next;
        gc_destroy_arena_fully(node->arena);
        free(node);
        node = next;
    }
}

/* ============================================================================
 * Pass 2: Block Compaction (Two-Phase)
 * ============================================================================
 * Similar to dead arena sweep, we collect all dead handles first, then
 * destroy them in two passes to avoid use-after-free when callbacks
 * reference other dead handles.
 * ============================================================================ */

/* Simple handle list for collection */
typedef struct HandleListNode {
    RtHandleV2 *handle;
    RtBlockV2 *block;
    struct HandleListNode *next;
} HandleListNode;

/* Collect dead handles from a single block */
static void gc_collect_dead_handles_block(RtBlockV2 *block, HandleListNode **list, RtArenaGCResult *result)
{
    RtHandleV2 *handle = block->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        if (handle->flags & RT_HANDLE_FLAG_DEAD) {
            /* Unlink from block now */
            rt_handle_v2_unlink(block, handle);
            result->bytes_freed += handle->size;
            result->handles_freed++;

            /* Add to collection list */
            HandleListNode *node = malloc(sizeof(HandleListNode));
            node->handle = handle;
            node->block = block;
            node->next = *list;
            *list = node;
        }
        handle = next;
    }
}

/* Collect dead handles from a single arena's blocks */
static void gc_collect_arena_handles(RtArenaV2 *arena, HandleListNode **list, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        /* Try to acquire block for GC */
        int acquire_result = gc_acquire_block(block);
        if (acquire_result != 1) {
            /* Collect dead handles from this block */
            gc_collect_dead_handles_block(block, list, result);
            gc_release_block(block);
        }
        block = block->next;
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* Recursively collect dead handles from all arenas */
static void gc_collect_all_handles(RtArenaV2 *arena, HandleListNode **list, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    gc_collect_arena_handles(arena, list, result);

    /* Recursively collect from children */
    pthread_mutex_lock(&arena->mutex);
    RtArenaV2 *child = arena->first_child;

    while (child != NULL) {
        gc_collect_all_handles(child, list, result);
        child = child->next_sibling;
    }
    
    pthread_mutex_unlock(&arena->mutex);
}

/* Clean up empty blocks after compaction */
static void gc_cleanup_empty_blocks(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    RtBlockV2 **bp = &arena->blocks_head;
    while (*bp != NULL) {
        RtBlockV2 *block = *bp;
        if (block->handles_head == NULL) {
            *bp = block->next;
            if (arena->current_block == block) {
                arena->current_block = NULL;
            }
            free(block);
            result->blocks_freed++;
        } else {
            bp = &block->next;
        }
    }

    /* Recursively clean children */
    RtArenaV2 *child = arena->first_child;

    while (child != NULL) {
        gc_cleanup_empty_blocks(child, result);
        child = child->next_sibling;
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* GC all arenas using two-phase approach:
 * Phase 1: Collect all dead handles, call their callbacks
 * Phase 2: Free all collected handles, cleanup empty blocks */
static void gc_compact_all(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    /* Collect all dead handles */
    HandleListNode *dead_handles = NULL;
    gc_collect_all_handles(arena, &dead_handles, result);

    if (dead_handles == NULL) return;

    /* Pass 1: Call all free_callbacks */
    for (HandleListNode *node = dead_handles; node != NULL; node = node->next) {
        if (node->handle->free_callback != NULL) {
            node->handle->free_callback(node->handle);
            node->handle->free_callback = NULL;
        }
    }

    /* Pass 2: Free all handle structs */
    HandleListNode *node = dead_handles;
    while (node != NULL) {
        HandleListNode *next = node->next;
        free(node->handle);
        free(node);
        node = next;
    }

    /* Cleanup empty blocks */
    gc_cleanup_empty_blocks(arena, result);
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/* Main GC entry point - two-pass collection.
 * Pass 1: Sweep dead arenas
 * Pass 2: Compact blocks in all live arenas */
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

    /* Temporarily identify this thread as GC so that free_callbacks
     * invoked during compaction can re-entrantly acquire blocks that
     * GC already holds (tx_holder == GC_OWNER_ID == our thread id). */
    uint64_t saved_thread_id = rt_arena_get_thread_id();
    rt_arena_set_thread_id(GC_OWNER_ID);

    /* Pass 1: Sweep dead arenas */
    gc_sweep_dead_arenas(arena);

    /* Pass 2: Compact blocks in all live arenas */
    gc_compact_all(arena, &result);

    /* Restore thread identity */
    rt_arena_set_thread_id(saved_thread_id);

    /* Record GC results */
    rt_arena_stats_record_gc(arena, &result);

    /* Lock briefly to clear gc_running */
    pthread_mutex_lock(&arena->mutex);
    arena->gc_running = false;
    pthread_mutex_unlock(&arena->mutex);

    return result.handles_freed;
}
