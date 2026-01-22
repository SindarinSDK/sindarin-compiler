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
| [Math](math.md) | `import "sdk/math"` | Mathematical functions and constants |
| [JSON](json.md) | `import "sdk/json"` | JSON parsing and serialization |
| [XML](xml.md) | `import "sdk/xml"` | XML parsing, XPath, and DOM manipulation |
| [YAML](yaml.md) | `import "sdk/yaml"` | YAML parsing and serialization |
| [ZLib](zlib.md) | `import "sdk/zlib"` | Compression and decompression |
| [Stdio](stdio.md) | `import "sdk/stdio"` | Standard input/output/error streams |
| [I/O](io/readme.md) | `import "sdk/io/..."` | File and directory operations |
| [Net](net/readme.md) | `import "sdk/net/..."` | TCP, UDP, TLS, DTLS, SSH, and QUIC networking |

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
| `TlsStream` | TLS-encrypted TCP connection |
| `DtlsConnection` | DTLS-encrypted UDP connection |
| `SshConnection` | SSH client connection |
| `SshListener` | SSH server listener |
| `QuicConnection` | QUIC connection |
| `QuicListener` | QUIC server listener |
| `QuicStream` | QUIC stream |
| `Json` | JSON value |
| `Xml` | XML node |
| `Yaml` | YAML node |
| `Stdin` | Standard input |
| `Stdout` | Standard output |
| `Stderr` | Standard error |

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

### Math

Mathematical functions, constants, and helpers for double and float precision.

```sindarin
import "sdk/math" as math

var angle: double = math.degToRad(45.0)
var s: double = math.sin(angle)
var dist: double = math.distance2D(0.0, 0.0, 3.0, 4.0)
```

[Full documentation →](math.md)

### JSON

JSON parsing, manipulation, and serialization using yyjson.

```sindarin
import "sdk/json"

var doc: Json = Json.parse("{\"name\": \"Alice\"}")
var name: str = doc.get("name").asString()

var obj: Json = Json.object()
obj.set("key", Json.ofString("value"))
print(obj.toPrettyString())
```

[Full documentation →](json.md)

### XML

XML parsing, XPath queries, and DOM manipulation using libxml2.

```sindarin
import "sdk/xml"

var doc: Xml = Xml.parseFile("data.xml")
var items: Xml[] = doc.findAll("//item")
for i: int = 0; i < items.length; i += 1 =>
    print($"{items[i].text()}\n")
```

[Full documentation →](xml.md)

### YAML

YAML parsing, manipulation, and serialization using libyaml.

```sindarin
import "sdk/yaml"

var config: Yaml = Yaml.parseFile("config.yaml")
var host: str = config.get("server").get("host").value()
var port: int = config.get("server").get("port").asInt()
```

[Full documentation →](yaml.md)

### ZLib

Compression and decompression using zlib.

```sindarin
import "sdk/zlib"

var compressed: byte[] = compressData(data)
var original: byte[] = decompressData(compressed, expectedSize)
```

[Full documentation →](zlib.md)

### Stdio

Structured access to standard input, output, and error streams.

```sindarin
import "sdk/stdio"

Stdout.write("Enter name: ")
Stdout.flush()
var name: str = Stdin.readLine()
Stderr.writeLine("Debug: got input")
```

[Full documentation →](stdio.md)

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

TCP, UDP, TLS, DTLS, SSH, and QUIC operations for network communication.

```sindarin
import "sdk/net/tcp"
import "sdk/net/udp"
import "sdk/net/tls"
import "sdk/net/dtls"
import "sdk/net/ssh"
import "sdk/net/quic"

var server: TcpListener = TcpListener.bind(":8080")
var client: TcpStream = server.accept()
var line: str = client.readLine()
client.writeLine($"Echo: {line}")

// Secure connections
var secure: TlsStream = TlsStream.connect("example.com:443")
secure.writeLine("GET / HTTP/1.1")
secure.writeLine("Host: example.com")
secure.writeLine("")

// SSH remote execution
var ssh: SshConnection = SshConnection.connectPassword("server:22", "user", "pass")
print(ssh.run("ls -la"))
ssh.close()

// QUIC multiplexed streams
var quic: QuicConnection = QuicConnection.connect("server:4433")
var stream: QuicStream = quic.openStream()
stream.writeLine("hello")
```

[Full documentation →](net/readme.md)

---

## See Also

- [Language Overview](../readme.md) - Core language documentation
- SDK source code: `sdk/` directory
