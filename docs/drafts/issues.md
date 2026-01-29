# Sindarin Compiler Issues

This document tracks issues discovered during development.

---

## Issue: `rt_unbox_struct` Signature Mismatch in Thread Thunks

**Date:** 2026-01-28
**Severity:** High
**Status:** FIXED

### Description

When spawning threads with struct parameters, the generated C code was calling `rt_unbox_struct` with only one argument instead of two, causing compilation failures.

### Fix

Added special handling for struct types in thread thunk argument generation in `code_gen_expr_thread.c`. The fix properly passes the type_id as the second argument to `rt_unbox_struct`.

### Test

See `tests/integration/test_thread_struct_param.sn`

---

## Issue: Thread Handles Cannot Be Conditionally Assigned

**Date:** 2026-01-28
**Severity:** Medium
**Status:** FIXED

### Description

Thread handles in Sindarin must be initialized at declaration time with the `&fn()` spawn syntax. They cannot be reassigned or conditionally initialized after declaration.

### Original Problem

```sindarin
// This works - handle initialized at declaration
var h: int = &compute()
h!  // sync

// This did NOT work - conditional assignment failed
var h: TestResult = TestResult { ... }  // pre-initialize
if condition =>
    h = &runSingleTest(...)  // ERROR: incompatible types
h!
```

### Fix

Implemented the "always include pending variables" approach:

1. **Variable declarations** now always declare a pending variable (`__varname_pending__`) for thread-compatible types (primitives, arrays, structs)
2. **Assignment code gen** detects thread spawn assignments and assigns to the pending variable instead
3. **Type checker** allows sync (`!`) on any thread-compatible variable, not just known pending ones
4. **Sync code gen** checks if the pending variable is NULL before syncing (safe no-op if not pending)

### Implementation Details

- Modified `code_gen_stmt_core.c` to always emit dual variables (pending + result)
- Modified `code_gen_expr_core.c` to handle thread spawn in assignments
- Modified `type_checker_expr.c` to allow sync on thread-compatible types
- Modified `code_gen_expr_thread.c` to generate NULL-safe sync code

### Exclusions

The pending variable pattern is NOT applied to:
- Global scope variables
- Variables with `as ref` or `as val` memory qualifiers
- Primitive variables captured by closures (need special reference handling)

### Test

See:
- `tests/integration/test_thread_conditional_spawn.sn` - Conditional int spawn
- `tests/integration/test_thread_conditional_struct.sn` - Conditional struct spawn
- `tests/integration/test_thread_original_approach.sn` - Verifies original approach still works

---

## Issue: Thread Handles Cannot Be Stored in Arrays

**Date:** 2026-01-28
**Severity:** Medium
**Status:** Open (Design Limitation)
**Related:** Thread handle conditional assignment issue (now fixed)

### Description

Thread handles typed as their return type cannot be stored in arrays or collections, preventing dynamic parallel execution patterns like spawning N threads based on runtime values.

### Design Notes

While conditional assignment is now fixed using the "always include pending variables" approach, arrays present additional challenges:
- Arrays are heap-allocated, so we can't use the dual-variable pattern directly
- Would need a parallel array of pending handles, or a more complex tracking mechanism
- Array indices add complexity to the pending variable naming scheme

### Workarounds

Use fixed parallelism with explicit variables:
```sindarin
var t1: Result = &compute(items[0])
var t2: Result = &compute(items[1])
var t3: Result = &compute(items[2])
t1!
t2!
t3!
```

### Potential Future Solutions

1. Introduce explicit handle types: `Thread<T>[]`
2. Runtime thread pool API with work queues
3. Parallel array tracking (e.g., `__arr_pending__: RtThreadHandle*[]`)

---

## Issue: `Path.join` Only Accepts 2 Arguments

**Date:** 2026-01-28
**Severity:** Low
**Status:** FIXED

### Description

The `Path.join` function only accepts 2 arguments.

### Fix

Added `Path.joinAll(parts: str[]): str` function that accepts an array of path components.

Also documented `Path.join3(path1, path2, path3)` which was already available.

### Usage

```sindarin
// For 2 parts
var path: str = Path.join("home", "user")

// For 3 parts
var path: str = Path.join3("home", "user", "file.txt")

// For variable number of parts
var parts: str[] = {"vcpkg", "installed", "x64-windows", "bin"}
var path: str = Path.joinAll(parts)
```

### Test

See `tests/integration/test_path_join_all.sn`

---

## Notes

These issues were discovered while attempting to implement parallel test execution in `sindarin-pkg-sdk/scripts/run_tests.sn` to achieve feature parity with the Python version (`run_tests.py`).
