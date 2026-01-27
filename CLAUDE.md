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
  -o <file>          Output executable (default: source without extension)
  --emit-c           Only output C code, don't compile
  --keep-c           Keep intermediate C file after compilation

Debug options:
  -v                 Verbose mode
  -g                 Debug build (symbols + address sanitizer)
  -l <level>         Log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)

Optimization:
  -O0                No Sn optimization
  -O1                Basic (dead code elimination, string merging)
  -O2                Full (default: + tail call optimization)
  --unchecked        Unchecked arithmetic (no overflow checking)
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
   - timeout 600 python3 scripts/run_tests.py sdk
   - timeout 900 python3 scripts/run_tests.py all

 - NEVER implement work arounds, ALWAYS FIX THE PROBLEMS PROPERLY.
 - NEVER ignore test failures as "pre-existing", TESTS SHOULD ALWAYS PASS. 
 - NEVER generate extern forwards for native sindarin functions without bodies.

## 📖 Documentation

Is available at 'docs/'

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
