# Sindarin (Sn) Programming Language

**Sindarin** is a statically-typed procedural programming language that compiles to C. It features clean arrow-based syntax, powerful string interpolation, and built-in array operations.

```
.sn source → Sn Compiler → C code → GCC/Clang → executable
```

## Features

- **Static typing** with explicit type annotations
- **Arrow syntax** (`=>`) for clean, readable code blocks
- **String interpolation** with `$"Hello {name}!"`
- **Arrays** with built-in operations (push, pop, slice, join, etc.)
- **Structs** for structured data and C library interop
- **String methods** (toUpper, toLower, trim, split, splitLines, isBlank, etc.)
- **File I/O** with TextFile and BinaryFile types
- **Control flow** with for, for-each, while, match, break, continue
- **Module imports** for code organization
- **Arena memory** with shared/private scopes and copy semantics
- **C interoperability** with native functions, pointers, and callbacks
- **SDK modules** for crypto, networking, JSON/XML/YAML, compression, and more

## Installation

### Linux / macOS

```bash
curl -fsSL https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install.sh | bash
```

### Windows

```powershell
irm https://raw.githubusercontent.com/SindarinSDK/sindarin-compiler/main/scripts/install.ps1 | iex
```

See [docs/building.md](docs/building.md) for building from source and other installation options.

## Quick Start

```bash
# Compile a program
sn samples/main.sn -o myprogram
./myprogram

# Or emit C code only
sn samples/main.sn --emit-c -o output.c
```

## Example

```sn
fn is_prime(n: int): bool =>
  if n <= 1 =>
    return false
  var i: int = 2
  while i * i <= n =>
    if n % i == 0 =>
      return false
    i = i + 1
  return true

fn main(): void =>
  var primes: int[] = {}
  for var n: int = 2; n <= 50; n++ =>
    if is_prime(n) =>
      primes.push(n)
  print($"Primes: {primes.join(\", \")}\n")
```

## Documentation

### Language

| Document | Description |
|----------|-------------|
| [Overview](docs/readme.md) | Language philosophy, syntax overview, examples |
| [Building](docs/building.md) | Build instructions for Linux, macOS, Windows |
| [Strings](docs/strings.md) | String methods and interpolation |
| [Arrays](docs/arrays.md) | Array operations and slicing |
| [Structs](docs/structs.md) | Struct declarations and C interop |
| [Match](docs/match.md) | Match expressions for multi-way branching |
| [Lambdas](docs/lambdas.md) | Lambda expressions and closures |
| [Memory](docs/memory.md) | Arena memory management |
| [Threading](docs/threading.md) | Threading with spawn and sync |
| [Namespaces](docs/namespaces.md) | Namespaced imports for collision resolution |
| [Interop](docs/interop.md) | C interoperability and native functions |
| [Interceptors](docs/interceptors.md) | Function interception for debugging and mocking |

### SDK Modules

SDK documentation is available at [sindarin-pkg-sdk](https://github.com/SindarinSDK/sindarin-pkg-sdk).

| Document | Description |
|----------|-------------|
| [SDK Overview](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/readme.md) | All SDK modules |
| [Crypto](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/crypto.md) | Hashing, encryption, HMAC, PBKDF2, secure random |
| [Date](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/date.md) | Calendar date operations |
| [Time](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/time.md) | Time and duration operations |
| [Random](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/random.md) | Random number generation |
| [UUID](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/uuid.md) | UUID generation and manipulation |
| [Environment](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/env.md) | Environment variable access |
| [Process](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/process.md) | Process execution and output capture |
| [Math](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/math.md) | Mathematical functions and constants |
| [JSON](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/json.md) | JSON parsing and serialization |
| [XML](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/xml.md) | XML parsing, XPath, and DOM manipulation |
| [YAML](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/yaml.md) | YAML parsing and serialization |
| [ZLib](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/zlib.md) | Compression and decompression |
| [Stdio](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/stdio.md) | Standard input/output/error streams |
| [File I/O](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/io/readme.md) | TextFile, BinaryFile, Path, Directory, Bytes |
| [Networking](https://github.com/SindarinSDK/sindarin-pkg-sdk/blob/main/docs/net/readme.md) | TCP, UDP, TLS, DTLS, SSH, QUIC |

## Architecture

```
Source (.sn)
    ↓
Lexer → Parser → Type Checker → Optimizer → Code Gen → GCC
    ↓
Executable
```

See `src/` for compiler implementation details.

## Testing

```bash
make test              # All tests (unit + integration + SDK + exploratory)
make test-unit         # Unit tests only
make test-integration  # Integration tests only
make test-sdk          # SDK tests only
make test-explore      # Exploratory tests only
```

Or use the test runner directly (cross-platform):

```bash
python3 scripts/run_tests.py all            # All tests
python3 scripts/run_tests.py unit           # Unit tests
python3 scripts/run_tests.py integration    # Integration tests
python3 scripts/run_tests.py sdk            # SDK tests
python3 scripts/run_tests.py explore        # Exploratory tests
```

## Project Structure

```
├── src/           # Compiler source (lexer, parser, type checker, codegen, runtime)
├── sdk/           # SDK modules (.sn definitions + .c implementations)
├── tests/         # Unit, integration, SDK, and exploratory tests
├── docs/          # Language and SDK documentation
├── samples/       # Example .sn programs
├── scripts/       # Test runner and dependency setup
├── packaging/     # Distribution packaging (deb, rpm, homebrew, winget, arch)
├── benchmark/     # Performance benchmarks
├── bin/           # Compiled outputs (sn compiler, runtime library, SDK)
└── CMakeLists.txt # CMake build configuration
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Run tests with `make test`
4. Submit a pull request

## License

MIT License - feel free to use, modify, and distribute!

---

*Named after the Elvish language from Tolkien's legendarium*
