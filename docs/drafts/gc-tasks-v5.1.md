# Promotion Code Analysis

This document analyzes all promote functions in the runtime and how they should be refactored to use `RtHandleV2CopyCallback` and `RtHandleV2FreeCallback`.

---

## Current Promote Functions

### 1. `rt_arena_v2_promote` (arena_v2.c)

**Purpose:** Shallow copy a handle from one arena to another, mark old as dead.

**Current implementation:**
```c
RtHandleV2 *rt_arena_v2_promote(RtArenaV2 *dest, RtHandleV2 *handle)
{
    if (dest == NULL || handle == NULL) return NULL;
    if (handle->arena == dest) return handle;

    RtHandleV2 *new_handle = rt_arena_v2_clone(dest, handle);
    if (new_handle != NULL) {
        rt_arena_v2_free(handle);
    }
    return new_handle;
}
```

**Status:** Already updated to call `copy_callback` via `rt_arena_v2_clone`.

---

### 2. `rt_promote_array_string_v2` (runtime_array_v2_alloc.c:197)

**Purpose:** Promote `str[]` - promotes array AND all string elements.

**Current implementation:**
- Allocates new array in dest arena
- Copies metadata (arena, size, capacity)
- Iterates elements, calls `rt_arena_v2_promote` on each string
- Marks old array dead

**Copy callback:**
```c
static void copy_array_string(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        arr[i] = rt_arena_v2_promote(dest, arr[i]);
    }
}
```

**Free callback:**
```c
static void free_array_string(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        if (arr[i] != NULL) {
            rt_arena_v2_free(arr[i]);
        }
    }
}
```

---

### 3. `rt_promote_array_handle_v2` (runtime_array_v2_alloc.c:238)

**Purpose:** Promote `T[][]` - promotes outer array AND all inner array handles.

**Current implementation:**
- Same pattern as string array
- Iterates elements, calls `rt_arena_v2_promote` on each inner array

**Copy callback:**
```c
static void copy_array_handle(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        arr[i] = rt_arena_v2_promote(dest, arr[i]);
    }
}
```

**Free callback:**
```c
static void free_array_handle(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        if (arr[i] != NULL) {
            rt_arena_v2_free(arr[i]);  // Inner handle's free_callback handles recursion
        }
    }
}
```

**Note:** Copy and free callbacks are identical in structure - both iterate elements. The inner handles have their own callbacks which handle recursion.

---

### 4. `rt_promote_array_handle_3d_v2` (runtime_array_v2_alloc.c:276)

**Purpose:** Promote `T[][][]` - promotes all three levels.

**Current implementation:**
- Iterates elements, calls `rt_promote_array_handle_v2` on each 2D array

**With callbacks:** Function not needed. Each array level needs:
- `copy_callback`: promote element handles (recursive via inner callbacks)
- `free_callback`: free element handles (recursive via inner callbacks)

---

### 5. `rt_promote_array2_string_v2` (runtime_array_v2_alloc.c:314)

**Purpose:** Promote `str[][]` - promotes outer, inner arrays, AND strings.

**Current implementation:**
- Iterates elements, calls `rt_promote_array_string_v2` on each `str[]`

**With callbacks:** Function not needed. Both outer `str[][]` and inner `str[]` need:
- `copy_callback`: promote element handles
- `free_callback`: free element handles

---

### 6. `rt_promote_array3_string_v2` (runtime_array_v2_alloc.c:352)

**Purpose:** Promote `str[][][]` - promotes all three levels AND strings.

**With callbacks:** Function not needed. All three levels need both `copy_callback` and `free_callback`.

---

### 7. `rt_any_promote_v2` (runtime_any.c:529)

**Purpose:** Promote an `RtAny` value to a target arena.

**Problem:** `RtAny` is a VALUE type (stored inline), not a handle. It doesn't have `copy_callback`/`free_callback` because it's not allocated via `rt_arena_v2_alloc`. But `RtAny` can CONTAIN handles (strings, arrays, structs stored as `RtHandleV2 *`).

**Solution:** Helper functions that structs/arrays containing `RtAny` fields call from their callbacks:

**Deep copy helper:**
```c
void rt_any_deep_copy(RtArenaV2 *dest, RtAny *any) {
    switch (any->tag) {
        case RT_ANY_STRING:
            any->value.str_val = rt_arena_v2_promote(dest, any->value.str_val);
            break;
        case RT_ANY_ARRAY:
            any->value.arr_val = rt_arena_v2_promote(dest, any->value.arr_val);
            break;
        case RT_ANY_STRUCT:
            any->value.struct_val = rt_arena_v2_promote(dest, any->value.struct_val);
            break;
        // Primitives (int, double, bool, etc.) - no action needed
        default:
            break;
    }
}
```

**Deep free helper:**
```c
void rt_any_deep_free(RtAny *any) {
    switch (any->tag) {
        case RT_ANY_STRING:
            rt_arena_v2_free(any->value.str_val);
            any->value.str_val = NULL;
            break;
        case RT_ANY_ARRAY:
            rt_arena_v2_free(any->value.arr_val);
            any->value.arr_val = NULL;
            break;
        case RT_ANY_STRUCT:
            rt_arena_v2_free(any->value.struct_val);
            any->value.struct_val = NULL;
            break;
        // Primitives - no action needed
        default:
            break;
    }
}
```

**Usage in struct with `any` field:**
```c
// Struct: { name: str, data: any }
static void copy_MyStruct(RtArenaV2 *dest, void *ptr) {
    MyStruct *s = (MyStruct *)ptr;
    s->name = rt_arena_v2_promote(dest, s->name);
    rt_any_deep_copy(dest, &s->data);
}

static void free_MyStruct(RtHandleV2 *handle) {
    MyStruct *s = (MyStruct *)handle->ptr;
    rt_arena_v2_free(s->name);
    rt_any_deep_free(&s->data);
}
```

**Usage in `any[]` array:**
```c
static void copy_array_any(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    RtAny *arr = (RtAny *)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_any_deep_copy(dest, &arr[i]);
    }
}

static void free_array_any(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    RtAny *arr = (RtAny *)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_any_deep_free(&arr[i]);
    }
}
```

**Current issues in `rt_any_promote_v2`:**
- Leaks transaction on new string (line 538-539 calls begin but never end)
- Array/struct promotion not implemented
- Should be replaced with `rt_any_deep_copy` helper

---

## Callback Strategy

### Copy Callbacks Needed

| Type | Callback | Description |
|------|----------|-------------|
| `str[]` | `rt_array_copy_callback` | Promote each string element |
| `T[][]` | `rt_array_copy_callback` | Promote each inner array (recursive) |
| `str[][]` | `rt_array_copy_callback` | Same - inner arrays have string callback |
| `T[][][]` | `rt_array_copy_callback` | Same - recursive |
| Struct with handles | `__copy_StructName__` | Promote each handle field (codegen) |
| Array of structs | `rt_array_struct_copy_callback` | Iterate and call struct's copy on each |

**Key insight:** `rt_array_copy_callback` is generic - works for any array of handles. The inner handles have their own `copy_callback`, so recursion is automatic.

**Important:** Arrays of structs (`StructType[]`) are NOT arrays of handles - structs are stored inline. These require codegen to generate type-specific callbacks. See "Codegen Responsibilities" section below.

### Free Callbacks Needed

Free callbacks now receive `RtHandleV2 *` (not `void *`), allowing them to check handle flags and reuse dispose functions.

| Type | Callback | Description |
|------|----------|-------------|
| `str[]`, `T[][]`, etc. | `rt_array_free_callback` | Free element handles (deep free) |
| Structs with handles | `__free_StructName__` | Free handle fields (codegen) |
| `RtThread` | `rt_thread_v3_dispose` | Single function for explicit and GC cleanup |
| File handles | `rt_file_dispose` | Close file descriptor |
| Network sockets | `rt_socket_dispose` | Close socket |

**Key insight:** Any type containing handles needs `free_callback` to avoid leaks in long-lived arenas (globals in main arena). Only primitive types and primitive arrays don't need callbacks.

---

## Private Arena GC

Private arenas (created with `parent=NULL` or `RT_ARENA_MODE_PRIVATE`) are invisible to the main program's GC. The owner of a private arena must manage its own GC:

```c
#include "runtime/arena/arena_gc.h"

// Create private arena (not in main GC tree)
RtArenaV2 *sdk_arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_PRIVATE, "sdk");

// SDK calls GC on its own arena when it wants
rt_arena_v2_gc(sdk_arena);

// Or destroy entirely when done
rt_arena_v2_destroy(sdk_arena, false);
```

This is critical for:
- SDK packages with their own memory pools
- Thread-private arenas in `RT_THREAD_MODE_PRIVATE`
- Any detached arena not reachable from main

---

## Codegen Responsibilities

Codegen must generate copy and free callbacks for Sindarin types. There is NO generic runtime solution for structs - codegen knows the type layout.

### For each struct with handle fields

Given:
```sindarin
struct Person {
    name: str
    friends: str[]
    metadata: any
}
```

Codegen generates:

```c
/* Copy callback - called after shallow copy during promote/clone */
static void __copy_Person__(RtArenaV2 *dest, void *ptr) {
    Person *p = (Person *)ptr;
    p->name = rt_arena_v2_promote(dest, p->name);
    p->friends = rt_arena_v2_promote(dest, p->friends);
    rt_any_deep_copy(dest, &p->metadata);
}

/* Free callback - called before GC frees handle */
static void __free_Person__(RtHandleV2 *handle) {
    Person *p = (Person *)handle->ptr;
    rt_arena_v2_free(p->name);
    rt_arena_v2_free(p->friends);
    rt_any_deep_free(&p->metadata);
}
```

At allocation site:
```c
RtHandleV2 *h = rt_arena_v2_alloc(arena, sizeof(Person));
rt_handle_begin_transaction(h);
rt_handle_set_copy_callback(h, __copy_Person__);
rt_handle_set_free_callback(h, __free_Person__);
// ... initialize fields ...
rt_handle_end_transaction(h);
```

### For nested structs (inline)

Nested structs are stored INLINE, not as handles. Codegen must recursively walk nested struct fields.

Given:
```sindarin
struct Address {
    street: str
    city: str
}

struct Person {
    name: str
    home: Address   // INLINE - not a handle!
    work: Address   // INLINE - not a handle!
}
```

Codegen generates:

```c
/* Address copy/free helpers (not callbacks - called inline) */
static void __copy_Address_inline__(RtArenaV2 *dest, Address *a) {
    a->street = rt_arena_v2_promote(dest, a->street);
    a->city = rt_arena_v2_promote(dest, a->city);
}

static void __free_Address_inline__(Address *a) {
    rt_arena_v2_free(a->street);
    rt_arena_v2_free(a->city);
}

/* Person callbacks - call nested helpers */
static void __copy_Person__(RtArenaV2 *dest, void *ptr) {
    Person *p = (Person *)ptr;
    p->name = rt_arena_v2_promote(dest, p->name);
    __copy_Address_inline__(dest, &p->home);
    __copy_Address_inline__(dest, &p->work);
}

static void __free_Person__(RtHandleV2 *handle) {
    Person *p = (Person *)handle->ptr;
    rt_arena_v2_free(p->name);
    __free_Address_inline__(&p->home);
    __free_Address_inline__(&p->work);
}
```

**Key point:** Inline helpers (`__copy_X_inline__`, `__free_X_inline__`) take a pointer to the struct, NOT a handle. They're called from parent struct's callbacks. Only top-level allocations get actual callbacks set.

### For deeply nested structs

Codegen recursively generates inline helpers for each struct type, regardless of nesting depth:

```sindarin
struct Country { name: str }
struct City { name: str, country: Country }
struct Address { street: str, city: City }
struct Person { name: str, home: Address }
```

**Copy chain** - promotes all handles at every level:
```c
__copy_Person__(dest, ptr)
    p->name = rt_arena_v2_promote(dest, p->name);
    __copy_Address_inline__(dest, &p->home)
        a->street = rt_arena_v2_promote(dest, a->street);
        __copy_City_inline__(dest, &a->city)
            c->name = rt_arena_v2_promote(dest, c->name);
            __copy_Country_inline__(dest, &c->country)
                co->name = rt_arena_v2_promote(dest, co->name);
```

**Free chain** - frees all handles at every level:
```c
__free_Person__(handle)
    rt_arena_v2_free(p->name);           // Free Person.name
    __free_Address_inline__(&p->home)
        rt_arena_v2_free(a->street);     // Free Address.street
        __free_City_inline__(&a->city)
            rt_arena_v2_free(c->name);   // Free City.name
            __free_Country_inline__(&c->country)
                rt_arena_v2_free(co->name);  // Free Country.name
```

**What happens when Person handle is freed:**
1. GC calls `__free_Person__(handle)`
2. `rt_arena_v2_free(p->name)` marks Person's name handle DEAD
3. `__free_Address_inline__(&p->home)` called for inline Address
4. `rt_arena_v2_free(a->street)` marks Address's street handle DEAD
5. `__free_City_inline__(&a->city)` called for inline City
6. `rt_arena_v2_free(c->name)` marks City's name handle DEAD
7. `__free_Country_inline__(&c->country)` called for inline Country
8. `rt_arena_v2_free(co->name)` marks Country's name handle DEAD
9. All 4 string handles are now DEAD, will be collected by GC

**Without this:** Freeing Person would leak 4 string handles forever in long-lived arenas.

### For arrays of structs (`StructType[]`)

Arrays of structs store structs INLINE (not as handles). The runtime's generic `rt_array_copy_callback` only works for arrays of handles. Codegen must generate type-specific callbacks.

Given `Person[]`:

```c
/* Copy callback for Person[] */
static void __copy_array_Person__(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    Person *arr = (Person *)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        /* Call struct's copy logic on each element (inline, not via callback) */
        arr[i].name = rt_arena_v2_promote(dest, arr[i].name);
        arr[i].friends = rt_arena_v2_promote(dest, arr[i].friends);
        rt_any_deep_copy(dest, &arr[i].metadata);
    }
}

/* Free callback for Person[] */
static void __free_array_Person__(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    Person *arr = (Person *)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_arena_v2_free(arr[i].name);
        rt_arena_v2_free(arr[i].friends);
        rt_any_deep_free(&arr[i].metadata);
    }
}
```

At array allocation site:
```c
RtHandleV2 *h = rt_array_create_v2(arena, capacity, sizeof(Person));
rt_handle_set_copy_callback(h, __copy_array_Person__);
rt_handle_set_free_callback(h, __free_array_Person__);
```

### For nested struct arrays (`Person[][]`)

Outer array contains handles to inner `Person[]` arrays. Use generic `rt_array_copy_callback` and `rt_array_free_callback` on outer array - the inner arrays have their own `__copy_array_Person__` / `__free_array_Person__` callbacks.

### Summary: What codegen generates

| Sindarin Type | Copy Callback | Free Callback |
|---------------|---------------|---------------|
| `struct Foo` with handles | `__copy_Foo__` | `__free_Foo__` |
| `struct Foo` (nested inline) | `__copy_Foo_inline__` | `__free_Foo_inline__` |
| `Foo[]` | `__copy_array_Foo__` | `__free_array_Foo__` |
| `Foo[][]` | generic `rt_array_copy_callback` | generic `rt_array_free_callback` |
| `str[]` | generic `rt_array_copy_callback` | generic `rt_array_free_callback` |
| `any[]` | `__copy_array_any__` | `__free_array_any__` |
| `int[]` | none (primitives) | none |
| `struct Bar` (no handles) | none | none |

**Note:** Inline helpers (`__copy_X_inline__`, `__free_X_inline__`) are NOT callbacks - they're called from parent struct's callbacks. Only top-level allocations have callbacks set.

---

## Refactoring Plan

### Phase 1: Generic Array Copy and Free Callbacks

Create generic callbacks for arrays of handles:

```c
/* In runtime_array_v2.c */

/* Copy callback - promotes all element handles */
void rt_array_copy_callback(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    size_t elem_size = meta->element_size;

    if (elem_size == sizeof(RtHandleV2 *)) {
        /* Array of handles - promote each */
        RtHandleV2 **arr = (RtHandleV2 **)((char *)ptr + sizeof(RtArrayMetadataV2));
        for (size_t i = 0; i < meta->size; i++) {
            if (arr[i] != NULL) {
                arr[i] = rt_arena_v2_promote(dest, arr[i]);
            }
        }
    }
    /* Primitive arrays: no action needed (shallow copy sufficient) */
}

/* Free callback - frees all element handles */
void rt_array_free_callback(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    size_t elem_size = meta->element_size;

    if (elem_size == sizeof(RtHandleV2 *)) {
        /* Array of handles - free each */
        RtHandleV2 **arr = (RtHandleV2 **)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
        for (size_t i = 0; i < meta->size; i++) {
            if (arr[i] != NULL) {
                rt_arena_v2_free(arr[i]);  /* Element's free_callback handles recursion */
            }
        }
    }
    /* Primitive arrays: no action needed */
}
```

### Phase 2: Add element_size to RtArrayMetadataV2

Current:
```c
typedef struct {
    RtArenaV2 *arena;
    size_t size;
    size_t capacity;
} RtArrayMetadataV2;
```

Needed:
```c
typedef struct {
    RtArenaV2 *arena;
    size_t size;
    size_t capacity;
    size_t element_size;  /* For generic copy_callback */
} RtArrayMetadataV2;
```

### Phase 3: Set Callbacks at Array Creation

Modify `rt_array_create_v2` and similar functions:

```c
RtHandleV2 *rt_array_create_v2(RtArenaV2 *arena, size_t capacity, size_t elem_size) {
    // ... existing allocation code ...

    meta->element_size = elem_size;

    if (elem_size == sizeof(RtHandleV2 *)) {
        rt_handle_set_copy_callback(handle, rt_array_copy_callback);
        rt_handle_set_free_callback(handle, rt_array_free_callback);
    }

    return handle;
}
```

### Phase 4: Struct Array Callbacks

For arrays of structs with handle fields, a generic runtime solution would require storing per-struct callback pointers:

```c
/* Generic approach - too complex */
typedef struct {
    size_t struct_size;
    RtHandleV2CopyCallback struct_copy;
    RtHandleV2FreeCallback struct_free;
} RtArrayStructContext;
```

**Decision:** Codegen creates type-specific callbacks instead. Simpler than runtime metadata.

For each `StructType[]`, codegen generates:
- `__copy_array_StructType__` - iterates elements, promotes handle fields
- `__free_array_StructType__` - iterates elements, frees handle fields

See "Codegen Responsibilities" section for examples.

### Phase 5: Remove Explicit Promote Functions

Once callbacks are set at allocation, these functions become unnecessary:
- `rt_promote_array_string_v2`
- `rt_promote_array_handle_v2`
- `rt_promote_array_handle_3d_v2`
- `rt_promote_array2_string_v2`
- `rt_promote_array3_string_v2`

Just call `rt_arena_v2_promote` and callbacks handle everything.

### Phase 6: Fix rt_any_promote_v2

Issues to fix:
1. Transaction leak on string promotion
2. Array promotion not implemented
3. Struct promotion not implemented

Options:
1. Store `RtHandleV2 *` in `RtAny` instead of raw pointers
2. Deprecate in favor of handle-based approach

---

## Summary

| Current Function | Replacement |
|------------------|-------------|
| `rt_arena_v2_promote` | Keep - uses `copy_callback` for deep copy |
| `rt_promote_array_string_v2` | Remove - `copy_callback` + `free_callback` on handle |
| `rt_promote_array_handle_v2` | Remove - `copy_callback` + `free_callback` on handle |
| `rt_promote_array_handle_3d_v2` | Remove - `copy_callback` + `free_callback` on handle |
| `rt_promote_array2_string_v2` | Remove - `copy_callback` + `free_callback` on handle |
| `rt_promote_array3_string_v2` | Remove - `copy_callback` + `free_callback` on handle |
| `rt_any_promote_v2` | Replace with `rt_any_deep_copy` + `rt_any_deep_free` helpers |

**Key:** Every type that needs `copy_callback` also needs `free_callback`. They are symmetric.

---

## Codegen Audit: Current Gaps

Scanned all promote-related code in `src/code_gen/`. Found critical gaps.

### Files with promote code

| File | Lines | What it does |
|------|-------|--------------|
| `stmt/code_gen_stmt_thread.c` | 94-445 | Thread sync promotion |
| `stmt/code_gen_stmt_func_promote.c` | 2-184 | Return value promotion |
| `util/code_gen_util_promote.c` | 114-292 | Struct field promote helpers |
| `expr/thread/code_gen_expr_thread_sync.c` | 135-650 | Thread sync promotion |
| `expr/code_gen_expr_core.c` | 598-684 | Global assignment promotion |

### GAP 1: All explicit `rt_promote_*` calls must go

Current code uses:
- `rt_promote_array_string_v2`
- `rt_promote_array2_string_v2`
- `rt_promote_array3_string_v2`
- `rt_promote_array_handle_v2`
- `rt_promote_array_handle_3d_v2`

**Fix:** Replace ALL with just `rt_arena_v2_promote`. The `copy_callback` on each handle does the deep copy automatically.

### GAP 2: `any[]` is shallow copy (BUG)

In `code_gen_util_promote.c:210`:
```c
/* any[] - shallow promote for now */
return arena_sprintf(arena, "        %s = rt_arena_v2_promote(%s, %s);\n", ...);
```

**Fix:** `any[]` needs `copy_callback` that calls `rt_any_deep_copy` on each element. See RtAny section above.

### GAP 3: Nested structs not handled

In `code_gen_util_promote.c`, only top-level fields are promoted. Nested inline structs are NOT walked.

**Fix:** Generate `__copy_X_inline__` and `__free_X_inline__` for each struct type. Parent callbacks call nested helpers. See "Nested structs" section above.

### GAP 4: NO FREE CALLBACKS GENERATED ANYWHERE

**This is the critical gap.** Codegen generates copy/promote code but NEVER generates free code. Old values leak on every reassignment.

---

## Reassignment Must FREE OLD VALUE

**THE COMPLETE PATTERN:**
1. `copy_callback` - deep copy on promote/clone
2. `free_callback` - deep free when handle is freed
3. **On reassignment: FREE OLD VALUE FIRST, then assign new**

### Global assignment (`code_gen_expr_core.c:598-684`)

**Current (LEAKS):**
```c
global_var = rt_arena_v2_promote(__main_arena__, new_value);
```

**Required:**
```c
rt_arena_v2_free(global_var);  // FREE OLD FIRST!
global_var = rt_arena_v2_promote(__main_arena__, new_value);
```

**Why it leaks:** Main arena is NEVER condemned. Old value stays forever.

### Struct field reassignment

**Current (LEAKS):**
```c
person.name = new_string;
```

**Required:**
```c
rt_arena_v2_free(person.name);  // FREE OLD FIRST!
person.name = new_string;
```

**Why it leaks:** If struct is in main arena or long-lived arena, old field value leaks forever.

### Array element reassignment

**Current (LEAKS):**
```c
arr[i] = new_value;
```

**Required:**
```c
rt_arena_v2_free(arr[i]);  // FREE OLD FIRST!
arr[i] = new_value;
```

**Why it leaks:** If array is in main arena or long-lived arena, old element leaks forever.

### Local variable reassignment

**Current (LEAKS in long-lived arenas):**
```c
x = new_value;
```

**Required (for long-lived arenas):**
```c
rt_arena_v2_free(x);  // FREE OLD FIRST!
x = new_value;
```

**Note:** For function-local variables in function arenas, the arena is condemned on return so this is less critical. But for module-level variables, thread-static variables, or any variable in main arena - OLD VALUE LEAKS.

### Nested struct field reassignment

**Current (LEAKS):**
```c
person.home.city.name = new_string;
```

**Required:**
```c
rt_arena_v2_free(person.home.city.name);  // FREE OLD FIRST!
person.home.city.name = new_string;
```

**Why it leaks:** Nested field in long-lived struct, old value leaks forever.

### Array of structs element field reassignment

**Current (LEAKS):**
```c
people[i].name = new_string;
```

**Required:**
```c
rt_arena_v2_free(people[i].name);  // FREE OLD FIRST!
people[i].name = new_string;
```

---

## Codegen Changes Required

### 1. Set callbacks at allocation

For every `rt_arena_v2_alloc` of a type with handle fields:
```c
RtHandleV2 *h = rt_arena_v2_alloc(arena, sizeof(MyStruct));
rt_handle_set_copy_callback(h, __copy_MyStruct__);
rt_handle_set_free_callback(h, __free_MyStruct__);
```

For every array of handles:
```c
RtHandleV2 *h = rt_array_create_v2(arena, capacity, sizeof(RtHandleV2 *));
rt_handle_set_copy_callback(h, rt_array_copy_callback);
rt_handle_set_free_callback(h, rt_array_free_callback);
```

For every array of structs:
```c
RtHandleV2 *h = rt_array_create_v2(arena, capacity, sizeof(MyStruct));
rt_handle_set_copy_callback(h, __copy_array_MyStruct__);
rt_handle_set_free_callback(h, __free_array_MyStruct__);
```

### 2. Generate copy and free callbacks for each struct

```c
static void __copy_MyStruct__(RtArenaV2 *dest, void *ptr) { ... }
static void __free_MyStruct__(RtHandleV2 *handle) { ... }
```

### 3. Generate copy and free callbacks for struct arrays

```c
static void __copy_array_MyStruct__(RtArenaV2 *dest, void *ptr) { ... }
static void __free_array_MyStruct__(RtHandleV2 *handle) { ... }
```

### 4. Generate inline helpers for nested structs

```c
static void __copy_Address_inline__(RtArenaV2 *dest, Address *a) { ... }
static void __free_Address_inline__(Address *a) { ... }
```

### 5. FREE OLD on every handle reassignment

For assignment to global, struct field, array element, or any variable holding a handle:
```c
// Before: target = new_value;
// After:
if (target != NULL) rt_arena_v2_free(target);
target = new_value;
```

### 6. Remove all explicit rt_promote_* calls

Replace:
```c
result = rt_promote_array_string_v2(dest, result);
```

With:
```c
result = rt_arena_v2_promote(dest, result);
```

The `copy_callback` handles everything.

### 7. Fix any[] deep copy/free

Generate callbacks for `any[]`:
```c
static void __copy_array_any__(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    RtAny *arr = (RtAny *)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_any_deep_copy(dest, &arr[i]);
    }
}

static void __free_array_any__(RtHandleV2 *handle) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;
    RtAny *arr = (RtAny *)((char *)handle->ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_any_deep_free(&arr[i]);
    }
}
```

---

## Benefits

1. **Single promotion path:** `rt_arena_v2_promote` for everything
2. **Self-describing types:** Handles know how to copy themselves
3. **Automatic recursion:** Nested structures just work
4. **Cleaner codegen:** No need to determine promotion strategy at compile time
5. **Consistent cleanup:** `free_callback` ensures resources are released even on GC sweep
6. **No leaks on reassignment:** FREE OLD before assigning new
