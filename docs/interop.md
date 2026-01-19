# C Interoperability in Sindarin

Since Sindarin compiles to C, interoperability is natural but requires explicit declarations for external functions, headers, and linking.

---

## Overview

```sindarin
#pragma include <math.h>
#pragma link m

native fn sin(x: double): double
native fn cos(x: double): double

fn main(): int =>
    var angle: double = 3.14159 / 4.0
    print($"sin(45°) = {sin(angle)}\n")
    print($"cos(45°) = {cos(angle)}\n")
    return 0
```

---

## Native Function Declarations

External C functions are declared using the `native` keyword. These declarations tell the compiler the function exists externally and specify its signature.

### Syntax

```sindarin
native fn <name>(<params>): <return_type>
```

### Examples

```sindarin
// Math library
native fn sin(x: double): double
native fn cos(x: double): double
native fn sqrt(x: double): double

// Standard I/O
native fn puts(s: str): int

// Memory (if exposed)
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

---

## Pragma Directives

Pragma statements control compilation behavior for C interop. They use **WYSIWYG (What You See Is What You Get) syntax** — what you write is exactly what gets emitted.

### Header Inclusion

```sindarin
#pragma include <math.h>
#pragma include <stdlib.h>
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
#pragma link m
#pragma link pthread
#pragma link z
```

**Compiler behavior:** These directives instruct the compiler to pass `-lm`, `-lpthread`, `-lz`, etc. to the C compiler/linker.

### C Source File Compilation

The `#pragma source` directive compiles and links additional C source files with your Sindarin code:

```sindarin
#pragma source "helper.c"
#pragma source "wrapper.c"
```

**Use cases:**
- Custom C wrapper functions for type compatibility
- C helper code for variadic function wrappers
- Integration with C libraries that require additional source files

**Path resolution:** Paths are resolved relative to the Sindarin source file's directory.

**Example — Variadic Printf Wrapper:**

```sindarin
# test_variadic.sn
#pragma source "printf_helper.c"

# Custom printf wrapper defined in the helper C file
native fn test_printf(format: str, ...): int32

fn main(): int =>
    test_printf("Hello, %s! Value: %ld\n", "World", 42)
    return 0
```

```c
// printf_helper.c
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

// Wrapper with explicit int32_t return to match Sindarin's int32
int32_t test_printf(char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return (int32_t)result;
}
```

This is useful when C library function signatures don't match Sindarin's type system exactly (e.g., `printf` returns `int`, not `int32_t`).

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

### Built-in Object Types

| Sindarin Type | C Type | Notes |
|---------------|--------|-------|
| `Date` | `RtDate *` | Pointer to runtime date struct |
| `Time` | `RtTime *` | Pointer to runtime time struct |
| `UUID` | `RtUuid *` | Pointer to runtime UUID struct |
| `Random` | `RtRandom *` | Pointer to PRNG state |
| `TextFile` | `RtTextFile *` | Pointer to text file handle |
| `BinaryFile` | `RtBinaryFile *` | Pointer to binary file handle |
| `Process` | `RtProcess *` | Pointer to subprocess handle |
| `TcpListener` | `RtTcpListener *` | Pointer to TCP listener |
| `TcpStream` | `RtTcpStream *` | Pointer to TCP connection |
| `UdpSocket` | `RtUdpSocket *` | Pointer to UDP socket |

### Composite Types

| Sindarin Type | C Type | Notes |
|---------------|--------|-------|
| `T[]` | `RtArray_{T} *` | Pointer to typed array struct |
| `*T` | `T *` | Pointer to T |
| `fn(...): T` | `__Closure__ *` | Pointer to closure struct (non-native) |
| `fn(...): T` (native) | Custom typedef | C function pointer |
| `struct S` | `S` or `S *` | Value or pointer depending on context |
| `native struct S` | `S` | Matches C struct layout exactly |
| `opaque` | `void` or named | Opaque handle type |

### Type Mismatch Considerations

When declaring native functions, ensure Sindarin type mappings match the actual C function signatures. Common issues:

| Sindarin Declaration | Generated C | Potential Issue |
|---------------------|-------------|-----------------|
| `native fn foo(x: int): int` | `extern long long foo(long long)` | C function may use `long` or `int` |
| `native fn bar(): bool` | `extern bool bar()` | C function may return `int` (0/1) |

**Solutions:**
1. Use `#pragma source` to provide a C wrapper function with matching types
2. Use explicit sized types (`int32`, `uint32`) when interfacing with C APIs that use fixed-width types

---

## Pointer Types

Sindarin uses C-style pointer syntax for native interop, with safety restrictions.

### Syntax

```sindarin
var p: *int = ...
var pp: **char = ...    // pointer to pointer
var vp: *void = ...     // void pointer
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

// Native function - can work with pointers directly
native fn process_ptr(): int =>
    var p: *int = get_number()      // OK: store pointer
    var val: int = p as val         // Read value from pointer
    return val

// Regular function - must unwrap immediately with 'as val'
fn process(): int =>
    var val: int = get_number() as val    // OK: unwrapped to int
    var name: str = get_name() as val     // OK: unwrapped to str
    return val

// ERROR: cannot store pointer in non-native function
fn bad(): void =>
    var p: *int = get_number()            // Compile error: pointer type not allowed
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
    // C string - null terminated, length implicit
    var home: str = getenv("HOME") as val

    // Buffer - need explicit length via slice syntax
    var len: int = get_len()
    var data: byte[] = get_data()[0..len] as val
```

### Out Parameters with `as ref`

When C functions need to write results back, use `as ref` parameters. The compiler automatically handles the address-of operation at the call site.

#### Basic Usage

```sindarin
// C function: void get_dimensions(int* width, int* height)
native fn get_dimensions(width: int as ref, height: int as ref): void

fn example(): void =>
    var w: int = 0
    var h: int = 0
    get_dimensions(w, h)    // Compiler passes &w, &h automatically
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
// Native function with body that writes to out-parameters
native fn compute_stats(a: int, b: int, sum: int as ref, product: int as ref): void =>
    sum = a + b
    product = a * b

// Usage
var s: int = 0
var p: int = 0
compute_stats(7, 6, s, p)
print($"Sum: {s}, Product: {p}\n")  // Sum: 13, Product: 42
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
print($"17 / 5 = {q} remainder {r}\n")  // 17 / 5 = 3 remainder 2
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

// Regular function
fn example(): void =>
    var val: int = get_handle() as val   // OK: unwrapped

    use_handle(get_handle())             // OK: pointer passed directly

    var p: *int = get_handle()           // ERROR: can't store pointer
    use_handle(p)                        // ERROR: can't use stored pointer

    free(malloc(100))                    // OK: pointer passed directly
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
    // C string - unwrap to str
    var home: str = getenv("HOME") as val

    // Buffer with length
    var len: int = get_buffer_len()
    var data: byte[] = get_buffer()[0..len] as val
```

### String/Array Returns

For native functions declared with `str` or array return types, the default is to copy into the arena:

```sindarin
// C's malloc'd result is copied into Sindarin's arena, C memory is freed
native fn strdup(s: str): str

fn example(): void =>
    var dup: str = strdup("hello")  // Automatically copied to arena
```

### Parameter Semantics

| Annotation | Meaning | Use When | Code Generated |
|------------|---------|----------|----------------|
| (default) | Pass arena pointer | C needs to read (or safely mutate) your data | `func(data)` |
| `as val` | Copy, pass the copy | C mutates data and you want to preserve original | `func(copy_of_data)` |
| `as ref` | Pass pointer for out-param | C writes results back to caller's variable | `func(&variable)` |

```sindarin
// Default: pass by reference (C sees pointer to arena memory)
native fn strlen(s: str): int
native fn process(data: byte[]): void

// as val: copy first, then pass the copy
native fn sort_inplace(data: int[] as val): void

// as ref: compiler generates &variable, C writes back through pointer
native fn get_size(width: int as ref, height: int as ref): void
```

#### Key Difference: `as ref` vs Default

For arrays and strings, the **default** already passes a pointer (to arena memory). Use `as ref` specifically for **primitives** when C needs to write back:

```sindarin
// Default for arrays - already passes pointer, C can modify contents
native fn fill_buffer(buf: byte[]): void

// as ref for primitives - enables write-back to caller's variable
native fn get_count(count: int as ref): void
```

### Expression `as ref` for Pointers

Inside native function bodies, `as ref` can be used as an expression operator (counterpart to `as val`) to get a pointer from a value or array:

```sindarin
native fn call_c_api(data: byte[]): void =>
    // Get pointer from array - equivalent to C's array decay
    var ptr: *byte = data as ref

    // Get pointer from scalar value - equivalent to &value in C
    var count: int = 42
    var count_ptr: *int = count as ref

    // Common usage: pass array as pointer to C functions
    c_function(data as ref, data.length)
```

**Key differences from parameter `as ref`:**

| Context | Syntax | Purpose |
|---------|--------|---------|
| Parameter declaration | `fn(x: int as ref)` | C writes back through pointer |
| Expression in body | `arr as ref` | Get pointer from array/value |

This is particularly useful for calling C APIs that expect raw pointers:

```sindarin
#pragma link z

native fn compress(dest: *byte, destLen: uint as ref, source: *byte, sourceLen: uint): int

native fn compress_data(source: byte[]): byte[] =>
    var dest: byte[1024]
    var destLen: uint = 1024

    // Use 'as ref' to get pointers from arrays
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
#pragma include <stdio.h>

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
// Native callback type - C-compatible function pointer
type EventCallback = native fn(event: int, userdata: *void): void
type Comparator = native fn(a: *void, b: *void): int
type SignalHandler = native fn(signal: int): void
```

### Declaring C Functions That Accept Callbacks

```sindarin
#pragma include <stdlib.h>
#pragma include <signal.h>

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

    signal(2, handler)  // SIGINT
```

### Restrictions: No Closures

Native callbacks **cannot capture variables** from their enclosing scope. C function pointers have no mechanism for closures.

```sindarin
native fn setup(): void =>
    var counter: int = 0

    // ERROR: Native lambda cannot capture 'counter'
    var handler: Callback = fn(event: int, data: *void): void =>
        counter = counter + 1  // Compile error!
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
#pragma include <stdlib.h>

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

    qsort(arr as ref, arr.length, 8, cmp)  // 8 = sizeof(int64_t)

fn main(): void =>
    var numbers: int[] = {5, 2, 8, 1, 9}
    sort_integers(numbers)
    print($"Sorted: {numbers}\n")  // {1, 2, 5, 8, 9}
```

---

## Native Functions with Bodies

Native functions can have Sindarin implementations, allowing them to work with pointers while providing a safe wrapper:

```sindarin
type FILE = opaque

native fn fopen(path: str, mode: str): *FILE
native fn fclose(f: *FILE): int
native fn fread(buf: byte[], size: int, count: int, f: *FILE): int

// Wrapper handles the pointer lifecycle
native fn read_file(path: str): str =>
    var f: *FILE = fopen(path, "rb")
    if f == nil =>
        panic($"Cannot open: {path}")
    var buf: byte[4096] = {}
    var n: int = fread(buf, 1, 4096, f)
    fclose(f)
    return buf[0..n].toString()

// Regular function uses the safe wrapper
fn process(path: str): void =>
    var data: str = read_file(path)
    print(data)
```

### Expression-bodied Native Functions

For simple native functions, use the expression-bodied syntax where the expression follows `=>` on the same line:

```sindarin
// Simple arithmetic
native fn double_it(x: int): int => x * 2
native fn negate(x: double): double => -x

// Pointer operations with expression body
native fn is_null(ptr: *void): bool => ptr == nil

// Wrapping a C function call
native fn abs_val(x: int): int => (x < 0) ? -x : x
```

This is equivalent to:

```sindarin
native fn double_it(x: int): int =>
    return x * 2
```

Expression-bodied syntax is particularly useful for thin wrappers around C library functions:

```sindarin
#pragma include <math.h>
#pragma link m

native fn sin(x: double): double     // External C function
native fn cos(x: double): double     // External C function

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
    var ptr_size: int = sizeof(*int)     // 8
    var void_ptr: int = sizeof(*void)    // 8
    var struct_ptr: int = sizeof(*Point) // 8
```

### Arrays vs Element Size

For arrays, use `sizeof` on the element type, not the array:

```sindarin
native fn process_array(arr: int[]): void =>
    // sizeof(arr) returns 8 (pointer size)
    // sizeof(int) returns 8 (element size)
    var total_bytes: int = arr.length * sizeof(int)
```

The `sizeof` operator returns the size in bytes of any type or variable.

---

## Complete Example: Using the Math Library

```sindarin
#pragma include <math.h>
#pragma link m

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

## See Also

- [Structs](structs.md) - Struct declarations and native struct interop
- [Memory](memory.md) - Arena memory management
- [Arrays](arrays.md) - Array types and byte arrays
- [Lambdas](lambdas.md) - Lambda expressions (regular vs native)
- [SDK I/O documentation](sdk/io/readme.md) - File I/O operations
