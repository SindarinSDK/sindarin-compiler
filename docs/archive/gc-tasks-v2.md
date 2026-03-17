# GC Phase 6: RtHandleV2 as First-Class Citizen Everywhere

## Principle

**Memory cannot be held using direct pointers.** All runtime operations must accept
`RtHandleV2*` parameters and dereference `handle->ptr` inside a transaction. The
codegen must never extract `char*` or `void*` from a handle and pass it across
function boundaries. This breaks garbage collection.

## Current State (COMPLETED)

All phases (6.1-6.10) have been implemented and validated. The codegen now passes
`RtHandleV2*` handles directly to runtime functions. All pin/ptr extraction patterns
have been removed. All tests pass across compiler (~3290 tests), SDK (28/28), and
HTTP package (4/4).

Previously the codegen emitted this pattern ~138 times across 25+ files:

```c
// OLD (removed): pin is a no-op, char* escapes with no transaction protection
({ RtHandleV2 *__pin = handle; rt_handle_v2_pin(__pin); (char *)__pin->ptr; })
```

Previously runtime string functions accepted `const char*`:
```c
// OLD (fixed): forces codegen to extract char* from handle
RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, const char *a, const char *b);
```

## Target State

Runtime functions accept `RtHandleV2*` and manage transactions internally:
```c
// RIGHT: handle stays opaque, function manages its own transaction
RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, RtHandleV2 *a, RtHandleV2 *b);
```

Codegen passes handles, never raw pointers:
```c
// RIGHT: no ptr extraction, no escape
__sn__result = rt_str_concat_v2(__local_arena__, __sn__left, __sn__right);
```

## String Literal Convention

String literals (`"hello"`) are not arena-allocated and have no `RtHandleV2*`.
Functions that accept `RtHandleV2*` for strings also need to handle the literal
case. Two approaches (choose one):

**Option A: Wrap literals at creation** - codegen wraps every string literal in
`rt_arena_v2_strdup(arena, "hello")` at point of use. This is already done in
most places. Ensure it's universal.

**Option B: Dual-signature functions** - provide both `_h` (handle) and `_lit`
(literal) variants. Rejected: too much API surface.

**Decision: Option A.** All strings are `RtHandleV2*` by the time they reach
runtime functions. The codegen already wraps literals via `rt_arena_v2_strdup`.
Verify this is consistent.

---

## Phase 6.1: String Functions Accept RtHandleV2*

### Scope
Change all string V2 functions in `runtime_string_v2.h` to accept `RtHandleV2*`
instead of `const char*` for string parameters.

### Functions to Change

**runtime_string_v2.h** (16 functions):
```
rt_str_concat_v2(arena, const char *a, const char *b)
  → rt_str_concat_v2(arena, RtHandleV2 *a, RtHandleV2 *b)

rt_str_append_v2(arena, const char *old_str, const char *suffix)
  → rt_str_append_v2(arena, RtHandleV2 *old_str, RtHandleV2 *suffix)

rt_to_string_string_v2(arena, const char *val)
  → rt_to_string_string_v2(arena, RtHandleV2 *val)

rt_format_string_v2(arena, const char *val, const char *fmt)
  → rt_format_string_v2(arena, RtHandleV2 *val, const char *fmt)
  Note: fmt is a compile-time format string, can stay const char*

rt_str_substring_v2(arena, const char *str, long start, long end)
  → rt_str_substring_v2(arena, RtHandleV2 *str, long start, long end)

rt_str_toUpper_v2(arena, const char *str)
  → rt_str_toUpper_v2(arena, RtHandleV2 *str)

rt_str_toLower_v2(arena, const char *str)
  → rt_str_toLower_v2(arena, RtHandleV2 *str)

rt_str_trim_v2(arena, const char *str)
  → rt_str_trim_v2(arena, RtHandleV2 *str)

rt_str_replace_v2(arena, const char *str, const char *old_s, const char *new_s)
  → rt_str_replace_v2(arena, RtHandleV2 *str, RtHandleV2 *old_s, RtHandleV2 *new_s)

rt_str_split_v2(arena, const char *str, const char *delimiter)
  → rt_str_split_v2(arena, RtHandleV2 *str, RtHandleV2 *delimiter)

rt_str_split_n_v2(arena, const char *str, const char *delimiter, int limit)
  → rt_str_split_n_v2(arena, RtHandleV2 *str, RtHandleV2 *delimiter, int limit)

rt_str_split_whitespace_v2(arena, const char *str)
  → rt_str_split_whitespace_v2(arena, RtHandleV2 *str)

rt_str_split_lines_v2(arena, const char *str)
  → rt_str_split_lines_v2(arena, RtHandleV2 *str)
```

Note: `rt_to_string_long_v2`, `rt_to_string_double_v2`, `rt_to_string_char_v2`,
`rt_to_string_bool_v2`, `rt_to_string_byte_v2` take primitive values, NOT
strings - they stay unchanged.

### Implementation Pattern

Each function wraps its ptr access in a transaction:

```c
RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, RtHandleV2 *a, RtHandleV2 *b) {
    if (a == NULL || b == NULL) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(a);
    rt_handle_begin_transaction(b);
    const char *sa = (const char *)a->ptr;
    const char *sb = (const char *)b->ptr;
    size_t la = sa ? strlen(sa) : 0;
    size_t lb = sb ? strlen(sb) : 0;
    RtHandleV2 *result = rt_arena_v2_alloc(arena, la + lb + 1);
    rt_handle_begin_transaction(result);
    char *dest = (char *)result->ptr;
    if (la) memcpy(dest, sa, la);
    if (lb) memcpy(dest + la, sb, lb);
    dest[la + lb] = '\0';
    rt_handle_end_transaction(result);
    rt_handle_end_transaction(b);
    rt_handle_end_transaction(a);
    return result;
}
```

### Tasks

- [x] 6.1.1: Update `runtime_string_v2.h` - change all 16 function signatures
- [x] 6.1.2: Update `runtime_string_v2.c` - rewrite all 16 implementations with transactions
- [x] 6.1.3: Update internal callers in runtime that call these functions
- [x] 6.1.4: Build runtime, fix compilation errors
- [x] 6.1.5: Run unit tests

---

## Phase 6.2: String Query Functions Accept RtHandleV2*

### Scope
Change string query functions in `runtime_string.h` that take `const char*` and
are used by generated code.

### Functions to Change

**runtime_string.h** (read-only queries):
```
rt_str_length(const char *str)
  → rt_str_length(RtHandleV2 *str)

rt_str_indexOf(const char *str, const char *search)
  → rt_str_indexOf(RtHandleV2 *str, RtHandleV2 *search)

rt_str_contains(const char *str, const char *search)
  → rt_str_contains(RtHandleV2 *str, RtHandleV2 *search)

rt_str_charAt(const char *str, long index)
  → rt_str_charAt(RtHandleV2 *str, long index)

rt_str_startsWith(const char *str, const char *prefix)
  → rt_str_startsWith(RtHandleV2 *str, RtHandleV2 *prefix)

rt_str_endsWith(const char *str, const char *suffix)
  → rt_str_endsWith(RtHandleV2 *str, RtHandleV2 *suffix)
```

**String parsing functions** (consume string, produce primitive):
```
rt_str_to_int(const char *str) → rt_str_to_int(RtHandleV2 *str)
rt_str_to_long(const char *str) → rt_str_to_long(RtHandleV2 *str)
rt_str_to_double(const char *str) → rt_str_to_double(RtHandleV2 *str)
```

Note: `rt_str_concat` (V1) and `rt_str_substring` (V1) also return
`RtHandleV2*` already but accept `const char*`. Update to accept handles.

### Tasks

- [x] 6.2.1: Update `runtime_string.h` - change function signatures
- [x] 6.2.2: Update `runtime_string_query.c` - rewrite with transactions
- [x] 6.2.3: Update `runtime_string_parse.c` - rewrite with transactions
- [x] 6.2.4: Update `runtime_string.c` (rt_str_concat) - accept handles
- [x] 6.2.5: Build and test

---

## Phase 6.3: I/O Functions Accept RtHandleV2*

### Scope
Change I/O functions that accept `const char*` strings to accept `RtHandleV2*`.

### Functions to Change

**runtime_io.h**:
```
rt_println(const char *text) → rt_println(RtHandleV2 *text)
rt_print_err(const char *text) → rt_print_err(RtHandleV2 *text)
rt_print_err_ln(const char *text) → rt_print_err_ln(RtHandleV2 *text)
rt_stdout_write(const char *text) → rt_stdout_write(RtHandleV2 *text)
rt_stdout_write_line(const char *text) → rt_stdout_write_line(RtHandleV2 *text)
rt_stderr_write(const char *text) → rt_stderr_write(RtHandleV2 *text)
rt_stderr_write_line(const char *text) → rt_stderr_write_line(RtHandleV2 *text)
rt_assert(int cond, const char *msg) → rt_assert(int cond, RtHandleV2 *msg)
```

### Implementation Pattern

```c
void rt_println(RtHandleV2 *text) {
    if (text == NULL) { puts("null"); return; }
    rt_handle_begin_transaction(text);
    const char *s = (const char *)text->ptr;
    puts(s ? s : "null");
    rt_handle_end_transaction(text);
}
```

### Tasks

- [x] 6.3.1: Update `runtime_io.h` - change signatures
- [x] 6.3.2: Update `runtime_io.c` - rewrite with transactions
- [x] 6.3.3: Build and test

---

## Phase 6.4: Boxing/Unboxing Accept RtHandleV2*

### Scope
The `RtAny` tagged union stores strings as `char *s`. This must become
`RtHandleV2*` so boxed strings are GC-safe.

### Changes

**runtime_any.h**:
```c
// RtAny union field:
char *s;           // BEFORE
RtHandleV2 *s;    // AFTER

// Boxing:
RtAny rt_box_string(const char *value)     // BEFORE
RtAny rt_box_string(RtHandleV2 *value)     // AFTER

// Unboxing:
const char *rt_unbox_string(RtAny value)   // BEFORE
RtHandleV2 *rt_unbox_string(RtAny value)   // AFTER
```

### Impact
This ripples through interceptors, thread thunks, and any codegen that
boxes/unboxes strings. The codegen currently does:
```c
rt_box_string((char *)handle->ptr)   // WRONG: extracts ptr
```
It should become:
```c
rt_box_string(handle)                // RIGHT: passes handle
```

### Tasks

- [x] 6.4.1: Update `RtAny` struct - `char *s` → `RtHandleV2 *s`
- [x] 6.4.2: Update `rt_box_string` and `rt_unbox_string` signatures + impls
- [x] 6.4.3: Update `rt_any_to_string` - internal transaction for string case
- [x] 6.4.4: Update `rt_any_equals` - transaction for string comparison
- [x] 6.4.5: Update `rt_any_promote_v2` - handle-aware string promotion
- [x] 6.4.6: Build and test

---

## Phase 6.5: Array Element Access via Transactions

### Scope
Array functions that return raw pointers or accept raw `const char*` for string
elements must use handles.

### Key Changes

**rt_array_data_v2** (the biggest offender):
```c
// BEFORE: returns raw void*, caller must hold transaction (nobody does)
static inline void *rt_array_data_v2(RtHandleV2 *arr_h);

// AFTER: removed from public API or made internal-only
// Element access goes through typed accessor functions instead
```

**New typed element access functions** (if not already present):
```c
long long rt_array_get_long_v2(RtHandleV2 *arr_h, long index);
void rt_array_set_long_v2(RtHandleV2 *arr_h, long index, long long value);
RtHandleV2 *rt_array_get_string_v2(RtHandleV2 *arr_h, long index);
void rt_array_set_string_v2(RtHandleV2 *arr_h, long index, RtHandleV2 *value);
// ... for each element type
```

These functions internally begin/end transactions on the array handle.

**String element functions** - change `const char*` to `RtHandleV2*`:
```
rt_array_push_string_v2(arena, arr_h, const char *element)
  → rt_array_push_string_v2(arena, arr_h, RtHandleV2 *element)

rt_array_ins_string_v2(arr_h, const char *elem, long index)
  → rt_array_ins_string_v2(arr_h, RtHandleV2 *elem, long index)

rt_array_push_copy_string_v2(arr_h, const char *elem)
  → rt_array_push_copy_string_v2(arr_h, RtHandleV2 *elem)

rt_array_create_string_v2(arena, count, const char **data)
  → rt_array_create_string_v2(arena, count, RtHandleV2 **data)

rt_array_indexOf_string_v2(arr_h, const char *elem)
  → rt_array_indexOf_string_v2(arr_h, RtHandleV2 *elem)

rt_array_contains_string_v2(arr_h, const char *elem)
  → rt_array_contains_string_v2(arr_h, RtHandleV2 *elem)
```

**Join/toString functions** - currently return `char*`, must return `RtHandleV2*`:
```
char *rt_array_join_long_v2(arr_h, separator)
  → RtHandleV2 *rt_array_join_long_v2(arr_h, RtHandleV2 *separator)

char *rt_to_string_array_long_v2(arr_h)
  → RtHandleV2 *rt_to_string_array_long_v2(arr_h)
// ... for all types
```

### Tasks

- [x] 6.5.1: Add typed element get/set functions with internal transactions
- [x] 6.5.2: Update string element functions to accept RtHandleV2*
- [x] 6.5.3: Update join functions to return RtHandleV2* and accept handle separator
- [x] 6.5.4: Update toString functions to return RtHandleV2*
- [x] 6.5.5: Make rt_array_data_v2 internal-only (move to internal header)
- [x] 6.5.6: Update all array .c files with transactions
- [x] 6.5.7: Build and test

---

## Phase 6.6: Thread Codegen - Transactional Arg Packing

### Scope
Thread wrappers access `t->args->ptr` without transactions. The codegen for
thread spawn and sync must use transactions.

### Current Pattern (Broken)
```c
// Spawn site - packing args:
rt_handle_v2_pin(__spawn_handle->args);  // no-op
__ThreadArgs *args = (__ThreadArgs *)__spawn_handle->args->ptr;  // unprotected
args->arg0 = 42;

// Wrapper - unpacking args:
rt_handle_v2_pin(__t__->args);  // no-op
__ThreadArgs *args = (__ThreadArgs *)__t__->args->ptr;  // unprotected, held for entire thread life

// Storing result:
rt_handle_v2_pin(__result_handle__);  // no-op
*(long long *)__result_handle__->ptr = __result__;  // unprotected write

// Sync site - reading result:
rt_handle_v2_pin(__sync_h__);  // no-op
__sn__result = *(long long *)__sync_h__->ptr;  // unprotected read
```

### Target Pattern
```c
// Spawn site - packing args:
rt_handle_begin_transaction(__spawn_handle->args);
__ThreadArgs *args = (__ThreadArgs *)__spawn_handle->args->ptr;
args->arg0 = 42;
rt_handle_end_transaction(__spawn_handle->args);

// Wrapper - unpacking args:
rt_handle_begin_transaction(__t__->args);
__ThreadArgs *a = (__ThreadArgs *)__t__->args->ptr;
long long arg0 = a->arg0;  // copy out
rt_handle_end_transaction(__t__->args);
// use arg0 (stack copy) for rest of thread

// Storing result:
rt_handle_begin_transaction(__result_handle__);
*(long long *)__result_handle__->ptr = __result__;
rt_handle_end_transaction(__result_handle__);

// Sync site - reading result:
rt_handle_begin_transaction(__sync_h__);
__sn__result = *(long long *)__sync_h__->ptr;
rt_handle_end_transaction(__sync_h__);
```

### Tasks

- [x] 6.6.1: Update `code_gen_expr_thread.c` - spawn site arg packing
- [x] 6.6.2: Update `code_gen_expr_thread.c` - wrapper arg unpacking
- [x] 6.6.3: Update `code_gen_expr_thread.c` - result storage
- [x] 6.6.4: Update `code_gen_stmt_thread.c` - sync site result reading
- [x] 6.6.5: Update `code_gen_expr_thread_sync.c` - all sync patterns
- [x] 6.6.6: Build and test thread integration tests

---

## Phase 6.7: Core Codegen - Remove Pin/Ptr Pattern

### Scope
Replace all `rt_handle_v2_pin` + `->ptr` extraction patterns with direct
handle passing. This is the largest phase.

### Pattern Replacement

**String variable access** - no more ptr extraction:
```c
// BEFORE:
rt_println(({ rt_handle_v2_pin(__sn__result); (char *)__sn__result->ptr; }));

// AFTER:
rt_println(__sn__result);
```

**String concatenation** - pass handles directly:
```c
// BEFORE:
rt_str_concat_v2(__local_arena__,
    ({ rt_handle_v2_pin(__sn__a); (char *)__sn__a->ptr; }),
    ({ rt_handle_v2_pin(__sn__b); (char *)__sn__b->ptr; }));

// AFTER:
rt_str_concat_v2(__local_arena__, __sn__a, __sn__b);
```

**String method calls** - pass handle, get handle back:
```c
// BEFORE:
rt_str_toUpper_v2(__local_arena__,
    ({ rt_handle_v2_pin(__sn__name); (char *)__sn__name->ptr; }));

// AFTER:
rt_str_toUpper_v2(__local_arena__, __sn__name);
```

### Files to Update (25+ files)

**Expression codegen:**
- [x] `code_gen_expr_core.c` - variable references, parameter passing
- [x] `code_gen_expr_access.c` - struct member access, compound assignment
- [x] `code_gen_expr_array.c` - array element access (string elements)
- [x] `code_gen_expr_binary.c` - string concatenation operator
- [x] `code_gen_expr_string.c` - string interpolation, struct toString
- [x] `code_gen_expr_struct.c` - struct field initialization
- [x] `code_gen_expr_static.c` - static variable access
- [x] `code_gen_expr_misc.c` - deep copy, nullable string handling
- [x] `code_gen_expr_member.c` - member access on string fields
- [x] `code_gen_expr_match.c` - match expressions with strings
- [x] `code_gen_expr_incr.c` - string increment operations (+=)

**Call codegen:**
- [x] `code_gen_expr_call.c` - general function calls
- [x] `code_gen_expr_call_builtin.c` - println, readLine, etc
- [x] `code_gen_expr_call_string.c` - string method calls
- [x] `code_gen_expr_call_struct.c` - struct constructor calls
- [x] `code_gen_expr_call_namespace.c` - namespace function calls
- [x] `code_gen_expr_call_closure.c` - closure invocations
- [x] `code_gen_expr_call_char.c` - char operations
- [x] `code_gen_expr_call_array_byte.c` - byte array ops
- [x] `code_gen_expr_call_intercept_func.c` - intercepted function calls
- [x] `code_gen_expr_call_intercept_method.c` - intercepted method calls

**Statement codegen:**
- [x] `code_gen_stmt_var.c` - variable declarations
- [x] `code_gen_stmt_core.c` - core statements
- [x] `code_gen_stmt_return.c` - return statements

**Lambda/closure codegen:**
- [x] `code_gen_expr_lambda.c` - lambda capture and invocation

**Utility codegen:**
- [x] `code_gen_util_boxing.c` - box/unbox operations
- [x] `code_gen_util_tostring.c` - toString generation
- [x] `code_gen_util_promote.c` - arena promotion

### Tasks

- [x] 6.7.1: Remove `rt_handle_v2_pin` no-op stubs (break intentionally)
- [x] 6.7.2: Update expression codegen files (11 files)
- [x] 6.7.3: Update call codegen files (11 files)
- [x] 6.7.4: Update statement codegen files (3 files)
- [x] 6.7.5: Update lambda codegen (1 file)
- [x] 6.7.6: Update utility codegen (3 files)
- [x] 6.7.7: Full build
- [x] 6.7.8: Run all tests (unit + integration + exploratory)

---

## Phase 6.8: Array Codegen - Transactional Element Access

### Scope
Array element access in codegen currently uses:
```c
((long long *)rt_array_data_v2(__sn__nums))[index]
```
This must use the new typed accessor functions.

### Pattern Replacement

**Read element:**
```c
// BEFORE:
((long long *)rt_array_data_v2(__sn__nums))[i]

// AFTER:
rt_array_get_long_v2(__sn__nums, i)
```

**Write element:**
```c
// BEFORE:
((long long *)rt_array_data_v2(__sn__nums))[i] = value

// AFTER:
rt_array_set_long_v2(__sn__nums, i, value)
```

**String array element:**
```c
// BEFORE:
({ RtHandleV2 *__pin_h__ = ((RtHandleV2 **)rt_array_data_v2(arr))[i];
   rt_handle_v2_pin(__pin_h__); (char *)__pin_h__->ptr; })

// AFTER:
rt_array_get_string_v2(arr, i)  // returns RtHandleV2*
```

### Files to Update
- [x] `code_gen_expr_array.c` - array indexing, slicing
- [x] `code_gen_expr_call_array.c` - array method calls
- [x] `code_gen_expr_call_array_mutate.c` - array mutation
- [x] `code_gen_expr_call_array_query.c` - array queries
- [x] `code_gen_expr_call_array_push.c` - array push operations
- [x] `code_gen_expr_call_array_byte.c` - byte array ops
- [x] `code_gen_expr_call_byte_array.c` - byte array ops
- [x] `code_gen_stmt_loop.c` - for-each loops over arrays

### Tasks

- [x] 6.8.1: Update array element read codegen
- [x] 6.8.2: Update array element write codegen
- [x] 6.8.3: Update string array element access codegen
- [x] 6.8.4: Update for-each loop codegen
- [x] 6.8.5: Build and test array integration tests

---

## Phase 6.9: Native Function Interop

### Scope
Native functions (marked `native` with `@alias`) require raw `const char*` at
the FFI boundary. The codegen must extract `char*` within a transaction at the
call site.

### Pattern
```c
// Native call site - transaction scoped to the call:
rt_handle_begin_transaction(__sn__str);
native_function((const char *)__sn__str->ptr);
rt_handle_end_transaction(__sn__str);
```

This is the ONE place where `->ptr` extraction is allowed, because native C
functions genuinely need raw pointers. The transaction must encompass the
entire native call.

### Tasks

- [x] 6.9.1: Update `code_gen_util_native.c` - wrap native calls in transactions
- [x] 6.9.2: Update `code_gen_native_extern.c` - extern declarations
- [x] 6.9.3: Build and test with SDK native functions

---

## Phase 6.10: Cleanup and Validation

### Tasks

- [x] 6.10.1: Remove `rt_handle_v2_pin` / `rt_handle_v2_unpin` stubs from arena_handle.h
- [x] 6.10.2: Search entire codebase for remaining `->ptr` outside transactions
- [x] 6.10.3: Search for remaining `const char *` in V2 function signatures
- [x] 6.10.4: Full build: `make clean && make build`
- [x] 6.10.5: Full test: `timeout 900 make test`
- [x] 6.10.6: Copy to global and test SDK: `cp -r bin/* ~/.sn/bin/ && cd ../sindarin-pkg-sdk && make test`
- [x] 6.10.7: Test HTTP package: `cd ../sindarin-pkg-http && sn --clear-cache && rm -rf ./.sn && sn --install && make test`

---

## Execution Order

The phases must be done in order because each builds on the previous:

1. **6.1** String V2 functions → establishes the new API pattern
2. **6.2** String query functions → completes string API
3. **6.3** I/O functions → println etc work with handles
4. **6.4** Boxing/unboxing → RtAny stores handles
5. **6.5** Array functions → arrays work with handles
6. **6.6** Thread codegen → threads use transactions
7. **6.7** Core codegen → the big rewrite (depends on 6.1-6.5)
8. **6.8** Array codegen → array access uses accessors (depends on 6.5)
9. **6.9** Native interop → transaction-scoped ptr extraction
10. **6.10** Cleanup → remove stubs, validate

Each phase should compile and pass tests before moving to the next.

---

## Risk Assessment

**Highest risk:** Phase 6.7 (core codegen rewrite) - touches 25+ files,
every string expression changes. Must be done carefully with incremental
testing.

**Medium risk:** Phase 6.4 (boxing) - RtAny is used in interceptors,
threads, and the any type. Ripple effects across codegen.

**Lower risk:** Phases 6.1-6.3 (runtime API changes) - well-contained,
mostly mechanical. Can be validated with unit tests before codegen changes.

## Estimated Scope

- ~16 runtime string function rewrites
- ~9 string query function rewrites
- ~8 I/O function rewrites
- ~6 boxing/unboxing changes
- ~100+ array function signature changes
- ~138 codegen pin→handle changes across 25+ files
- ~20 thread codegen changes
- Total: roughly 300+ individual changes
