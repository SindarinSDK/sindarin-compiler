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

3. **Simple Memory Model** - Arena-based memory management with clear ownership semantics. No manual malloc/free, no garbage collector pauses.

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
var letter: char = 'S'
```

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

### Boolean Operators

```sindarin
if hasTicket && hasID =>
  print("Entry allowed\n")

if isAdmin || isModerator =>
  print("Can moderate\n")

if !isBlocked =>
  print("Access granted\n")
```

## Module System

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

## Memory Management

Sindarin uses arena-based memory with optional control:

```sindarin
// Shared function - uses caller's arena (no promotion overhead)
shared fn helper(a: int, b: int): int =>
  return a + b

// Private function - isolated arena, only primitives can escape
private fn count_items(path: str): int =>
  var contents: str = read_file(path)
  return contents.split("\n").length  // Only int escapes

// Value copy semantics
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
