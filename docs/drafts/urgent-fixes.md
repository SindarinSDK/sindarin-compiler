# Urgent Compiler Fixes

## 1. Arena Memory Corruption with Returned String Arrays

**Priority:** High
**Discovered:** 2026-01-28
**Context:** Rewriting `sindarin-pkg-sdk/scripts/setup_deps.py` in Sindarin

### Problem Description

When a function returns a `str[]` array, the strings within that array become corrupted after subsequent function calls. This appears to be an arena-based memory management issue where the returned array's string data is allocated in the callee's arena and gets overwritten when other functions execute.

### Reproduction

```sindarin
fn getTools(): str[] =>
    return {"cmake", "git", "curl", "zip"}

fn checkTool(name: str): bool =>
    var p: Process = Process.runArgs("which", {name})
    return p.success()

fn main(): void =>
    var tools: str[] = getTools()

    for tool in tools =>
        var ok: bool = checkTool(tool)  // After this call, `tool` is corrupted
        print($"  {tool}: {ok}\n")       // Prints garbage like "@e?&Y: true"
```

### Observed Behavior

```
Checking build tools...
  @e?&Y: MISSING
  OK: MISSING
  MISSING: MISSING
    @e?&Y: MISSING
```

The tool names are corrupted and display as garbage characters, while the status strings ("OK", "MISSING") which are defined inline remain intact.

### Root Cause Analysis

The issue appears to be that:

1. `getTools()` returns an array with string literals
2. These strings are allocated in `getTools()`'s arena
3. When the function returns, the array is "promoted" to the caller's arena, but the string data may not be properly copied
4. When `checkTool()` is called, it uses arena memory that overlaps with where the strings were stored
5. The string pointers now point to corrupted/reused memory

### Current Workaround

Avoid iterating over arrays returned from functions. Instead, inline the data:

```sindarin
fn checkAllTools(): bool =>
    // DON'T DO THIS:
    // var tools: str[] = getTools()
    // for tool in tools => ...

    // DO THIS INSTEAD:
    checkAndPrint("cmake")
    checkAndPrint("git")
    checkAndPrint("curl")
    checkAndPrint("zip")
```

Or use index-based access with immediate use:

```sindarin
var tools: str[] = getTools()
var i: int = 0
while i < tools.length =>
    // Use tools[i] directly in function calls, don't store in variable
    checkTool(tools[i])
    print($"  {tools[i]}\n")  // May still be corrupted
    i = i + 1
```

### Expected Behavior

Strings in arrays returned from functions should remain valid and accessible throughout the caller's scope, regardless of subsequent function calls.

### Suggested Fix Areas

1. **String promotion on function return** - Ensure string data (not just pointers) is copied when arrays are promoted from callee to caller arena
2. **For-each loop variable binding** - The loop variable `tool` should hold a stable reference/copy that survives function calls within the loop body
3. **Arena isolation** - Function calls within a scope shouldn't corrupt data that was already in scope before the call

### Impact

This bug makes it impractical to:
- Return configuration arrays from functions
- Use factory/builder patterns that return collections
- Write idiomatic code with helper functions returning data

It forces developers to inline data definitions, leading to code duplication and reduced maintainability.

### Test Case for Verification

```sindarin
// test_arena_string_array.sn
import "sdk/os/process"

fn getNames(): str[] =>
    return {"alice", "bob", "charlie"}

fn doSomething(s: str): int =>
    // This function uses the arena, potentially corrupting returned arrays
    var p: Process = Process.run("echo test")
    return s.length

fn main(): void =>
    var names: str[] = getNames()

    for name in names =>
        var len: int = doSomething(name)
        // After fix, this should print: "alice: 5", "bob: 3", "charlie: 7"
        // Currently prints garbage
        print($"{name}: {len}\n")

    // Verify expected output
    if names[0] == "alice" && names[1] == "bob" && names[2] == "charlie" =>
        print("PASS: Array strings preserved\n")
    else =>
        print("FAIL: Array strings corrupted\n")
```

---

## Additional Notes

This issue was discovered while porting `setup_deps.py` (a 529-line Python script) to Sindarin. The workarounds required added approximately 80 lines to the Sindarin version and significantly reduced code clarity.

The full reproduction case is available at:
- `sindarin-pkg-sdk/scripts/setup_deps.sn` (working version with workarounds)
- `sindarin-pkg-sdk/scripts/setup_deps.py` (original Python for comparison)
