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
- **Control flow** with for, for-each, while, break, continue
- **Module imports** for code organization
- **Arena memory** with shared/private scopes and copy semantics
- **C interoperability** with native functions, pointers, and callbacks
- **SDK modules** for compression, math, and more

## Installation

### Linux (Debian/Ubuntu)

```bash
# Download the .deb package from the latest release
sudo dpkg -i sindarin_*.deb

# Dependencies (gcc, zlib) are installed automatically
sn --version
```

### Linux (Fedora/RHEL)

```bash
# Download the .rpm package from the latest release
sudo rpm -i sindarin-*.rpm

sn --version
```

### Linux (Arch)

```bash
# Download PKGBUILD from the release
curl -LO https://github.com/RealOrko/sindarin/releases/latest/download/PKGBUILD

# Build and install
makepkg -si
```

### macOS (Homebrew)

```bash
# Download the formula from the release
curl -LO https://github.com/RealOrko/sindarin/releases/latest/download/sindarin.rb

# Install (will prompt for Xcode CLT if needed)
brew install --formula ./sindarin.rb
```

### Linux/macOS (Tarball)

```bash
# Download and extract
tar xzf sindarin-*-linux-x64.tar.gz   # or macos-x64
cd sindarin-*/

# Add to PATH or copy to /usr/local
export PATH="$PWD/bin:$PATH"
sn --version
```

### Windows

**Option 1: Winget (local manifest)**

```powershell
# Download winget-manifests.zip from the release
Expand-Archive winget-manifests.zip -DestinationPath winget-manifests
winget install --manifest ./winget-manifests
```

**Option 2: Manual installation**

```powershell
# Download and extract the ZIP
Expand-Archive sindarin-*-windows-x64.zip -DestinationPath C:\sindarin

# Add to PATH
$env:PATH += ";C:\sindarin\bin"
sn --version
```

**Prerequisite for Windows:** [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw) is required (provides clang).

### Build from Source

```bash
# Prerequisites: CMake, Ninja, GCC or Clang, zlib

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

bin/sn --version
```

See [docs/building.md](docs/building.md) for detailed build instructions.

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
| [Lambdas](docs/lambdas.md) | Lambda expressions and closures |
| [Memory](docs/memory.md) | Arena memory management |
| [Threading](docs/threading.md) | Threading with spawn and sync |
| [Namespaces](docs/namespaces.md) | Namespaced imports for collision resolution |
| [Interop](docs/interop.md) | C interoperability and native functions |
| [Interceptors](docs/interceptors.md) | Function interception for debugging and mocking |

### SDK Modules

| Document | Description |
|----------|-------------|
| [SDK Overview](docs/sdk/readme.md) | All SDK modules |
| [Date](docs/sdk/date.md) | Calendar date operations |
| [Time](docs/sdk/time.md) | Time and duration operations |
| [Random](docs/sdk/random.md) | Random number generation |
| [UUID](docs/sdk/uuid.md) | UUID generation and manipulation |
| [Environment](docs/sdk/env.md) | Environment variable access |
| [Process](docs/sdk/process.md) | Process execution and output capture |
| [File I/O](docs/sdk/io/readme.md) | TextFile, BinaryFile, Path, Directory |
| [Networking](docs/sdk/net/readme.md) | TCP and UDP operations |

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
bin/tests                              # Unit tests
./scripts/run_tests.sh integration     # Integration tests
./scripts/run_tests.sh explore         # Exploratory tests
```

On Windows, use the PowerShell test runner:

```powershell
.\scripts\run_integration_test.ps1 -TestType integration -All
.\scripts\run_integration_test.ps1 -TestType explore -All
```

## Project Structure

```
├── src/           # Compiler source code
├── tests/         # Unit, integration, and exploratory tests
├── samples/       # Example .sn programs
├── docs/          # Documentation
├── benchmark/     # Performance benchmarks
├── bin/           # Compiled outputs (sn, tests, runtime)
├── build/         # CMake build directory
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
