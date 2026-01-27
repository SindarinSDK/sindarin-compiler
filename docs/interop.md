---
title: "C Interop"
description: "Calling C code and native function bindings"
permalink: /language/interop/
---

Since Sindarin compiles to C, interoperability is natural but requires explicit declarations for external functions, headers, and linking.

---

## Overview

```sindarin
@include <math.h>
@link m

native fn sin(x: double): double
native fn cos(x: double): double

fn main(): int =>
    var angle: double = 3.14159 / 4.0
    print($"sin(45°) = {sin(angle)}\n")
    print($"cos(45°) = {cos(angle)}\n")
    return 0
```

---

## Importing SDK Modules

Use `import` to include SDK modules or other Sindarin files:

```sindarin
import "sdk/time/date"
import "sdk/net/tcp"
import "sdk/encoding/json"

fn main(): void =>
    var today: Date = Date.today()
    print($"Today: {today.toIso()}\n")
```

Import paths are resolved relative to the compiler's SDK directory or the current file's directory.

---

## Native Function Declarations

External C functions are declared using the `native` keyword. These declarations tell the compiler the function exists externally and specify its signature.

### Syntax

```sindarin
native fn <name>(<params>): <return_type>
```

### Examples

```sindarin
# Math library
native fn sin(x: double): double
native fn cos(x: double): double
native fn sqrt(x: double): double

# Standard I/O
native fn puts(s: str): int

# Memory (if exposed)
native fn malloc(size: int): *void
native fn free(ptr: *void): void
```

### Code Generation

Native declarations generate C function prototypes:

```c
// Sindarin: native fn sin(x: double): double
// Generated: (prototype only, no implementation)
extern double sin(double x);
```

### The `@alias` Annotation

Use `@alias` to specify a different C function name than the Sindarin name:

```sindarin
@alias "compress2"
native fn compressLevel(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint, level: int): int

@alias "uncompress"
native fn decompress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int
```

This generates calls to the C function name specified in `@alias` while using the Sindarin name in your code.

### Implicit Arena Parameter

Native functions that allocate memory can receive the arena implicitly by declaring `arena` as the first parameter (without a type):

```sindarin
# Arena is automatically passed by the compiler
native fn sn_date_today(arena): Date
native fn sn_string_concat(arena, a: str, b: str): str

# Called without explicitly passing arena
fn example(): void =>
    var today: Date = sn_date_today()  # Arena passed automatically
```

This is primarily used in SDK implementations where C code needs to allocate into the Sindarin arena.

---

## Compiler Directives

Directives control compilation behavior for C interop. Sindarin supports two syntaxes: **annotation syntax** (`@directive`) and **pragma syntax** (`#pragma directive`). Both are equivalent; the annotation syntax is preferred for new code.

### Header Inclusion

```sindarin
# Annotation syntax (preferred)
@include <math.h>
@include <stdlib.h>
@include "mylib.h"

# Pragma syntax (also supported)
#pragma include <math.h>
#pragma include "mylib.h"
```

**Generated C:**
```c
#include <math.h>
#include <stdlib.h>
#include "mylib.h"
```

System headers use angle brackets (`<header.h>`), local headers use quotes (`"header.h"`).

### Library Linking

```sindarin
# Annotation syntax (preferred)
@link m
@link pthread
@link z

# Pragma syntax (also supported)
#pragma link m
#pragma link pthread
```

**Compiler behavior:** These directives instruct the compiler to pass `-lm`, `-lpthread`, `-lz`, etc. to the C compiler/linker.

### C Source File Compilation

The `@source` directive compiles and links additional C source files with your Sindarin code:

```sindarin
# Annotation syntax (preferred)
@source "helper.sn.c"
@source "wrapper.c"

# Pragma syntax (also supported)
#pragma source "helper.c"
```

**Use cases:**
- Custom C wrapper functions for type compatibility
- C helper code for variadic function wrappers
- Integration with C libraries that require additional source files

**Path resolution:** Paths are resolved relative to the Sindarin source file's directory.

**Example — SDK Module Pattern:**

```sindarin
# sdk/time/date.sn
@source "date.sn.c"

@alias "RtDate"
native struct Date as ref =>
    @alias "days"
    _days: int32

    static fn today(): Date =>
        return sn_date_today()

native fn sn_date_today(arena): Date
```

```c
// date.sn.c
#include "runtime/runtime_arena.h"

typedef struct RtDate {
    int32_t days;
} RtDate;

RtDate *sn_date_today(RtArena *arena) {
    // Implementation...
}
```

---

## Type Mapping

Sindarin types naturally map to C types.

### Primitive Types

| Sindarin Type | C Type | Size | Notes |
|---------------|--------|------|-------|
| `int` | `long long` | 64-bit | Signed integer |
| `long` | `long long` | 64-bit | Same as `int` |
| `int32` | `int32_t` | 32-bit | Explicit 32-bit signed |
| `uint` | `uint64_t` | 64-bit | Unsigned integer |
| `uint32` | `uint32_t` | 32-bit | Explicit 32-bit unsigned |
| `double` | `double` | 64-bit | IEEE 754 double precision |
| `float` | `float` | 32-bit | IEEE 754 single precision |
| `char` | `char` | 8-bit | Single character |
| `byte` | `unsigned char` | 8-bit | Unsigned byte |
| `bool` | `bool` | 1-bit | C99 `_Bool` |
| `void` | `void` | - | No value |
| `nil` | `NULL` | pointer | Null pointer constant |
| `str` | `char *` | pointer | Null-terminated UTF-8 string |
| `any` | `RtAny` | 16 bytes | Tagged union for dynamic typing |

### Composite Types

| Sindarin Type | C Type | Notes |
|---------------|--------|-------|
| `T[]` | `RtArray_{T} *` | Pointer to typed array struct |
| `*T` | `T *` | Pointer to T |
| `fn(...): T` | `__Closure__ *` | Pointer to closure struct (non-native) |
| `fn(...): T` (native) | Custom typedef | C function pointer |
| `struct S` | `S` or `S *` | Value or pointer depending on context |
| `native struct S` | `S` | Matches C struct layout exactly |
| `native struct S as ref` | `S *` | Pointer to C struct (handle type) |
| `opaque` | `void` or named | Opaque handle type |

### Type Mismatch Considerations

When declaring native functions, ensure Sindarin type mappings match the actual C function signatures. Common issues:

| Sindarin Declaration | Generated C | Potential Issue |
|---------------------|-------------|-----------------|
| `native fn foo(x: int): int` | `extern long long foo(long long)` | C function may use `long` or `int` |
| `native fn bar(): bool` | `extern bool bar()` | C function may return `int` (0/1) |

**Solutions:**
1. Use `@source` to provide a C wrapper function with matching types
2. Use explicit sized types (`int32`, `uint32`) when interfacing with C APIs that use fixed-width types

---

## Native Structs with Methods

Native structs can include methods, enabling an object-oriented interface to C code. This is the primary pattern used in the SDK.

### Basic Syntax

```sindarin
@alias "CStructName"
native struct MyType as ref =>
    @alias "c_field_name"
    _field: int32

    # Static factory method
    static fn create(): MyType =>
        return c_create_function()

    # Instance method (non-native, receives arena)
    fn format(): str =>
        return c_format_function(self)

    # Native instance method (direct C binding)
    @alias "c_get_value"
    native fn getValue(): int
```

### The `as ref` Modifier

When a native struct is declared with `as ref`, instances are passed by pointer:

```sindarin
# Without 'as ref' - passed by value (copied)
native struct Point =>
    x: double
    y: double

# With 'as ref' - passed by pointer (handle type)
@alias "RtDate"
native struct Date as ref =>
    _days: int32
```

Use `as ref` for:
- Handle types that should not be copied
- Structs that need to be modified by C code
- Large structs where copying is expensive

### The `@alias` Annotation

`@alias` maps Sindarin names to C names at three levels:

**1. Struct type alias:**
```sindarin
@alias "RtProcess"
native struct Process as ref =>
    ...
```
Maps `Process` to `RtProcess*` in generated C code.

**2. Field alias:**
```sindarin
@alias "exit_code"
_exitCode: int32
```
Maps `_exitCode` to the C field `exit_code`.

**3. Method alias:**
```sindarin
@alias "sn_process_get_exit_code"
native fn exitCode(): int
```
Calls the C function `sn_process_get_exit_code` when `exitCode()` is invoked.

### Method Types

Native structs support three types of methods:

**1. Static methods (`static fn`)** - Factory methods and utilities:
```sindarin
static fn today(): Date =>
    return sn_date_today()

static fn fromYmd(year: int, month: int, day: int): Date =>
    return sn_date_from_ymd(year, month, day)
```

**2. Instance methods (`fn`)** - Methods that need arena access:
```sindarin
fn format(pattern: str): str =>
    return sn_date_format(self, pattern)

fn addDays(days: int): Date =>
    return sn_date_add_days(self, days)
```

**3. Native instance methods (`native fn`)** - Direct C bindings:
```sindarin
@alias "sn_date_get_year"
native fn year(): int

@alias "sn_date_get_month"
native fn month(): int
```

### The `self` Keyword

Inside instance methods, `self` refers to the current instance:

```sindarin
native struct Process as ref =>
    _exitCode: int32

    fn success(): bool =>
        return self.exitCode() == 0

    fn failed(): bool =>
        return self.exitCode() != 0
```

For non-native methods, `self` is automatically passed to C functions that need the instance.

### Complete SDK Example

```sindarin
# sdk/os/process.sn
@source "process.sn.c"

@alias "RtProcess"
native struct Process as ref =>
    @alias "exit_code"
    _exitCode: int32

    @alias "stdout_h"
    _stdout: str

    @alias "stderr_h"
    _stderr: str

    # Static factory methods
    static fn run(cmd: str): Process =>
        return sn_process_run(cmd)

    static fn runArgs(cmd: str, args: str[]): Process =>
        return sn_process_run_args(cmd, args)

    # Native getter methods
    @alias "sn_process_get_exit_code"
    native fn exitCode(): int

    @alias "sn_process_get_stdout"
    native fn stdout(): str

    @alias "sn_process_get_stderr"
    native fn stderr(): str

    # Convenience methods using self
    fn success(): bool =>
        return self.exitCode() == 0

    fn failed(): bool =>
        return self.exitCode() != 0

# Runtime function declarations
native fn sn_process_run(arena, cmd: str): Process
native fn sn_process_run_args(arena, cmd: str, args: str[]): Process
```

**Usage:**
```sindarin
import "sdk/os/process"

fn main(): void =>
    var p: Process = Process.run("pwd")
    if p.success() =>
        print(p.stdout())
    else =>
        print($"Error: {p.stderr()}\n")
```

---

## Pointer Types

Sindarin uses C-style pointer syntax for native interop, with safety restrictions.

### Syntax

```sindarin
var p: *int = ...
var pp: **char = ...    # pointer to pointer
var vp: *void = ...     # void pointer
```

### Null Pointers

The `nil` constant represents a null pointer:

```sindarin
var p: *int = nil

if p != nil =>
    process(p)
```

`nil` is only valid for pointer types. Attempting to use `nil` with non-pointer types is a compile error.

### Safety Restrictions

1. **No pointer arithmetic** - `p + 1` is a compile error
2. **Pointer types only in `native` functions** - regular functions cannot have pointer parameters, return types, or variables
3. **Immediate unwrapping required** - regular functions must use `as val` when calling pointer-returning natives

### Unwrapping Pointers with `as val`

The `as val` operator copies the data a pointer points to into the arena:

```sindarin
native fn get_number(): *int
native fn get_name(): *char

# Native function - can work with pointers directly
native fn process_ptr(): int =>
    var p: *int = get_number()      # OK: store pointer
    var val: int = p as val         # Read value from pointer
    return val

# Regular function - must unwrap immediately with 'as val'
fn process(): int =>
    var val: int = get_number() as val    # OK: unwrapped to int
    var name: str = get_name() as val     # OK: unwrapped to str
    return val

# ERROR: cannot store pointer in non-native function
fn bad(): void =>
    var p: *int = get_number()            # Compile error: pointer type not allowed
```

### `as val` Semantics for Pointers

| Pointer Type | `as val` Result | Behavior |
|--------------|-----------------|----------|
| `*int`, `*double`, etc. | `int`, `double`, etc. | Copies single value |
| `*char` | `str` | Copies null-terminated string to arena |
| `*byte` with length | `byte[]` | Use slice syntax: `ptr[0..len] as val` |

```sindarin
native fn getenv(name: str): *char
native fn get_data(): *byte
native fn get_len(): int

fn example(): void =>
    # C string - null terminated, length implicit
    var home: str = getenv("HOME") as val

    # Buffer - need explicit length via slice syntax
    var len: int = get_len()
    var data: byte[] = get_data()[0..len] as val
```

### Out Parameters with `as ref`

When C functions need to write results back, use `as ref` parameters. The compiler automatically handles the address-of operation at the call site.

#### Basic Usage

```sindarin
# C function: void get_dimensions(int* width, int* height)
native fn get_dimensions(width: int as ref, height: int as ref): void

fn example(): void =>
    var w: int = 0
    var h: int = 0
    get_dimensions(w, h)    # Compiler passes &w, &h automatically
    print($"Size: {w}x{h}\n")
```

#### How It Works

When `as ref` is used in a parameter declaration, the code generator:
1. Creates a local variable to hold the value
2. Passes `&variable` (address-of) to the C function
3. After the call, the modified value is available in the original variable

This happens transparently - the caller just passes the variable normally.

#### Native Functions with Bodies

`as ref` also works in native function bodies, allowing you to write Sindarin wrappers that modify out-parameters:

```sindarin
# Native function with body that writes to out-parameters
native fn compute_stats(a: int, b: int, sum: int as ref, product: int as ref): void =>
    sum = a + b
    product = a * b

# Usage
var s: int = 0
var p: int = 0
compute_stats(7, 6, s, p)
print($"Sum: {s}, Product: {p}\n")  # Sum: 13, Product: 42
```

#### Supported Types

`as ref` works with all primitive types:

```sindarin
native fn modify_int(n: int as ref): void =>
    n = n * 2

native fn modify_double(d: double as ref): void =>
    d = d / 2.0

native fn modify_bool(b: bool as ref): void =>
    b = !b
```

#### Mixed Parameters

You can mix regular parameters with `as ref` out-parameters:

```sindarin
native fn divide_with_remainder(
    dividend: int,
    divisor: int,
    quotient: int as ref,
    remainder: int as ref
): void =>
    quotient = dividend / divisor
    remainder = dividend % divisor

var q: int = 0
var r: int = 0
divide_with_remainder(17, 5, q, r)
print($"17 / 5 = {q} remainder {r}\n")  # 17 / 5 = 3 remainder 2
```

See `tests/integration/test_as_ref_out_params.sn` and `tests/integration/test_interop_pointers.sn` for comprehensive examples.

### The `native` Function Boundary

Regular functions can call any native function, but:
- Must immediately unwrap pointer returns with `as val`
- Cannot declare variables of pointer types
- Cannot pass pointers as arguments (unless unwrapped from a call in the same expression)

```sindarin
native fn malloc(size: int): *void
native fn free(ptr: *void): void
native fn get_handle(): *int
native fn use_handle(h: *int): void

# Regular function
fn example(): void =>
    var val: int = get_handle() as val   # OK: unwrapped

    use_handle(get_handle())             # OK: pointer passed directly

    var p: *int = get_handle()           # ERROR: can't store pointer
    use_handle(p)                        # ERROR: can't use stored pointer

    free(malloc(100))                    # OK: pointer passed directly
```

---

## Memory Ownership

Memory ownership for native functions uses the existing `as val` and `as ref` semantics.

### Pointer Returns

For C functions returning pointers, use explicit pointer types and unwrap with `as val`:

```sindarin
native fn getenv(name: str): *char
native fn get_buffer(): *byte
native fn get_buffer_len(): int

fn example(): void =>
    # C string - unwrap to str
    var home: str = getenv("HOME") as val

    # Buffer with length
    var len: int = get_buffer_len()
    var data: byte[] = get_buffer()[0..len] as val
```

### String/Array Returns

For native functions declared with `str` or array return types, the default is to copy into the arena:

```sindarin
# C's malloc'd result is copied into Sindarin's arena, C memory is freed
native fn strdup(s: str): str

fn example(): void =>
    var dup: str = strdup("hello")  # Automatically copied to arena
```

### Parameter Semantics

| Annotation | Meaning | Use When | Code Generated |
|------------|---------|----------|----------------|
| (default) | Pass arena pointer | C needs to read (or safely mutate) your data | `func(data)` |
| `as val` | Copy, pass the copy | C mutates data and you want to preserve original | `func(copy_of_data)` |
| `as ref` | Pass pointer for out-param | C writes results back to caller's variable | `func(&variable)` |

```sindarin
# Default: pass by reference (C sees pointer to arena memory)
native fn strlen(s: str): int
native fn process(data: byte[]): void

# as val: copy first, then pass the copy
native fn sort_inplace(data: int[] as val): void

# as ref: compiler generates &variable, C writes back through pointer
native fn get_size(width: int as ref, height: int as ref): void
```

#### Key Difference: `as ref` vs Default

For arrays and strings, the **default** already passes a pointer (to arena memory). Use `as ref` specifically for **primitives** when C needs to write back:

```sindarin
# Default for arrays - already passes pointer, C can modify contents
native fn fill_buffer(buf: byte[]): void

# as ref for primitives - enables write-back to caller's variable
native fn get_count(count: int as ref): void
```

### Expression `as ref` for Pointers

Inside native function bodies, `as ref` can be used as an expression operator (counterpart to `as val`) to get a pointer from a value or array:

```sindarin
native fn call_c_api(data: byte[]): void =>
    # Get pointer from array - equivalent to C's array decay
    var ptr: *byte = data as ref

    # Get pointer from scalar value - equivalent to &value in C
    var count: int = 42
    var count_ptr: *int = count as ref

    # Common usage: pass array as pointer to C functions
    c_function(data as ref, data.length)
```

**Key differences from parameter `as ref`:**

| Context | Syntax | Purpose |
|---------|--------|---------|
| Parameter declaration | `fn(x: int as ref)` | C writes back through pointer |
| Expression in body | `arr as ref` | Get pointer from array/value |

This is particularly useful for calling C APIs that expect raw pointers:

```sindarin
@link z

native fn compress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int

native fn compress_data(source: byte[]): byte[] =>
    var dest: byte[1024]
    var destLen: uint = 1024

    # Use 'as ref' to get pointers from arrays
    compress(dest as ref, destLen, source as ref, source.length)

    return dest[0..destLen] as val
```

---

## Opaque Types

Opaque types are handles to C structures that cannot be inspected or allocated from Sindarin.

### Declaration

```sindarin
type FILE = opaque
type SQLite = opaque
type regex_t = opaque
```

### Usage

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
native fn fread(buf: byte[], size: int, count: int, f: *FILE): int

native fn read_file(path: str): byte[] =>
    var f: *FILE = fopen(path, "rb")
    if f == nil =>
        panic("Failed to open file")

    var buffer: byte[1024] = {}
    var n: int = fread(buffer, 1, 1024, f)
    fclose(f)

    return buffer[0..n]
```

### Opaque Type Rules

1. Can declare variables of pointer-to-opaque type (`*FILE`)
2. Can pass opaque pointers to native functions
3. Can compare to `nil`
4. **Cannot** dereference or inspect contents
5. **Cannot** allocate directly in Sindarin

---

## Variadic Functions

Sindarin supports calling C variadic functions with pass-through semantics.

### Syntax

```sindarin
native fn printf(format: str, ...): int
native fn sprintf(buf: byte[], format: str, ...): int
native fn fprintf(f: *FILE, format: str, ...): int
```

### Semantics

- Type checker allows any arguments after `...`
- Variadic arguments must be primitive types or `str`
- Arguments are passed directly to C's varargs mechanism
- No format string validation (matches C behavior)

### Example

```sindarin
@include <stdio.h>

native fn printf(format: str, ...): int

fn main(): int =>
    var name: str = "World"
    var count: int = 42
    var pi: double = 3.14159

    printf("Hello, %s!\n", name)
    printf("Count: %d, Pi: %.2f\n", count, pi)

    return 0
```

### Allowed Variadic Argument Types

| Type | C Format |
|------|----------|
| `int`, `long` | `%d`, `%ld`, `%lld` |
| `double` | `%f`, `%e`, `%g` |
| `str` | `%s` |
| `char` | `%c` |
| `bool` | `%d` (as 0/1) |
| `*T` (pointers) | `%p` |

Arrays cannot be passed as variadic arguments.

---

## Native Callbacks

Sindarin's lambda syntax extends to C-compatible function pointers using the `native` modifier.

### Defining Native Callback Types

```sindarin
# Native callback type - C-compatible function pointer
type EventCallback = native fn(event: int, userdata: *void): void
type Comparator = native fn(a: *void, b: *void): int
type SignalHandler = native fn(signal: int): void
```

### Declaring C Functions That Accept Callbacks

```sindarin
@include <stdlib.h>
@include <signal.h>

type SignalHandler = native fn(sig: int): void
type QsortComparator = native fn(a: *void, b: *void): int

native fn signal(sig: int, handler: SignalHandler): void
native fn qsort(base: *void, count: int, size: int, cmp: QsortComparator): void
```

### Creating Native Callbacks

Native callbacks are created using lambda syntax, but must be declared in `native` functions:

```sindarin
native fn setup_signal_handler(): void =>
    var handler: SignalHandler = fn(sig: int): void =>
        print($"Received signal: {sig}\n")

    signal(2, handler)  # SIGINT
```

### Restrictions: No Closures

Native callbacks **cannot capture variables** from their enclosing scope. C function pointers have no mechanism for closures.

```sindarin
native fn setup(): void =>
    var counter: int = 0

    # ERROR: Native lambda cannot capture 'counter'
    var handler: Callback = fn(event: int, data: *void): void =>
        counter = counter + 1  # Compile error!
        print($"Event: {event}\n")
```

Use the `void* userdata` pattern instead for state passing.

### Regular vs Native Lambdas

| Aspect | Regular `fn(...)` | `native fn(...)` |
|--------|-------------------|------------------|
| Captures variables | Yes (closures) | No - compile error |
| Pointer types in signature | No | Yes |
| Opaque types in signature | No | Yes |
| Declared in | Any function | `native fn` only |
| Passed to | Sindarin functions | C functions |

### Complete Example: qsort

```sindarin
@include <stdlib.h>

type Comparator = native fn(a: *void, b: *void): int

native fn qsort(base: *void, count: int, size: int, cmp: Comparator): void

native fn sort_integers(arr: int[]): void =>
    var cmp: Comparator = fn(a: *void, b: *void): int =>
        var x: int = a as val
        var y: int = b as val
        if x < y =>
            return -1
        if x > y =>
            return 1
        return 0

    qsort(arr as ref, arr.length, 8, cmp)  # 8 = sizeof(int64_t)

fn main(): void =>
    var numbers: int[] = {5, 2, 8, 1, 9}
    sort_integers(numbers)
    print($"Sorted: {numbers}\n")  # {1, 2, 5, 8, 9}
```

---

## Native Functions with Bodies

Native functions can have Sindarin implementations, allowing them to work with pointers while providing a safe wrapper:

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
native fn fread(buf: byte[], size: int, count: int, f: *FILE): int

# Wrapper handles the pointer lifecycle
native fn read_file(path: str): str =>
    var f: *FILE = fopen(path, "rb")
    if f == nil =>
        panic($"Cannot open: {path}")
    var buf: byte[4096] = {}
    var n: int = fread(buf, 1, 4096, f)
    fclose(f)
    return buf[0..n].toString()

# Regular function uses the safe wrapper
fn process(path: str): void =>
    var data: str = read_file(path)
    print(data)
```

### Expression-bodied Native Functions

For simple native functions, use the expression-bodied syntax where the expression follows `=>` on the same line:

```sindarin
# Simple arithmetic
native fn double_it(x: int): int => x * 2
native fn negate(x: double): double => -x

# Pointer operations with expression body
native fn is_null(ptr: *void): bool => ptr == nil

# Wrapping a C function call
native fn abs_val(x: int): int => (x < 0) ? -x : x
```

This is equivalent to:

```sindarin
native fn double_it(x: int): int =>
    return x * 2
```

Expression-bodied syntax is particularly useful for thin wrappers around C library functions:

```sindarin
@include <math.h>
@link m

native fn sin(x: double): double     # External C function
native fn cos(x: double): double     # External C function

native fn degrees_to_radians(deg: double): double => deg * 3.14159 / 180.0
native fn sin_deg(deg: double): double => sin(degrees_to_radians(deg))
native fn cos_deg(deg: double): double => cos(degrees_to_radians(deg))
```

---

## sizeof for C Interop

The `sizeof` operator is essential for C interoperability. It delegates directly to C's `sizeof` and works on types, variables, and pointer types.

### Basic Usage

```sindarin
native fn allocate_buffer(count: int): *byte =>
    return malloc(count * sizeof(byte)) as *byte

native fn allocate_ints(count: int): *int =>
    return malloc(count * sizeof(int)) as *int
```

### With Structs

```sindarin
struct Point =>
    x: double
    y: double

native fn allocate_points(n: int): *Point =>
    return malloc(n * sizeof(Point)) as *Point

native fn copy_point(dest: *Point, src: *Point): void =>
    memcpy(dest as *void, src as *void, sizeof(Point))
```

### With qsort and Similar C Functions

```sindarin
native fn compare_ints(a: *void, b: *void): int32 =>
    var ia: *int = a as *int
    var ib: *int = b as *int
    return (*ia - *ib) as int32

native fn sort_integers(arr: int[]): void =>
    qsort(arr as ref, arr.length, sizeof(int), compare_ints)
```

### Pointer Types

In native functions, `sizeof` works on pointer types:

```sindarin
native fn example(): void =>
    var ptr_size: int = sizeof(*int)     # 8
    var void_ptr: int = sizeof(*void)    # 8
    var struct_ptr: int = sizeof(*Point) # 8
```

### Arrays vs Element Size

For arrays, use `sizeof` on the element type, not the array:

```sindarin
native fn process_array(arr: int[]): void =>
    # sizeof(arr) returns 8 (pointer size)
    # sizeof(int) returns 8 (element size)
    var total_bytes: int = arr.length * sizeof(int)
```

The `sizeof` operator returns the size in bytes of any type or variable.

---

## Complete Example: Using the Math Library

```sindarin
@include <math.h>
@link m

native fn sin(x: double): double
native fn cos(x: double): double
native fn sqrt(x: double): double
native fn pow(base: double, exp: double): double

fn main(): int =>
    var angle: double = 3.14159 / 4.0
    var s: double = sin(angle)
    var c: double = cos(angle)

    print($"sin(45deg) = {s}\n")
    print($"cos(45deg) = {c}\n")
    print($"sqrt(2) = {sqrt(2.0)}\n")

    return 0
```

**Generated C (conceptual):**

```c
#include <math.h>
#include "runtime.h"

int main() {
    double angle = 3.14159 / 4.0;
    double s = sin(angle);
    double c = cos(angle);

    printf("sin(45deg) = %f\n", s);
    printf("cos(45deg) = %f\n", c);
    printf("sqrt(2) = %f\n", sqrt(2.0));

    return 0;
}
```

---

## Native Struct Instantiation

Native struct literals are **stack-allocated** (not arena-allocated) and generate C compound literals.

### Memory Allocation

```sindarin
native struct Point =>
    x: double
    y: double

native fn create_point(x: double, y: double): Point =>
    return Point { x: x, y: y }
```

**Generated C:**
```c
Point create_point(double x, double y) {
    return (Point){ .x = x, .y = y };  // Stack-allocated compound literal
}
```

**Implications:**
- Small structs are passed by value efficiently
- No arena cleanup required — automatic storage duration
- Returned values are copied to the caller's stack frame

### Context Restriction

Native struct literals can only be created inside native functions:

```sindarin
native struct SdkDate =>
    _days: int

# ERROR: Cannot create native struct in non-native function
fn convert(days: int): SdkDate =>
    return SdkDate { _days: days }

# OK: Native function can create native struct
native fn convert(days: int): SdkDate =>
    return SdkDate { _days: days }
```

This restriction ensures native structs maintain C-compatible memory layout and are only manipulated in contexts where the code generator produces direct C struct operations.

---

## Writing C Code for SDK Functions

This section covers how to write C implementations for Sindarin SDK functions, including memory management patterns and the arena system.

### Arena Memory Model Overview

Sindarin uses **arena-based memory management** with a handle table and background garbage collection:

```
┌─────────────────────────────────────────────────────────────────┐
│                        RtManagedArena                           │
├─────────────────────────────────────────────────────────────────┤
│  Handle Table (paged)           Backing Blocks                  │
│  ┌─────┬─────┬─────┐           ┌────────────────┐              │
│  │  1  │  2  │  3  │ ... ───►  │  Block 1 (64KB)│              │
│  │ ptr │ ptr │ ptr │           │  [data...]     │              │
│  │size │size │size │           ├────────────────┤              │
│  │lease│lease│lease│           │  Block 2 (64KB)│              │
│  └─────┴─────┴─────┘           │  [data...]     │              │
│                                 └────────────────┘              │
│  Background Threads:                                            │
│  - Cleaner: recycles dead handles                               │
│  - Compactor: defragments memory                                │
└─────────────────────────────────────────────────────────────────┘
```

**Key concepts:**

- **Handle (`RtHandle`)**: A 32-bit index into the handle table. Handles are stable; the underlying pointer may change during compaction.
- **Pin/Unpin**: To access data, you must **pin** the handle (get a raw pointer) and **unpin** when done. While pinned, the compactor cannot move the data.
- **Arena hierarchy**: Arenas form a tree. The root arena owns GC threads; child arenas are created for function scopes.

### Allocating Memory for Return Values

When a C function returns a value to Sindarin, it must allocate in the arena:

```c
#include "runtime/runtime_arena.h"
#include "runtime/arena/managed_arena.h"

// Struct definition (matches Sindarin native struct)
typedef struct RtDate {
    int32_t days;
} RtDate;

// Factory function - allocates and returns a handle
RtDate *sn_date_create(RtArena *arena, int32_t days)
{
    // Allocate in arena - memory is managed by GC
    RtDate *date = rt_arena_alloc(arena, sizeof(RtDate));
    date->days = days;
    return date;
}
```

**For simple structs returned by pointer**, use `rt_arena_alloc`:

```c
void *rt_arena_alloc(RtArena *arena, size_t size);
```

### Returning Strings

For strings, use `rt_arena_strdup` to copy into the arena:

```c
char *sn_date_format(RtArena *arena, RtDate *date, const char *pattern)
{
    // Create the formatted string (using temporary stack/heap memory)
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);

    // Copy to arena - this is what Sindarin receives
    return rt_arena_strdup(arena, buffer);
}
```

**Available string functions:**

```c
char *rt_arena_strdup(RtArena *arena, const char *str);
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n);
```

### Returning Arrays

Arrays use the **handle-based API** with `RtHandle`:

```c
#include "runtime/runtime_array_h.h"

RtHandle sn_random_bytes(RtManagedArena *arena, RtRandom *rng, long count)
{
    // Allocate temporary buffer on heap
    unsigned char *buf = malloc((size_t)count);
    if (!buf) {
        return rt_array_create_byte_h(arena, 0, NULL);
    }

    // Fill with random data
    for (long i = 0; i < count; i++) {
        buf[i] = sn_random_byte(rng);
    }

    // Create array in arena (copies data, takes ownership)
    RtHandle result = rt_array_create_byte_h(arena, count, buf);

    // Free temporary buffer
    free(buf);

    return result;
}
```

**Array creation functions:**

```c
RtHandle rt_array_create_byte_h(RtManagedArena *arena, size_t count, unsigned char *data);
RtHandle rt_array_create_long_h(RtManagedArena *arena, size_t count, long long *data);
RtHandle rt_array_create_double_h(RtManagedArena *arena, size_t count, double *data);
RtHandle rt_array_create_bool_h(RtManagedArena *arena, size_t count, int *data);
RtHandle rt_array_create_str_h(RtManagedArena *arena, size_t count, char **data);
```

### Pinning and Unpinning

When you need to access data from a handle (e.g., reading function parameters):

```c
void process_array(RtManagedArena *arena, RtHandle arr_handle)
{
    // Pin to get raw pointer - compactor won't move this while pinned
    long long *arr = rt_managed_pin_array(arena, arr_handle);

    // Use the array...
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        process(arr[i]);
    }

    // Unpin when done - compactor can now move this memory
    rt_managed_unpin(arena, arr_handle);
}
```

**Pin functions:**

```c
void *rt_managed_pin(RtManagedArena *arena, RtHandle h);
void *rt_managed_pin_array(RtManagedArena *arena, RtHandle h);  // Skips array metadata
char *rt_managed_pin_str(RtManagedArena *arena, RtHandle h);
void rt_managed_unpin(RtManagedArena *arena, RtHandle h);
```

### Permanent Pinning for OS Resources

Some structures contain OS resources (mutexes, file handles) that **cannot be moved**. Use permanent pinning:

```c
RtHandle sn_create_thread_safe_resource(RtManagedArena *arena)
{
    // Permanently pinned - compactor will NEVER move this
    RtHandle h = rt_managed_alloc_pinned(arena, RT_HANDLE_NULL, sizeof(MyResource));

    MyResource *res = rt_managed_pin(arena, h);
    pthread_mutex_init(&res->mutex, NULL);  // OS resource - must not move
    rt_managed_unpin(arena, h);

    return h;
}
```

### Cleanup Callbacks

Register cleanup functions for resources that need explicit teardown:

```c
// Cleanup function - called when arena is destroyed
static void file_cleanup(void *data)
{
    FILE *f = (FILE *)data;
    if (f) fclose(f);
}

RtHandle sn_file_open(RtManagedArena *arena, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return RT_HANDLE_NULL;

    // Register cleanup to close file when arena is destroyed
    rt_arena_on_cleanup(arena, f, file_cleanup, RT_CLEANUP_PRIORITY_MEDIUM);

    // ... store file handle in arena-allocated struct
}
```

**Cleanup priorities:**

```c
RT_CLEANUP_PRIORITY_HIGH    = 0    // Threads synced first
RT_CLEANUP_PRIORITY_MEDIUM  = 10   // Files closed after threads
RT_CLEANUP_PRIORITY_DEFAULT = 50   // Default for user resources
```

### Memory Promotion (Escaping Values)

When a value needs to outlive its creating scope (escape analysis):

```c
RtHandle sn_promote_to_parent(RtManagedArena *child, RtManagedArena *parent, RtHandle h)
{
    // Copy data from child arena to parent arena
    // Source handle is marked dead; returns new handle in parent
    return rt_managed_promote(parent, child, h);
}
```

---

## Malloc Hooks and Redirection

Sindarin provides **malloc hooks** for debugging and a **malloc redirect** system that captures C library allocations into the arena.

### Malloc Hooks (Debugging)

Build with `-DSN_MALLOC_HOOKS` to intercept all malloc/free calls:

```c
// All malloc/free calls are logged
[SN_ALLOC] malloc(1024) = 0x7f8a2c000b20  [sn_date_format+0x42]
[SN_ALLOC] free(0x7f8a2c000b20)  [sn_date_destroy+0x18]
```

**Platform implementations:**
- **Linux**: Uses `plthook` to modify PLT/GOT entries
- **macOS**: Uses Facebook's `fishhook` for Mach-O symbol rebinding
- **Windows**: Uses `MinHook` for inline function hooking

### Malloc Redirect (Arena Capture)

Build with `-DSN_MALLOC_REDIRECT` to redirect malloc calls to the arena:

```c
#include "runtime/runtime_malloc_redirect.h"

void call_c_library(RtArena *arena)
{
    // Push redirect context - all malloc() calls go to arena
    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    rt_malloc_redirect_push(arena, &config);

    // C library allocations now use the arena
    char *str = strdup("hello");  // Goes to arena, not heap!
    cJSON *json = cJSON_Parse(data);  // All internal allocations in arena

    // Pop context - subsequent malloc() uses real heap
    rt_malloc_redirect_pop();

    // Arena cleanup frees everything automatically
}
```

**Configuration options:**

```c
typedef struct {
    size_t max_arena_size;           // Max bytes (0 = unlimited)
    RtRedirectOverflowPolicy overflow_policy;  // What to do when full
    RtRedirectFreePolicy free_policy;          // How to handle free()
    bool track_allocations;          // Track each allocation
    bool zero_on_free;               // Zero memory on free
    bool thread_safe;                // Enable mutex protection
    // Callbacks...
} RtRedirectConfig;
```

**Free policies:**

| Policy | Behavior |
|--------|----------|
| `RT_REDIRECT_FREE_IGNORE` | Do nothing (arena frees all at once) |
| `RT_REDIRECT_FREE_TRACK` | Track for leak detection |
| `RT_REDIRECT_FREE_WARN` | Print warning to stderr |
| `RT_REDIRECT_FREE_ERROR` | Abort program |

**Overflow policies:**

| Policy | Behavior |
|--------|----------|
| `RT_REDIRECT_OVERFLOW_GROW` | Continue allocating (default) |
| `RT_REDIRECT_OVERFLOW_FALLBACK` | Fall back to real malloc |
| `RT_REDIRECT_OVERFLOW_FAIL` | Return NULL |
| `RT_REDIRECT_OVERFLOW_PANIC` | Abort with error message |

### Complete Example: Wrapping a C Library

```c
// json_wrapper.sn.c
#include "runtime/runtime_arena.h"
#include "runtime/runtime_malloc_redirect.h"
#include <cJSON.h>

char *sn_json_get_string(RtArena *arena, const char *json_str, const char *key)
{
    // Redirect cJSON's malloc to arena
    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    rt_malloc_redirect_push(arena, &config);

    cJSON *root = cJSON_Parse(json_str);
    char *result = NULL;

    if (root) {
        cJSON *item = cJSON_GetObjectItem(root, key);
        if (item && cJSON_IsString(item)) {
            // Copy to arena (strdup also redirected)
            result = rt_arena_strdup(arena, item->valuestring);
        }
        cJSON_Delete(root);  // free() redirected to arena
    }

    rt_malloc_redirect_pop();

    return result;
    // All cJSON memory freed when arena is destroyed
}
```

---

## Summary: Annotation Reference

| Annotation | Target | Purpose |
|------------|--------|---------|
| `@include <header.h>` | File | Include C header |
| `@include "header.h"` | File | Include local C header |
| `@link library` | File | Link with C library |
| `@source "file.c"` | File | Compile and link C source |
| `@alias "name"` | Struct | Map Sindarin struct to C struct name |
| `@alias "name"` | Field | Map Sindarin field to C field name |
| `@alias "name"` | Method | Map Sindarin method to C function |

---

## See Also

- [Structs](structs.md) - Struct declarations and native struct interop
- [Memory](memory.md) - Arena memory management
- [Arrays](arrays.md) - Array types and byte arrays
- [Lambdas](lambdas.md) - Lambda expressions (regular vs native)
- [SDK I/O documentation](sdk/io/readme.md) - File I/O operations
