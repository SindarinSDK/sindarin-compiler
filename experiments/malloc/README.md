# Memory Allocation Hooks Experiment

This experiment intercepts all memory allocation calls (malloc, calloc, realloc, free) in compiled Sindarin programs, including allocations from both statically and dynamically linked libraries.

## Experiment Rules

1. **Isolation**: All changes stay within `experiments/malloc/`
2. **Copy Before Modify**: Files from the main codebase are copied here before modification
3. **Backwards Compatibility**: Modified files remain API-compatible with originals
4. **Build Isolation**: Compiles to `experiments/malloc/bin/`, substituting only changed files
5. **Clear Output**: Hooked functions print diagnostic output identifying the caller

## How the Interceptor Works

All platforms now use **runtime hooking** to intercept memory allocation calls. This means hooks are installed when the program starts (via constructor functions), not at link time.

### Runtime Hooking Libraries

| Platform | Library | Mechanism |
|----------|---------|-----------|
| Linux | [plthook](https://github.com/kubo/plthook) | Modifies PLT/GOT entries in ELF binaries |
| macOS | [fishhook](https://github.com/facebook/fishhook) | Modifies Mach-O symbol pointers |
| Windows | [MinHook](https://github.com/TsudaKageyu/minhook) | Inline function hooking via trampolines |

### Benefits of Runtime Hooking

Unlike link-time hooking (e.g., `--wrap`), runtime hooking:

- **Intercepts ALL allocations** - both static and dynamic libraries
- **Consistent behavior** - same interception regardless of how libraries are linked
- **No special linker flags** - hooks are installed at runtime via constructors
- **Works with any C library** - doesn't depend on how the library was compiled

### How It Works

1. **At program startup**: A constructor function (`sn_install_malloc_hooks`) runs before `main()`
2. **Hook installation**: The hooking library redirects malloc/free/calloc/realloc to our wrapper functions
3. **Original function storage**: Original function pointers are saved for calling through to libc
4. **Logging**: Each allocation/deallocation is logged with caller identification

### Caller Identification

Each allocation log includes the caller's function name:

```
[SN_ALLOC] malloc(72) = 0x55652b12a0  [rt_arena_create_sized]
[SN_ALLOC] malloc(5952) = 0x55652e2450  [deflateInit_]
[SN_ALLOC] free(0x55652e2450)  [deflateEnd]
```

This is achieved using:
- **Linux/macOS**: `__builtin_return_address(0)` + `dladdr()` for symbol lookup
- **Windows**: `__builtin_return_address(0)` + `DbgHelp` API for symbol lookup

## Platform Setup

### Linux (GCC/Clang)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

```
SN_CFLAGS=-g
SN_LDFLAGS=-rdynamic
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols for function name resolution |
| `-rdynamic` | Export all symbols so `dladdr()` can resolve function names |

**Runtime hooking library**: plthook (ELF PLT/GOT modification)

**Build requirements**:
- GCC or Clang
- plthook source (included in `src/runtime/plthook_elf.c`)

### macOS (Clang)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

```
SN_CFLAGS=-g
SN_LDFLAGS=
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols for function name resolution |

**Runtime hooking library**: fishhook (Mach-O symbol rebinding)

**Build requirements**:
- Clang (Xcode command line tools)
- fishhook source (included in `src/runtime/fishhook.c`)

### Windows (Clang/MinGW)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

```
SN_CFLAGS=-g -gcodeview
SN_LDFLAGS=-ldbghelp -Wl,-pdb=
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols |
| `-gcodeview` | Use CodeView debug format (required for DbgHelp) |
| `-Wl,-pdb=` | Generate PDB file alongside executable |
| `-ldbghelp` | Link DbgHelp library for symbol resolution |

**Runtime hooking library**: MinHook (inline function hooking)

**Build requirements**:
- LLVM-MinGW or MinGW-w64 with Clang (NOT MSVC - constructor attribute support needed)
- Windows SDK (for DbgHelp)
- MinHook source (included in `src/runtime/minhook/`)

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

## Platform Comparison

| Feature | Linux | macOS | Windows |
|---------|-------|-------|---------|
| Hooking Mechanism | plthook (PLT/GOT) | fishhook (Mach-O) | MinHook (trampolines) |
| Symbol Resolution | `dladdr()` | `dladdr()` | DbgHelp API |
| Static Libs Captured | **Yes** | **Yes** | **Yes** |
| Dynamic Libs Captured | **Yes** | **Yes** | **Yes** |
| Special Linker Flags | `-rdynamic` | None | `-ldbghelp` |
| Debug Format | DWARF (`-g`) | DWARF (`-g`) | CodeView (`-gcodeview`) |

## Configuration Files

### etc/sn.cfg

This file configures the Sindarin compiler's C backend. The Makefile exports the appropriate `SN_CFLAGS` and `SN_LDFLAGS` based on the detected platform.

### sdk/zlib.sn

Local copy of the zlib SDK module. With runtime hooking, both static and dynamic linking capture all allocations:

```sindarin
@link z          # Dynamic linking - allocations captured!
@link :libz.a    # Static linking - allocations captured!
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

**macOS**: Ensure `-g` flag is set for debug symbols.

**Windows**: Missing debug symbols. Ensure `-g -gcodeview` and `-Wl,-pdb=` are set.

### Recursion or crashes in hooks

The hooks include a thread-local guard (`sn_malloc_hook_guard`) to prevent recursion when `fprintf` itself calls malloc. If you see crashes, ensure the guard is working.

### Hooks not installed

Ensure the runtime library is linked and the constructor is running. The constructor uses `__attribute__((constructor))` which runs before `main()`.

## Files in This Experiment

| File | Purpose |
|------|---------|
| `src/runtime/runtime_malloc_hooks.h` | Hook declarations and documentation |
| `src/runtime/runtime_malloc_hooks.c` | Hook implementations (platform-dispatched) |
| `src/runtime/plthook.h` | plthook library header (Linux) |
| `src/runtime/plthook_elf.c` | plthook library implementation (Linux) |
| `src/runtime/fishhook.h` | fishhook library header (macOS) |
| `src/runtime/fishhook.c` | fishhook library implementation (macOS) |
| `src/runtime/minhook/` | MinHook library (Windows) |
| `src/runtime/runtime_arena.c` | Arena allocator (uses standard malloc, gets intercepted) |
| `etc/sn.cfg` | Compiler config template |
| `sdk/zlib.sn` | zlib SDK module |
| `tests/test_zlib.sn` | Test file using zlib |
| `Makefile` | Build system for the experiment |

---

## Future Direction: Arena-Managed Interop

The runtime hooking implementation provides a foundation for **redirecting all interop allocations through Sindarin's arena system**.

### The Vision

Sindarin's memory model uses arenas with different behaviors based on function call type:

| Call Type | Arena Behavior | Escape Rules |
|-----------|----------------|--------------|
| `default` | New arena, disposed after call | Relaxed - escaping data copied to parent |
| `shared` | Uses parent's arena | No copying needed |
| `private` | New arena, disposed after call | Strict - only primitives escape |

For interop to feel native, C library allocations should follow the same rules. When calling a C function:

```
sindarin_function()
  └── interop_call()        ← C allocations should go into an arena here
        └── malloc()        ← Redirected to arena allocation
```

### Proposed Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Thread-Local Arena Stack                                    │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ main()                    [root arena]                  ││
│  │   └── sindarin_fn()       [function arena]              ││
│  │         └── interop()     [interop arena] ← C mallocs   ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│  Hooked Allocators                                           │
│    malloc(size)  → rt_arena_alloc(current_arena, size)      │
│    free(ptr)     → no-op or mark freed (arena bulk-frees)   │
│    calloc(n, s)  → rt_arena_calloc(current_arena, n, s)     │
│    realloc(p, s) → rt_arena_realloc(current_arena, p, s)    │
├─────────────────────────────────────────────────────────────┤
│  Platform Hooking Layer                                      │
│    Linux:   plthook (ELF PLT/GOT modification)              │
│    macOS:   fishhook (Mach-O symbol rebinding)              │
│    Windows: MinHook (inline trampoline hooking)             │
└─────────────────────────────────────────────────────────────┘
```

### Next Steps

1. **Implement arena stack** - Thread-local stack of arenas for nested interop
2. **Compiler integration** - Generate arena push/pop around interop calls
3. **Escape analysis** - Determine what data escapes from interop calls
4. **Free handling** - Decide between no-op free vs mark-as-freed

---

## License

The following third-party libraries are included:

- **fishhook**: Copyright (c) 2013 Facebook, Inc. BSD 3-Clause License. See `src/runtime/fishhook.h`.
- **plthook**: Copyright (c) 2013-2024 Kubo Takehiro. BSD 2-Clause License. See `src/runtime/plthook.h`.
- **MinHook**: Copyright (c) 2009-2017 Tsuda Kageyu. BSD 2-Clause License. See `src/runtime/minhook/MinHook.h`.
