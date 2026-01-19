# Sindarin Memory Management

## First Principles

### 1. Primitives are values, arrays are references

```sindarin
// Primitives: assignment copies the value
var a: int = 42
var b: int = a
b = 99                    // a is still 42

// Arrays: assignment creates a reference (alias)
var x: int[] = {1, 2, 3}
var y: int[] = x          // y references the same array as x
y[0] = 99                 // x[0] is now 99
y.push(4)                 // x is now {99, 2, 3, 4}
```

### 2. Use `.clone()` for explicit copies

```sindarin
var x: int[] = {1, 2, 3}
var y: int[] = x.clone()  // y is an independent copy
y[0] = 99                 // x[0] is still 1
```

### 3. Strings are immutable, so reference vs copy doesn't matter

```sindarin
var a: str = "hello"
var b: str = a            // Reference to same string
b = b.toUpper()           // b now points to NEW string "HELLO"
                          // a still points to "hello"
```

---

## Stack vs Heap Allocation

### Primitives: Stack by Default

```sindarin
var count: int = 0        // Stack
var pi: double = 3.14     // Stack
var flag: bool = true     // Stack
var letter: char = 'A'    // Stack
```

### Primitives as References (Heap)

Use `as ref` to allocate primitives on the heap with reference semantics:

```sindarin
var count: int as ref = 0       // Heap allocated
var alias: int = count          // alias references same heap location
alias = 42                       // count is now 42

// Useful for sharing mutable state
fn increment(n: int as ref): void =>
  n = n + 1

var value: int as ref = 0
increment(value)
print(value)                     // 1
increment(value)
print(value)                     // 2
```

### Escape Behavior for Primitive References

Primitive references follow the same promotion rules as arrays:

```sindarin
var outer: int as ref

if condition =>
  var inner: int as ref = 100   // Heap, in if-block's arena
  outer = inner                  // Promoted to parent arena
  // if-block arena freed, but value persists

print(outer)                     // 100 - safe
```

### Fixed Arrays: Stack or Heap (automatic)

```sindarin
var buffer: int[256] = {}       // Stack (2KB) - small, stays on stack
var matrix: double[4][4] = {}   // Stack (128 bytes)
var large: int[1024] = {}       // Stack (8KB) - at threshold
var huge: int[10000] = {}       // Heap (80KB) - auto-promoted, too large for stack
```

**Threshold**: ~8KB (1024 `int`/`long` elements). Larger fixed arrays are automatically heap-allocated in the current arena. This is transparent to the programmer.

### Dynamic Arrays: Heap (Arena-managed)

```sindarin
var items: int[] = {}           // Heap
var names: str[] = {"a", "b"}   // Heap
```

### Strings: Heap (Arena-managed)

```sindarin
var name: str = "hello"         // Heap
var msg: str = $"Hi {name}"     // Heap
```

---

## Reference Semantics for Arrays

All array assignments create references, regardless of stack or heap allocation.

### Same Array, Multiple Names

```sindarin
var original: int[4] = {1, 2, 3, 4}
var alias: int[4] = original    // Both point to same memory

alias[0] = 100                  // original[0] is now 100
print(original)                 // {100, 2, 3, 4}
```

### Function Parameters are References (by default)

```sindarin
fn double_all(arr: int[]): void =>
  for var i: int = 0; i < arr.length; i++ =>
    arr[i] = arr[i] * 2

var nums: int[] = {1, 2, 3}
double_all(nums)
print(nums)                     // {2, 4, 6} - modified!
```

### Function Parameters with `as val` (copy on call)

Use `as val` to receive a copy. The copy behaves like a reference inside the function, but changes don't affect the caller's original.

```sindarin
fn double_all_safe(arr: int[] as val): void =>
  // arr is a copy of the caller's array
  for var i: int = 0; i < arr.length; i++ =>
    arr[i] = arr[i] * 2
  // modifications stay local to this function

var nums: int[] = {1, 2, 3}
double_all_safe(nums)
print(nums)                     // {1, 2, 3} - unchanged!
```

### When to Use `as val`

```sindarin
// Default (reference): when you want to modify the original
fn sort_in_place(arr: int[]): void =>
  // ... sorting modifies arr directly ...

// as val: when you need to work with data without affecting caller
fn calculate_sum(arr: int[] as val): int =>
  var sum: int = 0
  while arr.length > 0 =>
    sum = sum + arr.pop()       // Safe: only modifies local copy
  return sum

var data: int[] = {1, 2, 3, 4, 5}
var total: int = calculate_sum(data)
print(data.length)              // 5 - still intact
```

### Returning Arrays

```sindarin
fn make_array(): int[] =>
  var result: int[] = {1, 2, 3}
  return result                 // Returns reference

var arr: int[] = make_array()   // arr references the returned array
```

### Nested Arrays (Arrays of References)

Arrays of arrays (`int[][]`) are heap-allocated and follow the same reference and escape rules:

```sindarin
var matrix: int[][] = {}              // Heap: array of references to arrays

// Each element is a reference to another array
matrix.push({1, 2, 3})                // Inner array allocated on heap
matrix.push({4, 5, 6})

var row: int[] = matrix[0]            // row references same array as matrix[0]
row[0] = 99                           // matrix[0][0] is now 99
```

### Escape Behavior for Nested Arrays

```sindarin
fn build_matrix(): int[][] =>
  var result: int[][] = {}

  for var i: int = 0; i < 3 =>
    var row: int[] = {i, i+1, i+2}    // Allocated in loop arena
    result.push(row)                   // row promoted to function arena
    // Loop arena freed, but row data persists in function arena

  return result                        // Promoted to caller's arena

var m: int[][] = build_matrix()        // All nested arrays live in caller's arena
```

### Assignment Semantics

```sindarin
var a: int[][] = {{1, 2}, {3, 4}}
var b: int[][] = a                    // b references same outer array

b[0] = {9, 9}                         // a[0] is now {9, 9}
b[0][0] = 5                           // a[0][0] is now 5

// For independent copy, use clone (deep copy)
var c: int[][] = a.clone()            // c is independent
c[0][0] = 100                         // a[0][0] unchanged
```

---

## Explicit Copies with `.clone()` or `as val`

When you need an independent copy, use `.clone()` or `as val`:

```sindarin
var original: int[] = {1, 2, 3}

// Using .clone()
var copy1: int[] = original.clone()

// Using as val (equivalent)
var copy2: int[] = original as val

copy1[0] = 99                   // original[0] is still 1
copy2[0] = 88                   // original[0] is still 1
```

### Example: Safe Modification

```sindarin
fn safe_modify(arr: int[]): int[] =>
  var copy: int[] = arr as val  // Independent copy
  for var i: int = 0; i < copy.length; i++ =>
    copy[i] = copy[i] * 2
  return copy

var original: int[] = {1, 2, 3}
var doubled: int[] = safe_modify(original)
print(original)                 // {1, 2, 3} - unchanged
print(doubled)                  // {2, 4, 6}
```

---

## Block-Scoped Arenas

Every block has an arena that manages heap allocations within that scope.

### Basic Model

```sindarin
fn process(): void =>
  // Function arena created

  var data: str[] = {"a", "b", "c"}    // Allocated in function arena

  for item in data =>
    // Loop arena created
    var temp: str = item.toUpper()     // Allocated in loop arena
    print(temp)
    // Loop arena destroyed - temp freed

  // data still valid (in function arena)
  print(data.length)

  // Function arena destroyed - data freed
```

### Escaping References (Automatic Promotion)

When an inner-scope allocation is assigned to an outer-scope variable, it's promoted:

```sindarin
fn find_longest(items: str[]): str =>
  var longest: str = ""               // Function arena

  for item in items =>
    // Loop arena
    var upper: str = item.toUpper()   // Loop arena
    if upper.length > longest.length =>
      longest = upper                 // PROMOTED to function arena
    // Loop arena freed, but longest survives

  return longest                      // Promoted to caller's arena
```

### Arena Hierarchy

```
Caller's Arena
  └── Function Arena
        ├── allocations (data, longest)
        └── Loop Arena (per iteration)
              └── allocations (temp, upper) - freed each iteration
```

---

## `shared` Functions

A `shared` function uses the caller's arena directly, avoiding promotion overhead.

### Syntax

```sindarin
fn build_message(name: str) shared: str =>
  var greeting: str = "Hello, "
  var result: str = greeting + name + "!"
  return result                       // No promotion - already in caller's arena

fn main(): void =>
  var msg: str = build_message("World")
  print(msg)
```

### When to Use `shared`

- Functions that build and return heap-allocated values
- Builder patterns
- Functions called frequently in loops

```sindarin
// Without shared: each call promotes return value
fn format(n: int): str =>
  return $"Value: {n}"

// With shared: allocates directly in caller's arena
fn format_fast(n: int) shared: str =>
  return $"Value: {n}"

fn main(): void =>
  for var i: int = 0; i < 1000; i++ =>
    var s: str = format_fast(i)       // No promotion overhead
    print(s)
```

### `shared` Propagation

```sindarin
fn helper() shared: str =>
  return "helper result"

fn outer() shared: str =>
  var h: str = helper()               // helper uses outer's arena
  return "outer: " + h                // which is caller's arena
```

### `shared` Loops

Apply `shared` to a loop to avoid arena creation/destruction per iteration:

```sindarin
fn collect_names(ids: int[]): str[] =>
  var names: str[] = {}

  // Default: each iteration creates/destroys an arena
  for id in ids =>
    var name: str = lookup_name(id)   // Allocated in loop arena
    names.push(name)                  // Promoted to function arena
    // Loop arena destroyed

  return names
```

```sindarin
fn collect_names_fast(ids: int[]): str[] =>
  var names: str[] = {}

  // Shared: loop uses function's arena directly
  for id in ids shared =>
    var name: str = lookup_name(id)   // Allocated in function arena
    names.push(name)                  // No promotion needed
    // No arena destruction

  return names
```

### When to Use `shared` Loops

```sindarin
// Default loop: good when iterations are independent
// - Temporary allocations cleaned up each iteration
// - Prevents memory accumulation in long loops

// Shared loop: good when building results
// - Avoids promotion overhead
// - All allocations persist until parent scope ends
// - Use with caution in long loops (memory accumulates)
```

**Warning**: In a `shared` loop, temporary allocations accumulate:

```sindarin
// Dangerous: memory grows with each iteration
for var i: int = 0; i < 1000000 shared =>
  var temp: str = compute_something()  // Never freed until loop ends!
  process(temp)

// Safe: temporaries freed each iteration
for var i: int = 0; i < 1000000 =>
  var temp: str = compute_something()  // Freed each iteration
  process(temp)
```

---

## `shared` Blocks

A `shared` block makes everything inside share the parent's arena - all nested loops, conditionals, and inner blocks.

### Syntax

```sindarin
fn process(items: str[]): str[] =>
  var results: str[] = {}

  shared =>
    // Everything here uses the function's arena
    // No per-iteration arenas, no promotion overhead
    for item in items =>
      var upper: str = item.toUpper()
      var trimmed: str = upper.trim()
      results.push(trimmed)
      // No arena destroyed here

    for result in results =>
      if result.length > 10 =>
        print(result)
        // Still no nested arenas

  return results
```

### Comparison: Default vs Shared Block

```sindarin
// Default: each loop iteration has its own arena
fn default_version(items: str[]): void =>
  for item in items =>              // Arena created
    var temp: str = item.toUpper()  // Allocated in loop arena
    process(temp)
    // Arena destroyed, temp freed

// Shared block: everything uses parent arena
fn shared_version(items: str[]): void =>
  shared =>
    for item in items =>            // No arena created
      var temp: str = item.toUpper() // Allocated in function arena
      process(temp)
      // Nothing freed here
  // All temps freed when shared block ends
```

### Use Cases

**1. Performance-critical sections**
```sindarin
fn hot_path(data: int[]): int =>
  var sum: int = 0
  shared =>
    // Entire computation shares one arena
    for var i: int = 0; i < data.length; i++ =>
      for var j: int = 0; j < data.length; j++ =>
        sum = sum + data[i] * data[j]
  return sum
```

**2. Building complex results**
```sindarin
fn build_report(records: str[][]): str =>
  shared =>
    var parts: str[] = {}
    for record in records =>
      var line: str = record.join(",")
      var formatted: str = $"[{line}]"
      parts.push(formatted)
    return parts.join("\n")
```

### Nesting Behavior

`shared` propagates inward - nested blocks don't create new arenas:

```sindarin
shared =>
  // Level 1: uses parent arena
  for i in items =>
    // Level 2: still uses parent arena
    if condition =>
      // Level 3: still uses parent arena
      while processing =>
        // Level 4: still uses parent arena
        var temp: str = compute()
```

### Combining with `private`

`private` inside `shared` creates an isolated arena (private wins):

```sindarin
shared =>
  var results: str[] = {}

  for item in items =>
    // Still shared

    private =>
      // Isolated! Nothing escapes from here
      var huge: str = load_big_data(item)
      var count: int = process(huge)
      results.push($"{count}")  // ERROR: string can't escape private

  return results
```

---

## `private` Blocks

A `private` block creates an isolated arena. Nothing heap-allocated can escape.

### Syntax

```sindarin
fn analyze(path: str): int =>
  var result: int = 0

  private =>
    var contents: str = read_file(path)
    var lines: str[] = contents.split("\n")
    result = lines.length             // Primitives can escape
    // contents and lines freed here - guaranteed

  return result
```

### What Can Escape `private`

| Type | Can Escape? |
|------|-------------|
| `int`, `double`, `bool`, `char` | Yes |
| `int[N]` (fixed array) | No - compile error |
| `int[]` (dynamic array) | No - compile error |
| `str` | No - compile error |

```sindarin
private =>
  var count: int = 42
  var buffer: int[100] = {}
  var dynamic: int[] = {1, 2, 3}
  var text: str = "hello"

  outer_count = count           // OK: primitive
  outer_buffer = buffer         // COMPILE ERROR: array cannot escape
  outer_array = dynamic         // COMPILE ERROR: array cannot escape
  outer_text = text             // COMPILE ERROR: string cannot escape
```

### `private` Functions

```sindarin
// Returns primitive - OK
fn count_lines(path: str) private: int =>
  var contents: str = read_file(path)
  var lines: str[] = contents.split("\n")
  return lines.length                 // int escapes, rest freed

// COMPILE ERROR: cannot return array from private
fn get_histogram(data: str) private: int[256] =>
  var counts: int[256] = {}
  for c in data =>
    counts[c]++
  return counts                       // ERROR: array cannot escape

// COMPILE ERROR: cannot return str from private
fn bad(path: str) private: str =>
  return read_file(path)              // ERROR: string cannot escape
```

Only primitives (`int`, `double`, `bool`, `char`) can be returned from `private` functions.

### Use Case: Processing Large Temporary Data

```sindarin
fn process_huge_file(path: str): int =>
  var total: int = 0

  private =>
    var contents: str = read_file(path)       // Maybe 100MB
    var records: str[] = contents.split("\n") // Thousands of strings

    for record in records =>
      var fields: str[] = record.split(",")
      total = total + parse_int(fields[0])

    // ALL memory freed here - no leaks possible

  return total
```

---

## Fixed Array Escape Rules

Fixed arrays start on the stack but are **auto-promoted to heap** when they escape their scope. This keeps reference semantics consistent.

### Within Same Scope: Stack + Reference

```sindarin
fn example(): void =>
  var a: int[4] = {1, 2, 3, 4}        // Stack allocated
  var b: int[4] = a                   // b references same stack memory
  b[0] = 99                           // a[0] is now 99
  // Both cleaned up when function ends
```

### Escaping Scope: Auto-Promotion to Heap

```sindarin
var outer: int[4]

if condition =>
  var inner: int[4] = {1, 2, 3, 4}   // Starts on stack
  outer = inner                       // PROMOTED: copied to outer arena (heap)
  // inner's stack space reclaimed, but data lives on in outer's arena

print(outer[0])                       // Safe - data is in heap arena
```

### Returning Fixed Arrays

```sindarin
fn make_buffer(): int[100] =>
  var buf: int[100] = {}              // Stack allocated
  for var i: int = 0; i < 100; i++ =>
    buf[i] = i
  return buf                          // PROMOTED to caller's arena

var result: int[100] = make_buffer()  // result references heap copy
```

### Why Auto-Promotion?

Keeps the mental model simple:
- Arrays are **always references**
- Compiler handles stack-to-heap promotion transparently
- No special cases for the programmer to remember

---

## Summary

| Concept | Behavior |
|---------|----------|
| Primitive assignment | Copy value (stack) |
| Primitive `as ref` | Reference (heap), auto-promotes on escape |
| Array assignment | Reference (alias) |
| Explicit copy | `.clone()` or `as val` |
| Function parameters | Reference by default, copy with `as val` |
| Fixed arrays (`int[N]`) | Stack if small, heap if large or escapes |
| Dynamic arrays (`int[]`) | Heap, arena-managed |
| Strings | Heap, arena-managed, immutable |
| Block scope | Creates arena, frees on exit |
| Escaping values | Promoted to outer arena |
| `shared` function | Uses caller's arena |
| `shared` loop | Uses parent's arena (no per-iteration arena) |
| `shared` block | All nested scopes use parent's arena |
| `private` block | Isolated arena, only primitives escape |

---

## Lifetime Errors

The compiler is strict about lifetime violations. These are compile-time errors, not runtime crashes.

### Escaping `private` Blocks

```sindarin
fn bad(): void =>
  var result: str
  private =>
    var temp: str = "hello"
    result = temp             // ERROR: string cannot escape private block
  print(result)
```

```
error[E0101]: cannot escape `private` block
  --> example.sn:4:5
   |
 3 |   private =>
   |   -------- private block starts here
 4 |     var temp: str = "hello"
 5 |     result = temp
   |     ^^^^^^^^^^^^^ `str` cannot escape private block
   |
   = note: only primitives (int, double, bool, char) can escape private blocks
```

### Invalid Return from `private` Function

```sindarin
fn bad() private: str[] =>
  return {1, 2, 3}            // ERROR: array cannot escape private function
```

```
error[E0102]: invalid return type for `private` function
  --> example.sn:1:20
   |
 1 | fn bad() private: str[] =>
   |          -------  ^^^^^ arrays cannot be returned from private functions
   |          |
   |          function marked private here
   |
   = note: private functions can only return primitives (int, double, bool, char)
```

---

## Defaults and Backward Compatibility

All memory management features are **opt-in**. Existing code compiles unchanged.

| Feature | Default | Opt-in Alternative |
|---------|---------|-------------------|
| Array assignment | Reference | `as val` for copy |
| Function parameters | Reference | `as val` for copy |
| Primitives | Stack (value) | `as ref` for heap |
| Blocks | Own arena | - |
| Functions | Own arena | `shared` to use caller's |
| Loops | Arena per iteration | `shared` to use parent's |

```sindarin
// This existing code works exactly as before
fn example(): void =>
  var items: int[] = {1, 2, 3}
  for item in items =>
    print(item)
```

---

## Performance Considerations

### Costs of Default Model

**1. Arena overhead per block**
```sindarin
for var i: int = 0; i < 1000000; i++ =>
  var temp: str = compute()    // Arena created/destroyed each iteration
  process(temp)
```
- Small overhead: arena metadata allocation/deallocation per iteration
- **Mitigation**: Use `shared` for hot loops

**2. Auto-promotion copies**
```sindarin
var result: str
for item in items =>
  result = item.toUpper()      // Promoted (copied) to outer arena each time
```
- Hidden copy when values escape inner scope
- **Mitigation**: Use `shared` loop, or restructure code

**3. Memory not freed until scope ends**
```sindarin
fn long_running(): void =>
  // Everything allocated here lives until function returns
  var data1: str = load_file("a.txt")
  var data2: str = load_file("b.txt")
  var data3: str = load_file("c.txt")
  // ... all three in memory until function ends
```
- **Mitigation**: Use `private` blocks for temporary processing

### Benefits of Default Model

**1. No reference counting overhead**
- No increment/decrement on every assignment
- Faster than RC for assignment-heavy code

**2. Bulk deallocation**
- Arena destruction is O(1), not O(n) individual frees
- Cache-friendly memory layout

**3. No fragmentation**
- Arena allocations are contiguous
- Better memory locality

**4. Deterministic cleanup**
- Memory freed at predictable points (scope exit)
- No GC pauses

### Performance Guidelines

| Scenario | Recommendation |
|----------|---------------|
| Hot inner loop | `shared` loop |
| Building/returning values | `shared` function |
| Large temporary processing | `private` block |
| Passing large arrays | Default (reference) is fast |
| Need isolated copy | `as val` (explicit cost) |

```sindarin
// Slow: arena per iteration, promotion overhead
fn slow(items: str[]): str[] =>
  var results: str[] = {}
  for item in items =>
    results.push(item.toUpper())
  return results

// Fast: shared loop, no per-iteration arena
fn fast(items: str[]) shared: str[] =>
  var results: str[] = {}
  for item in items shared =>
    results.push(item.toUpper())
  return results
```

---

## Design Decisions

### No Null

Sindarin does not have null. Use empty values instead:

| Type | Empty Value |
|------|-------------|
| `int[]` | `{}` |
| `str` | `""` |
| `int` | `0` |
| `double` | `0.0` |
| `bool` | `false` |
| `char` | `'\0'` |

```sindarin
fn find_items(query: str): int[] =>
  if nothing_found =>
    return {}              // Empty array, not null
  return results

var items: int[] = find_items("test")
if items.length == 0 =>
  print("nothing found")
```

**Rationale**:
- No null pointer crashes
- No nullable type complexity
- Simple and predictable
- Empty values cover most use cases

If absence tracking is needed, use a separate boolean:
```sindarin
var loaded: bool = false
var cache: str = ""
```

---

## See Also

- [Arrays](arrays.md) - Array operations and memory behavior
- [Structs](structs.md) - Struct memory model and escape behavior
- [Interop](interop.md) - C interoperability and native memory
