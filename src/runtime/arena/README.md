# Managed Arena

A handle-based memory manager built on top of `RtArena` that supports safe
reassignment, concurrent cleanup, and best-effort compaction.

## Problem

The standard `RtArena` is a bump allocator — fast allocation, bulk deallocation,
but no individual reclamation. When global variables are repeatedly reassigned,
dead allocations accumulate indefinitely until the arena is destroyed.

## Solution

Wrap arena allocations with a **handle table** that tracks liveness. Instead of
raw pointers, consumers receive opaque handles. Access requires pinning (leasing)
the handle to obtain a raw pointer. Background threads reclaim dead allocations
and compact live data.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Public API                                             │
│    rt_managed_alloc / rt_managed_pin / rt_managed_unpin │
├─────────────────────────────────────────────────────────┤
│  Handle Table                                           │
│    [handle] → { ptr, size, leased (atomic), dead }      │
├─────────────────────────────────────────────────────────┤
│  Backing Store (RtArena)                                │
│    [block1] → [block2] → [block3] → ...                │
├─────────────────────────────────────────────────────────┤
│  Background Threads                                     │
│    Cleaner:   zeros dead + unleased entries             │
│    Compactor: hot-swaps to fresh arena with live data   │
└─────────────────────────────────────────────────────────┘
```

## Memory Layout

Each allocation in the backing arena:
```
[data bytes .................]
 ^
 |-- HandleEntry.ptr points here
```

Handle table entry:
```c
typedef struct {
    void *ptr;            // pointer to data in arena block
    size_t size;          // allocation size
    _Atomic int leased;   // pin/lease counter (atomic)
    bool dead;            // marked for reclamation
} HandleEntry;
```

## Lifecycle

### Allocation
```
rt_managed_alloc(arena, old_handle, size)
  1. If old_handle != 0: mark old entry as dead
  2. Bump-allocate `size` bytes from backing RtArena
  3. Create new HandleEntry { ptr, size, leased=0, dead=false }
  4. Return new handle (table index)
```

### Pin (Lease)
```
rt_managed_pin(arena, handle) → void *
  1. atomic_fetch_add(&entry->leased, 1)
  2. Return entry->ptr (safe — can't be moved while leased)
```

### Unpin (Release)
```
rt_managed_unpin(arena, handle)
  1. atomic_fetch_sub(&entry->leased, 1)
```

### Cleaner Thread
```
Loop:
  For each entry where dead == true && leased == 0:
    memset(entry->ptr, 0, entry->size)   // zero the gap
    entry->ptr = NULL                     // slot reclaimable
    Add handle index to free list         // recycle handle
```

### Compactor Thread (Best-Effort Hot Swap)
```
When fragmentation_ratio > threshold:
  1. new_arena = rt_arena_create(NULL)
  2. For each live + unleased entry:
       - CAS leased: 0 → MOVING sentinel (-1)
       - If CAS fails (pinned): skip, best-effort
       - Copy data to new_arena
       - Update entry->ptr to new location
       - Set leased back to 0
  3. Retire old arena (freed when pin_count drains to 0)
```

## Concurrency Model

- **Application thread**: calls alloc/pin/unpin
- **Cleaner thread**: zeros dead entries (low priority, continuous)
- **Compactor thread**: hot-swaps arenas (periodic, best-effort)

Safety guarantees:
- Pin increments lease BEFORE reading ptr → compactor sees lease > 0
- Compactor checks lease == 0 BEFORE moving → pinned data stays put
- Retired arena stays alive until all outstanding pins drain
- No stop-the-world — application never waits for GC

## Integration (Future)

- Local/function arenas: unchanged (`RtArena` directly, no handles)
- Global arena (`__main_arena__`): replaced with `RtManagedArena`
- Code gen emits `rt_managed_pin`/`rt_managed_unpin` around global access
- Malloc hooks route third-party allocations through managed arena

### API Migration Table

The global arena (`__main_arena__`) transitions from `RtArena` to `RtManagedArena`.
Local/function arenas remain `RtArena` (no handles, no GC overhead for short-lived scopes).

| Current (`RtArena`)                              | New (`RtManagedArena`)                                         | Change                                                                                         |
|--------------------------------------------------|----------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| `RtArena *__main_arena__`                        | `RtManagedArena *__main_arena__`                               | Type change; root arena with GC threads                                                        |
| `rt_arena_create(NULL)` (main)                   | `rt_managed_arena_create()`                                    | Root arena, starts cleaner + compactor threads                                                 |
| `rt_arena_destroy(arena)`                        | `rt_managed_arena_destroy(ma)`                                 | Stops GC threads, frees all memory                                                            |
| `rt_arena_alloc(arena, size)`                    | `rt_managed_alloc(ma, old_handle, size)` + pin/unpin           | Returns `RtHandle`; `old` param auto-marks previous allocation dead                           |
| `rt_arena_strdup(arena, str)`                    | `rt_managed_strdup(ma, old_handle, str)` + pin/unpin           | Handle-based; old handle recycled by GC                                                        |
| `rt_arena_strndup(arena, str, n)`                | `rt_managed_strndup(ma, old_handle, str, n)` + pin/unpin       | Handle-based; old handle recycled by GC                                                        |
| `rt_arena_promote_string(dest, str)`             | `rt_managed_promote(dest, src_arena, handle)`                  | Copies data to dest arena, marks source dead; no raw pointer needed                            |
| `rt_arena_promote(dest, &val, size)`             | `rt_managed_promote(dest, src_arena, handle)`                  | Same as above; works for any type, not just strings                                            |
| Direct pointer: `char *p = (char *)ptr;`         | Pin bracket: `char *p = rt_managed_pin(ma, h); ... rt_managed_unpin(ma, h);` | Pointer valid only between pin/unpin; GC won't move pinned data                  |
| `global = value;` (raw ptr)                      | `global_handle = rt_managed_alloc(ma, global_handle, size);`   | Reassignment via `old` param; GC reclaims previous value asynchronously                        |
| (none — accumulates dead memory)                 | `rt_managed_gc_flush(ma)`                                      | Deterministic: blocks until GC completes one full iteration                                    |
| (none)                                           | `rt_managed_compact(ma)`                                       | Force compaction; relocates live data to fresh blocks                                          |
| (none)                                           | `rt_managed_arena_create_child(parent)`                        | Child arena for function scopes needing GC (e.g. globals modified in loops)                    |
| (none)                                           | `rt_managed_arena_destroy_child(child)`                        | Marks all child handles dead, GC reclaims; struct deferred-freed                               |
| `rt_arena_on_cleanup(arena, data, fn, prio)`     | `rt_managed_on_cleanup(ma, data, fn, prio)`                    | Same semantics; NULL data now supported (callback still fires)                                 |
| `rt_arena_reset(arena)`                          | `rt_managed_arena_reset(ma)`                                   | Marks all entries dead + invokes cleanups; GC reclaims asynchronously                          |

### Generated Code Pattern Changes

**Current (RtArena — global string reassignment):**
```c
// In function body:
char *local = rt_arena_strdup(__local_arena__, "value");
// Assigning to global:
global = rt_arena_promote_string(__main_arena__, local);
// Problem: old global value leaks until arena destruction
```

**New (RtManagedArena — global string reassignment):**
```c
// In function body (local arena unchanged):
char *local = rt_arena_strdup(__local_arena__, "value");
// Assigning to global (old value auto-marked dead):
global_handle = rt_managed_strdup(__main_arena__, global_handle, local);
// GC reclaims old value asynchronously — no leak
```

**Current (RtArena — global read):**
```c
printf("%s", global);  // raw pointer, always valid until arena dies
```

**New (RtManagedArena — global read):**
```c
char *__pin__ = (char *)rt_managed_pin(__main_arena__, global_handle);
printf("%s", __pin__);
rt_managed_unpin(__main_arena__, global_handle);
```

### Escape Analysis: Local Handle Reassignment & Dead-Marking

The managed arena's `old` parameter in `rt_managed_alloc(ma, old, size)` handles
global variable reassignment automatically. However, **local** handles in function
scopes assigned to the global arena also benefit from explicit dead-marking when
the escape analysis can prove they don't escape.

#### New API Required: `rt_managed_mark_dead`

The existing `rt_managed_alloc(ma, old, size)` marks `old` dead as a side-effect of
allocation. But scope-exit dead-marking needs to mark a handle dead *without* allocating.
`size == 0` causes an early return, so `rt_managed_alloc(ma, h, 0)` won't work.

**New function needed:**
```c
void rt_managed_mark_dead(RtManagedArena *ma, RtHandle h);
```
Marks `h` as dead (GC will reclaim). No allocation, no new handle returned.

#### Features Required

1. **Reassignment dead-marking (automatic via `old` param)**

   When a local variable backed by the global arena is reassigned in a loop or
   sequential code, the code generator passes the previous handle as `old`:
   ```c
   // Sindarin: var x: str = "a"; x = "b"; x = "c";
   RtHandle x = rt_managed_strdup(__main_arena__, RT_HANDLE_NULL, "a");
   x = rt_managed_strdup(__main_arena__, x, "b");  // "a" marked dead
   x = rt_managed_strdup(__main_arena__, x, "c");  // "b" marked dead
   ```
   No escape analysis needed — the `old` parameter handles this mechanically.

2. **Scope-exit dead-marking (requires escape analysis)**

   When a local handle allocated in the global arena goes out of scope *without*
   escaping (not returned, not assigned to a global, not captured by a closure),
   the code generator should emit an explicit dead-mark:
   ```c
   {
       RtHandle tmp = rt_managed_alloc(__main_arena__, RT_HANDLE_NULL, 64);
       // ... use tmp ...
       // Escape analysis proves: tmp does NOT escape
       rt_managed_mark_dead(__main_arena__, tmp);  // explicitly mark dead
   }
   ```
   **Implementation**: The escape analysis must track which handles are:
   - Returned from the function (→ must promote or leave alive)
   - Assigned to a global/outer-scope variable (→ already promoted)
   - Captured by a closure (→ must promote to closure's arena)
   - None of the above (→ safe to mark dead on scope exit)

3. **Loop-local handles (requires liveness analysis)**

   In loops where a handle is reassigned each iteration but never escapes, the
   `old` param handles iteration-to-iteration cleanup. But the *final* value at
   loop exit also needs dead-marking if it doesn't escape:
   ```c
   RtHandle h = RT_HANDLE_NULL;
   for (int i = 0; i < N; i++) {
       h = rt_managed_strdup(__main_arena__, h, buf);  // prev iteration marked dead
   }
   // If h doesn't escape:
   rt_managed_mark_dead(__main_arena__, h);  // mark final value dead
   ```

4. **Conditional escape (conservative)**

   If a handle *might* escape on some paths (e.g., conditionally returned), the
   analysis must be conservative and NOT mark it dead:
   ```c
   RtHandle result = rt_managed_alloc(__main_arena__, RT_HANDLE_NULL, 64);
   if (cond) return result;  // escapes on this path
   // Cannot mark dead here — might have been returned
   ```

#### Escape Classification

For each local handle allocated in the global arena, classify as:

| Classification | Escape? | Action on Scope Exit              |
|----------------|---------|-----------------------------------|
| **local-only** | No      | Mark dead via `old` param         |
| **returned**   | Yes     | Promote to caller arena           |
| **global-assigned** | Yes | Already in global arena, leave alive |
| **closure-captured** | Yes | Promote to closure's arena       |
| **conditional** | Maybe  | Conservative: leave alive (GC handles eventually) |

#### Symbol Table Extension

The escape analysis needs a per-symbol flag in the symbol table:

```
symbol.managed_handle  : bool    — true if this var is backed by a managed handle
symbol.escapes         : enum    — {ESCAPE_NONE, ESCAPE_RETURN, ESCAPE_GLOBAL, ESCAPE_CLOSURE, ESCAPE_MAYBE}
```

When `escapes == ESCAPE_NONE`, the code generator emits dead-marking at scope exit.
When `escapes == ESCAPE_MAYBE`, the handle is left alive; the GC reclaims it if/when
it becomes unreachable through normal reassignment or arena destruction.

## Build

```bash
make arena        # Build managed arena library
make test-arena   # Build and run arena tests
```

## API Reference

```c
// Lifecycle
RtManagedArena *rt_managed_arena_create(void);
RtManagedArena *rt_managed_arena_create_child(RtManagedArena *parent);
void            rt_managed_arena_destroy(RtManagedArena *ma);
void            rt_managed_arena_destroy_child(RtManagedArena *child);
RtManagedArena *rt_managed_arena_root(RtManagedArena *ma);

// Allocation (old=RT_HANDLE_NULL for first allocation)
RtHandle        rt_managed_alloc(RtManagedArena *ma, RtHandle old, size_t size);
RtHandle        rt_managed_strdup(RtManagedArena *ma, RtHandle old, const char *str);
RtHandle        rt_managed_strndup(RtManagedArena *ma, RtHandle old, const char *str, size_t n);

// Promotion (child → parent)
RtHandle        rt_managed_promote(RtManagedArena *dest, RtManagedArena *src, RtHandle h);
RtHandle        rt_managed_promote_string(RtManagedArena *dest, RtManagedArena *src, RtHandle h);

// Pin/Unpin (bracket data access)
void           *rt_managed_pin(RtManagedArena *ma, RtHandle h);
void            rt_managed_unpin(RtManagedArena *ma, RtHandle h);

// GC Control
void            rt_managed_gc_flush(RtManagedArena *ma);   // block until GC completes one iteration
void            rt_managed_compact(RtManagedArena *ma);    // force compaction cycle
void            rt_managed_arena_reset(RtManagedArena *ma);

// Cleanup Callbacks
RtManagedCleanupNode *rt_managed_on_cleanup(RtManagedArena *ma, void *data,
                                             RtManagedCleanupFn fn, int priority);
void            rt_managed_remove_cleanup(RtManagedArena *ma, void *data);

// Diagnostics
size_t          rt_managed_total_allocated(RtManagedArena *ma);
size_t          rt_managed_live_count(RtManagedArena *ma);
size_t          rt_managed_dead_count(RtManagedArena *ma);
double          rt_managed_fragmentation(RtManagedArena *ma);
size_t          rt_managed_arena_used(RtManagedArena *ma);
```
