# Arena V1 to V2 Upgrade Guide

## Key Conceptual Changes

| Aspect | V1 (old) | V2 (new) |
|--------|----------|----------|
| Handle type | `RtHandle` (uint32_t index) | `RtHandleV2*` (fat pointer) |
| Arena needed to use handle | Yes, always | No, handle knows its arena |
| Handle table | Paged table with indices | Intrusive linked list |
| Compaction | Yes (compactor thread) | No (trade memory for simplicity) |
| GC threads | 2 (cleaner + compactor) | 1 (single GC thread) |
| Locking | Lock-free atomics | Simple mutex |
| "any" variants | Many (pin_any, clone_any, etc.) | Not needed (fat handle) |

## API Mapping

### Arena Creation

```c
// V1
RtManagedArena *ma = rt_managed_arena_create();
RtManagedArena *child = rt_managed_arena_create_child(parent);

// V2
RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
RtArenaV2 *child = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child");
```

### Arena Destruction

```c
// V1
rt_managed_arena_destroy_child(child);
rt_managed_arena_destroy(root);

// V2
rt_arena_v2_destroy(child);  // Works for any arena
rt_arena_v2_destroy(root);
```

### Allocation

```c
// V1 - returns index, needs arena to use
RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, size);
RtHandle h2 = rt_managed_alloc(ma, old_handle, size);  // marks old dead

// V2 - returns fat pointer, self-describing
RtHandleV2 *h = rt_arena_v2_alloc(arena, size);
// For realloc behavior:
RtHandleV2 *h2 = rt_arena_v2_realloc(arena, old_handle, size);
```

### String Duplication

```c
// V1
RtHandle h = rt_managed_strdup(ma, RT_HANDLE_NULL, "hello");

// V2
RtHandleV2 *h = rt_arena_v2_strdup(arena, "hello");
```

### Marking Dead (free for GC)

```c
// V1
rt_managed_mark_dead(ma, h);

// V2
rt_arena_v2_free(h);  // Note: doesn't need arena!
```

### Pin/Unpin (getting raw pointer)

```c
// V1 - must pass arena, need "any" variants for parent chain
void *ptr = rt_managed_pin(ma, h);
void *ptr = rt_managed_pin_any(ma, h);  // walks parent chain
// ... use ptr ...
rt_managed_unpin(ma, h);

// V2 - no arena needed, handle knows everything
void *ptr = rt_handle_v2_pin(h);  // increments pin count, returns ptr
// ... use ptr ...
rt_handle_v2_unpin(h);
```

### Quick Pointer Access (no pinning)

```c
// V1 - must look up in table
RtHandleEntry *entry = rt_handle_get(ma, h);
void *ptr = entry->ptr;

// V2 - direct access
void *ptr = rt_handle_v2_ptr(h);
// or just: h->ptr
```

### Get Arena from Handle

```c
// V1 - impossible! Handle is just an index
// Must track which arena a handle came from separately

// V2 - trivial
RtArenaV2 *arena = rt_handle_v2_arena(h);
// or just: h->arena
```

### Promotion (moving between arenas)

```c
// V1
RtHandle new_h = rt_managed_promote(dest, src, h);
RtHandle new_h = rt_managed_clone(dest, src, h);  // keeps source valid

// V2
RtHandleV2 *new_h = rt_arena_v2_promote(dest, h);  // marks old dead
RtHandleV2 *new_h = rt_arena_v2_clone(dest, h);    // keeps source valid
// Note: source arena not needed - handle knows it!
```

### Cleanup Callbacks

```c
// V1
rt_managed_on_cleanup(ma, data, cleanup_fn, priority);
rt_managed_remove_cleanup(ma, data);

// V2
rt_arena_v2_on_cleanup(arena, data, cleanup_fn, priority);
rt_arena_v2_remove_cleanup(arena, data);
```

### GC Thread

```c
// V1 - automatic, started on root creation
// cleaner + compactor threads run automatically

// V2 - explicit control
rt_arena_v2_gc_thread_start(root, 100);  // 100ms interval
// ... application runs ...
rt_arena_v2_gc_thread_stop();
```

### Manual GC

```c
// V1
rt_managed_gc_flush(ma);  // waits for GC cycle
rt_managed_compact(ma);   // force compaction

// V2
rt_arena_v2_gc(arena);           // one GC pass
rt_arena_v2_gc_recursive(arena); // GC entire tree
rt_arena_v2_gc_flush(arena);     // keep running until nothing to collect
```

### Thread-Local Arena

```c
// V1 - not built-in, typically done manually

// V2 - built-in support
rt_arena_v2_thread_set(arena);
RtArenaV2 *current = rt_arena_v2_thread_current();
```

### Malloc Redirect

```c
// V1 - not built-in

// V2 - built-in
rt_arena_v2_redirect_push(arena);
void *ptr = malloc(100);  // goes to arena!
free(ptr);                // marks handle dead
rt_arena_v2_redirect_pop();
```

## What's Simpler in V2

1. **No arena parameter for most operations** - handle carries everything
2. **No "any" variants** - handle knows its arena, can find root via `h->arena->root`
3. **No handle table management** - handles are just structs in a linked list
4. **No compaction complexity** - we don't move data, just collect dead handles
5. **Validity check is trivial** - `rt_handle_v2_is_valid(h)` checks null and dead flag
6. **Direct pointer access** - `h->ptr` works, no table lookup needed

## What's Different (not necessarily harder)

1. **Handle size** - V2 handles are larger (fat pointers ~48-64 bytes vs 4 byte index)
2. **Memory overhead** - Each allocation has a handle struct overhead
3. **No compaction** - Fragmented memory isn't reclaimed until arena destroyed
4. **Explicit GC thread** - Must start/stop manually (more control, but more responsibility)

## Migration Pattern

For a typical function:

```c
// V1
void process_data(RtManagedArena *ma) {
    RtHandle data = rt_managed_alloc(ma, RT_HANDLE_NULL, 1024);
    char *ptr = rt_managed_pin(ma, data);
    // ... use ptr ...
    rt_managed_unpin(ma, data);
    rt_managed_mark_dead(ma, data);
}

// V2
void process_data(RtArenaV2 *arena) {
    RtHandleV2 *data = rt_arena_v2_alloc(arena, 1024);
    char *ptr = rt_handle_v2_pin(data);
    // ... use ptr ...
    rt_handle_v2_unpin(data);
    rt_arena_v2_free(data);
}
```

## Global Variables

```c
// V1 - globals stored as handles, need root arena to access
static RtHandle g_config = RT_HANDLE_NULL;

void init_config(RtManagedArena *root) {
    g_config = rt_managed_strdup(root, RT_HANDLE_NULL, "default");
}

void use_config(RtManagedArena *ma) {
    // Problem: need to know this came from root, use pin_any
    char *cfg = rt_managed_pin_any(ma, g_config);
    // ...
    rt_managed_unpin(ma, g_config);
}

// V2 - globals are fat handles, self-describing
static RtHandleV2 *g_config = NULL;

void init_config(RtArenaV2 *root) {
    g_config = rt_arena_v2_strdup(root, "default");
}

void use_config(void) {
    // No arena parameter needed!
    char *cfg = rt_handle_v2_pin(g_config);
    // ...
    rt_handle_v2_unpin(g_config);
}
```

## Real-World Example: Path Join

Here's an actual function from the SDK showing before/after:

### V1 (current)

```c
/* Join three path components */
RtHandle sn_path_join3(RtManagedArena *arena, const char *path1, const char *path2, const char *path3)
{
    /* First join path1 and path2 */
    RtHandle temp_h = sn_path_join2(arena, path1, path2);

    /* Pin to get pointer for second join */
    char *temp = rt_managed_pin_str(arena, temp_h);

    /* Join temp with path3 */
    RtHandle result = sn_path_join2(arena, temp, path3);

    rt_managed_unpin(arena, temp_h);

    /* Mark temp as dead since we don't need it anymore */
    rt_managed_mark_dead(arena, temp_h);

    return result;
}
```

### V2 (new)

```c
/* Join three path components */
RtHandleV2 *sn_path_join3(RtArenaV2 *arena, const char *path1, const char *path2, const char *path3)
{
    /* First join path1 and path2 */
    RtHandleV2 *temp = sn_path_join2(arena, path1, path2);

    /* Pin to get pointer for second join */
    char *temp_ptr = rt_handle_v2_pin(temp);

    /* Join temp with path3 */
    RtHandleV2 *result = sn_path_join2(arena, temp_ptr, path3);

    rt_handle_v2_unpin(temp);

    /* Mark temp as dead since we don't need it anymore */
    rt_arena_v2_free(temp);

    return result;
}
```

### Key Differences in This Example

1. **Return type**: `RtHandle` (uint32) → `RtHandleV2*` (pointer)
2. **Pin**: `rt_managed_pin_str(arena, h)` → `rt_handle_v2_pin(h)` (no arena needed!)
3. **Unpin**: `rt_managed_unpin(arena, h)` → `rt_handle_v2_unpin(h)` (no arena needed!)
4. **Mark dead**: `rt_managed_mark_dead(arena, h)` → `rt_arena_v2_free(h)` (no arena needed!)

The V2 version is cleaner because operations on handles don't need the arena parameter - the handle carries everything.

## Another Example: Join All Parts

### V1

```c
RtHandle sn_path_join_all(RtManagedArena *arena, char **parts)
{
    size_t count = rt_array_length(parts);
    if (count == 0) return rt_managed_strdup(arena, RT_HANDLE_NULL, "");
    if (count == 1) return rt_managed_strdup(arena, RT_HANDLE_NULL, parts[0]);

    RtHandle result = rt_managed_strdup(arena, RT_HANDLE_NULL, parts[0]);

    for (size_t i = 1; i < count; i++) {
        char *current = rt_managed_pin_str(arena, result);
        RtHandle new_result = sn_path_join2(arena, current, parts[i]);
        rt_managed_unpin(arena, result);
        rt_managed_mark_dead(arena, result);
        result = new_result;
    }
    return result;
}
```

### V2

```c
RtHandleV2 *sn_path_join_all(RtArenaV2 *arena, char **parts)
{
    size_t count = rt_array_length(parts);
    if (count == 0) return rt_arena_v2_strdup(arena, "");
    if (count == 1) return rt_arena_v2_strdup(arena, parts[0]);

    RtHandleV2 *result = rt_arena_v2_strdup(arena, parts[0]);

    for (size_t i = 1; i < count; i++) {
        char *current = rt_handle_v2_pin(result);
        RtHandleV2 *new_result = sn_path_join2(arena, current, parts[i]);
        rt_handle_v2_unpin(result);
        rt_arena_v2_free(result);
        result = new_result;
    }
    return result;
}
```

Note: The `rt_managed_strdup(arena, RT_HANDLE_NULL, str)` pattern (with the unused `old` parameter) becomes just `rt_arena_v2_strdup(arena, str)`.

## Summary

The V2 API is significantly simpler because:
- Handles are self-describing (fat pointers)
- No handle table indirection
- No compaction complexity
- Simpler locking model

The trade-off is:
- Higher per-handle memory overhead
- No memory compaction (fragmentation persists)
- Explicit GC thread management
