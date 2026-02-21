/*
 * Arena Stats - Statistics and Observability Implementation
 * ==========================================================
 * Stats are recomputed on demand when rt_arena_stats_get() is called.
 */

#include "arena_stats.h"
#include "arena_v2.h"
#include "arena_handle.h"

#include <stdio.h>
#include <string.h>

/* Forward declaration - used recursively by compute_children_stats */
static void recompute_stats(RtArenaV2 *arena);

/* ============================================================================
 * Internal: Stats Computation
 * ============================================================================ */

/* Compute local stats for a single arena (not including children) */
static void compute_local_stats(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    size_t live_handles = 0;
    size_t dead_handles = 0;
    size_t live_bytes = 0;
    size_t dead_bytes = 0;

    RtHandleV2 *h = arena->handles_head;
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

    stats->handles.local = live_handles;
    stats->bytes.local = live_bytes;
    stats->dead_handles = dead_handles;
    stats->dead_bytes = dead_bytes;
}

/* Recursively compute stats for children and aggregate */
static void compute_children_stats(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    size_t child_handles = 0;
    size_t child_bytes = 0;

    RtArenaV2 *child = arena->first_child;
    while (child != NULL) {
        /* Recursively recompute child stats first */
        recompute_stats(child);

        /* Aggregate child totals (which include their children) */
        child_handles += child->stats.handles.total;
        child_bytes += child->stats.bytes.total;

        child = child->next_sibling;
    }

    stats->handles.children = child_handles;
    stats->bytes.children = child_bytes;
}

static void recompute_stats(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    /* Preserve GC run count and last GC results */
    size_t gc_runs = arena->stats.gc_runs;
    size_t last_handles_freed = arena->stats.last_handles_freed;
    size_t last_bytes_freed = arena->stats.last_bytes_freed;

    /* Compute local stats */
    compute_local_stats(arena, &arena->stats);

    pthread_mutex_unlock(&arena->mutex);

    /* Compute children stats (recursive, needs mutex released) */
    compute_children_stats(arena, &arena->stats);

    /* Compute totals */
    arena->stats.handles.total = arena->stats.handles.local + arena->stats.handles.children;
    arena->stats.bytes.total = arena->stats.bytes.local + arena->stats.bytes.children;

    /* Restore GC metrics */
    arena->stats.gc_runs = gc_runs;
    arena->stats.last_handles_freed = last_handles_freed;
    arena->stats.last_bytes_freed = last_bytes_freed;
}

/* ============================================================================
 * GC Support Functions (called by arena_gc.c)
 * ============================================================================ */

void rt_arena_stats_record_gc(RtArenaV2 *arena, const RtArenaGCResult *result)
{
    if (arena == NULL || result == NULL) return;

    arena->stats.gc_runs++;
    arena->stats.last_handles_freed = result->handles_freed;
    arena->stats.last_bytes_freed = result->bytes_freed;

    /* Log if enabled */
    if (arena->gc_log_enabled) {
        recompute_stats(arena);
        fprintf(stderr, "[GC] arena=%s handles=%zu/%zu bytes=%zu/%zu freed=%zu/%zu\n",
                arena->name ? arena->name : "(unnamed)",
                arena->stats.handles.local, arena->stats.handles.total,
                arena->stats.bytes.local, arena->stats.bytes.total,
                result->handles_freed, result->bytes_freed);
    }
}

/* ============================================================================
 * Statistics API
 * ============================================================================ */

void rt_arena_stats_get(RtArenaV2 *arena, RtArenaV2Stats *stats)
{
    if (arena == NULL || stats == NULL) return;

    recompute_stats(arena);

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
    fprintf(stderr, "  GC runs:       %zu (last: %zu handles, %zu bytes freed)\n",
            s.gc_runs, s.last_handles_freed, s.last_bytes_freed);
}

void rt_arena_stats_snapshot(RtArenaV2 *arena)
{
    if (arena == NULL) return;

    pthread_mutex_lock(&arena->mutex);

    fprintf(stderr, "=== Arena Snapshot: '%s' ===\n",
            arena->name ? arena->name : "(unnamed)");

    size_t total_live = 0;
    size_t total_dead = 0;
    size_t live_bytes = 0;
    size_t dead_bytes = 0;

    RtHandleV2 *h = arena->handles_head;
    while (h != NULL) {
        if (h->flags & RT_HANDLE_FLAG_DEAD) {
            total_dead++;
            dead_bytes += h->size;
        } else {
            total_live++;
            live_bytes += h->size;
        }
        h = h->next;
    }

    fprintf(stderr, "  --- %zu live handles (%zu bytes), %zu dead handles (%zu bytes) ---\n",
            total_live, live_bytes, total_dead, dead_bytes);

    pthread_mutex_unlock(&arena->mutex);
}

long long rt_arena_v2_get_handle_count(RtArenaV2 *arena)
{
    if (arena == NULL) return 0;
    RtArenaV2Stats stats;
    rt_arena_stats_get(arena, &stats);
    return (long long)stats.handles.total;
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
