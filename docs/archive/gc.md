# Stop-the-World Garbage Collection Design

## Overview

This document describes the redesigned garbage collection system for the Sindarin runtime. The key change is replacing the background GC thread with a stop-the-world GC that uses transactional handle access for safe memory compaction.

## Problems with Previous Design

1. **Background GC thread race conditions**: The GC thread could traverse arena/block structures while worker threads were modifying them, causing crashes.

2. **Pinning prevents compaction**: Long-held pins on handles prevent memory from being moved or compacted, leading to fragmentation.

3. **No memory compaction**: Dead handles leave gaps in blocks, wasting memory over time.

## New Design Principles

### 1. Transactional Handle Access

All access to `handle->ptr` must occur within a transaction:

```c
rt_handle_begin_transaction(handle);
// Access handle->ptr safely here
char *data = (char *)handle->ptr;
process(data);
rt_handle_end_transaction(handle);
```

**Rules:**
- Never hold `handle->ptr` outside a transaction
- Transactions have a timeout (default 2 seconds)
- Long-running operations must call `rt_handle_renew_transaction()` periodically
- GC can force-acquire expired leases

### 2. Block-Level Locking

Locks are held at the block level, not per-handle:

```c
typedef struct RtBlockV2 {
    // ... existing fields ...
    _Atomic uint64_t tx_holder;        // 0 = free, thread ID = held, GC_OWNER_ID = GC
    _Atomic uint32_t tx_recurse_count; // nesting depth for same thread
    _Atomic uint64_t tx_start_ns;      // lease start time (monotonic)
    _Atomic uint64_t tx_timeout_ns;    // per-transaction timeout
} RtBlockV2;
```

### 3. Thread ID System

Each thread gets a unique runtime-assigned ID:

- Global atomic counter `g_thread_id_counter` generates unique IDs
- TLS variable `rt_current_thread_id` stores current thread's ID
- Main thread gets ID lazily on first `rt_thread_get_id()` call
- Worker threads get ID in `rt_thread_v2_create()`, set in TLS via `rt_tls_thread_set()`

```c
// In RtThread struct
typedef struct RtThread {
    // ...
    uint64_t thread_id;  // unique runtime thread ID
    // ...
} RtThread;

// Get current thread's ID (lazy init for main thread)
uint64_t rt_thread_get_id(void);
```

### 4. Nested Transactions

Same thread can nest transactions on the same block:

```c
rt_handle_begin_transaction(handle_a);  // block X: holder=42, count=1
rt_handle_begin_transaction(handle_b);  // same block X: count=2
// work with both
rt_handle_end_transaction(handle_b);    // count=1, still held
rt_handle_end_transaction(handle_a);    // count=0, released
```

The recursion counter tracks nesting depth. Only when count reaches 0 is the block actually released.

### 5. GC Always Wins

When a transaction exceeds its timeout:

1. GC can force-acquire the block by setting `tx_holder = GC_OWNER_ID`
2. The original holder will crash if they try to access `ptr` (their fault for violating the contract)
3. This ensures GC can always make progress

## Transaction API

```c
// Acquire lease with default 2s timeout
void rt_handle_begin_transaction(RtHandleV2 *handle);

// Acquire lease with custom timeout
void rt_handle_begin_transaction_with_timeout(RtHandleV2 *handle, uint64_t timeout_ns);

// Renew lease timestamp (panics if not held)
void rt_handle_renew_transaction(RtHandleV2 *handle);

// Release lease (decrements nesting count, releases when count=0)
void rt_handle_end_transaction(RtHandleV2 *handle);
```

## GC Algorithm

### Entry Point

Single consolidated function:

```c
size_t rt_arena_v2_gc(RtArenaV2 *root);
```

### Pass 1: Dead Arena Sweep

Starting from the root arena, recursively traverse all child arenas:
- If arena has `RT_ARENA_FLAG_DEAD`, unlink from parent and destroy
- Otherwise, recurse into live children

### Pass 2: Block Compaction

For each live arena, for each block:

1. Try to acquire block (check `tx_holder`)
2. If held with valid lease, skip this block
3. If held with expired lease, force-acquire
4. Create new block with only live handles (contiguous, no gaps)
5. For each live handle:
   - Copy data to new block
   - Update `handle->ptr` to new location
   - Update `handle->block` to new block
6. Free old block

### Handle Update During Compaction

The `RtHandleV2` struct is malloc'd separately from block data, so it doesn't move. Only these fields are updated:
- `handle->ptr` - points to new data location
- `handle->block` - points to new block

User code holding `RtHandleV2*` remains valid.

## Allocation Model

Key properties (from code analysis):

1. **Allocations never span blocks**: Large allocations get their own dedicated block
2. **Block sizing**: Default 64KB, expands for larger allocations
3. **Handle struct separate**: `RtHandleV2` is malloc'd, not in block memory
4. **One handle = one block**: Locking a block covers the entire allocation

## Migration from Pinning

The old pinning API is removed:
- `rt_handle_v2_pin()` - removed
- `rt_handle_v2_unpin()` - removed
- `handle->pin_count` field - removed

All code must migrate to transactions:

```c
// OLD (removed)
rt_handle_v2_pin(handle);
char *ptr = (char *)handle->ptr;
process(ptr);
rt_handle_v2_unpin(handle);

// NEW
rt_handle_begin_transaction(handle);
char *ptr = (char *)handle->ptr;
process(ptr);
rt_handle_end_transaction(handle);
```

## Long-Running Operations

For operations that may exceed the default timeout:

```c
// Option 1: Longer timeout
rt_handle_begin_transaction_with_timeout(handle, 30ULL * 1000000000ULL); // 30s

// Option 2: Periodic renewal
rt_handle_begin_transaction(handle);
for (size_t i = 0; i < huge_length; i++) {
    process(data[i]);
    if ((i & 0xFFFF) == 0) {
        rt_handle_renew_transaction(handle);  // Reset timeout
    }
}
rt_handle_end_transaction(handle);
```

## Constants

```c
#define TX_DEFAULT_TIMEOUT_NS (2ULL * 1000000000ULL)  // 2 seconds
#define GC_OWNER_ID UINT64_MAX                         // Special marker for GC
```
