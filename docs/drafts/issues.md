# Sindarin Compiler Issues

This document tracks issues discovered during development.

---

## Issue: Thread Handles Cannot Be Conditionally Assigned

**Date:** 2026-01-28
**Severity:** Medium
**Status:** Open

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

This limitation prevents implementing dynamic parallel execution patterns where the number of threads depends on runtime values (e.g., batch processing N tests in parallel where N is variable).

### Workaround

Currently, only sequential execution is possible for dynamic workloads. Fixed parallelism (spawning a known number of threads at declaration time) still works.

### Suggested Fix

Either:
1. Allow thread handle reassignment with proper type coercion
2. Support storing thread handles in arrays: `var handles: TestResult[] = {}; handles.push(&fn())`
3. Provide a runtime thread pool API

---

## Issue: Thread Handles Cannot Be Stored in Arrays

**Date:** 2026-01-28
**Severity:** Medium
**Status:** Open
**Related:** Thread handle conditional assignment issue

### Description

Thread handles typed as their return type (e.g., `var h: TestResult = &fn()`) cannot be stored in arrays or collections, preventing dynamic parallel execution patterns.

### Reproduction

```sindarin
// Desired pattern (does not work):
var handles: TestResult[] = {}
for i in 0..10 =>
    handles.push(&runTest(i))  // Would need to store thread handle in array

for h in handles =>
    h!  // Sync all
```

### Impact

Cannot implement thread pool or batch-based parallelism where the number of concurrent operations is determined at runtime.

---

## Issue: `Path.join` Only Accepts 2 Arguments

**Date:** 2026-01-28
**Severity:** Low
**Status:** By Design (documentation needed)

### Description

The `Path.join` function only accepts 2 arguments, unlike Python's `os.path.join` which accepts variadic arguments.

### Reproduction

```sindarin
// This fails:
var path: str = Path.join("vcpkg", "installed", "x64-windows", "bin")
// Error: Path.join expects 2 argument(s), got 4

// Must chain calls instead:
var path: str = Path.join(Path.join(Path.join("vcpkg", "installed"), "x64-windows"), "bin")
```

### Suggested Improvement

Consider adding variadic support or a `Path.joinAll(parts: str[]): str` function for convenience.

---

## Issue: `rt_unbox_struct` Signature Mismatch in Thread Thunks

**Date:** 2026-01-28
**Severity:** High
**Status:** Open

### Description

When spawning threads with struct parameters, the generated C code calls `rt_unbox_struct` with incorrect arguments, causing compilation failures.

### Compiler Error

```
error: too few arguments to function 'rt_unbox_struct'
note: expected 'RtAny value, int expected_type_id' but got only 'RtAny value'
```

### Generated Code (Incorrect)

```c
__sn__TestResult __tmp_result = __sn__runSingleTest(
    (RtArena *)__rt_thunk_arena,
    rt_unbox_struct(__rt_thunk_args[0]),  // Missing type_id argument
    ...
);
```

### Expected

```c
rt_unbox_struct(__rt_thunk_args[0], TYPE_ID_TestRunner)
```

### Impact

Cannot spawn threads for functions that take struct parameters.

---

## Notes

These issues were discovered while attempting to implement parallel test execution in `sindarin-pkg-sdk/scripts/run_tests.sn` to achieve feature parity with the Python version (`run_tests.py`).
