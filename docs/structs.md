---
title: "Structs"
description: "Struct types, fields, and methods"
permalink: /language/structs/
---

Sindarin supports C-compatible structs for structured data and C library interoperability. Structs enable accessing fields of C library data structures (like zlib's `z_stream`), parsing binary file formats, and organizing related data.

## Declaration

Structs are declared using the `struct` keyword with arrow syntax:

```sindarin
struct Point =>
    x: double
    y: double

struct Rectangle =>
    origin: Point
    width: double
    height: double
```

### Default Values

Fields can have default values:

```sindarin
struct Config =>
    timeout: int = 30
    retries: int = 3
    verbose: bool = false

struct ServerConfig =>
    host: str = "localhost"
    port: int = 8080
    maxConnections: int = 100
```

### Native Structs

Structs containing pointer fields must be declared with `native struct`:

```sindarin
native struct Buffer =>
    data: *byte
    size: int
    capacity: int

native struct ZStream =>
    next_in: *byte
    avail_in: uint
    next_out: *byte
    avail_out: uint
```

Native structs can only be instantiated inside `native fn` functions.

## Instantiation

Create struct instances using the struct name followed by field initializers in braces:

```sindarin
// Full initialization (required for structs without defaults)
var p: Point = Point { x: 10.0, y: 20.0 }

// Empty initialization (only when ALL fields have defaults)
var cfg: Config = Config {}

// Partial initialization (unspecified fields use their defaults)
var cfg2: Config = Config { timeout: 60 }

// Multiple fields
var srv: ServerConfig = ServerConfig { port: 443, maxConnections: 1000 }
```

**Required fields**: Structs without default values must have all fields specified:
```sindarin
struct Point =>
    x: double   // No default - REQUIRED
    y: double   // No default - REQUIRED

var p: Point = Point { x: 1.0, y: 2.0 }  // OK: all fields provided
// var p: Point = Point {}              // ERROR: missing required fields
```

Struct literals can span multiple lines for better readability:

```sindarin
var config: Config = Config {
    timeout: 60,
    retries: 5,
    verbose: true
}

var srv: ServerConfig = ServerConfig {
    host: "api.example.com",
    port: 443,
    maxConnections: 1000
}
```

## Field Access

Access fields using dot notation:

```sindarin
// Reading fields
var x_val: double = p.x
print($"Point: ({p.x}, {p.y})\n")

// Writing fields
p.x = 30.0
p.y = 40.0

// Nested access
rect.origin.x = 5.0
```

## Value Semantics

Struct assignment copies the entire struct (value semantics):

```sindarin
var p1: Point = Point { x: 1.0, y: 2.0 }
var p2: Point = p1       // p2 is a COPY of p1
p2.x = 99.0              // p1.x is still 1.0
```

This matches C struct behavior and ensures predictable, aliasing-free code.

## Memory Model

### Stack vs Heap Allocation

Structs follow the same rules as fixed arrays:

| Struct Size | Location |
|-------------|----------|
| Small (<8KB) | Stack |
| Large (≥8KB) | Heap (heap-allocated) |
| Escaping scope | Copied to outer scope |

```sindarin
fn process(): void =>
    var p: Point = Point { x: 1.0, y: 2.0 }  // Stack allocated (small)
    // p freed automatically when function returns
```

### Escape Behavior

The escape behavior depends on the struct's memory model:

- **`as val` structs (default):** Assignment copies the entire struct. An inner-scope struct
  assigned to an outer variable is copied by value; the outer variable owns its own copy.

```sindarin
var outer: Point

if condition =>
    var inner: Point = Point { x: 1.0, y: 2.0 }  // Stack allocated
    outer = inner                                  // COPIED to outer variable

print($"x = {outer.x}\n")  // Safe - outer owns its copy
```

- **`as ref` structs:** The struct is heap-allocated and reference-counted. Assignment retains
  the object (increments the reference count). When a variable goes out of scope its reference
  is released, and the object is freed when the count reaches zero. There is no copy — both
  variables point to the same heap object.

```sindarin
struct Node as ref =>
    value: int

fn main(): void =>
    var n: Node = Node { value: 42 }
    var n2: Node = n   // n2 retains the same heap object (refcount = 2)
    // When both n and n2 go out of scope the object is freed
```

### Returning Structs

Structs can be returned from functions. The returned value is copied into the caller's variable:

```sindarin
fn make_point(x: double, y: double): Point =>
    var p: Point = Point { x: x, y: y }
    return p  // Copied into caller's variable

var result: Point = make_point(1.0, 2.0)
```

See [Memory](memory.md) for more details on memory management.

## Memory Semantics: `as ref` and `as val`

Any struct — native or regular — can declare its memory model explicitly with `as ref` or
`as val`. Without an explicit annotation, regular structs default to value semantics.

### `as ref` — reference-counted heap allocation

```sindarin
struct Node as ref =>
    value: int
```

An `as ref` struct is heap-allocated and reference-counted. Each variable holds a pointer to
the heap object. Assignment retains (increments the reference count); leaving scope releases
it. This is appropriate when a struct needs shared ownership or must outlive the scope it was
created in.

```sindarin
fn make_node(v: int): Node =>
    var n: Node = Node { value: v }
    return n

fn main(): void =>
    var n: Node = make_node(1)
    n = make_node(2)  // old Node is released, n now points to new Node
    assert(n.value == 2, "value should be 2 after reassign")
```

For native structs mapped to C types, `as ref` means the C struct is pointer-typed:

```sindarin
@alias "Vec2"
native struct Vec2 as ref =>
    @alias "x"
    x: double
    @alias "y"
    y: double
```

### `as val` — value semantics (stack/copy)

```sindarin
@alias "Color"
native struct Color as val =>
    @alias "r"
    r: byte
    @alias "g"
    g: byte
    @alias "b"
    b: byte
```

An `as val` native struct is used by value — it maps directly to the C struct layout with no
pointer indirection. The compiler generates copy and cleanup helpers but no reference counting.
This is the default for regular (non-native) structs and matches standard C struct behavior.

## Methods

Structs can define both instance methods and static methods.

### Instance methods

Instance methods are declared inside the struct body and receive an implicit `self` reference
to the current instance. They can read and mutate fields via `self`:

```sindarin
struct Counter =>
    value: int

    fn increment(): void =>
        self.value += 1

    fn getValue(): int =>
        return self.value

fn main(): void =>
    var c: Counter = Counter { value: 0 }
    c.increment()
    assert(c.getValue() == 1, "expected counter to be 1 after increment")
```

### Static methods

Static methods are declared with the `static fn` keyword inside the struct body. They have no
`self` parameter and are called using `TypeName.method()` syntax:

```sindarin
struct Counter =>
    value: int

    static fn zero(): int =>
        return 0

fn main(): int =>
    var z: int = Counter.zero()
    return z
```

## Handle Fields

A struct field is a "handle field" when its type requires heap-managed memory — for example,
`str` fields, array fields, or fields holding another `as ref` struct. The compiler
automatically generates correct copy and cleanup code for these fields.

```sindarin
struct Person =>
    name: str   // handle field: str is heap-allocated
    age: int    // plain field: copied by value

fn main(): int =>
    var p: Person = Person { name: "Alice", age: 30 }
    return 0
```

For `Person` above, the compiler generates:
- A `copy` helper that calls `strdup` on `name` so copies are independent.
- A `cleanup` helper that calls `free` on `name` when the struct goes out of scope.

This means you can assign structs freely without worrying about double-free or dangling
references — the compiler handles it:

```sindarin
var p1: Person = Person { name: "Alice", age: 30 }
var p2: Person = p1   // p2.name is a fresh strdup copy
```

Handle fields contrast with plain primitive fields (`int`, `double`, `bool`, `byte`) which are
copied by value with no heap involvement. If all fields are primitives, cleanup is a no-op.

## Operators

### `sizeof`

Get the size of a struct in bytes (includes padding for alignment):

```sindarin
struct Packet =>
    header: int32
    flags: byte
    payload: byte[256]

var size: int = sizeof(Packet)
var size2: int = sizeof Packet  // Parentheses optional
```

Works on both types and struct variables:

```sindarin
struct Point =>
    x: double
    y: double

sizeof(Point)           // 16 (type)

var p: Point = Point { x: 1.0, y: 2.0 }
sizeof(p)               // 16 (variable)
```

Useful for C interop when allocating memory or working with binary data:

```sindarin
native fn allocate_points(count: int): *Point =>
    return malloc(count * sizeof(Point)) as *Point
```

The `sizeof` operator works on both types and variables, returning the size in bytes.

### Equality (`==` and `!=`)

Structs support equality comparison (byte-wise):

```sindarin
var p1: Point = Point { x: 1.0, y: 2.0 }
var p2: Point = Point { x: 1.0, y: 2.0 }
var p3: Point = Point { x: 3.0, y: 4.0 }

if p1 == p2 =>
    print("Points are equal\n")      // This prints

if p1 != p3 =>
    print("Points are different\n")  // This prints
```

## Packed Structs

For binary formats requiring exact layouts, use `#pragma pack`:

```sindarin
#pragma pack(1)
struct FileHeader =>
    magic: int32       // offset 0
    version: byte      // offset 4
    flags: byte        // offset 5
    size: int32        // offset 6
    // Total: 10 bytes (no padding)
#pragma pack()
```

Without packing, the struct would have padding for alignment.

## Nested Structs

Structs can contain other structs:

```sindarin
struct Point =>
    x: double
    y: double

struct Line =>
    start: Point
    end: Point

struct Canvas =>
    name: str
    bounds: Line
    background: str = "white"

// Nested initialization
var line: Line = Line { start: Point { x: 0.0, y: 0.0 }, end: Point { x: 10.0, y: 20.0 } }

// Deep field access
var startX: double = line.start.x

// Deep field modification
line.end.y = 30.0
```

## Arrays of Structs

Arrays can contain structs:

```sindarin
struct Point =>
    x: double
    y: double

// Array literal
var points: Point[] = { Point { x: 0.0, y: 0.0 }, Point { x: 1.0, y: 1.0 } }

// Access elements
var first: Point = points[0]
print($"First point: ({first.x}, {first.y})\n")
```

## Structs Containing Arrays

Structs can have array fields:

```sindarin
struct Shape =>
    name: str
    points: Point[]
    color: str = "black"

var triangle: Shape = Shape { name: "triangle", points: { Point { x: 0.0, y: 0.0 }, Point { x: 1.0, y: 0.0 }, Point { x: 0.5, y: 1.0 } } }
```

## @serializable

Annotating a struct with `@serializable` instructs the compiler to auto-generate four
encode/decode methods on the struct:

| Method | Description |
|--------|-------------|
| `instance.encode(enc: Encoder)` | Encodes the instance to an `Encoder` |
| `TypeName.decode(dec: Decoder): TypeName` | Decodes an instance from a `Decoder` |
| `TypeName.encodeArray(arr: TypeName[], enc: Encoder)` | Encodes an array of instances |
| `TypeName.decodeArray(dec: Decoder): TypeName[]` | Decodes an array of instances |

`Encoder` and `Decoder` are vtable types — the SDK provides a `FastJson` implementation, but
the interface is generic so other formats can be plugged in.

```sindarin
@serializable
struct Address =>
    street: str
    city: str

@serializable
struct Person =>
    name: str
    age: int
    score: double
    active: bool
    address: Address
    tags: str[]
```

Encoding an instance:

```sindarin
var a: Address = Address { street: "123 Main St", city: "NYC" }
var enc: Encoder = sn_test_json_encoder()
a.encode(enc)
var json: str = enc.result()
// json == "{\"street\":\"123 Main St\",\"city\":\"NYC\"}"
```

Decoding an instance (static method call):

```sindarin
var dec: Decoder = sn_test_json_decoder("{\"street\":\"789 Pine Rd\",\"city\":\"Chicago\"}")
var a2: Address = Address.decode(dec)
// a2.street == "789 Pine Rd", a2.city == "Chicago"
```

Encoding and decoding arrays:

```sindarin
var addrs: Address[] = {
    Address { street: "1 Main", city: "NYC" },
    Address { street: "2 Oak", city: "LA" }
}
var arrEnc: Encoder = sn_test_json_array_encoder()
Address.encodeArray(addrs, arrEnc)
var arrJson: str = arrEnc.result()

var arrDec: Decoder = sn_test_json_decoder(arrJson)
var decoded: Address[] = Address.decodeArray(arrDec)
```

Nested `@serializable` structs are encoded/decoded recursively. `str[]` fields are encoded as
JSON arrays of strings.

### Requirements and Limitations

- **Field types must be serializable:** Only `str`, `int`, `double`, `bool`, arrays of those
  types, and nested `@serializable` structs are supported. Non-serializable field types cause a
  compile error.
- **Encoder/Decoder must be provided at runtime:** `@serializable` only generates the
  encode/decode methods. A concrete `Encoder`/`Decoder` implementation (e.g. `FastJson` from
  the SDK) must be supplied by the caller — the compiler has no built-in format.
- **No circular references:** Structs that directly or transitively reference themselves are
  not supported and will produce infinite recursion at runtime.

## C Interoperability

### Passing Structs to C Functions

Use `as ref` to pass structs by pointer to C functions:

```sindarin
struct TimeVal =>
    tv_sec: int
    tv_usec: int

native fn gettimeofday(tv: TimeVal as ref, tz: *void): int

fn get_time(): TimeVal =>
    var tv: TimeVal = TimeVal {}
    gettimeofday(tv, nil)  // Compiler passes &tv
    return tv
```

### Native Struct Interop

Native structs enable full C library integration:

```sindarin
native struct Buffer =>
    data: *byte
    size: int
    capacity: int

native fn init_buffer(buf: Buffer as ref, cap: int): void =>
    buf.data = nil
    buf.size = 0
    buf.capacity = cap

native fn use_buffer(): void =>
    var buf: Buffer = Buffer {}
    init_buffer(buf, 1024)
    // C function modifies buf through pointer
```

### Pointer Field Access

In native functions, pointer fields use automatic dereference:

```sindarin
native fn example(cfg: *Config): void =>
    var timeout: int = cfg.timeout  // Automatic dereference (like cfg->timeout in C)
```

See [Interop](interop.md) for more details on C interoperability.

## Practical Examples

### Configuration Pattern

```sindarin
struct DatabaseConfig =>
    driver: str = "postgres"
    host: str = "localhost"
    port: int = 5432
    database: str = "myapp"
    maxPoolSize: int = 10
    enableSSL: bool = false

fn connect(cfg: DatabaseConfig): void =>
    print($"Connecting to {cfg.driver}://{cfg.host}:{cfg.port}/{cfg.database}\n")

fn main(): void =>
    // Development config (all defaults)
    var devDb: DatabaseConfig = DatabaseConfig {}

    // Production config (override some values)
    var prodDb: DatabaseConfig = DatabaseConfig { host: "db.prod.example.com", enableSSL: true, maxPoolSize: 50 }

    connect(devDb)
    connect(prodDb)
```

### Binary Format Parsing

```sindarin
#pragma pack(1)
struct BinaryHeader =>
    magic: int32
    version: byte
    flags: byte
    reserved: byte
    headerSize: byte
#pragma pack()

fn validate_header(header: BinaryHeader): bool =>
    if header.magic != 1234567890 =>
        return false
    if header.version < 1 =>
        return false
    return true

fn main(): void =>
    var header: BinaryHeader = BinaryHeader { magic: 1234567890, version: 2, flags: 5, reserved: 0, headerSize: 8 }

    if validate_header(header) =>
        print("Header is valid\n")
```

### Streaming Pattern (zlib-style)

```sindarin
native struct StreamState =>
    avail_in: uint = 0
    total_in: uint = 0
    avail_out: uint = 0
    total_out: uint = 0
    state: int = 0

native fn stream_init(strm: StreamState as ref): void =>
    strm.avail_in = 0
    strm.total_in = 0
    strm.avail_out = 0
    strm.total_out = 0
    strm.state = 0

native fn stream_process(strm: StreamState as ref): int =>
    var to_process: uint = strm.avail_in
    if strm.avail_out < to_process =>
        to_process = strm.avail_out

    strm.total_in = strm.total_in + to_process
    strm.total_out = strm.total_out + to_process
    strm.avail_in = strm.avail_in - to_process
    strm.avail_out = strm.avail_out - to_process

    if strm.avail_in == 0 =>
        return 1  // Complete
    return 0  // More to process
```

## Design Decisions

### Value Semantics

Structs use value semantics (copy on assignment) to:
- Match C struct behavior for interop
- Prevent hidden aliasing
- Keep code predictable

### Struct Methods

Structs support both instance methods (accessed via `self`) and static methods (called on the type).

See the [Methods](#methods) section for full examples.

### No Inheritance

Structs do not support inheritance. Use composition (nested structs) instead.

### Named Initialization Only

Struct literals use named fields only (no positional arguments):

```sindarin
// Correct
var p: Point = Point { x: 1.0, y: 2.0 }

// Not supported
var p: Point = Point(1.0, 2.0)
```

### All Fields Public

All struct fields are publicly accessible. There are no access modifiers.

## Limitations

1. **No anonymous structs** - All structs must have named declarations
2. **No inheritance** - Use composition (nested structs) instead
3. **Native structs require native context** - Can only be used in `native fn` functions

## See Also

- [Memory](memory.md) - Ownership model, `as ref`/`as val` semantics, and memory management
- [Interop](interop.md) - C interoperability
- [Arrays](arrays.md) - Array operations
