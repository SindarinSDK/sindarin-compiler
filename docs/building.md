---
title: "Building"
description: "Compile and run the Sindarin compiler"
permalink: /language/building/
---

This document explains how to build the Sindarin compiler from source on Linux, macOS, and Windows.

## Prerequisites

Minimum requirements:
- **Make** (GNU Make)
- **Git** with LFS support
- **CMake** 3.16 or later
- **Ninja** build system (recommended, falls back to Unix Makefiles)
- A C99-compatible compiler (**GCC** on Linux, **Clang** on macOS/Windows)

Dependencies (zlib, yyjson, libgit2, etc.) are provided as pre-built libraries downloaded by `make setup`.

## Quick Start

The simplest way to build is using the Makefile wrapper:

```bash
# Download pre-built native libraries
make setup

# Build
make build

# Test
make test

# Run a sample program
make run
```

Or using CMake directly:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Using CMake Presets

CMake presets provide pre-configured build settings. List available presets:

```bash
cmake --list-presets
```

Build with a preset:

```bash
# Linux with GCC
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release

# macOS with Clang
cmake --preset macos-clang-release
cmake --build --preset macos-clang-release

# Windows with Clang
cmake --preset windows-clang-release
cmake --build --preset windows-clang-release
```

## Platform-Specific Instructions

### Linux

**Prerequisites (provides make, gcc, python):**

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential python3

# Fedora/RHEL
sudo dnf install -y gcc make python3

# Arch Linux
sudo pacman -S base-devel python
```

**Build:**

```bash
make setup   # Download pre-built native libraries
make build
```

Or using CMake directly (if pre-built libraries are already downloaded):

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify:**

```bash
bin/sn --version
bin/tests  # Run unit tests
```

### macOS

**Prerequisites:** Xcode Command Line Tools (provides make, clang). Install if needed:

```bash
xcode-select --install
```

Also install cmake and ninja:

```bash
brew install cmake ninja
```

**Build:**

```bash
make setup   # Download pre-built native libraries
make build
```

Or using CMake directly (if pre-built libraries are already downloaded):

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify:**

```bash
bin/sn --version
bin/tests
```

### Windows

**Prerequisites:** Install LLVM-MinGW (provides clang), Make, and Ninja:

```powershell
# Via Chocolatey
choco install make ninja -y

# LLVM-MinGW (via winget or download from https://github.com/mstorsjo/llvm-mingw/releases)
winget install mstorsjo.llvm-mingw
```

Alternatively install CMake and Ninja via winget:

```powershell
winget install Kitware.CMake
winget install Ninja-build.Ninja
```

**Build:**

```bash
make setup   # Download pre-built native libraries
make build
```

Or using CMake directly (if pre-built libraries are already downloaded):

```powershell
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Verify:**

```powershell
bin\sn.exe --version
bin\tests.exe
```

## Build Outputs

After a successful build:

| Path | Description |
|------|-------------|
| `bin/sn` (or `bin/sn.exe`) | The Sindarin compiler |
| `bin/tests` (or `bin/tests.exe`) | Unit test runner |
| `bin/lib/gcc/libsn_runtime.a` | Runtime library (Linux/macOS) |
| `bin/lib/clang/libsn_runtime.a` | Runtime library (Windows) |
| `bin/include/` | Runtime headers |
| `bin/deps/` | Bundled dependencies (zlib, yyjson headers + libs) |
| `bin/sn.<platform>.cfg` | Platform-specific compiler configuration file (`sn.linux.cfg`, `sn.darwin.cfg`, `sn.windows.cfg`) |

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build type: `Release` or `Debug` |
| `CMAKE_C_COMPILER` | System default | C compiler: `gcc`, `clang`, etc. |
| `SN_DEBUG` | `OFF` | Enable debug symbols and reduced optimization |
| `SN_ASAN` | `OFF` | Enable AddressSanitizer (GCC/Clang only) |

**Debug build with AddressSanitizer:**

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSN_DEBUG=ON \
  -DSN_ASAN=ON
cmake --build build
```

## Running Tests

The unified Python test runner works on all platforms:

```bash
# Run all tests
python3 scripts/run_tests.py all

# Unit tests only
python3 scripts/run_tests.py unit

# Integration tests
python3 scripts/run_tests.py integration
python3 scripts/run_tests.py integration-errors

# Exploratory tests
python3 scripts/run_tests.py explore
python3 scripts/run_tests.py explore-errors
```

Or use Make targets:

```bash
make test                    # All tests
make test-unit               # Unit tests only
make test-cgen               # Code generation tests (compare generated C)
make test-mgen               # Model generation tests (compare JSON model)
make test-integration        # Integration tests
make test-integration-errors # Integration error tests
make test-explore            # Exploratory tests
make test-explore-errors     # Exploratory error tests
```

**Test runner options:**

```bash
python3 scripts/run_tests.py --help

# Exclude specific tests
python3 scripts/run_tests.py integration --exclude test_foo,test_bar

# Verbose output
python3 scripts/run_tests.py all --verbose
```

## Creating Packages

Build distributable packages using CPack:

```bash
make package
# or
cd build && cpack
```

This creates platform-specific packages:
- **Linux**: `.tar.gz`, `.deb`, `.rpm`
- **Windows**: `.zip`
- **macOS**: `.tar.gz`

## Compiling Sindarin Programs

Once built, compile `.sn` files:

```bash
# Compile to executable
bin/sn myprogram.sn -o myprogram
./myprogram

# Emit C code only (don't compile)
bin/sn myprogram.sn --emit-c -o myprogram.c

# Debug build (with symbols, enables ASAN on Linux)
bin/sn myprogram.sn -g -o myprogram

# Profile build (optimized with frame pointers, no ASAN, no LTO)
bin/sn myprogram.sn -p -o myprogram
```

> **Note:** `-p` and `-g` cannot be used together.

### Optimization and Arithmetic Checking Flags

| Flag | Effect |
|------|--------|
| `-O0` | No Sn optimizer passes (useful for debugging codegen) |
| `-O1` | Basic optimizations: dead code elimination, string merging |
| `-O2` | Full optimizations (default): all `-O1` passes plus tail call optimization and unchecked arithmetic |
| `--unchecked` | Disable integer overflow checking regardless of optimization level |
| `--checked` | Force integer overflow checking even when `-O2` is active |

Integer arithmetic is overflow-checked by default. `-O2` implies `--unchecked` for performance. Use `--checked` to re-enable checking in an optimized build, or `--unchecked` to disable it at any level.

## C Compiler Configuration

The Sindarin compiler uses a C backend. It reads configuration from a platform-specific config file (`sn.linux.cfg`, `sn.darwin.cfg`, or `sn.windows.cfg`) and can be overridden via environment variables.

### Configuration File Search Order

The compiler searches for `sn.<platform>.cfg` in these locations (first found wins):

1. Next to the compiler executable (portable/development mode)
2. Platform-specific system paths:
   - **Linux**: `/etc/sindarin/sn.linux.cfg`, `/usr/local/etc/sindarin/sn.linux.cfg`
   - **macOS**: `/usr/local/etc/sindarin/sn.darwin.cfg`, `/opt/homebrew/etc/sindarin/sn.darwin.cfg`
   - **Windows**: `%LOCALAPPDATA%\Sindarin\sn.windows.cfg`, `%ProgramFiles%\Sindarin\sn.windows.cfg`
3. Built-in defaults

### Runtime Library Search Order

The compiler searches for the runtime library archive (`libsn_runtime.a`) in:

1. Next to the compiler executable: `<exe_dir>/lib/<backend>/`
2. FHS-relative: `<exe_dir>/../lib/sindarin/<backend>/`
3. Platform-specific system paths:
   - **Linux**: `/usr/lib/sindarin/<backend>`, `/usr/local/lib/sindarin/<backend>`
   - **macOS**: `/usr/local/lib/sindarin/<backend>`, `/opt/homebrew/lib/sindarin/<backend>`
   - **Windows**: `%ProgramFiles%\Sindarin\lib\<backend>`

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SN_CC` | `gcc` (Linux), `clang` (macOS/Windows) | C compiler |
| `SN_STD` | `c99` | C standard |
| `SN_DEBUG_CFLAGS` | Platform-specific | Debug mode flags |
| `SN_RELEASE_CFLAGS` | `-O3 -flto` | Release mode flags |
| `SN_PROFILE_CFLAGS` | Platform-specific | Profile mode flags (optimized with frame pointers) |
| `SN_CFLAGS` | (empty) | Additional compiler flags |
| `SN_LDFLAGS` | (empty) | Additional linker flags |
| `SN_LDLIBS` | (empty) | Additional libraries |

**Example: Using Clang on Linux:**

```bash
SN_CC=clang bin/sn myprogram.sn -o myprogram
```

**Example: Linking additional libraries:**

```bash
SN_LDLIBS="-lssl -lcrypto" bin/sn myprogram.sn -o myprogram
```

## Troubleshooting

### "Runtime object not found"

The compiler can't find the pre-built runtime library. This can happen if:

1. **Development mode**: Ensure CMake completed successfully and check that `bin/lib/gcc/` (Linux/macOS) or `bin/lib/clang/` (Windows) contains `libsn_runtime.a`.

2. **Installed package**: The compiler searches system paths like `/usr/lib/sindarin/gcc/`. Ensure the package was installed correctly.

### "C compiler not found"

Set `SN_CC` to your compiler path:

```bash
SN_CC=/usr/bin/gcc bin/sn myprogram.sn -o myprogram
```

### Windows: "clang not found"

If using LLVM-MinGW, add to PATH:

```powershell
$env:PATH = "C:\llvm-mingw\llvm-mingw-20241217-ucrt-x86_64\bin;$env:PATH"
```

### macOS: AddressSanitizer errors

ASAN has known issues with signal handling on macOS. Some threading tests are excluded on macOS. To explicitly disable ASAN:

```bash
SN_DEBUG_CFLAGS="-g" bin/sn myprogram.sn -g -o myprogram
```

### Ninja not found

The build system falls back to Unix Makefiles if Ninja isn't installed. For faster builds, install Ninja:

```bash
# Linux (Ubuntu/Debian)
sudo apt-get install ninja-build

# macOS
brew install ninja

# Windows
winget install Ninja-build.Ninja
```

## CI/CD Reference

The project uses GitHub Actions for continuous integration. CI uses the same `make` targets as local development:

```bash
make setup                   # Download pre-built native libraries
make build                   # Configure and build via cmake
make test-unit               # Run unit tests
make test-cgen               # Run code generation tests
make test-mgen               # Run model generation tests
make test-integration        # Run integration tests
make test-integration-errors # Run integration error tests
make test-explore            # Run exploratory tests
make test-explore-errors     # Run exploratory error tests
```

Platform-specific differences are handled via environment variables:

| Platform | `SN_CC` | `SN_CFLAGS` | Notes |
|----------|---------|-------------|-------|
| Linux    | `gcc`   | (none)      | |
| macOS    | `clang` | (none)      | Excludes `test_thread_panic_propagate` |
| Windows  | `clang` | `--target=x86_64-w64-mingw32 -fuse-ld=lld -rtlib=compiler-rt -unwindlib=none` | Requires LLVM-MinGW |

See `.github/workflows/ci.yml` for the full configuration.
