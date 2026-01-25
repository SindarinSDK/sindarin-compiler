# Compiler Issues Found During Integration Test Expansion

Issues discovered while expanding the integration test suite from 153 to 1024 tests.

---

## 1. Fixed Array Initialization Not Implemented

**Severity:** Feature gap (documented but not implemented)

**Description:** The `docs/memory.md` documentation shows fixed array initialization with values:
```sindarin
var original: int[4] = {1, 2, 3, 4}
var buffer: int[256] = {}
```

However, the compiler rejects all fixed array declarations with:
```
error: Default value type does not match array element type
```

Both value-initialized (`{1, 2, 3}`) and empty-initialized (`{}`) forms fail.

**Reproduction:**
```sindarin
fn main(): void =>
    var arr: int[3] = {1, 2, 3}
    print($"{arr[0]}\n")
```

**Expected:** Compiles and prints `1`
**Actual:** Type checker error: "Default value type does not match array element type"

**Root Cause:** The type checker likely types the initializer list `{1, 2, 3}` as a dynamic array (`int[]`) and then fails the assignment to the fixed array type (`int[3]`).

---

## 2. Compound Assignment (`+=`) Fails in Shared Closures

**Severity:** Bug

**Description:** Using compound assignment operators (`+=`, `-=`, `*=`, etc.) on captured variables inside `shared` closures causes C compilation errors, even though simple assignment (`x = x + y`) works correctly.

**Reproduction:**
```sindarin
fn main(): void =>
    var total: int = 0
    var add_to: fn(int): void = fn(x: int) shared: void =>
        total += x    // Fails
    add_to(10)
    print($"{total}\n")
```

**Error:**
```
error: use of undeclared identifier '__sn__total'
```

**Workaround:** Use explicit assignment: `total = total + x`

**Root Cause:** Code generation for compound assignment in shared closures doesn't properly resolve the captured variable reference.

---

## 3. Lambda Expressions Cannot Be Used Inline in String Interpolation

**Severity:** Parser limitation

**Description:** When a lambda expression appears as an argument to a function call inside string interpolation (`$"{...}"`), the parser misinterprets the `)` after the lambda body as ending the interpolation expression.

**Reproduction:**
```sindarin
fn main(): void =>
    var arr: int[] = {1, 2, 3}
    print($"{any_match(arr, fn(x: int): bool => x > 2)}\n")
```

**Error:**
```
error: Expected ')' after lambda parameters at end of file
error: Invalid expression in interpolation (got ')')
```

**Workaround:** Assign the result to a variable first:
```sindarin
var result: bool = any_match(arr, fn(x: int): bool => x > 2)
print($"{result}\n")
```

---

## 4. `char` Type Cannot Be Initialized with Integer Literals

**Severity:** Bug / Missing feature

**Description:** The `char` type cannot be initialized with an integer literal representing the character code. The type checker rejects the assignment.

**Reproduction:**
```sindarin
fn main(): void =>
    var ch: char = 65    // ASCII 'A'
    print($"{ch}\n")
```

**Error:**
```
error: Initializer type does not match variable type
```

**Note:** There's no documented char literal syntax (e.g., `'A'`), so there appears to be no way to initialize a `char` variable at all from user code.

---

## 5. No Implicit Narrowing from `double` to `int`

**Severity:** By design (but undocumented)

**Description:** Assigning a `double` expression to an `int` variable is rejected. There is no implicit narrowing conversion or truncation.

**Reproduction:**
```sindarin
fn main(): void =>
    var d: double = 7.9
    var i: int = d          // Fails
    var j: int = 10.0 / 3.0  // Also fails
```

**Error:**
```
error: Initializer type does not match variable type
```

**Note:** There doesn't appear to be an explicit cast syntax either. The only workaround is to avoid cross-type assignment entirely.

---

## 6. Array/String Reassignment Blocked in All Loop Bodies (Private Block Semantics)

**Severity:** Significant limitation

**Description:** For-loop and while-loop bodies are "private blocks" that prevent reassignment of heap-allocated variables (arrays, strings) declared in outer scopes. This blocks common patterns like iterative algorithms that update an accumulator array.

**Reproduction:**
```sindarin
fn main(): void =>
    var prev: int[] = {1}
    while condition =>
        var curr: int[] = compute(prev)
        prev = curr    // Fails - can't reassign heap var in private block
```

**Error:**
```
error: Cannot assign to variable declared outside private block: array type is heap-allocated
```

**Affected types:** `str`, `int[]`, `str[]`, and all other array/heap-allocated types.

**Workarounds:**
- Restructure algorithms to avoid reassignment (use index-based mutation instead)
- Use `append` for strings instead of reassignment
- For arrays, modify elements in-place rather than replacing the whole array

**Note:** This also affects string concatenation in loops (`s = s + "x"` fails).

---

## 7. `any` Type Cannot Be Used in String Interpolation

**Severity:** Bug

**Description:** Variables of type `any` cannot be used inside string interpolation expressions. The generated C code produces a type incompatibility error.

**Reproduction:**
```sindarin
fn main(): void =>
    var x: any = 42
    print($"{x}\n")
```

**Error (during C compilation):**
```
error: passing 'RtAny' to parameter of incompatible type 'void *'
```

**Note:** `typeof(x)` and `x is int` work correctly with `any` type; only string interpolation is broken.

---

## 8. Nested `for-each` Loops Fail to Parse

**Severity:** Bug

**Description:** Nested `for x in arr =>` loops cause parser errors. The inner loop variable declaration is misinterpreted.

**Reproduction:**
```sindarin
fn main(): void =>
    var grid: int[][] = {{1, 2}, {3, 4}}
    for row in grid =>
        for val in row =>
            print($"{val} ")
```

**Workaround:** Use index-based loops instead:
```sindarin
for var i: int = 0; i < grid.length; i++ =>
    for var j: int = 0; j < grid[i].length; j++ =>
        print($"{grid[i][j]} ")
```

---

## 9. `nil` Can Only Be Assigned to Pointer Types

**Severity:** By design (but undocumented)

**Description:** `nil` cannot be assigned to `str`, `int[]`, or other reference types - only to explicit pointer types.

**Reproduction:**
```sindarin
var s: str = nil    // Fails
```

**Error:**
```
error: nil can only be assigned to pointer types
```

---

## 10. Missing Language Features (Documented vs Implemented)

**Severity:** Documentation gaps

The following features are referenced in documentation or would be expected but don't exist:

| Feature | Status |
|---------|--------|
| Hex literals (`0xFF`) | Not implemented - `0` parsed then `xFF` as identifier |
| Binary literals (`0b1010`) | Not implemented |
| Octal literals (`0o77`) | Not implemented |
| Bitwise operators (`&`, `\|`, `^`, `<<`, `>>`) | Not implemented |
| `trimStart()` / `trimEnd()` on strings | Not implemented (only `trim()` exists) |
| Explicit type casting (e.g., `int(x)`) | No syntax available |
| Char literals (`'A'`) | Parser doesn't support single-quote char literals |

---

## Summary

| # | Issue | Severity | Workaround Available |
|---|-------|----------|---------------------|
| 1 | Fixed array init | Feature gap | Use dynamic arrays |
| 2 | `+=` in shared closures | Bug | Use `x = x + y` |
| 3 | Lambda in interpolation | Parser limitation | Assign to variable first |
| 4 | `char` init with int | Bug | Use `byte` type instead |
| 5 | No double-to-int narrowing | By design | Avoid cross-type assignment |
| 6 | Heap var reassign in loops | Limitation | Restructure algorithm |
| 7 | `any` in interpolation | Bug | Use `typeof()` checks |
| 8 | Nested for-each | Bug | Use index-based loops |
| 9 | `nil` assignment | By design | N/A |
| 10 | Missing features | Docs gap | N/A |
