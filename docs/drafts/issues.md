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
**Status:** Open (By Design)

### Description

Thread handles in Sindarin must be initialized at declaration time with the `&fn()` spawn syntax. They cannot be reassigned or conditionally initialized after declaration.

### Reproduction

```sindarin
// This works - handle initialized at declaration
var h: int = &compute()
h!  // sync

// This does NOT work - conditional assignment fails
var h: TestResult = TestResult { ... }  // pre-initialize
if condition =>
    h = &runSingleTest(...)  // ERROR: incompatible types
h!
```

### Compiler Error

```
error: incompatible types when assigning to type '__sn__TestResult' from type 'RtThreadHandle *'
```

### Impact

This limitation prevents implementing dynamic parallel execution patterns where the number of threads depends on runtime values.

### Design Notes

This is a fundamental limitation of the current type system. Thread spawns have a dual nature:
- Before sync: they are `RtThreadHandle*`
- After sync: they are the result type

The current design uses a "pending variable" pattern at declaration time to handle this duality, but this pattern cannot be easily extended to reassignment scenarios.

### Workarounds

1. Use fixed parallelism (spawn a known number of threads at declaration time)
2. Structure code to declare thread handles within conditional blocks:
   ```sindarin
   if condition =>
       var h: TestResult = &runSingleTest(...)
       var result: TestResult = h!
       // use result
   ```

### Potential Future Solutions

1. Introduce explicit handle types: `Thread<T>`
2. Runtime thread pool API
3. Significant refactoring of variable declaration to always include pending variables

---

## Issue: Thread Handles Cannot Be Stored in Arrays

**Date:** 2026-01-28
**Severity:** Medium
**Status:** Open (By Design)
**Related:** Thread handle conditional assignment issue

### Description

Thread handles typed as their return type cannot be stored in arrays or collections, preventing dynamic parallel execution patterns.

### Design Notes

This is related to the same dual-typing issue as conditional assignment. An array typed as `TestResult[]` cannot hold `RtThreadHandle*` values.

### Potential Future Solutions

Same as conditional assignment issue - requires introducing explicit handle types or significant type system changes.

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
