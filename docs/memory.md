---
title: "Memory"
description: "Two-tier ownership system: as ref and as val"
permalink: /language/memory/
---

## Overview

Sindarin uses a two-tier ownership system controlled by `as ref` and `as val` annotations on struct declarations and call sites. The compiler generates all retain/release or copy/cleanup logic automatically based on these annotations. Primitives (`int`, `double`, `bool`, `char`) are plain stack values with no ownership machinery. Strings have their own deterministic ownership rules.

---

## `as ref` Structs

A struct declared with `as ref` is heap-allocated and reference-counted. The compiler generates `create`, `retain`, `release`, and `copy` functions, plus a `sn_auto_<Type>` cleanup macro that calls `release` when the variable goes out of scope.

```sindarin
struct Node as ref =>
    value: int
    label: str
```

- Allocation: `calloc` with `__rc__` initialized to 1
- Assignment of a struct field pointing to another `as ref` struct calls `retain` on the nested struct
- On reassignment to a variable: old pointer is released, new pointer takes its place
- String fields inside `as ref` structs: `strdup` on write, `free` on release
- Cleanup on scope exit is automatic via the `sn_auto_Node` attribute

When a nested `as ref` struct is stored as a field, the release function recursively releases it:

```sindarin
struct Address as ref =>
    city: str

struct Person as ref =>
    name: str
    addr: Address

fn main(): void =>
    var a: Address = Address { city: "NYC" }
    var p: Person = Person { name: "Alice", addr: a }
    assert(p.name.length == 5, "name should be Alice")
```

Assigning `addr: a` in the struct literal calls `retain` on `a`. When `p` is released, `release` is called on `p->addr`.

---

## `as val` Structs

A struct declared with `as val` uses value semantics. The compiler generates `copy` and `cleanup` functions. The struct lives on the stack; cleanup frees any heap-owned fields (strings) when the variable goes out of scope.

```sindarin
struct Person as val =>
    name: str
    age: int

fn main(): void =>
    var a: Person = Person { name: "Alice", age: 30 }
    var b: Person = a
    b.age = 99
    assert(a.age == 30, "original age should be unchanged")
    assert(b.age == 99, "copy should have new age")
```

Assigning `b = a` calls `__sn__Person_copy`, which performs a deep copy (including `strdup` for string fields). Each copy is independent. The `sn_auto_Person` cleanup macro calls `__sn__Person_cleanup` at scope exit, which frees string fields.

A struct declared without either keyword defaults to `as val` behavior.

---

## String Semantics

Every string assignment (`strdup`) creates an owned copy of the string data. The caller is responsible for freeing it. The `sn_auto_str` cleanup attribute handles this automatically for local variables.

```sindarin
fn main(): void =>
    var a: str = "hello"
    var b: str = a
    var c: str = $"value: {a}"
    assert(a.length == 5, "a should be 5 chars")
```

- `var a: str = "hello"` — calls `strdup("hello")`
- `var b: str = a` — calls `strdup(a)`, b is an independent copy
- String interpolation `$"..."` produces a `strdup` of the formatted result
- Each variable is independently owned and freed on scope exit

**Reassignment** frees the old string before storing the new one:

```sindarin
fn main(): void =>
    var s: str = "hello"
    s = "world"
    assert(s.length == 5, "s should be 5 chars after reassign")
```

**String member assignment** on a struct frees the old field value then stores the new `strdup`:

```sindarin
struct Person as ref =>
    name: str
    age: int

fn main(): void =>
    var p: Person = Person { name: "Alice", age: 30 }
    p.name = "Bob"
    assert(p.name.length == 3, "name should be Bob")
```

**String parameters are borrowed** — the callee receives the raw pointer without copying or taking ownership:

```sindarin
fn print_name(name: str): void =>
    assert(name.length > 0, "name should not be empty")

fn main(): void =>
    var s: str = "Alice"
    print_name(s)
```

`print_name` receives `s` as a plain `char *`. No copy is made at the call site. The caller retains ownership.

**String arrays** manage element lifecycle automatically. Each element is `strdup`-ed when pushed and freed when the array is cleaned up:

```sindarin
fn main(): void =>
    var names: str[] = {"Alice", "Bob"}
    assert(names.length == 2, "should have 2 names")
```

---

## Variable Semantics

### Default — compiler follows the type declaration

```sindarin
var n: Node = Node { value: 42 }
```

If `Node` is declared `as ref`, the variable holds a pointer with `sn_auto_Node` cleanup. If `Node` is declared `as val`, the variable holds an inline struct value with `sn_auto_Node` cleanup.

### `as ref` on a variable declaration

Forces heap allocation for a primitive — the variable is stored on the heap and accessed via pointer. Useful when you need stable address semantics or intend to pass the variable's address to functions:

```sindarin
fn main(): void =>
    var x: int as ref = 42
    var y: double as ref = 3.14
    var flag: bool as ref = true
    x = 100
    y = 2.71
    flag = false
```

### `as val` on a variable declaration

Forces a copy at the point of initialization, even when assigning from an existing variable:

```sindarin
struct Point =>
    x: double
    y: double

fn main(): int =>
    var p: Point = Point { x: 1.0, y: 2.0 }
    var q: Point as val = p
    return 0
```

---

## Parameter Passing

### Default — borrowed, no copy

The default calling convention passes struct pointers and strings directly. The callee borrows the value; the caller retains ownership. No copy is made.

```sindarin
fn print_name(name: str): void =>
    assert(name.length > 0, "name should not be empty")
```

### `as val` parameter — copy made at call site

Annotating a parameter with `as val` tells the compiler to call `__sn__<Type>_copy` at the call site. The callee owns the copy.

```sindarin
struct Node as ref =>
    value: int

fn consume(n: Node as val): int =>
    return n.value

fn main(): void =>
    var n: Node = Node { value: 42 }
    var v: int = consume(n)
    assert(v == 42, "should get value from copy")
```

The generated call is `__sn__consume(__sn__Node_copy(__sn__n))` — the copy is created at the call site, and the callee is responsible for releasing it.

### `as ref` parameter — pointer passed directly

Annotating a parameter with `as ref` passes the address of the variable. The callee can mutate the caller's value.

```sindarin
struct Point as val =>
    x: int
    y: int

fn mutate(p: Point as ref): void =>
    p.x = 99

fn main(): void =>
    var p: Point = Point { x: 1, y: 2 }
    mutate(p)
    assert(p.x == 99, "should be mutated via ref")
```

For primitives, `as ref` on a parameter passes `&variable` and the function signature receives a pointer:

```sindarin
fn increment(x: int as ref): void =>
    x = x + 1
```

---

## Return Ownership

When a function returns a locally-owned value, the compiler nullifies the local variable before returning. This prevents the `sn_auto_*` cleanup attribute from freeing the value when the local goes out of scope — ownership transfers to the caller.

```sindarin
fn make_greeting(name: str): str =>
    var result: str = $"Hello {name}"
    return result
```

The generated return pattern is:

```c
char * __ret__ = __sn__result;
__sn__result = NULL;
return __ret__;
```

Setting the local to `NULL` before return means the `sn_auto_str` cleanup attribute sees `NULL` and does nothing. The returned pointer is owned by the caller.

The same pattern applies to `as ref` structs:

```sindarin
struct Box as ref =>
    value: int

fn make_box(v: int): Box =>
    var b: Box = Box { value: v }
    return b
```

Generated:

```c
__sn__Box * __ret__ = __sn__b;
__sn__b = NULL;
return __ret__;
```

---

## `copyOf(x)`

`copyOf` performs an explicit deep copy by calling `__sn__<Type>_copy`. Use it when you need a fully independent copy of an existing value.

```sindarin
struct Node as ref =>
    value: int

fn main(): void =>
    var a: Node = Node { value: 10 }
    var b: Node = copyOf(a)
    b.value = 20
    assert(a.value == 10, "original should be unchanged")
    assert(b.value == 20, "copy should be 20")
```

`copyOf` works on both `as ref` structs (returns a new heap-allocated copy with `__rc__ = 1`) and `as val` structs (returns a deep-copied stack value).

---

## `addressOf(x)` and `valueOf(x)`

These are pointer intrinsics for native C interop. They map directly to C address-of (`&`) and dereference (`*`) operators. They are only meaningful in `native` function bodies where you are working with raw pointers.

```sindarin
@alias "test_addr"
native fn test_addr(x: int): *int =>
    return addressOf(x)
```

```sindarin
@alias "test_val"
native fn test_val(p: *int): int =>
    return valueOf(p)
```

Do not use `addressOf` on locals in regular Sindarin functions — locals may be stack-allocated and the pointer becomes invalid after the function returns.

---

## Global Variables

Global variables are declared at the top level. Primitives are initialized at the declaration site (as a C global initializer). Expressions that require computation (including string literals that need `strdup`) are initialized inside `main()` before any user code runs.

```sindarin
var counter: int = 0

fn main(): void =>
    counter = 42
    assert(counter == 42, "expected counter to be 42")
```

Primitive globals with a literal value are emitted as C global initializers directly.

Globals that depend on other globals or runtime expressions are zero-initialized at the C level and assigned at the start of `main()`:

```sindarin
var x: int = 10
var y: int = x + 5

fn main(): void =>
    assert(y == 15, "expected y to be 15")
```

String globals are initialized with `strdup` at the start of `main()` and freed explicitly at program end:

```sindarin
var greeting: str = "hello"

fn main(): void =>
    assert(greeting.length == 5, "greeting should be 5 chars")
```

String globals do not use `sn_auto_str` — they are freed with an explicit `free` call at the end of `main()`.

---

## `using` Statement

The `using` statement provides RAII-style resource management. The resource is created, its body executes, then `dispose()` is called automatically when the block exits — before the struct's normal cleanup macro runs.

```sindarin
var disposed: bool = false

struct Resource =>
    name: str

    fn dispose(): void =>
        disposed = true

fn main(): void =>
    using r = Resource{ name: "test" } =>
        assert(r.name == "test", "expected resource name to be test")
    assert(disposed, "expected dispose to be called after using block")
```

The `using` block wraps the body in a C block scope. At the end of the inner block, `dispose()` is called explicitly. The struct's `sn_auto_Resource` cleanup then fires at the end of the outer block. This ensures the user's cleanup logic runs before memory is freed.

---

## Escape Analysis

The compiler performs escape analysis to determine whether a locally-created value outlives its scope. When it detects that data escapes (for example, a local string that is returned), it ensures the data is heap-allocated and ownership is transferred correctly via the return nullification pattern described above.

The key rule: any local with a `sn_auto_*` attribute will be cleaned up when its C scope ends. The return pattern (`local = NULL; return ptr;`) is how the compiler prevents double-free when a value escapes via return.

---

## Scope Cleanup Lifecycle

All owned locals — strings, `as ref` structs, and `as val` structs — carry a `sn_auto_*` cleanup attribute. C's `__attribute__((cleanup(...)))` guarantees cleanup runs in reverse declaration order when the scope exits, whether by falling off the end or via `return`.

The lifecycle for a typical owned local:

1. Declared with `sn_auto_str` / `sn_auto_<Type>` attribute
2. Used within scope
3. On `return`: local is nullified (`= NULL`), ownership transferred to caller
4. On scope exit without return: cleanup attribute fires, frees the value

For `as ref` structs, cleanup calls `release`, which decrements the refcount and frees when it reaches zero (including freeing any string fields or nested `as ref` fields). For `as val` structs, cleanup calls `cleanup`, which frees string fields. For strings, cleanup calls `free`.

Reassignment within a scope:

- **String**: new `strdup` is stored in a temp, old pointer is freed, temp is stored
- **`as ref` struct**: new pointer is stored, old pointer is released via `release`
- **`as ref` struct field pointing to `as ref`**: old field value is released, new value is retained and stored

---

## Summary

| Type | Allocation | Assignment | Cleanup |
|------|-----------|-----------|---------|
| `int`, `double`, `bool`, `char` | Stack | Value copy | None |
| `str` | Heap (`strdup`) | `strdup` new, `free` old | `free` via `sn_auto_str` |
| `struct as ref` | Heap (`calloc`, `__rc__=1`) | `retain` new, `release` old | `release` via `sn_auto_<Type>` |
| `struct as val` | Stack (inline) | Deep `copy` | `cleanup` via `sn_auto_<Type>` |
| `str[]` and typed arrays | Heap | Element `strdup`/`copy` on push | Array cleanup frees all elements |
| Global `str` | Heap (in `main`) | `strdup` new, `free` old | Explicit `free` at end of `main` |
| Global primitive | C global | Direct assign | None |
