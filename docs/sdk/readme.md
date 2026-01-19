# Sindarin SDK

The Sindarin SDK provides a collection of modules that extend the language's capabilities. SDK modules are imported using the `import` statement and provide types and functions for common programming tasks.

## Modules

| Module | Import | Description |
|--------|--------|-------------|
| [Date](date.md) | `import "sdk/date"` | Calendar date operations |
| [Time](time.md) | `import "sdk/time"` | Time and duration operations |
| [Environment](env.md) | `import "sdk/env"` | Environment variable access |
| [Process](process.md) | `import "sdk/process"` | Process execution and output capture |
| [Random](random.md) | `import "sdk/random"` | Random number generation |
| [UUID](uuid.md) | `import "sdk/uuid"` | UUID generation and parsing |
| [I/O](io/readme.md) | `import "sdk/io/..."` | File and directory operations |
| [Net](net/readme.md) | `import "sdk/net/..."` | TCP and UDP networking |

## Quick Start

```sindarin
import "sdk/date"
import "sdk/time"
import "sdk/env"
import "sdk/process"
import "sdk/random"
import "sdk/uuid"
import "sdk/io/textfile"
import "sdk/net/tcp"

fn main(): int =>
  // Current date and time
  var today: Date = Date.today()
  var now: Time = Time.now()
  print($"Today: {today.toIso()}\n")
  print($"Now: {now.format("HH:mm:ss")}\n")

  // Environment variables
  var user: str = Environment.getOr("USER", "unknown")
  print($"User: {user}\n")

  // Run external command
  var p: Process = Process.run("pwd")
  print($"Current dir: {p.stdout()}")

  // Random values
  var dice: int = Random.int(1, 6)
  print($"Dice roll: {dice}\n")

  // Generate UUID
  var id: UUID = UUID.create()
  print($"ID: {id}\n")

  // File I/O
  TextFile.writeAll("/tmp/hello.txt", "Hello, SDK!")
  var content: str = TextFile.readAll("/tmp/hello.txt")
  print($"File: {content}\n")

  return 0
```

---

## Shared Concepts

### Import Syntax

SDK modules are imported by path relative to the SDK root:

```sindarin
import "sdk/date"           // Top-level module
import "sdk/io/textfile"    // Nested module
import "sdk/io/path"        // Another nested module
```

### Naming Conventions

SDK types use the `Sn` prefix to distinguish them from built-in types:

| SDK Type | Description |
|----------|-------------|
| `Date` | Calendar date |
| `Time` | Timestamp |
| `Environment` | Environment variables |
| `Process` | Process execution |
| `TextFile` | Text file handle |
| `BinaryFile` | Binary file handle |
| `Path` | Path utilities |
| `Directory` | Directory operations |
| `Bytes` | Byte encoding/decoding |
| `TcpListener` | TCP server socket |
| `TcpStream` | TCP connection |
| `UdpSocket` | UDP socket |

Some types like `Random` and `UUID` don't use the prefix as they have no built-in equivalent.

### Static vs Instance Methods

SDK types typically provide both static methods (called on the type) and instance methods (called on values):

```sindarin
// Static method - called on the type
var today: Date = Date.today()
var exists: bool = TextFile.exists("file.txt")

// Instance method - called on a value
var formatted: str = today.format("YYYY-MM-DD")
var line: str = file.readLine()
```

### Error Handling

SDK operations that can fail will panic with a descriptive error message. Use existence checks to avoid panics:

```sindarin
// Check before operations that require existing files
if TextFile.exists(path) =>
  var content: str = TextFile.readAll(path)
else =>
  print("File not found\n")
```

Future SDK versions may introduce a `Result` type for recoverable error handling.

### Memory Management

SDK types integrate with Sindarin's arena-based memory management:

- **Automatic cleanup**: Resources are released when their arena is destroyed
- **Private blocks**: Use `private =>` for guaranteed cleanup scope
- **Explicit release**: Call `.close()` for immediate resource release

```sindarin
// Automatic - file closes when function returns
fn readConfig(): str =>
  var f: TextFile = TextFile.open("config.txt")
  return f.readRemaining()

// Explicit - immediate cleanup
var f: TextFile = TextFile.open("large.log")
processFile(f)
f.close()  // Release now, don't wait for arena
```

See the [I/O documentation](io/readme.md) for detailed memory management patterns with file handles.

---

## Module Reference

### Date

Calendar date operations with formatting, arithmetic, and comparison.

```sindarin
import "sdk/date"

var today: Date = Date.today()
var birthday: Date = Date.fromYmd(2025, 6, 15)
var days: int = birthday.diffDays(today)
print($"Days until birthday: {days}\n")
```

[Full documentation →](date.md)

### Time

Timestamps, duration calculations, and time formatting.

```sindarin
import "sdk/time"

var start: Time = Time.now()
doWork()
var elapsed: int = Time.now().diff(start)
print($"Elapsed: {elapsed}ms\n")
```

[Full documentation →](time.md)

### Environment

Environment variable access with required and optional (default) variants.

```sindarin
import "sdk/env"

var user: str = Environment.getOr("USER", "unknown")
var apiKey: str = Environment.get("API_KEY")  // panics if not set
if Environment.has("DEBUG") =>
  enableDebugMode()
```

[Full documentation →](env.md)

### Process

Execute external commands and capture their output.

```sindarin
import "sdk/process"

var p: Process = Process.runArgs("ls", {"-la"})
if p.success() =>
    print(p.stdout())
else =>
    print($"Failed: {p.stderr()}")
```

[Full documentation →](process.md)

### Random

Secure random number generation with optional seeding for reproducibility.

```sindarin
import "sdk/random"

var dice: int = Random.int(1, 6)
var coin: bool = Random.bool()
var pick: str = Random.choice(colors)
```

[Full documentation →](random.md)

### UUID

UUID generation (v4, v5, v7) and parsing.

```sindarin
import "sdk/uuid"

var id: UUID = UUID.create()     // v7 (recommended)
var random: UUID = UUID.v4()     // v4 (pure random)
print($"ID: {id}\n")
```

[Full documentation →](uuid.md)

### I/O

File operations, path utilities, and directory management.

```sindarin
import "sdk/io/textfile"
import "sdk/io/binaryfile"
import "sdk/io/path"
import "sdk/io/directory"
import "sdk/io/bytes"

var content: str = TextFile.readAll("data.txt")
var dir: str = Path.directory("/home/user/file.txt")
var files: str[] = Directory.list("/home/user")
```

[Full documentation →](io/readme.md)

### Net

TCP and UDP socket operations for network communication.

```sindarin
import "sdk/net/tcp"
import "sdk/net/udp"

var server: TcpListener = TcpListener.bind(":8080")
var client: TcpStream = server.accept()
var line: str = client.readLine()
client.writeLine($"Echo: {line}")
```

[Full documentation →](net/readme.md)

---

## See Also

- [Language Overview](../readme.md) - Core language documentation
- SDK source code: `sdk/` directory
