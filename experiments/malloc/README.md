# Memory Allocation Hooks Experiment

This experiment intercepts all memory allocation calls (malloc, calloc, realloc, free) in compiled Sindarin programs, including allocations from statically linked libraries like zlib.

## Experiment Rules

1. **Isolation**: All changes stay within `experiments/malloc/`
2. **Copy Before Modify**: Files from the main codebase are copied here before modification
3. **Backwards Compatibility**: Modified files remain API-compatible with originals
4. **Build Isolation**: Compiles to `experiments/malloc/bin/`, substituting only changed files
5. **Clear Output**: Hooked functions print diagnostic output identifying the caller

## How the Interceptor Works

### Linux/Windows: Linker Wrapping

Uses the **linker's `--wrap` flag** to redirect memory allocation calls at link time.

When you link with `--wrap=malloc`:
- All calls to `malloc()` are redirected to `__wrap_malloc()`
- The original `malloc()` becomes available as `__real_malloc()`

This happens at the linker level, meaning **ALL** code in the final executable goes through the wrapper, including the Sindarin runtime and any statically linked libraries.

### macOS: fishhook Runtime Rebinding

Apple's linker does not support `--wrap`. Instead, we use [fishhook](https://github.com/facebook/fishhook), a library from Facebook that rebinds Mach-O symbols at runtime.

fishhook works by:
1. Modifying the lazy/non-lazy symbol pointers in the `__DATA` segment
2. Redirecting calls through a constructor that runs at program startup
3. Storing original function pointers for calling through to libc

**Advantages over DYLD_INSERT_LIBRARIES**:
- No SIP (System Integrity Protection) issues
- Works with dynamically linked libraries
- No environment variables needed
- Single C file to include in the build

### Caller Identification

Each allocation log includes the caller's function name:

```
[SN_ALLOC] malloc(72) = 0x55652b12a0  [rt_arena_create_sized]
[SN_ALLOC] malloc(5952) = 0x55652e2450  [deflateInit_]
```

This is achieved using:
- **Linux/macOS**: `__builtin_return_address(0)` + `dladdr()` for symbol lookup
- **Windows**: `__builtin_return_address(0)` + `DbgHelp` API for symbol lookup

## Static Linking Requirement (Linux/Windows)

On Linux and Windows, **the `--wrap` flag only intercepts statically linked code.**

Dynamically linked libraries (`.so`, `.dll`) resolve their malloc calls directly to libc at runtime, completely bypassing the wrapper.

| Linking Type | SDK Directive | What Gets Captured |
|--------------|---------------|-------------------|
| Dynamic | `@link z` | Only Sindarin runtime |
| **Static** | `@link :libz.a` | **Everything** (runtime + zlib) |

The experiment includes a local `sdk/zlib.sn` configured for static linking.

**Note**: On macOS, fishhook intercepts both static and dynamic library calls because it rebinds symbols at runtime after all libraries are loaded.

## Platform Setup

### Linux (GCC/Clang)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

```
SN_CFLAGS=-g
SN_LDFLAGS=-rdynamic -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols for function name resolution |
| `-rdynamic` | Export all symbols so `dladdr()` can resolve function names |
| `-Wl,--wrap=malloc` | Redirect `malloc()` calls to `__wrap_malloc()` |

**Runtime dependency**: Links with `-ldl` for `dladdr()` symbol resolution.

**Build requirements**:
- GCC or Clang
- Static library for any SDK dependency you want to intercept (e.g., `libz.a`)

### macOS (Clang)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

```
SN_CFLAGS=-g
SN_LDFLAGS=
```

| Flag | Purpose |
|------|---------|
| `-g` | Generate debug symbols for function name resolution |

No special linker flags are needed - fishhook installs hooks at runtime via a constructor.

**Build requirements**:
- Clang (Xcode command line tools)
- fishhook source files (included in this experiment)

**How fishhook is integrated**:
- `src/runtime/fishhook.h` and `src/runtime/fishhook.c` are included in the build
- A constructor function (`sn_install_malloc_hooks`) runs at program startup
- Hooks are installed before `main()` executes

### Windows (Clang/MinGW)

**Compiler flags** (set via Makefile → `etc/sn.cfg`):

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
| Hooking Mechanism | `--wrap` linker flag | fishhook runtime rebinding | `--wrap` linker flag |
| Symbol Resolution | `dladdr()` | `dladdr()` | DbgHelp API |
| Static Libs Required | Yes (for full capture) | No | Yes (for full capture) |
| Dynamic Libs Captured | No | **Yes** | No |
| Special Linker Flags | `-Wl,--wrap=...` | None | `-Wl,--wrap=...` |
| Debug Format | DWARF (`-g`) | DWARF (`-g`) | CodeView (`-gcodeview`) |

## Configuration Files

### etc/sn.cfg

This file configures the Sindarin compiler's C backend. The Makefile exports the appropriate `SN_CFLAGS` and `SN_LDFLAGS` based on the detected platform.

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

**macOS**: Ensure `-g` flag is set for debug symbols.

**Windows**: Missing debug symbols. Ensure `-g -gcodeview` and `-Wl,-pdb=` are set.

### Library allocations not captured (Linux/Windows)

The library is dynamically linked. Change from `@link z` to `@link :libz.a` in the SDK module.

### Recursion or crashes in hooks

The hooks include a thread-local guard (`sn_malloc_hook_guard`) to prevent recursion when `fprintf` itself calls malloc. If you see crashes, ensure the guard is working.

### macOS: hooks not intercepting some calls

fishhook only works for external symbols resolved through the dynamic linker. Direct calls within the same compilation unit won't be intercepted. This is rarely an issue in practice since malloc/free are always external.

## Files in This Experiment

| File | Purpose |
|------|---------|
| `src/runtime/runtime_malloc_hooks.h` | Platform-specific hook declarations |
| `src/runtime/runtime_malloc_hooks.c` | Hook implementations (uses fishhook on macOS, `__wrap_*` elsewhere) |
| `src/runtime/fishhook.h` | Facebook's fishhook library header (macOS only) |
| `src/runtime/fishhook.c` | Facebook's fishhook library implementation (macOS only) |
| `src/runtime/runtime_arena.c` | Arena allocator (uses standard malloc, gets intercepted) |
| `etc/sn.cfg` | Compiler config template |
| `sdk/zlib.sn` | zlib SDK module with static linking |
| `tests/test_zlib.sn` | Test file using zlib |
| `Makefile` | Build system for the experiment |

---

## Future Direction: Runtime Hooking for Arena-Managed Interop

The current implementation uses a hybrid approach (`--wrap` on Linux/Windows, fishhook on macOS). While this works for observing allocations, the ultimate goal is to **redirect all interop allocations through Sindarin's arena system**.

### The Problem with Link-Time Hooking

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

**The `--wrap` approach fails here** because:
- It only works with statically linked libraries
- Users may link against dynamic libraries (`.so`, `.dll`)
- Behavior becomes unpredictable based on how libraries are linked

### The Runtime Hooking Solution

Runtime hooking intercepts **all** allocations regardless of static/dynamic linking, providing consistent behavior.

| Platform | Library | How It Works |
|----------|---------|--------------|
| Linux | [plthook](https://github.com/kubo/plthook) | Modifies PLT/GOT entries in ELF binaries at runtime |
| macOS | [fishhook](https://github.com/facebook/fishhook) | Modifies lazy symbol pointers in Mach-O `__DATA` segment |
| Windows | [MinHook](https://github.com/TsudaKageworku/minhook) or [Detours](https://github.com/microsoft/Detours) | Inline hooking via trampoline functions |

### Why Runtime Hooking is Better

| Requirement | `--wrap` (Link-time) | Runtime Hooking |
|-------------|---------------------|-----------------|
| Catches static lib allocations | Yes | Yes |
| Catches dynamic lib allocations | **No** | **Yes** |
| Predictable behavior | Depends on linking | Always consistent |
| Works with any C library | Only if statically linked | **Yes** |
| Suitable for production interop | Fragile | **Robust** |

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

### Interop Call Flow

**Before interop call (compiler-generated):**
```c
// Push new interop arena
RtArena *interop_arena = rt_arena_create(parent_arena);
rt_push_interop_arena(interop_arena);
```

**During interop call:**
```c
// C library calls malloc()
void *ptr = malloc(size);
// Hook redirects to: rt_arena_alloc(current_interop_arena, size)
```

**After interop call (compiler-generated):**
```c
// Pop arena and apply escape rules
rt_pop_interop_arena();

if (return_value_escapes) {
    // Copy to parent arena (like default Sindarin calls)
    result = rt_arena_promote(parent_arena, result, size);
}

rt_arena_destroy(interop_arena);  // Bulk-free all C allocations
```

### Benefits for Sindarin Developers

1. **Uniform mental model** - Interop calls behave like Sindarin calls
2. **Automatic cleanup** - No manual memory management for C library results
3. **Escape analysis** - Returned data automatically promoted to parent arena
4. **No leaks** - Arena destruction catches anything the C library forgot to free

### Implementation Considerations

**Handling `free()` from C code:**

C libraries often allocate and free internally. Options:
- **No-op free**: Arena bulk-frees everything. Simple, slightly more memory usage.
- **Mark-as-freed**: Track freed pointers, exclude from escape analysis.

**Stateful interop (e.g., zlib streams):**

Some C libraries maintain state across calls. Use `shared` semantics:
```sindarin
// Stream lives in parent arena, persists across calls
var stream: ZlibStream = zlib.createStream() @shared
stream.compress(data1)
stream.compress(data2)
stream.end()
```

**Pointer identity:**

When data is copied to parent arena (escape), pointers change. For complex structures with internal pointers, either:
- Use `shared` to avoid copying
- Implement deep copy with pointer remapping

### Platform-Specific Implementation Notes

**Linux (plthook):**
```c
#include "plthook.h"

plthook_t *plthook;
plthook_open(&plthook, NULL);  // Hook current executable
plthook_replace(plthook, "malloc", hooked_malloc, (void**)&orig_malloc);
plthook_replace(plthook, "free", hooked_free, (void**)&orig_free);
// ... calloc, realloc
```

**macOS (fishhook):** Already implemented in this experiment.

**Windows (MinHook):**
```c
#include "MinHook.h"

MH_Initialize();
MH_CreateHook(&malloc, hooked_malloc, (void**)&orig_malloc);
MH_CreateHook(&free, hooked_free, (void**)&orig_free);
// ... calloc, realloc
MH_EnableHook(MH_ALL_HOOKS);
```

### Next Steps

1. **Add plthook for Linux** - Replace `--wrap` with runtime hooking
2. **Add MinHook for Windows** - Replace `--wrap` with runtime hooking
3. **Implement arena stack** - Thread-local stack of arenas for nested interop
4. **Compiler integration** - Generate arena push/pop around interop calls
5. **Escape analysis** - Determine what data escapes from interop calls

---

## License

The fishhook library is Copyright (c) 2013 Facebook, Inc. and is distributed under the BSD 3-Clause License. See `src/runtime/fishhook.h` for the full license text.
