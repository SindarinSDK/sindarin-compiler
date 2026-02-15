# GC Implementation Tasks

## Status Legend
- [ ] Not started
- [~] In progress
- [x] Complete

---

## Phase 1: Thread ID System

### Task 1.1: Add thread_id to RtThread struct
- [x] Add `uint64_t thread_id` field to `RtThread` in `runtime_thread_v2.h`

### Task 1.2: Add global thread ID counter and TLS
- [x] Add `static _Atomic uint64_t g_thread_id_counter = 1` in `runtime_thread_v2.c`
- [x] Add `RT_THREAD_LOCAL_V2 uint64_t rt_current_thread_id = 0` in `runtime_thread_v2.c`

### Task 1.3: Implement rt_thread_get_id()
- [x] Add declaration in `runtime_thread_v2.h`
- [x] Implement with lazy init for main thread in `runtime_thread_v2.c`

### Task 1.4: Modify rt_thread_v2_create()
- [x] Assign `t->thread_id = atomic_fetch_add(&g_thread_id_counter, 1)`

### Task 1.5: Modify rt_tls_thread_set()
- [x] Also set `rt_current_thread_id = t->thread_id` when `t != NULL`

---

## Phase 2: Block Transaction State

### Task 2.1: Add transaction fields to RtBlockV2
- [x] Add `_Atomic uint64_t tx_holder` (0 = free, thread ID = held)
- [x] Add `_Atomic uint32_t tx_recurse_count` (nesting depth)
- [x] Add `_Atomic uint64_t tx_start_ns` (lease start time)
- [x] Add `_Atomic uint64_t tx_timeout_ns` (per-transaction timeout)

### Task 2.2: Initialize transaction fields in block_create()
- [x] Set all transaction fields to 0 on block creation

### Task 2.3: Add constants
- [x] `#define TX_DEFAULT_TIMEOUT_NS (2ULL * 1000000000ULL)`
- [x] `#define GC_OWNER_ID UINT64_MAX`

---

## Phase 3: Transaction API

### Task 3.1: Implement rt_handle_begin_transaction()
- [x] Add declaration in `arena_v2.h`
- [x] Implement: check if same thread holds (increment count), else CAS to acquire
- [x] Set start time and timeout on acquire

### Task 3.2: Implement rt_handle_begin_transaction_with_timeout()
- [x] Add declaration in `arena_v2.h`
- [x] Implement with custom timeout parameter

### Task 3.3: Implement rt_handle_end_transaction()
- [x] Add declaration in `arena_v2.h`
- [x] Verify caller holds lease (panic if not)
- [x] Decrement recurse count, release when count = 0

### Task 3.4: Implement rt_handle_renew_transaction()
- [x] Add declaration in `arena_v2.h`
- [x] Verify caller holds lease (panic if not)
- [x] Update tx_start_ns to current time

---

## Phase 4: Remove Pinning API

### Task 4.1: Remove pin_count from RtHandleV2
- [x] Remove `int pin_count` field from struct in `arena_v2.h`

### Task 4.2: Remove rt_handle_v2_pin() and rt_handle_v2_unpin()
- [x] Remove function declarations from `arena_v2.h`
- [x] Remove function implementations from `arena_v2.c`

### Task 4.3: Update all pin/unpin call sites
- [x] Search for all `rt_handle_v2_pin` calls in runtime
- [x] Replace with `rt_handle_begin_transaction` in runtime
- [x] Search for all `rt_handle_v2_unpin` calls in runtime
- [x] Replace with `rt_handle_end_transaction` in runtime
- Note: Codegen still emits pin/unpin calls - will be updated in Phase 6

---

## Phase 5: GC Implementation

### Task 5.1: Remove old GC functions
- [x] Remove `rt_arena_v2_gc_recursive()`
- [x] Remove `rt_arena_v2_gc_flush()`
- [x] Remove declarations from `arena_v2.h`

### Task 5.2: Implement gc_acquire_block()
- [x] Try CAS to acquire block
- [x] If held, check timeout
- [x] Force-acquire if expired, skip if valid lease

### Task 5.3: Implement gc_release_block()
- [x] Set tx_holder to 0

### Task 5.4: Implement gc_sweep_dead_arenas() (Pass 1)
- [x] Recursive traversal from root
- [x] Unlink and destroy dead arenas
- [x] Recurse into live children

### Task 5.5: Implement gc_compact_block()
- [x] Free dead handles (basic compaction)
- Note: Full memory compaction (copy live data contiguously) deferred for now - current implementation frees dead handles but doesn't relocate live data

### Task 5.6: Implement gc_compact_arena() (Pass 2)
- [x] For each block in arena
- [x] Try to acquire block
- [x] If acquired, compact it
- [x] Release block

### Task 5.7: Implement consolidated rt_arena_v2_gc()
- [x] Single entry point taking root arena
- [x] Call gc_sweep_dead_arenas() (Pass 1)
- [x] Call gc_compact_arena() recursively (Pass 2)
- [x] Return total handles collected

---

## Phase 5.5: Header Refactoring (Separation of Concerns)

### Task 5.5.1: Create arena_handle.h
- [x] Move handle operations (rt_handle_v2_arena, rt_handle_v2_is_valid)
- [x] Move transaction API declarations
- [x] Move handle flags enum
- [x] Include from arena_v2.h

### Task 5.5.2: Create arena_gc.h
- [x] Move GC function declarations (rt_arena_v2_gc)
- [x] Move GC_OWNER_ID constant
- [x] Include from arena_v2.h

### Task 5.5.3: Create arena_stats.h
- [x] Move RtArenaV2Stats typedef
- [x] Move RtArenaV2GCReport typedef
- [x] Move all rt_arena_stats_* function declarations
- [x] Move backward compatibility wrappers
- [x] Include from arena_v2.h

### Task 5.5.4: Create arena_id.h
- [x] Add rt_arena_get_thread_id() declaration
- [x] Implement wrapper that calls rt_thread_get_id()
- [x] Update arena_v2.c to use arena_id.h for thread IDs

### Task 5.5.5: Clean up arena_v2.h
- [x] Keep only core types (structs, enums for arena/block)
- [x] Keep arena lifecycle functions
- [x] Keep allocation functions
- [x] Keep promotion functions
- [x] Keep TLS arena functions
- [x] Include split headers for full API

---

## Phase 5.6: Move Stats Implementation to arena_stats.c

### Task 5.6.1: Create arena_stats.c
- [x] Create new source file with stats implementations
- [x] Move rt_arena_stats_get() implementation
- [x] Move rt_arena_stats_print() implementation
- [x] Move rt_arena_stats_last_gc() implementation
- [x] Move rt_arena_stats_snapshot() implementation
- [x] Move rt_arena_stats_enable_gc_log() implementation
- [x] Move rt_arena_stats_disable_gc_log() implementation

### Task 5.6.2: Update arena_v2.c
- [x] Remove stats implementations from arena_v2.c

### Task 5.6.3: Update build system
- [x] Add arena_stats.c to CMakeLists.txt (SN_RUNTIME_SOURCES)
- [x] Add arena_stats.c to CMakeLists.txt (SN_RUNTIME_LIB_SOURCES)

---

## Phase 5.7: Stats Structure Refactoring

Design: Single stats object per arena, updated only by GC cycle. Normal arena operations don't touch stats.

### Final Structure
```c
/* Metric with local/child/total breakdown */
typedef struct {
    size_t local;      /* This arena only */
    size_t children;   /* Sum of child arenas (recursive) */
    size_t total;      /* local + children */
} RtArenaV2Metric;

typedef struct {
    /* Live resource metrics (local/children/total) */
    RtArenaV2Metric handles;     /* Live handle counts */
    RtArenaV2Metric bytes;       /* Live byte counts */
    RtArenaV2Metric blocks;      /* Block counts */

    /* Reclaimable resources (local only) */
    size_t dead_handles;
    size_t dead_bytes;

    /* Block utilization (local only) */
    size_t block_capacity;
    size_t block_used;

    /* GC metrics */
    size_t gc_runs;
    size_t last_handles_freed;
    size_t last_bytes_freed;
    size_t last_blocks_freed;

    /* Computed */
    double fragmentation;
} RtArenaV2Stats;
```

### Task 5.7.1: Create arena_gc.c and update arena_gc.h
- [x] Add RtArenaGCResult struct to arena_gc.h:
```c
typedef struct {
    size_t handles_freed;
    size_t bytes_freed;
    size_t blocks_freed;
} RtArenaGCResult;
```
- [x] Create arena_gc.c
- [x] Move GC functions from arena_v2.c to arena_gc.c:
  - gc_acquire_block()
  - gc_release_block()
  - gc_sweep_dead_arenas()
  - gc_compact_block()
  - gc_compact_arena()
  - gc_compact_all()
  - rt_arena_v2_gc()
- [x] Move get_monotonic_ns() helper kept in arena_v2.c (used by transactions)
- [x] Add arena_gc.c to CMakeLists.txt
- [x] Created arena_internal.h for shared internal functions

### Task 5.7.2: Update arena_stats.h
- [x] Add RtArenaV2Metric typedef
- [x] Replace RtArenaV2Stats with new structure
- [x] Remove RtArenaV2GCReport (folded into RtArenaV2Stats)
- [x] Add declaration: `void rt_arena_stats_recompute(RtArenaV2 *arena)`
- [x] Add declaration: `void rt_arena_stats_record_gc(RtArenaV2 *arena, const RtArenaGCResult *result)`
- [x] Include arena_gc.h for RtArenaGCResult type

### Task 5.7.3: Update arena_v2.h
- [x] Remove scattered stats fields from RtArenaV2 struct:
  - total_allocated, total_freed, gc_runs
  - handles_created, handles_collected
  - blocks_created, blocks_freed
  - last_gc_handles_swept, last_gc_handles_collected
  - last_gc_blocks_swept, last_gc_blocks_freed
  - last_gc_bytes_collected
- [x] Add single `RtArenaV2Stats stats` field to RtArenaV2
- [x] Keep gc_log_enabled as separate field

### Task 5.7.4: Update arena_v2.c
- [x] Remove stats field updates from allocation functions
- [x] Remove stats field updates from free functions
- [x] Update rt_arena_v2_create() to zero-init stats
- [x] Remove backward compat wrappers (rt_arena_total_allocated)

### Task 5.7.5: Update arena_stats.c with GC support functions
- [x] Implement rt_arena_stats_recompute():
  - Walk blocks/handles to compute local metrics
  - Recurse into children, sum their stats
  - Compute total = local + children
  - Compute fragmentation
- [x] Implement rt_arena_stats_record_gc(arena, result):
  - Increment gc_runs
  - Copy result->handles_freed, bytes_freed, blocks_freed to stats

### Task 5.7.6: Update GC functions (arena_gc.c)
- [x] Use RtArenaGCResult to track collection counts during GC
- [x] At end of rt_arena_v2_gc(), call rt_arena_stats_record_gc(arena, &result)
- [x] At end of rt_arena_v2_gc(), call rt_arena_stats_recompute(arena)
- [x] Keep GC code clean - no inline stats computation

### Task 5.7.7: Update arena_stats.c API functions
- [x] Update rt_arena_stats_get() to copy arena->stats
- [x] Update rt_arena_stats_print() for new structure
- [x] Remove rt_arena_stats_last_gc() (data now in main stats)
- [x] Update rt_arena_stats_snapshot() for new structure

### Task 5.7.8: Update call sites
- [x] Find all references to removed fields
- [x] Update to use arena->stats.X instead
- [x] Update tests that use old stats API (no changes needed)

### Task 5.7.9: Build and test
- [x] Full rebuild
- [x] Run all tests (1907 passed)
- [x] Verify stats are correct after GC cycles

---

## Phase 5.8: Remove rt_arena_destroy Wrapper

Note: Intentionally deferred. The `rt_arena_destroy()` shim was kept (and re-added)
for backward compatibility with SDK native `.sn.c` files (e.g., `random.sn.c`) that
call `rt_arena_destroy()`. It maps to `rt_arena_v2_condemn()` internally.

### Task 5.8.1: Remove rt_arena_destroy from arena_v2.h
- [x] Remove `rt_arena_destroy()` inline function
- [x] Keep only `rt_arena_v2_condemn()` for arena destruction

### Task 5.8.2: Update call sites
- [x] Find all uses of `rt_arena_destroy()`
- [x] Replace with `rt_arena_v2_condemn()`

### Task 5.8.3: Build and test
- [x] Full rebuild
- [x] Run all tests

---

## Phase 5.9: Create arena_handle.c

All functions declared in arena_handle.h should be implemented in arena_handle.c.

### Task 5.9.1: Create arena_handle.c
- [x] Create arena_handle.c source file
- [x] Move `rt_handle_v2_arena()` from inline to arena_handle.c
- [x] Move `rt_handle_v2_is_valid()` from inline to arena_handle.c

### Task 5.9.2: Move transaction functions from arena_v2.c
- [x] Move `rt_handle_begin_transaction()` to arena_handle.c
- [x] Move `rt_handle_begin_transaction_with_timeout()` to arena_handle.c
- [x] Move `rt_handle_end_transaction()` to arena_handle.c
- [x] Move `rt_handle_renew_transaction()` to arena_handle.c
- [x] Move `rt_get_monotonic_ns()` helper to arena_handle.c (used by transactions)

### Task 5.9.3: Update arena_handle.h
- [x] Remove inline implementations
- [x] Keep only function declarations

### Task 5.9.4: Update build system
- [x] Add arena_handle.c to CMakeLists.txt (SN_RUNTIME_SOURCES)
- [x] Add arena_handle.c to CMakeLists.txt (SN_RUNTIME_LIB_SOURCES)

### Task 5.9.5: Build and test
- [x] Full rebuild
- [x] Run all tests

---

## Phase 6: Codegen Updates

See `gc-tasks-v2.md` for the detailed breakdown (Phases 6.1-6.10, all complete).

### Task 6.1: Update handle access in codegen
- [x] Emit `rt_handle_begin_transaction` before ptr access
- [x] Emit `rt_handle_end_transaction` after ptr access

### Task 6.2: Update array operations in codegen
- [x] Wrap array element access in transactions

### Task 6.3: Update string operations in codegen
- [x] Wrap string operations in transactions

### Task 6.4: Add renewal for long operations
- [ ] Identify long-running loops in generated code
- [ ] Add periodic `rt_handle_renew_transaction` calls
- Note: Deferred — not yet needed since current transaction timeout (2s) is sufficient for all existing use cases

---

## Phase 7: Testing

Note: All existing tests pass (~3290 compiler, 28 SDK, 4 HTTP). This phase is for
adding dedicated transaction/GC unit tests beyond integration coverage.

### Task 7.1: Update existing tests
- [ ] Replace pin/unpin with transactions in test code

### Task 7.2: Add transaction tests
- [ ] Test basic begin/end
- [ ] Test nested transactions (same thread, same block)
- [ ] Test timeout behavior
- [ ] Test renewal

### Task 7.3: Add GC compaction tests
- [ ] Test dead handle removal
- [ ] Test block compaction
- [ ] Test handle pointer updates after compaction

### Task 7.4: Stress test
- [ ] High concurrency transaction test
- [ ] GC under load test

---

## Phase 8: Cleanup

### Task 8.1: Remove old GC thread code (already done)
- [x] Remove gc_thread_start/stop from codegen
- [x] Remove gc thread functions from arena_v2.c
- [x] Remove declarations from arena_v2.h

### Task 8.2: Update documentation
- [ ] Update code comments
- [ ] Update any existing docs referencing old GC

### Task 8.3: Final compilation and test run
- [ ] Full rebuild
- [ ] Run all tests
- [ ] Run benchmark to verify no memory leaks
