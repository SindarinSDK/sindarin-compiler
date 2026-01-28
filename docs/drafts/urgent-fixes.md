# Urgent Fixes

Issues discovered while converting Python scripts to Sindarin.

---

## 1. Array Literal Assignment in If Blocks Produces Empty Strings

**Severity:** High

**Description:** When a string array is assigned a literal value inside an if block, the array has the correct length but all string elements are empty.

**Reproduction:**
```sindarin
fn main(): int =>
    var dep: str = "zlib"

    var patterns: str[] = {}
    if dep == "zlib" =>
        patterns = {"libz.a", "libzlib.a", "z.lib"}

    print($"patterns length: {patterns.length}\n")  // prints 3
    for pattern in patterns =>
        print($"  pattern: '{pattern}' len={pattern.length}\n")  // all empty!

    return 0
```

**Expected:** Array contains `"libz.a"`, `"libzlib.a"`, `"z.lib"`.

**Actual:** Array has length 3, but all elements are empty strings (length 0).

**Workaround:** Use `push()` instead of array literal assignment:
```sindarin
var patterns: str[] = {}
if dep == "zlib" =>
    patterns.push("libz.a")
    patterns.push("libzlib.a")
    patterns.push("z.lib")
```

---

## 2. Thread Spawn Does Not Support Struct Return Types

**Severity:** Medium

**Description:** Attempting to spawn a thread for a function that returns a user-defined struct causes code generation to produce invalid C code.

**Reproduction:**
```sindarin
struct Result =>
    value: int
    ok: bool

fn compute(): Result =>
    return Result { value: 42, ok: true }

fn main(): int =>
    var h: Result = &compute()  // Thread spawn
    var r: Result = h!          // Sync
    print($"value: {r.value}\n")
    return 0
```

**Expected:** Thread spawns and returns struct correctly.

**Actual:** Generated C code has type errors - thread handle is typed as the struct instead of `RtThreadHandle*`.

**Workaround:** Use void threads with shared state, or use primitive return types (int, str, etc.).

---

## Summary

| Issue | Severity | Blocking? |
|-------|----------|-----------|
| Array literal in if blocks | High | Partial - workaround with push() |
| Struct return in thread spawn | Medium | Yes - limits parallel execution patterns |
