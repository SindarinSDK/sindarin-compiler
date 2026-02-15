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
#include "arena_stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

/* ============================================================================
 * Destruction Functions
 * ============================================================================
 * GC owns all actual destruction. Public APIs only mark intentions.
 * ============================================================================ */

static void rt_handle_v2_destroy(RtHandleV2 *handle)
{
    free(handle);
}

static void rt_block_destroy(RtBlockV2 *block)
{
    free(block);
}

/* Forward declaration for recursive call */
static void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);

/* Destroy an arena and all its handles/blocks */
static void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent)
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
        rt_arena_v2_destroy(child, false);
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
            rt_handle_v2_destroy(handle);
            handle = next_handle;
        }
        rt_block_destroy(block);
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
        return 0;  /* Successfully acquired free block */
    }

    /* Block is held - check timeout */
    uint64_t start_ns = atomic_load(&block->tx_start_ns);
    uint64_t timeout_ns = atomic_load(&block->tx_timeout_ns);
    uint64_t now_ns = rt_get_monotonic_ns();

    if (timeout_ns > 0 && (now_ns - start_ns) > timeout_ns) {
        /* Lease expired - force acquire */
        atomic_store(&block->tx_holder, GC_OWNER_ID);
        return -1;  /* Force acquired expired lease */
    }

    /* Valid lease - skip this block */
    return 1;
}

/* Release a block after GC processing */
static void gc_release_block(RtBlockV2 *block)
{
    atomic_store(&block->tx_holder, 0);
}

/* ============================================================================
 * Pass 1: Dead Arena Sweep
 * ============================================================================ */

/* Sweep dead arenas recursively.
 * Destroys arenas marked with RT_ARENA_FLAG_DEAD. */
static void gc_sweep_dead_arenas(RtArenaV2 *arena)
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

            pthread_mutex_unlock(&arena->mutex);
            rt_arena_v2_destroy(child, false);
            pthread_mutex_lock(&arena->mutex);
            /* pp already points to the next child after unlinking */
        } else {
            pthread_mutex_unlock(&arena->mutex);
            gc_sweep_dead_arenas(child);  /* Recurse into live children */
            pthread_mutex_lock(&arena->mutex);
            pp = &child->next_sibling;
        }
    }

    pthread_mutex_unlock(&arena->mutex);
}

/* ============================================================================
 * Pass 2: Block Compaction
 * ============================================================================ */

/* Compact a single block - free dead handles.
 * Block must be acquired by GC before calling.
 * Updates result with handles and bytes freed. */
static void gc_compact_block(RtBlockV2 *block, RtArenaGCResult *result)
{
    RtHandleV2 *handle = block->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        if (handle->flags & RT_HANDLE_FLAG_DEAD) {
            rt_handle_v2_unlink(block, handle);
            result->bytes_freed += handle->size;
            rt_handle_v2_destroy(handle);
            result->handles_freed++;
        }
        handle = next;
    }
}

/* GC a single arena's blocks.
 * Acquires blocks before processing.
 * Updates result with collection stats. */
static void gc_compact_arena(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    if (arena->gc_running) {
        pthread_mutex_unlock(&arena->mutex);
        return;
    }
    arena->gc_running = true;

    RtBlockV2 **bp = &arena->blocks_head;

    while (*bp != NULL) {
        RtBlockV2 *block = *bp;

        /* Try to acquire block for GC */
        int acquire_result = gc_acquire_block(block);
        if (acquire_result == 1) {
            /* Skip - valid lease held */
            bp = &block->next;
            continue;
        }

        /* Compact the block */
        gc_compact_block(block, result);

        /* Release the block */
        gc_release_block(block);

        /* If block is empty, free it */
        if (block->handles_head == NULL) {
            *bp = block->next;  /* Unlink from chain */
            if (arena->current_block == block) {
                arena->current_block = NULL;
            }
            rt_block_destroy(block);
            result->blocks_freed++;
        } else {
            bp = &block->next;
        }
    }

    arena->gc_running = false;

    pthread_mutex_unlock(&arena->mutex);
}

/* GC all arenas in tree recursively (Pass 2 helper) */
static void gc_compact_all(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    gc_compact_arena(arena, result);

    /* Recursively GC children */
    pthread_mutex_lock(&arena->mutex);
    RtArenaV2 *child = arena->first_child;
    pthread_mutex_unlock(&arena->mutex);

    while (child != NULL) {
        gc_compact_all(child, result);

        pthread_mutex_lock(&arena->mutex);
        child = child->next_sibling;
        pthread_mutex_unlock(&arena->mutex);
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/* Main GC entry point - two-pass collection.
 * Pass 1: Sweep dead arenas
 * Pass 2: Compact blocks in all live arenas */
size_t rt_arena_v2_gc(RtArenaV2 *arena)
{
    if (arena == NULL) return 0;

    /* Initialize result */
    RtArenaGCResult result = {0};

    /* Pass 1: Sweep dead arenas */
    gc_sweep_dead_arenas(arena);

    /* Pass 2: Compact blocks in all live arenas */
    gc_compact_all(arena, &result);

    /* Record GC results */
    rt_arena_stats_record_gc(arena, &result);

    return result.handles_freed;
}
