/*
 * Arena Stats - Statistics and Observability
 * ===========================================
 * Opt-in observability for arena memory behavior.
 * Stats are recomputed on demand when rt_arena_stats_get() is called.
 */

#ifndef ARENA_STATS_H
#define ARENA_STATS_H

#include <stddef.h>
#include "arena_gc.h"

/* Forward declaration */
typedef struct RtArenaV2 RtArenaV2;

/* ============================================================================
 * Statistics Types
 * ============================================================================ */

/* Metric with local/child/total breakdown */
typedef struct {
    size_t local;      /* This arena only */
    size_t children;   /* Sum of child arenas (recursive) */
    size_t total;      /* local + children */
} RtArenaV2Metric;

/* Arena statistics - snapshot computed during GC */
typedef struct {
    /* Live resource metrics (local/children/total) */
    RtArenaV2Metric handles;     /* Live handle counts */
    RtArenaV2Metric bytes;       /* Live byte counts */
    RtArenaV2Metric blocks;      /* Block counts */

    /* Reclaimable resources (local only) */
    size_t dead_handles;         /* Dead handles awaiting collection */
    size_t dead_bytes;           /* Bytes held by dead handles */

    /* Block utilization (local only) */
    size_t block_capacity;       /* Sum of all block capacities */
    size_t block_used;           /* Sum of all block used (bump pointer) */

    /* GC metrics */
    size_t gc_runs;              /* Total GC passes on this arena */
    size_t last_handles_freed;   /* Handles freed in last GC */
    size_t last_bytes_freed;     /* Bytes freed in last GC */
    size_t last_blocks_freed;    /* Blocks freed in last GC */

    /* Computed */
    double fragmentation;        /* Wasted space ratio */
} RtArenaV2Stats;

/* ============================================================================
 * GC Support Functions (called by arena_gc.c)
 * ============================================================================ */

/* Record results from a GC cycle */
void rt_arena_stats_record_gc(RtArenaV2 *arena, const RtArenaGCResult *result);

/* ============================================================================
 * Statistics API
 * ============================================================================ */

/* Get stats for an arena (copies arena->stats) */
void rt_arena_stats_get(RtArenaV2 *arena, RtArenaV2Stats *stats);

/* Print human-readable summary to stderr */
void rt_arena_stats_print(RtArenaV2 *arena);

/* Print detailed per-block breakdown to stderr */
void rt_arena_stats_snapshot(RtArenaV2 *arena);

/* Enable/disable one-line GC logging per pass to stderr */
void rt_arena_stats_enable_gc_log(RtArenaV2 *arena);
void rt_arena_stats_disable_gc_log(RtArenaV2 *arena);

#endif /* ARENA_STATS_H */
