# Structs in Sindarin

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
| Large (≥8KB) | Heap (arena-managed) |
| Escaping scope | Copied to outer arena |

```sindarin
fn process(): void =>
    var p: Point = Point { x: 1.0, y: 2.0 }  // Stack allocated (small)
    // p freed automatically when function returns
```

### Escape Behavior

When a struct escapes its scope, it is copied to the outer arena:

```sindarin
var outer: Point

if condition =>
    var inner: Point = Point { x: 1.0, y: 2.0 }  // Stack or inner arena
    outer = inner                                  // COPIED to outer scope

print($"x = {outer.x}\n")  // Safe - outer owns its copy
```

### Returning Structs

Structs can be returned from functions. They are copied to the caller's arena:

```sindarin
fn make_point(x: double, y: double): Point =>
    var p: Point = Point { x: x, y: y }
    return p  // Copied to caller's arena

var result: Point = make_point(1.0, 2.0)
```

### Integration with `shared`

Use `shared` functions to avoid copy overhead:

```sindarin
fn make_point(x: double, y: double) shared: Point =>
    var p: Point = Point { x: x, y: y }  // Allocated in caller's arena
    return p                              // No copy needed
```

### Integration with `private`

Structs with only primitive fields can escape `private` blocks:

```sindarin
fn get_origin() private: Point =>
    var p: Point = Point { x: 0.0, y: 0.0 }
    return p  // OK: struct contains only primitives

// Error: structs with heap data cannot escape
struct NamedPoint =>
    name: str
    x: double
    y: double

fn bad() private: NamedPoint =>
    var p: NamedPoint = NamedPoint { name: "origin", x: 0.0, y: 0.0 }
    return p  // COMPILE ERROR: struct contains heap data (str)
```

See [Memory](memory.md) for more details on arena memory management.

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

### No Methods

Structs are plain data containers without methods. Use standalone functions:

```sindarin
// Instead of p.distance(other)
fn point_distance(a: Point, b: Point): double =>
    var dx: double = b.x - a.x
    var dy: double = b.y - a.y
    return sqrt(dx * dx + dy * dy)
```

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
2. **No struct methods** - Use standalone functions instead
3. **Native structs require native context** - Can only be used in `native fn` functions

## See Also

- [Memory](memory.md) - Arena memory management
- [Interop](interop.md) - C interoperability
- [Arrays](arrays.md) - Array operations
