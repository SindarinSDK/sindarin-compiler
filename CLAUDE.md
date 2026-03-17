# Sn Compiler

A statically-typed procedural language that compiles `.sn` → C → executable.

## Build & Run

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

## Architecture

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
│  Code Gen (cgen/gen_model*.c + templates/c/*.hbs)    │
│    → JSON model → split by source file               │
│    → sn_types.h + per-module .c files                │
├─────────────────────────────────────────────────────┤
│  GCC Backend (gcc_backend.c)                         │
│    → compile each .c → .o, link → executable         │
└─────────────────────────────────────────────────────┘
```

Key modules:
- `main.c` → `compiler.c`: Entry point and orchestration
- `cgen/`: Code generation (JSON model building, template rendering, model splitting)
- `symbol_table.c`: Scope and symbol management
- `diagnostic.c`: Error reporting and phase tracking
- `arena.c`: Memory management (compiler internals only)

## Compilation Pipeline

The compiler uses **modular compilation**:

1. Parse all source files into a merged AST
2. Type-check the merged AST
3. Build a JSON model from the AST
4. Split the model by source file (each `.sn` file → its own module)
5. Render `sn_types.h` (shared header) + per-module `.c` files via Handlebars templates
6. Compile each `.c` to `.o` independently
7. Link all `.o` files + runtime library → executable

Build artifacts go to `.sn/build/<source_basename>/`. The `.o` files persist between builds.

## Usage

```bash
bin/sn <source.sn> [-o <executable>] [options]

Output options:
  -o <file>          Specify output executable (default: source_file without extension)
  --emit-c           Output generated C code, don't compile to executable
  --emit-model       Output JSON model, don't generate C
  --keep-c           Keep generated C files after compilation

Debug options:
  -v                 Verbose mode (show compilation steps)
  -g                 Debug build (includes symbols and address sanitizer)
  -p                 Profile build (optimized with frame pointers, no ASAN or LTO)
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
  --clean            Remove build cache (.sn/build/)
  --no-install       Skip automatic dependency installation
```

## Tests

- **Unit:** `tests/unit/*_tests.c` → `bin/tests`
- **Integration:** `tests/integration/*.sn`
- **Exploratory:** `tests/exploratory/*.sn`

## Syntax

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

## IMPORTANT:

 - This project uses ASAN not valgrind. All memory issues are detected using `-fsanitize=address`. Do not introduce valgrind into this project!

 - ALWAYS execute compiled artifacts (both the compiler and sindarin code files) using a `timeout` prefix to avoid crashing the host machine if there is an accidental/unintended infinite loops.

 - ALWAYS debug with output written to the /tmp/ directory, this avoids accidentally comitting generated artifacts which do not form part of the solution.

 - When tests fail, ALWAYS FIX THEM. Do not label them as "pre-existing issue" and ignore.

 - Timeouts for running tests using python3 scripts/run_tests.py are as follows:
   - timeout 60 python3 scripts/run_tests.py unit
   - timeout 60 python3 scripts/run_tests.py cgen
   - timeout 600 python3 scripts/run_tests.py integration
   - timeout 600 python3 scripts/run_tests.py explore
   - timeout 900 python3 scripts/run_tests.py all

 - NEVER implement work arounds, ALWAYS FIX THE PROBLEMS PROPERLY.
 - NEVER ignore test failures as "pre-existing failures", TESTS SHOULD ALWAYS PASS.
 - NEVER generate extern forwards for native sindarin functions without bodies.

## Documentation

Is available at https://sindarinsdk.github.io/.

## CRITICAL: File Editing on Windows

### MANDATORY: Always Use Backslashes on Windows for File Paths

**When using Edit or MultiEdit tools on Windows, you MUST use backslashes (`\`) in file paths, NOT forward slashes (`/`).**

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
git clone git@github.com:SindarinSDK/sindarin-pkg-libs.git
cd sindarin-compiler
```

### Installing the Compiler Globally

```bash
make build && make install && sn --clear-cache
```

**IMPORTANT**: All packages (sindarin-pkg-sdk, sindarin-pkg-http, etc.) use the globally installed compiler. To test compiler changes in other packages, you MUST run `make install`.

### Package Dependencies

Packages declare dependencies in `sn.yaml`:
```yaml
dependencies:
- name: sindarin-pkg-sdk
  git: git@github.com:SindarinSDK/sindarin-pkg-sdk.git
  branch: main
```

Dependencies are installed to `.sn/<package-name>/` within each project directory.
Build cache is stored in `.sn/build/`.

### Workflow: Compiler Changes

```bash
# 1. Build
make build

# 2. Run all tests
timeout 900 make test

# 3. Install globally (REQUIRED to test in other packages)
make install && sn --clear-cache
```

### Workflow: Full Integration Test

```bash
# 1. Build, test, install compiler
make build && timeout 900 make test && make install && sn --clear-cache

# 2. Test SDK
cd ../sindarin-pkg-sdk
rm -rf .sn && sn --install
cp -a ../sindarin-pkg-sdk/* .sn/sindarin-pkg-sdk/
cp -a ../sindarin-pkg-test/* .sn/sindarin-pkg-test/
make test

# 3. Test HTTP package
cd ../sindarin-pkg-http
rm -rf .sn && sn --install
cp -a ../sindarin-pkg-sdk/* .sn/sindarin-pkg-sdk/
cp -a ../sindarin-pkg-test/* .sn/sindarin-pkg-test/
make test

# 4. Return to compiler
cd ../sindarin-compiler
```

### Native Function Codegen

When calling native functions (marked with `native` keyword and `@alias`):
- `str` parameters map to `char *` in generated C
- `int` parameters map to `long long`
- Struct parameters map to `__sn__<StructName>` (value) or `__sn__<StructName> *` (as ref)
- Arrays map to `SnArray *`
- Regular Sindarin functions (not marked `native`) use the same types

### Package Library Discovery

The compiler discovers package libraries by:
1. Reading `sn.yaml` in the current directory
2. For each dependency, checking `.sn/<pkg>/libs/<platform>/include/` and `/lib/`
3. Recursively checking transitive deps in `.sn/<pkg>/.sn/<transitive>/...`
4. Parsing `.pc` files in `lib/pkgconfig/` for additional flags

Platform is one of: `windows`, `darwin`, `linux`
