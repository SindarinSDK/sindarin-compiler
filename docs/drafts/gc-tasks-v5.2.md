# Thread System V3 Design

## Overview

Thread V3 is a clean redesign addressing all issues identified in V2. The key principles:

1. **RtHandleV2 is first-class** - All thread access via handle, always within transaction
2. **RtThread is arena-allocated** - Transactions protect pthread primitives from GC
3. **Clean 4-function public API** - create, start, sync, dispose
4. **All functions are transactional** - Each function manages its own transaction
5. **Single cleanup path** - `dispose()` is called from all cleanup paths
6. **Deep copy via copy_callback, deep free via free_callback** - Handles know how to copy and free themselves

## Files

- `runtime/thread/runtime_thread_v3.h` - Header with API
- `runtime/thread/runtime_thread_v3.c` - Implementation
- `runtime/arena/arena_handle.h` - Added `RtHandleV2CopyCallback`, `RtHandleV2FreeCallback`, and callback fields
- `runtime/arena/arena_handle.c` - Added callback setter/getter functions
- `runtime/arena/arena_v2.c` - `rt_arena_v2_clone()` calls `copy_callback` after shallow copy
- `runtime/arena/arena_gc.c` - `rt_handle_v2_destroy()` calls `free_callback` before freeing

---

## Public API

### Thread Lifecycle

```c
/* Create thread handle (RtThread allocated in caller arena) */
RtHandleV2 *rt_thread_v3_create(RtArenaV2 *caller, RtThreadMode mode);

/* Start thread execution (wrapper receives handle, not RtThread*) */
void rt_thread_v3_start(RtHandleV2 *thread_handle, void *(*wrapper)(void *));

/* Sync: wait, promote result, cleanup */
RtHandleV2 *rt_thread_v3_sync(RtHandleV2 *thread_handle);

/* Dispose: cleanup without waiting (fire-and-forget) */
void rt_thread_v3_dispose(RtHandleV2 *thread_handle);

/* Sync multiple void-returning threads */
void rt_thread_v3_sync_all(RtHandleV2 **thread_handles, int count);
```

### Wrapper Helpers

```c
/* Get arena for allocations */
RtArenaV2 *rt_thread_v3_get_arena(RtHandleV2 *thread_handle);

/* Store result handle */
void rt_thread_v3_set_result(RtHandleV2 *thread_handle, RtHandleV2 *result);

/* Signal completion */
void rt_thread_v3_signal_done(RtHandleV2 *thread_handle);
```

### Panic

```c
/* Panic - stores message, signals done, exits thread (or exits process) */
void rt_panic_v3(const char *msg);
```

### TLS

```c
void rt_tls_thread_set_v3(RtHandleV2 *thread_handle);
RtHandleV2 *rt_tls_thread_get_v3(void);
```

---

## Thread Modes

```c
typedef enum {
    RT_THREAD_MODE_DEFAULT,  /* Own arena (child of caller), promote on sync */
    RT_THREAD_MODE_SHARED,   /* Use caller's arena directly */
    RT_THREAD_MODE_PRIVATE   /* Isolated arena, void/primitives only */
} RtThreadMode;
```

---

## Deep Copy via copy_callback

All deep copying is handled by `RtHandleV2CopyCallback` set on each handle at allocation time.

### In arena_handle.h

```c
typedef void (*RtHandleV2CopyCallback)(RtArenaV2 *dest, void *ptr);
typedef void (*RtHandleV2FreeCallback)(RtHandleV2 *handle);  /* Takes handle, not ptr */

struct RtHandleV2 {
    void *ptr;
    size_t size;
    RtArenaV2 *arena;
    RtBlockV2 *block;
    uint16_t flags;
    RtHandleV2CopyCallback copy_callback;  /* Called after shallow copy */
    RtHandleV2FreeCallback free_callback;  /* Called before GC frees handle */
    RtHandleV2 *next;
    RtHandleV2 *prev;
};

void rt_handle_set_copy_callback(RtHandleV2 *handle, RtHandleV2CopyCallback callback);
RtHandleV2CopyCallback rt_handle_get_copy_callback(RtHandleV2 *handle);
void rt_handle_set_free_callback(RtHandleV2 *handle, RtHandleV2FreeCallback callback);
RtHandleV2FreeCallback rt_handle_get_free_callback(RtHandleV2 *handle);
```

### In arena_v2.c

```c
RtHandleV2 *rt_arena_v2_clone(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;

    RtHandleV2 *new_handle = rt_arena_v2_alloc(dest, handle->size);
    if (new_handle != NULL) {
        /* Shallow copy */
        memcpy(new_handle->ptr, handle->ptr, handle->size);

        /* Inherit callbacks */
        new_handle->copy_callback = handle->copy_callback;
        new_handle->free_callback = handle->free_callback;

        /* Deep copy if callback registered */
        if (new_handle->copy_callback != NULL) {
            rt_handle_begin_transaction(new_handle);
            new_handle->copy_callback(dest, new_handle->ptr);
            rt_handle_end_transaction(new_handle);
        }
    }
    return new_handle;
}
```

### Codegen Responsibilities

Codegen sets BOTH `copy_callback` AND `free_callback` at allocation time for types that need deep copy/free:

```c
/* For struct with handle fields */
void __copy_MyStruct__(RtArenaV2 *dest, void *ptr) {
    MyStruct *s = (MyStruct *)ptr;
    s->name = rt_arena_v2_promote(dest, s->name);
    s->data = rt_arena_v2_promote(dest, s->data);
}

void __free_MyStruct__(RtHandleV2 *handle) {
    MyStruct *s = (MyStruct *)handle->ptr;
    rt_arena_v2_free(s->name);
    rt_arena_v2_free(s->data);
}

/* At allocation */
RtHandleV2 *h = rt_arena_v2_alloc(arena, sizeof(MyStruct));
rt_handle_begin_transaction(h);
rt_handle_set_copy_callback(h, __copy_MyStruct__);
rt_handle_set_free_callback(h, __free_MyStruct__);
/* ... initialize struct ... */
rt_handle_end_transaction(h);
```

Types and their callbacks:
- **Primitives / primitive arrays**: No callback (shallow copy/free sufficient)
- **str[]**: Copy promotes each string, free frees each string
- **T[][]**: Copy promotes each inner array, free frees each inner array (recursively)
- **Structs with handles**: Copy promotes handle fields, free frees handle fields
- **Arrays of structs**: Copy/free iterates and handles each struct's fields

**Key:** Every type that needs `copy_callback` also needs `free_callback`. They are symmetric. See gc-tasks-v5.1.md for detailed examples.

---

## Cleanup via free_callback

GC calls `free_callback` before freeing a handle. This ensures resources are released even if code doesn't explicitly clean up.

### In arena_gc.c

```c
static void rt_handle_v2_destroy(RtHandleV2 *handle)
{
    if (handle->free_callback != NULL) {
        handle->free_callback(handle);  /* Pass handle, not ptr */
    }
    free(handle);
}
```

### RtThread uses dispose as free_callback

`rt_thread_v3_dispose` serves as both the explicit cleanup function AND the GC free_callback. It detects whether it's being called from GC by checking the DEAD flag:

```c
void rt_thread_v3_dispose(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) return;

    /* Check disposed before transaction - fast path */
    RtThread *t = (RtThread *)thread_handle->ptr;
    if (t == NULL || t->disposed) return;

    rt_handle_begin_transaction(thread_handle);

    /* Re-check under transaction */
    if (t->disposed) {
        rt_handle_end_transaction(thread_handle);
        return;
    }

    t->disposed = true;

    /* ... cleanup (signal done, condemn arena, destroy mutex/cond, free panic_msg) ... */

    rt_handle_end_transaction(thread_handle);

    /* Only free handle if not already being destroyed by GC */
    if (!(thread_handle->flags & RT_HANDLE_FLAG_DEAD)) {
        rt_arena_v2_free(thread_handle);
    }
}
```

In `rt_thread_v3_create`:
```c
rt_handle_set_free_callback(handle, rt_thread_v3_dispose);
```

### Types needing free_callback

**External resources** (must release on cleanup):

| Type | Callback | Description |
|------|----------|-------------|
| `RtThread` | `rt_thread_v3_dispose` | Single function for both explicit and GC cleanup |
| File handles | `rt_file_dispose` | Close file descriptor |
| Network sockets | `rt_socket_dispose` | Close socket |

**Types with handle fields** (must free to avoid leaks in long-lived arenas):

| Type | Callback | Description |
|------|----------|-------------|
| `str[]`, `T[][]`, etc. | `rt_array_free_callback` | Free element handles |
| Structs with handles | `__free_StructName__` | Free handle fields (codegen) |
| `any[]` | `__free_array_any__` | Free any values containing handles |

**Key:** Any type containing handles needs `free_callback` to avoid leaks in long-lived arenas (globals in main arena). Only primitive types and primitive arrays don't need callbacks.

---

## dispose() as Single Cleanup Path

`rt_thread_v3_dispose()` handles all cleanup:
- Checks `disposed` before transaction (fast path)
- Signals done
- Condemns thread arena
- Destroys pthread mutex/cond
- Frees panic_msg
- Only calls `rt_arena_v2_free` if not already being destroyed by GC

Called from:
- `rt_thread_v3_create()` on error
- `rt_thread_v3_start()` on error
- `rt_thread_v3_sync()` after completion
- Directly for fire-and-forget threads
- GC via `free_callback` (when handle is abandoned)

---

## Panic Flow

### Worker thread panics:

```c
void rt_panic_v3(const char *msg)
{
    RtHandleV2 *th = rt_tls_thread_get_v3();

    if (th != NULL) {
        rt_handle_begin_transaction(th);
        RtThread *t = (RtThread *)th->ptr;
        t->panic_msg = msg ? strdup(msg) : NULL;  /* Simple malloc */
        rt_thread_v3_signal_done(th);
        rt_handle_end_transaction(th);
        rt_tls_thread_set_v3(NULL);
        pthread_exit(NULL);
    }

    fprintf(stderr, "panic: %s\n", msg ? msg : "(no message)");
    exit(1);
}
```

### Main thread in sync:

```c
/* Capture panic message before dispose */
char *panic_msg = t->panic_msg;

/* Promote result, then dispose */
...
rt_thread_v3_dispose(thread_handle);

/* Re-raise panic */
if (panic_msg != NULL) {
    fprintf(stderr, "panic: %s\n", panic_msg);
    free(panic_msg);
    exit(1);
}
```

---

## Codegen Tasks

### 1. Update thread wrapper generation

```c
void *__thread_wrapper_N__(void *arg)
{
    RtHandleV2 *th = (RtHandleV2 *)arg;
    rt_tls_thread_set_v3(th);

    RtArenaV2 *arena = rt_thread_v3_get_arena(th);

    /* ... call actual function ... */
    RtHandleV2 *result = ...;

    rt_thread_v3_set_result(th, result);
    rt_thread_v3_signal_done(th);

    /* Fire-and-forget: */
    rt_thread_v3_dispose(th);
    /* OR synced: just return */

    return NULL;
}
```

### 2. Update spawn site

```c
RtHandleV2 *__thread_handle__ = rt_thread_v3_create(arena, RT_THREAD_MODE_DEFAULT);
/* Pack args into thread arena if needed */
rt_thread_v3_start(__thread_handle__, __thread_wrapper_N__);
```

### 3. Update sync site

```c
RtHandleV2 *__result__ = rt_thread_v3_sync(__thread_handle__);
/* copy_callback already called during promotion */
```

### 4. Generate copy_callback AND free_callback functions

For each type with handle fields, generate BOTH:
- `__copy_TypeName__` - promotes all handle fields
- `__free_TypeName__` - frees all handle fields

Set both on handles at allocation. See gc-tasks-v5.1.md for complete examples including nested structs and struct arrays.

### 5. Set callbacks at allocation

Whenever allocating a type that needs deep copy or cleanup:

```c
/* For types needing deep copy */
rt_handle_set_copy_callback(handle, __copy_TypeName__);

/* For types wrapping external resources */
rt_handle_set_free_callback(handle, __free_TypeName__);
```

---

## Questions Resolved

| Question | Answer |
|----------|--------|
| Should RtThread be malloc'd? | No. Arena-allocated, transactions protect pthread primitives. |
| Separate sync methods? | No. Single `sync()`, mode determines behavior. |
| How to handle `__pending__`? | `RtHandleV2*` instead of `RtThread*`. |
| Promotion strategy? | No enum. `copy_callback` on each handle. |
