/*
 * Arena GC - Garbage Collection
 * ==============================
 * Stop-the-world garbage collection for arena memory management.
 * Two-pass algorithm:
 * 1. Dead arena sweep - remove arenas marked with RT_ARENA_FLAG_DEAD
 * 2. Block compaction - acquire blocks, compact live handles, free dead ones
 */

#ifndef ARENA_GC_H
#define ARENA_GC_H

#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct RtArenaV2 RtArenaV2;

/* Special marker for GC ownership of blocks */
#define GC_OWNER_ID UINT64_MAX

/* GC result - tracks what was freed during a GC cycle */
typedef struct {
    size_t handles_freed;
    size_t bytes_freed;
    size_t blocks_freed;
} RtArenaGCResult;

/* Run GC on arena tree. Returns total handles collected.
 * This is the main GC entry point - call on root arena.
 *
 * Pass 1: Sweeps dead arenas (marked with RT_ARENA_FLAG_DEAD)
 * Pass 2: Compacts blocks in all live arenas (acquires block locks)
 */
size_t rt_arena_v2_gc(RtArenaV2 *arena);

#endif /* ARENA_GC_H */
