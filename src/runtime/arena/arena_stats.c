/*
 * Arena Stats - Statistics and Observability Implementation
 * ==========================================================
 * Stats are computed during GC cycles only.
 */

#include "arena_stats.h"
#include "arena_v2.h"
#include "arena_handle.h"

#include <stdio.h>
#include <string.h>

/* ============================================================================
 * GC Support Functions
 * ============================================================================ */

void rt_arena_stats_record_gc(RtArenaV2 *arena, const RtArenaGCResult *result)
{
    if (arena == NULL || result == NULL) return;

    arena->stats.gc_runs++;
    arena->stats.last_handles_freed = result->handles_freed;
    arena->stats.last_bytes_freed = result->bytes_freed;
    arena->stats.last_blocks_freed = result->blocks_freed;
}

/* Compute local stats for a single arena (not including children) */
static void compute_local_stats(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    size_t live_handles = 0;
    size_t dead_handles = 0;
    size_t live_bytes = 0;
    size_t dead_bytes = 0;
    size_t block_count = 0;
    size_t block_capacity = 0;
    size_t block_used = 0;

    RtBlockV2 *block = arena->blocks_head;
    while (block != NULL) {
        block_count++;
        block_capacity += block->capacity;
        block_used += block->used;

        RtHandleV2 *h = block->handles_head;
        while (h != NULL) {
            if (h->flags & RT_HANDLE_FLAG_DEAD) {
                dead_handles++;
                dead_bytes += h->size;
            } else {
                live_handles++;
                live_bytes += h->size;
            }
            h = h->next;
        }
        block = block->next;
    }

    stats->handles.local = live_handles;
    stats->bytes.local = live_bytes;
    stats->blocks.local = block_count;
    stats->dead_handles = dead_handles;
    stats->dead_bytes = dead_bytes;
    stats->block_capacity = block_capacity;
    stats->block_used = block_used;

    /* Compute fragmentation */
    if (block_used > 0) {
        stats->fragmentation = 1.0 - ((double)live_bytes / (double)block_used);
    } else {
        stats->fragmentation = 0.0;
    }
}

/* Recursively compute stats for children and aggregate */
static void compute_children_stats(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    size_t child_handles = 0;
    size_t child_bytes = 0;
    size_t child_blocks = 0;

    RtArenaV2 *child = arena->first_child;
    while (child != NULL) {
        /* Recursively recompute child stats first */
        rt_arena_stats_recompute(child);

        /* Aggregate child totals (which include their children) */
        child_handles += child->stats.handles.total;
        child_bytes += child->stats.bytes.total;
        child_blocks += child->stats.blocks.total;

        child = child->next_sibling;
    }

    stats->handles.children = child_handles;
    stats->bytes.children = child_bytes;
    stats->blocks.children = child_blocks;
}

void rt_arena_stats_recompute(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    /* Preserve GC run count and last GC results */
    size_t gc_runs = arena->stats.gc_runs;
    size_t last_handles_freed = arena->stats.last_handles_freed;
    size_t last_bytes_freed = arena->stats.last_bytes_freed;
    size_t last_blocks_freed = arena->stats.last_blocks_freed;

    /* Compute local stats */
    compute_local_stats(arena, &arena->stats);

    pthread_mutex_unlock(&arena->mutex);

    /* Compute children stats (recursive, needs mutex released) */
    compute_children_stats(arena, &arena->stats);

    /* Compute totals */
    arena->stats.handles.total = arena->stats.handles.local + arena->stats.handles.children;
    arena->stats.bytes.total = arena->stats.bytes.local + arena->stats.bytes.children;
    arena->stats.blocks.total = arena->stats.blocks.local + arena->stats.blocks.children;

    /* Restore GC metrics */
    arena->stats.gc_runs = gc_runs;
    arena->stats.last_handles_freed = last_handles_freed;
    arena->stats.last_bytes_freed = last_bytes_freed;
    arena->stats.last_blocks_freed = last_blocks_freed;

    /* Log if enabled */
    if (arena->gc_log_enabled) {
        fprintf(stderr, "[GC] arena=%s handles=%zu/%zu bytes=%zu/%zu blocks=%zu freed=%zu/%zu/%zu\n",
                arena->name ? arena->name : "(unnamed)",
                arena->stats.handles.local, arena->stats.handles.total,
                arena->stats.bytes.local, arena->stats.bytes.total,
                arena->stats.blocks.local,
                last_handles_freed, last_bytes_freed, last_blocks_freed);
    }
}

/* ============================================================================
 * Statistics API
 * ============================================================================ */

void rt_arena_stats_get(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    if (arena == NULL || stats == NULL) return;

    pthread_mutex_lock(&arena->mutex);
    memcpy(stats, &arena->stats, sizeof(RtArenaV2Stats));
    pthread_mutex_unlock(&arena->mutex);
}

void rt_arena_stats_print(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    RtArenaV2Stats s;
    rt_arena_stats_get(arena, &s);

    fprintf(stderr, "Arena '%s' stats:\n", arena->name ? arena->name : "(unnamed)");
    fprintf(stderr, "  Handles:       %zu local, %zu children, %zu total (%zu dead)\n",
            s.handles.local, s.handles.children, s.handles.total, s.dead_handles);
    fprintf(stderr, "  Bytes:         %zu local, %zu children, %zu total (%zu dead)\n",
            s.bytes.local, s.bytes.children, s.bytes.total, s.dead_bytes);
    fprintf(stderr, "  Blocks:        %zu local, %zu children, %zu total\n",
            s.blocks.local, s.blocks.children, s.blocks.total);
    fprintf(stderr, "  Block space:   %zu used / %zu capacity\n",
            s.block_used, s.block_capacity);
    fprintf(stderr, "  Fragmentation: %.1f%%\n", s.fragmentation * 100.0);
    fprintf(stderr, "  GC runs:       %zu (last: %zu handles, %zu bytes, %zu blocks freed)\n",
            s.gc_runs, s.last_handles_freed, s.last_bytes_freed, s.last_blocks_freed);
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
