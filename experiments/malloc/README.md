# Memory Allocation Hooks Experiment

This experiment intercepts all memory allocation calls (malloc, calloc, realloc, free) in compiled Sindarin programs, including allocations from statically linked libraries like zlib.

## Experiment Rules

1. **Isolation**: All changes stay within `experiments/malloc/`
2. **Copy Before Modify**: Files from the main codebase are copied here before modification
3. **Backwards Compatibility**: Modified files remain API-compatible with originals
4. **Build Isolation**: Compiles to `experiments/malloc/bin/`, substituting only changed files
5. **Clear Output**: Hooked functions print diagnostic output identifying the caller

## How the Interceptor Works

The interceptor uses the **linker's `--wrap` flag** to redirect memory allocation calls at link time.

When you link with `--wrap=malloc`:
- All calls to `malloc()` are redirected to `__wrap_malloc()`
- The original `malloc()` becomes available as `__real_malloc()`

This happens at the linker level, meaning:
- **ALL** code in the final executable goes through the wrapper
- This includes the Sindarin runtime AND any statically linked libraries
- No source code modifications are needed in the libraries

### Caller Identification

Each allocation log includes the caller's function name:

```
[SN_ALLOC] malloc(72) = 0x55652b12a0  [rt_arena_create_sized]
[SN_ALLOC] malloc(5952) = 0x55652e2450  [deflateInit_]
```

This is achieved using:
- **Linux**: `__builtin_return_address(0)` + `dladdr()` for symbol lookup
- **Windows**: `__builtin_return_address(0)` + `DbgHelp` API for symbol lookup

## Static Linking Requirement

**The `--wrap` flag only intercepts statically linked code.**

Dynamically linked libraries (`.so`, `.dylib`, `.dll`) resolve their malloc calls directly to libc at runtime, completely bypassing the wrapper.

| Linking Type | SDK Directive | What Gets Captured |
|--------------|---------------|-------------------|
| Dynamic | `@link z` | Only Sindarin runtime |
| **Static** | `@link :libz.a` | **Everything** (runtime + zlib) |

The experiment includes a local `sdk/zlib.sn` configured for static linking.

## Platform Setup

### Linux (GCC/Clang)

**Compiler flags** (set in `etc/sn.cfg`):

```
SN_CFLAGS=-g
SN_LDFLAGS=-rdynamic -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols for function name resolution |
| `-rdynamic` | Export all symbols so `dladdr()` can resolve function names |
| `-Wl,--wrap=malloc` | Redirect `malloc()` calls to `__wrap_malloc()` |

**Runtime dependency**: The `-ldl` flag links libdl for `dladdr()` symbol resolution.

**Build requirements**:
- GCC or Clang
- Static library for any SDK dependency you want to intercept (e.g., `libz.a`)

### Windows (Clang/MinGW)

**Compiler flags** (set in `etc/sn.cfg`):

```
SN_CFLAGS=-g -gcodeview
SN_LDFLAGS=-ldbghelp -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc -Wl,-pdb=
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols |
| `-gcodeview` | Use CodeView debug format (required for DbgHelp) |
| `-Wl,-pdb=` | Generate PDB file alongside executable |
| `-ldbghelp` | Link DbgHelp library for symbol resolution |
| `-Wl,--wrap=malloc` | Redirect `malloc()` calls to `__wrap_malloc()` |

**Build requirements**:
- LLVM-MinGW or MinGW-w64 with Clang (NOT MSVC - it lacks `--wrap` support)
- Windows SDK (for DbgHelp)
- Static libraries for SDK dependencies (e.g., `zlib.lib` or `libz.a`)

**Installing dependencies** (using vcpkg):
```bash
vcpkg install zlib:x64-mingw-static
```

### macOS

**The Apple linker does not support `--wrap`.**

For macOS, use the `DYLD_INSERT_LIBRARIES` approach instead:

1. Build a shared library containing hook functions:
```bash
clang -shared -fPIC -o libhooks.dylib malloc_hooks.c
```

2. Run your program with the library preloaded:
```bash
DYLD_INSERT_LIBRARIES=./libhooks.dylib ./your_program
```

The hooks use dyld interposition (see `__DATA,__interpose` section) to redirect calls.

**Limitations**:
- System Integrity Protection (SIP) blocks `DYLD_INSERT_LIBRARIES` for system binaries
- Must be disabled or work with non-system executables only
- Interposition affects ALL loaded code (similar to static linking benefit)

## Building the Experiment

```bash
cd experiments/malloc

# Build compiler and runtime with hooks
make build

# Run the test (compiles tests/test_zlib.sn and runs it)
make test

# Run samples/main.sn
make run

# Clean build artifacts
make clean
```

## Configuration Files

### etc/sn.cfg

This file configures the Sindarin compiler's C backend. Key settings for the hooks:

```
# Compiler flags - debug symbols for caller name resolution
SN_CFLAGS=-g

# Linker flags - wrap memory functions
SN_LDFLAGS=-rdynamic -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
```

### sdk/zlib.sn

Local copy of the zlib SDK module configured for static linking:

```sindarin
@link :libz.a    # Colon prefix = static library
```

Compare to the standard SDK which uses dynamic linking:
```sindarin
@link z          # No colon = dynamic library (-lz)
```

## Runtime Flags

The hooks are compiled into the runtime when `SN_MALLOC_HOOKS` is defined:

```makefile
CFLAGS += -DSN_MALLOC_HOOKS
```

This is set automatically in the experiment's Makefile.

## Expected Output

When running a program that uses zlib:

```
[SN_ALLOC] malloc(72) = 0x5565282b12a0  [rt_arena_create_sized]
[SN_ALLOC] malloc(65560) = 0x5565282b12f0  [rt_arena_new_block]
[SN_ALLOC] malloc(5952) = 0x5565282e2450  [deflateInit_]
[SN_ALLOC] malloc(65536) = 0x5565282e3ba0  [deflateInit_]
[SN_ALLOC] malloc(7160) = 0x5565282f3c00  [inflateInit_]
[SN_ALLOC] free(0x5565282e2450)  [deflateEnd]
[SN_ALLOC] free(0x5565282e3ba0)  [deflateEnd]
[SN_ALLOC] free(0x5565282b12f0)  [rt_arena_destroy]
[SN_ALLOC] free(0x5565282b12a0)  [rt_arena_destroy]
```

**Identifying the source by function name**:

| Function Name | Source |
|---------------|--------|
| `rt_arena_*` | Sindarin runtime |
| `rt_string_*` | Sindarin runtime |
| `deflateInit_`, `deflateEnd` | zlib compression |
| `inflateInit_`, `inflateEnd` | zlib decompression |

**Identifying the source by allocation size**:

| Size | Typical Source |
|------|----------------|
| 72 | Sindarin arena struct |
| 65560 | Sindarin arena block (64KB + header) |
| 5952 | zlib deflate state |
| 7160 | zlib inflate state |
| 65536 | zlib internal buffers |

## Troubleshooting

### Function names show "???"

**Linux**: Missing `-rdynamic` flag. This exports symbols so `dladdr()` can resolve them.

**Windows**: Missing debug symbols. Ensure `-g -gcodeview` and `-Wl,-pdb=` are set.

### Library allocations not captured

The library is dynamically linked. Change from `@link z` to `@link :libz.a` in the SDK module.

### Recursion or crashes in hooks

The hooks include a thread-local guard (`sn_malloc_hook_guard`) to prevent recursion when `fprintf` itself calls malloc. If you see crashes, ensure the guard is working.

### macOS: hooks not working

Apple's linker doesn't support `--wrap`. Use `DYLD_INSERT_LIBRARIES` with interposition instead.

## Files in This Experiment

| File | Purpose |
|------|---------|
| `src/runtime/runtime_malloc_hooks.h` | Declares `__wrap_*` and `__real_*` functions |
| `src/runtime/runtime_malloc_hooks.c` | Implements hooks with caller name resolution |
| `src/runtime/runtime_arena.c` | Arena allocator (uses standard malloc, gets intercepted) |
| `etc/sn.cfg` | Compiler config with `--wrap` linker flags |
| `sdk/zlib.sn` | zlib SDK module with static linking |
| `tests/test_zlib.sn` | Test file using zlib |
| `Makefile` | Build system for the experiment |
