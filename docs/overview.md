---
title: Overview
description: Sindarin language philosophy, syntax overview, and compilation pipeline
permalink: /language/overview/
---

Sindarin is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

<div class="pipeline">
  <div class="pipeline-step">.sn source</div>
  <span class="pipeline-arrow">&rarr;</span>
  <div class="pipeline-step">Sn Compiler</div>
  <span class="pipeline-arrow">&rarr;</span>
  <div class="pipeline-step">C code</div>
  <span class="pipeline-arrow">&rarr;</span>
  <div class="pipeline-step">GCC</div>
  <span class="pipeline-arrow">&rarr;</span>
  <div class="pipeline-step">executable</div>
</div>

## Philosophy

### Design Principles

1. **Explicit Over Implicit** - All types are explicitly annotated. No type inference means code is always clear about what types are being used.

2. **Safety First** - Panic on errors rather than returning null or error codes. This keeps code clean and avoids pervasive null checks.

3. **Simple Memory Model** - Explicit ownership with `as ref`/`as val` semantics. No manual malloc/free, no garbage collector pauses.

4. **Clean Syntax** - Arrow-based blocks (`=>`) provide consistent, readable structure. No curly braces for blocks.

5. **Batteries Included** - Built-in string methods, array operations, and file I/O. Common tasks don't require external libraries.

6. **C Interoperability** - Compiles to readable C code. Easy to integrate with existing C libraries and tools.

### Why Sindarin?

- **Learning**: Clear syntax and explicit types make it easy to understand what code does
- **Scripting**: Built-in file I/O and string processing for automation tasks
- **Performance**: Compiles to native code via C with no runtime overhead
- **Simplicity**: Small language with consistent rules, easy to master

## Syntax Overview

### Arrow Blocks

Sindarin uses `=>` to introduce code blocks instead of curly braces:

```sindarin
fn greet(name: str): void =>
  print($"Hello, {name}!\n")

if condition =>
  doSomething()
else =>
  doOtherThing()

while running =>
  processNext()
```

### Variables

Variables are declared with `var` and require type annotations:

```sindarin
var name: str = "Sindarin"
var count: int = 42
var pi: double = 3.14159
var active: bool = true
var letter: char = 'A'
var b: byte = 255
var p: str = nil
```

| Type   | Description |
|--------|-------------|
| `int`  | 64-bit signed integer |
| `long` | 64-bit signed integer (alias) |
| `double` | 64-bit floating point |
| `float` | 32-bit floating point (IEEE 754 single precision) |
| `bool` | `true` or `false` |
| `str`  | String (pointer type, can be `nil`) |
| `char` | Single character, written with single quotes: `'A'` |
| `byte` | 8-bit unsigned integer (0–255) |
| `void` | No value (used as function return type) |

`nil` can be assigned to pointer types such as `str` and struct references.

### Functions

Functions use the `fn` keyword with explicit parameter and return types:

```sindarin
fn add(a: int, b: int): int =>
  return a + b

fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)
```

#### Expression-bodied Functions

For simple functions that return a single expression, use the expression-bodied syntax:

```sindarin
fn add(a: int, b: int): int => a + b
fn square(x: int): int => x * x
fn greet(name: str): str => $"Hello, {name}!"
fn isEven(n: int): bool => n % 2 == 0
```

The expression after `=>` is implicitly returned.

Expression-bodied syntax works with all function types including `native` functions:

```sindarin
native fn double_it(x: int): int => x * 2
```

#### Variadic Functions

Native functions can accept a variable number of arguments using `...` after the fixed parameters:

```sindarin
native fn my_variadic_fn(fmt: str, ...): int
```

### String Interpolation

Embed expressions in strings with `$"..."` syntax:

```sindarin
var name: str = "World"
var count: int = 42
print($"Hello, {name}! Count is {count}.\n")
```

### Arrays

Arrays use curly braces for literals and have built-in methods:

```sindarin
var numbers: int[] = {1, 2, 3, 4, 5}
numbers.push(6)
var first: int = numbers[0]
var last: int = numbers[-1]
var slice: int[] = numbers[1..4]
```

### Structs

Structs group related data with named fields:

```sindarin
struct Point =>
    x: double
    y: double

struct Config =>
    timeout: int = 30
    enabled: bool = true

var p: Point = Point { x: 10.0, y: 20.0 }
var cfg: Config = Config { timeout: 60 }  // enabled uses default
```

### Control Flow

```sindarin
// If-else
if condition =>
  doSomething()
else =>
  doOtherThing()

// While loop
while i < 10 =>
  process(i)
  i = i + 1

// For loop
for var i: int = 0; i < 10; i++ =>
  print($"{i}\n")

// For-each loop
for item in items =>
  process(item)

// Match expression
match status =>
    200 => print("OK\n")
    404, 405 => print("Not Found\n")
    else => print("Error\n")

// Match as expression
var msg: str = match code =>
    200 => "OK"
    404 => "Not Found"
    else => "Unknown"
```

Use `break` to exit a loop early and `continue` to skip to the next iteration:

```sindarin
// break — exit the loop
var x: int = 0
while true =>
    x++
    if x == 5 =>
        break

// continue — skip to next iteration
var sum: int = 0
for var i: int = 0; i < 10; i++ =>
    if i == 5 =>
        continue
    sum += i
```

### Boolean Operators

```sindarin
if hasTicket && hasID =>
  print("Entry allowed\n")

if isAdmin || isModerator =>
  print("Can moderate\n")

if !isBlocked =>
  print("Access granted\n")
```

### Operators

#### Arithmetic and Unary

Integer arithmetic is overflow-checked by default — an overflow causes a runtime panic rather than silent wraparound. The compiler emits safe wrapper calls instead of bare C arithmetic operators:

```sindarin
var a: int = 100
var b: int = a + 50   // checked add
```

Compiles to:

```c
long long __sn__a = 100LL;
long long __sn__b = sn_add_long(__sn__a, 50LL);  // panics on overflow
```

Without checking the same expression compiles to a plain C addition:

```c
long long __sn__b = __sn__a + 50LL;
```

Use the following flags to control overflow checking:

| Flag | Effect |
|------|--------|
| (default) | Checked arithmetic |
| `--unchecked` | Disable overflow checking (faster) |
| `-O2` | Full optimizations, implies `--unchecked` |
| `--checked` | Force checking even when `-O2` is active |

```sindarin
var x: int = 10
var y: int = -x    // unary negation
var b: bool = !true // logical NOT
```

#### Increment and Decrement

```sindarin
x++   // increment x by 1
x--   // decrement x by 1
```

#### Compound Assignment

```sindarin
x += 5   // x = x + 5
x -= 3   // x = x - 3
x *= 2   // x = x * 2
x /= 4   // x = x / 4
x %= 3   // x = x % 3
```

#### Bitwise Operators

```sindarin
var a: int = 0xFF
var b: int = 0x0F

var andResult: int = a & b    // AND  → 0x0F
var orResult: int = a | b     // OR   → 0xFF
var xorResult: int = a ^ b    // XOR  → 0xF0
var notResult: int = ~b       // NOT  → inverts all bits
var shl: int = b << 4         // Left shift  → 0xF0
var shr: int = a >> 4         // Right shift → 0x0F
```

| Operator | Description |
|----------|-------------|
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT (unary) |
| `<<` | Left shift |
| `>>` | Right shift |

Compound assignment forms are also available:

```sindarin
x &= 0xFF   // x = x & 0xFF
x |= 0x01   // x = x | 0x01
x ^= 0xFF   // x = x ^ 0xFF
x <<= 2     // x = x << 2
x >>= 4     // x = x >> 4
```

#### sizeof

```sindarin
var s: int = sizeof(Point)   // size of a struct type in bytes
```

## Module System

### Imports

Split code across files with imports:

```sindarin
// utils.sn
fn helper(): void =>
  print("I'm a helper!\n")

// main.sn
import "utils"

fn main(): void =>
  helper()
```

### Library Modules

A source file without a `main()` function is a library module — it exports only functions and types for use by other files:

```sindarin
// math.sn — no main(), just exports
fn add(a: int, b: int): int =>
    return a + b
```

Import it from any other file using `import "math"`.

### Global Variables

Variables declared at the top level (outside any function) are global and initialized at program start:

```sindarin
var counter: int = 0

fn main(): void =>
    counter = 42
```

A `static var` at module level persists its value across calls from other modules and is scoped to the declaring module:

```sindarin
static var staticCounter: int = 100
```

## Memory Management

Sindarin uses explicit ownership semantics. Use `as val` to request an independent value copy of a variable:

```sindarin
var original: int[] = {1, 2, 3}
var copy: int[] as val = original  // Independent copy
```

See [Memory](/language/memory/) for full documentation.

## Compilation

See [Building](/language/building/) for instructions on building the compiler from source.

```bash
# Compile to executable
bin/sn source.sn -o program
./program

# Emit C code only
bin/sn source.sn --emit-c -o output.c

# Debug build with symbols
bin/sn source.sn -g -o program
```

### C Backend Configuration

The C compiler backend can be configured via environment variables:

| Variable | Purpose | Default |
|----------|---------|---------|
| `SN_CC` | C compiler command | `gcc` |
| `SN_STD` | C standard | `c99` |
| `SN_DEBUG_CFLAGS` | Debug mode flags | `-no-pie -fsanitize=address -fno-omit-frame-pointer -g` |
| `SN_RELEASE_CFLAGS` | Release mode flags | `-O3 -flto` |
| `SN_CFLAGS` | Additional compiler flags | (empty) |
| `SN_LDFLAGS` | Additional linker flags | (empty) |
| `SN_LDLIBS` | Additional libraries | (empty) |

Examples:

```bash
# Use clang instead of gcc (requires runtime rebuilt without GCC LTO)
SN_CC=clang bin/sn source.sn -o program

# Add extra compiler flags
SN_CFLAGS="-march=native" bin/sn source.sn -o program

# Disable sanitizers in debug mode
SN_DEBUG_CFLAGS="-g" bin/sn source.sn -g -o program

# Link additional libraries
SN_LDLIBS="-lssl -lcrypto" bin/sn source.sn -o program
```
