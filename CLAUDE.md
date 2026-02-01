# 🐍 Sn Compiler

A statically-typed procedural language that compiles `.sn` → C → executable.

## 🔨 Build & Run

```bash
make build            # Build compiler and test binary
make run              # Compile and run samples/main.sn
make test             # All tests (unit + integration + exploratory)
make test-unit        # Unit tests only
make test-integration # Integration tests only
make test-explore     # Exploratory tests only
make clean            # Remove build artifacts
make help             # Show all targets
```

Binaries: `bin/sn` (compiler), `bin/tests` (unit test runner)

## 🏗️ Architecture

```
Source (.sn)
    ↓
┌─────────────────────────────────────────────────────┐
│  Lexer (lexer.c, lexer_scan.c, lexer_util.c)        │
│    → tokens                                          │
├─────────────────────────────────────────────────────┤
│  Parser (parser_stmt.c, parser_expr.c, parser_util.c)│
│    → AST                                             │
├─────────────────────────────────────────────────────┤
│  Type Checker (type_checker_stmt.c, _expr.c, _util.c)│
│    → typed AST                                       │
├─────────────────────────────────────────────────────┤
│  Optimizer (optimizer.c)                             │
│    → optimized AST                                   │
├─────────────────────────────────────────────────────┤
│  Code Gen (code_gen.c, _stmt.c, _expr.c, _util.c)   │
│    → C code                                          │
├─────────────────────────────────────────────────────┤
│  GCC Backend (gcc_backend.c)                         │
│    → executable                                      │
└─────────────────────────────────────────────────────┘
```

Key modules:
- `main.c` → `compiler.c`: Entry point and orchestration
- `symbol_table.c`: Scope and symbol management
- `runtime.c/h`: Built-in functions and types
- `diagnostic.c`: Error reporting and phase tracking
- `arena.c`: Memory management

## ⚙️ Usage

```bash
bin/sn <source.sn> [-o <executable>] [options]

Output options:
  -o <file>          Specify output executable (default: source without extension)
  --emit-c           Only output C code, don't compile to executable
  --keep-c           Keep intermediate C file after compilation

Debug options:
  -v                 Verbose mode (show compilation steps)
  -g                 Debug build (includes symbols and address sanitizer)
  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)

Code generation options:
  --checked          Force checked arithmetic (overflow detection, slower)
  --unchecked        Force unchecked arithmetic (no overflow checking, faster)
  -O0                No Sn optimization (for debugging)
  -O1                Basic Sn optimizations (dead code elimination, string merging)
  -O2                Full Sn optimizations (default: + tail call, unchecked arithmetic)

Help:
  -h, --help         Show this help message
  --version          Show version information

Package management:
  --init             Initialize a new project (creates sn.yaml)
  --install          Install dependencies from sn.yaml
  --install <url>    Install a package (e.g., https://github.com/user/lib.git@v1.0)
  --clear-cache      Clear the package cache (~/.sn-cache)
  --no-install       Skip automatic dependency installation
```

## 🧪 Tests

- **Unit:** `tests/unit/*_tests.c` → `bin/tests`
- **Integration:** `tests/integration/*.sn`
- **Exploratory:** `tests/exploratory/*.sn`

## 📚 Syntax

```
fn add(a: int, b: int): int => a + b
var x: int = 42
var b: byte = 255
if cond => ... else => ...
match expr => value => body, else => body
$"Hello {name}"
```

Types: `int`, `long`, `double`, `str`, `char`, `bool`, `byte`, `void`

Built-in types: `TextFile`, `BinaryFile`, `Date`, `Time`, `Process`

## 🚨 IMPORTANT: 
 
 - This project uses ASAN not valgrind. All memory issues are detected using `-fsanitize=address`. Do not introduce valgrind into this project!

 - ALWAYS execute compiled artifacts (both the compiler and sindarin code files) using a `timeout` prefix to avoid crashing the host machine if there is an accidental/unintended infinite loops.

 - ALWAYS debug with output written to the /tmp/ directory, this avoids accidentally comitting generated artifacts which do not form part of the solution. 

 - When tests fail, ALWAYS FIX THEM. Do not label them as "pre-existing issue" and ignore.

 - Timeouts for running tests using python3 scripts/run_tests.py are as follows:
   - timeout 60 python3 scripts/run_tests.py unit
   - timeout 600 python3 scripts/run_tests.py integration
   - timeout 600 python3 scripts/run_tests.py explore
   - timeout 900 python3 scripts/run_tests.py all

 - NEVER implement work arounds, ALWAYS FIX THE PROBLEMS PROPERLY.
 - NEVER ignore test failures as "pre-existing", TESTS SHOULD ALWAYS PASS. 
 - NEVER generate extern forwards for native sindarin functions without bodies.

## 📖 Documentation

Is available at https://sindarinsdk.github.io/.

## CRITICAL: File Editing on Windows

### ⚠️ MANDATORY: Always Use Backslashes on Windows for File Paths

**When using Edit or MultiEdit tools on Windows, you MUST use backslashes (`\`) in file paths, NOT forward slashes (`/`).**

#### ❌ WRONG - Will cause errors:
```
Edit(file_path: "tests/cgen/escape_default_3_loop_chained.sn", ...)
MultiEdit(file_path: "tests/cgen/escape_default_3_loop_chained.sn", ...)
```

#### ✅ CORRECT - Always works:
```
Edit(file_path: ".\tests\cgen\escape_default_3_loop_chained.sn", ...)
MultiEdit(file_path: ".\tests\cgen\escape_default_3_loop_chained.sn", ...)
```

## Sindarin Ecosystem Development

This section describes workflows for developing across the Sindarin ecosystem.
All paths are relative to the `sindarin-compiler` repository root.

### Repository Structure

```
../                            # Parent directory
├── sindarin-compiler/         # This repo - the Sindarin compiler (sn)
├── sindarin-pkg-sdk/          # Standard library SDK package
├── sindarin-pkg-http/         # HTTP server package (depends on SDK)
└── sindarin-pkg-libs/         # Pre-built native libraries (yyjson, libxml2, etc.)
```

#### Cloning Sibling Repositories

If the sibling repositories don't exist, clone them into the parent directory:

```bash
cd ..
git clone git@github.com:SindarinSDK/sindarin-pkg-sdk.git
git clone git@github.com:SindarinSDK/sindarin-pkg-http.git
git clone git@github.com:SindarinSDK/sindarin-pkg-libs-v2.git sindarin-pkg-libs
cd sindarin-compiler
```

### Global Compiler Location

The globally installed compiler lives at:

Linux:
```
~/.sn/bin/sn
```

Windows:
```
~/.sn/bin/sn.exe
```

**IMPORTANT**: All packages (sindarin-pkg-sdk, sindarin-pkg-http, etc.) use the globally installed compiler. To test compiler changes in other packages, you MUST copy the built compiler to the global location:

```bash
# From sindarin-compiler root
cp -r bin/* ~/.sn/bin/
```

Without this step, other packages will continue using the old compiler.

### Package Dependencies

Packages declare dependencies in `sn.yaml`:
```yaml
dependencies:
- name: sindarin-pkg-sdk
  git: git@github.com:SindarinSDK/sindarin-pkg-sdk.git
  branch: main
```

Dependencies are installed to `.sn/<package-name>/` within each project directory.
Transitive dependencies are nested: `.sn/<pkg>/.sn/<transitive-pkg>/`

### Workflow: Compiler Changes

When making changes to the compiler:

```bash
# 1. Make changes to source files

# 2. Build
make clean && make build

# 3. Run all tests (use timeout to prevent infinite loops)
timeout 900 make test

# 4. Copy to global location (REQUIRED to test in other packages)
cp -r bin/* ~/.sn/bin/

# 5. Commit and push
git add -A && git commit -m "Description" && git push origin main

# 6. Tag release (check latest tag first)
git tag --list --sort=-v:refname | head -5
git tag -a v0.0.XX-alpha -m "Release notes" && git push origin v0.0.XX-alpha
```

### Workflow: Testing Compiler Changes in Other Packages

To test compiler changes in sindarin-pkg-sdk or sindarin-pkg-http:

```bash
# 1. Build the compiler
make clean && make build

# 2. CRITICAL: Overwrite the global compiler installation
cp -r bin/* ~/.sn/bin/

# 3. Test in SDK package
cd ../sindarin-pkg-sdk
make test

# 4. Reinstall and test HTTP package
cd ../sindarin-pkg-http
sn --clear-cache && rm -rf ./.sn && sn --install
make test

# 5. Return to compiler directory
cd ../sindarin-compiler
```

The global compiler at `~/.sn/bin/` is what `sn` resolves to when running commands. If you don't copy the built compiler there, other packages will use the old version.

### Workflow: SDK Changes

When making changes to `sindarin-pkg-sdk`:

```bash
cd ../sindarin-pkg-sdk

# 1. Make changes to source files (.sn and .sn.c files)

# 2. Run SDK tests
make test

# 3. Commit and push
git add -A && git commit -m "Description" && git push origin main

cd ../sindarin-compiler
```

After pushing SDK changes, dependent packages need to reinstall:
```bash
cd ../sindarin-pkg-http
sn --clear-cache
rm -rf ./.sn
sn --install
cd ../sindarin-compiler
```

### Workflow: HTTP Package Changes

When making changes to `sindarin-pkg-http`:

```bash
cd ../sindarin-pkg-http

# 1. Make changes

# 2. Run tests
make test

# 3. Commit and push
git add -A && git commit -m "Description" && git push origin main

cd ../sindarin-compiler
```

### Workflow: Full Integration Test

To test changes across all repositories:

```bash
# 1. Build and test compiler
make clean && make build && timeout 900 make test
cp -r bin/* ~/.sn/bin/

# 2. Test SDK
cd ../sindarin-pkg-sdk
make test

# 3. Reinstall and test HTTP package
cd ../sindarin-pkg-http
sn --clear-cache && rm -rf ./.sn && sn --install
make test

# 4. Return to compiler
cd ../sindarin-compiler
```

### Runtime Include Paths

Native C files (`.sn.c`) that use runtime headers must use correct paths:
- `#include "runtime/array/runtime_array.h"` (NOT `runtime/runtime_array.h`)
- `#include "runtime/string/runtime_string_h.h"` (NOT `runtime/runtime_string_h.h`)
- `#include "runtime/arena/managed_arena.h"`

### Native Function Codegen

When calling native functions (marked with `native` keyword and `@alias`):
- `str` parameters are converted from `RtHandle` to `const char*` via `rt_managed_pin()`
- `str[]` parameters are converted via `rt_managed_pin_string_array()`
- Regular Sindarin functions (not marked `native`) do NOT get this conversion

### Package Library Discovery

The compiler discovers package libraries by:
1. Reading `sn.yaml` in the current directory
2. For each dependency, checking `.sn/<pkg>/libs/<platform>/include/` and `/lib/`
3. Recursively checking transitive deps in `.sn/<pkg>/.sn/<transitive>/...`
4. Parsing `.pc` files in `lib/pkgconfig/` for additional flags

Platform is one of: `windows`, `darwin`, `linux`
