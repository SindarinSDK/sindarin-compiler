# File I/O in Sindarin

Sindarin provides comprehensive file I/O capabilities through the SDK. This document provides an overview of the available types and shared concepts.

## SDK Modules

| Module | Description |
|--------|-------------|
| [TextFile](textfile.md) | Text-based file operations with string values |
| [BinaryFile](binaryfile.md) | Raw byte file operations |
| [Bytes](bytes.md) | Byte encoding/decoding (hex, base64) |
| [Path](path.md) | Path manipulation utilities |
| [Directory](directory.md) | Directory operations |

## Quick Start

```sindarin
import "sdk/io/textfile"
import "sdk/io/binaryfile"
import "sdk/io/path"
import "sdk/io/directory"
import "sdk/io/bytes"

// Read a text file
var content: str = TextFile.readAll("data.txt")

// Write a text file
TextFile.writeAll("output.txt", "Hello, World!")

// Read binary data
var data: byte[] = BinaryFile.readAll("image.bin")

// Path operations
var dir: str = Path.directory("/home/user/file.txt")
var name: str = Path.filename("/home/user/file.txt")

// Directory listing
var files: str[] = Directory.list("/home/user")

// Byte encoding
var hex: str = data.toHex()
var decoded: byte[] = Bytes.fromHex(hex)
```

---

## Standard Streams

Built-in file handles for console I/O:

```sindarin
// Read line from standard input
var line: str = Stdin.readLine()

// Write to standard output
Stdout.write("Hello")
Stdout.writeLine("Hello")

// Write to standard error
Stderr.write("Error!")
Stderr.writeLine("Error!")
```

### Convenience Functions

Global functions for simple console I/O:

```sindarin
// Read line from stdin
var input: str = readLine()

// Write to stdout
print("output")
println("output")

// Write to stderr
printErr("error")
printErrLn("error")
```

---

## Memory Management

File handles integrate with Sindarin's arena-based memory management.

### Automatic Cleanup

Files are automatically closed when their arena is destroyed:

```sindarin
fn processFile(path: str): str =>
  var f: TextFile = TextFile.open(path)
  var content: str = f.readRemaining()
  // f is automatically closed when function returns
  return content
```

### Private Blocks

Use private blocks for guaranteed cleanup:

```sindarin
var result: str = ""
private =>
  var f: TextFile = TextFile.open("data.txt")
  result = f.readRemaining()
  // f is GUARANTEED to close here, even on early return
```

**Important:** File handles opened within a private block cannot escape:

```sindarin
var f: TextFile    // Outer variable

private =>
  f = TextFile.open("data.txt")    // COMPILE ERROR: File cannot escape private block
```

### Shared Functions

Shared functions use the caller's arena:

```sindarin
fn processFile(f: TextFile) shared: str =>
  // Uses caller's arena - no new arena created
  // f remains open (caller owns it)
  return f.readLine()
```

### Explicit Closing

For long-running operations, close files explicitly:

```sindarin
for path in filePaths =>
  var f: TextFile = TextFile.open(path)
  process(f)
  f.close()  // Don't wait for arena cleanup
```

### Loop Iteration Behavior

**Non-shared loops** (default): Per-iteration arena cleanup

```sindarin
for path in paths =>
  var f: TextFile = TextFile.open(path)
  process(f)
  // f.close() called automatically at iteration end
```

**Shared loops**: File persists across iterations

```sindarin
var f: TextFile = TextFile.open("log.txt")
for item in items shared =>
  f.writeLine(item)    // Same file handle, no re-open
f.close()
```

---

## Threading

File operations work naturally with Sindarin's threading model. Use `&` to spawn I/O in background threads and `!` to synchronize.

### Parallel File Reads

```sindarin
var f1: str = &TextFile.readAll("file1.txt")
var f2: str = &TextFile.readAll("file2.txt")
var f3: str = &TextFile.readAll("file3.txt")

// Wait for all to complete
[f1, f2, f3]!

print($"Total: {f1.length + f2.length + f3.length} bytes\n")
```

### Background Write

```sindarin
// Fire and forget - write happens in background
&TextFile.writeAll("backup.txt", data)

// Continue with other work...
```

### Inline Sync

```sindarin
// Spawn and immediately wait
var content: str = &TextFile.readAll("large.txt")!

// Use in expressions
var total: int = &countLines("a.txt")! + &countLines("b.txt")!
```

### Memory Semantics

File handles and data follow the frozen reference rules:

```sindarin
var data: str = "content to write"
var result: void = &TextFile.writeAll("out.txt", data)

data = "modified"  // COMPILE ERROR: data frozen by thread

result!            // sync releases freeze
data = "modified"  // OK
```

---

## Error Handling

File operations panic on errors (file not found, permission denied, etc.). Always check existence before operations that require existing files:

```sindarin
var path: str = "config.txt"
if TextFile.exists(path) =>
  var config: str = TextFile.readAll(path)
  processConfig(config)
else =>
  print("Warning: Config file not found, using defaults\n")
  useDefaults()
```

Examples of panic conditions:
- `TextFile.open()` - File doesn't exist or permission denied
- `TextFile.delete()` - File doesn't exist or permission denied
- `f.readLine()` - I/O error during read
- `f.seek(pos)` - Invalid position

Future versions may introduce a `Result` type for recoverable error handling.

---

## Design Decisions

This section documents the design rationale for the file I/O system.

### Principles

1. **Consistency** - Method naming follows existing conventions (camelCase like arrays/strings)
2. **Simplicity** - No mode flags, files are always read/write capable
3. **Safety** - Panic on errors rather than returning null
4. **Clarity** - Separate types for text (`TextFile`) and binary (`BinaryFile`) operations
5. **Ergonomics** - Convenience methods for common operations

### Arena Lifecycle

File handles are bound to the arena in which they are opened:

| Context | Arena Behavior | File Behavior |
|---------|----------------|---------------|
| Function entry | New arena created | Files opened here close on function exit |
| `private =>` block | New isolated arena | Files **guaranteed** to close at block end |
| `shared` function | Uses caller's arena | Files persist in caller's scope |
| Loop iteration (non-shared) | Per-iteration arena | File opens/closes each iteration |
| Loop iteration (shared) | Parent arena reused | File persists across iterations |

### Escaping Rules

File handles follow these escaping rules:

| Scenario | Allowed? | Behavior |
|----------|----------|----------|
| Return file from function | Yes | Handle promoted to caller's arena |
| Assign to outer variable | Yes (non-private) | Handle ownership transfers |
| Assign to outer variable from `private` | **No** | Compile-time error |
| Pass to function | Yes | Reference passed, caller retains ownership |
| Pass to `shared` function | Yes | Same handle, no ownership change |
| Capture in closure | Yes | Handle lifetime extends to closure lifetime |

### Return Value Promotion

```sindarin
fn openConfig(): TextFile =>
  var f: TextFile = TextFile.open("config.txt")
  return f    // Handle promoted to caller's arena

fn main(): int =>
  var config: TextFile = openConfig()
  // config is valid here - we own it now
  var data: str = config.readRemaining()
  config.close()
  return 0
```

## See Also

- [SDK Overview](../readme.md) - All SDK modules
- [Threading](../../threading.md) - Threading model (`&` spawn, `!` sync)
- [Memory](../../memory.md) - Arena memory management details
- [Arrays](../../arrays.md) - Array operations including byte arrays
