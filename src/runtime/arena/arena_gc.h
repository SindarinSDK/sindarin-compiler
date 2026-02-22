/*
 * Arena GC - Garbage Collection
 * ==============================
 * Stop-the-world garbage collection for arena memory management.
 * Two-pass algorithm:
 * 1. Dead arena sweep - remove arenas marked with RT_ARENA_FLAG_DEAD
 * 2. Handle collection - collect dead handles, free data via free()
 */

#ifndef ARENA_GC_H
#define ARENA_GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct RtArenaV2 RtArenaV2;

/* GC result - tracks what was freed during a GC cycle */
typedef struct {
    size_t handles_freed;       /* Dead handles collected from live arenas (Phase 2) */
    size_t bytes_freed;         /* Bytes freed from dead handles (Phase 2) */
    size_t arenas_freed;        /* Condemned arenas destroyed (Phase 1+3) */
    size_t arena_bytes_freed;   /* Bytes freed from condemned arenas (Phase 3) */
    size_t gc_calls;            /* Cumulative GC entry attempts */
    size_t gc_skips;            /* Cumulative GC skips (gc_running was true) */
} RtArenaGCResult;

/* Run GC on arena tree. Returns total handles collected.
 * This is the main GC entry point - call on root arena.
 *
 * Pass 1: Sweeps dead arenas (marked with RT_ARENA_FLAG_DEAD)
 * Pass 2: Collects dead handles in all live arenas
 */
size_t rt_arena_v2_gc(RtArenaV2 *arena);

/* Synchronously destroy an arena and all its children/handles.
 * Use for detached arenas (parent=NULL) that GC cannot reach.
 * For arenas in the GC tree, use rt_arena_v2_condemn() instead. */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);

#endif /* ARENA_GC_H */
