# Memory Management Hooks Experiment

This experiment explores platform-specific hooks that override memory management functions (malloc, calloc, realloc, free) to add instrumentation while delegating to the original implementations.

## Experiment Rules

These rules govern how experiments are conducted to maintain isolation from the main codebase:

1. **Isolation**: All experiment changes stay within `experiments/malloc/`
2. **Copy Before Modify**: If files outside the experiment need changes, copy them into the experiment folder first
3. **Backwards Compatibility**: Modified files (e.g., `runtime_arena.h`, `runtime_arena.c`) must remain API-compatible with the originals
4. **Build Isolation**: The experiment Makefile compiles to `experiments/malloc/bin/`, substituting only the changed arena files
5. **Clear Output**: Hooked functions must print clear diagnostic output before delegating to original implementations
6. **Minimal Changes**: The main Makefile/CMakeLists.txt should not be modified; build changes go in `experiments/malloc/Makefile`

## Platform-Specific Memory Hooking Techniques

### Linux

Linux offers several approaches for hooking memory allocation:

#### 1. LD_PRELOAD (Recommended for instrumentation)

The simplest and most portable approach on Linux. Create a shared library that defines malloc/free/etc. and preload it.

```c
// malloc_hook.c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Function pointers to original implementations
static void *(*real_malloc)(size_t) = NULL;
static void (*real_free)(void *) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;

// Initialization - resolve original functions
static void init_hooks(void) {
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        real_free = dlsym(RTLD_NEXT, "free");
        real_calloc = dlsym(RTLD_NEXT, "calloc");
        real_realloc = dlsym(RTLD_NEXT, "realloc");
    }
}

void *malloc(size_t size) {
    init_hooks();
    void *ptr = real_malloc(size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

void free(void *ptr) {
    init_hooks();
    fprintf(stderr, "[HOOK] free(%p)\n", ptr);
    real_free(ptr);
}

void *calloc(size_t count, size_t size) {
    init_hooks();
    void *ptr = real_calloc(count, size);
    fprintf(stderr, "[HOOK] calloc(%zu, %zu) = %p\n", count, size, ptr);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    init_hooks();
    void *new_ptr = real_realloc(ptr, size);
    fprintf(stderr, "[HOOK] realloc(%p, %zu) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}
```

**Build and use:**
```bash
gcc -shared -fPIC -o libmalloc_hook.so malloc_hook.c -ldl
LD_PRELOAD=./libmalloc_hook.so ./your_program
```

#### 2. Compile-Time Macro Replacement

Define macros that replace malloc/free at compile time:

```c
// hook_macros.h
#ifndef HOOK_MACROS_H
#define HOOK_MACROS_H

#include <stdio.h>
#include <stdlib.h>

static inline void *hooked_malloc(size_t size, const char *file, int line) {
    void *ptr = malloc(size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p at %s:%d\n", size, ptr, file, line);
    return ptr;
}

static inline void hooked_free(void *ptr, const char *file, int line) {
    fprintf(stderr, "[HOOK] free(%p) at %s:%d\n", ptr, file, line);
    free(ptr);
}

#define malloc(size) hooked_malloc(size, __FILE__, __LINE__)
#define free(ptr) hooked_free(ptr, __FILE__, __LINE__)

#endif
```

#### 3. GNU Malloc Hooks (Deprecated but available)

```c
#include <malloc.h>

static void *(*old_malloc_hook)(size_t, const void *);
static void (*old_free_hook)(void *, const void *);

static void *my_malloc_hook(size_t size, const void *caller) {
    void *result;
    __malloc_hook = old_malloc_hook;
    result = malloc(size);
    old_malloc_hook = __malloc_hook;
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, result);
    __malloc_hook = my_malloc_hook;
    return result;
}

__attribute__((constructor))
static void init_hooks(void) {
    old_malloc_hook = __malloc_hook;
    __malloc_hook = my_malloc_hook;
}
```

**Note:** `__malloc_hook` is deprecated in glibc and not thread-safe.

---

### macOS

macOS provides different mechanisms for memory hooking:

#### 1. DYLD_INSERT_LIBRARIES (Similar to LD_PRELOAD)

```c
// malloc_hook_macos.c
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Use interposition to hook functions
typedef struct {
    const void *replacement;
    const void *replacee;
} interpose_t;

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t count, size_t size);
void *my_realloc(void *ptr, size_t size);

// Interposition array - tells dyld to replace functions
__attribute__((used, section("__DATA,__interpose")))
static const interpose_t interposers[] = {
    { (const void *)my_malloc, (const void *)malloc },
    { (const void *)my_free, (const void *)free },
    { (const void *)my_calloc, (const void *)calloc },
    { (const void *)my_realloc, (const void *)realloc },
};

void *my_malloc(size_t size) {
    void *ptr = malloc(size);  // Calls original due to interposition
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

void my_free(void *ptr) {
    fprintf(stderr, "[HOOK] free(%p)\n", ptr);
    free(ptr);  // Calls original
}

void *my_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    fprintf(stderr, "[HOOK] calloc(%zu, %zu) = %p\n", count, size, ptr);
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    fprintf(stderr, "[HOOK] realloc(%p, %zu) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}
```

**Build and use:**
```bash
clang -shared -fPIC -o libmalloc_hook.dylib malloc_hook_macos.c
DYLD_INSERT_LIBRARIES=./libmalloc_hook.dylib ./your_program
```

**Note:** On macOS 10.11+ with SIP enabled, `DYLD_INSERT_LIBRARIES` doesn't work for system binaries.

#### 2. malloc_zone API (macOS-specific)

macOS uses malloc zones for memory management. You can create a custom zone:

```c
#include <malloc/malloc.h>

static malloc_zone_t *default_zone;
static void *(*original_malloc)(malloc_zone_t *, size_t);

static void *hooked_malloc(malloc_zone_t *zone, size_t size) {
    void *ptr = original_malloc(zone, size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

__attribute__((constructor))
static void init_hooks(void) {
    default_zone = malloc_default_zone();

    // Save original and replace
    original_malloc = default_zone->malloc;
    default_zone->malloc = hooked_malloc;
}
```

---

### Windows

Windows requires different techniques depending on the runtime:

#### 1. Detours (Microsoft Research Library)

Microsoft Detours is a professional-grade hooking library:

```c
#include <windows.h>
#include <detours.h>
#include <stdio.h>

// Pointers to original functions
static void *(*TrueMalloc)(size_t) = malloc;
static void (*TrueFree)(void *) = free;
static void *(*TrueCalloc)(size_t, size_t) = calloc;
static void *(*TrueRealloc)(void *, size_t) = realloc;

void *HookedMalloc(size_t size) {
    void *ptr = TrueMalloc(size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

void HookedFree(void *ptr) {
    fprintf(stderr, "[HOOK] free(%p)\n", ptr);
    TrueFree(ptr);
}

void *HookedCalloc(size_t count, size_t size) {
    void *ptr = TrueCalloc(count, size);
    fprintf(stderr, "[HOOK] calloc(%zu, %zu) = %p\n", count, size, ptr);
    return ptr;
}

void *HookedRealloc(void *ptr, size_t size) {
    void *new_ptr = TrueRealloc(ptr, size);
    fprintf(stderr, "[HOOK] realloc(%p, %zu) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueMalloc, HookedMalloc);
        DetourAttach(&(PVOID&)TrueFree, HookedFree);
        DetourAttach(&(PVOID&)TrueCalloc, HookedCalloc);
        DetourAttach(&(PVOID&)TrueRealloc, HookedRealloc);
        DetourTransactionCommit();
    } else if (reason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueMalloc, HookedMalloc);
        DetourDetach(&(PVOID&)TrueFree, HookedFree);
        DetourDetach(&(PVOID&)TrueCalloc, HookedCalloc);
        DetourDetach(&(PVOID&)TrueRealloc, HookedRealloc);
        DetourTransactionCommit();
    }
    return TRUE;
}
```

#### 2. Import Address Table (IAT) Hooking

Modify the Import Address Table to redirect function calls:

```c
#include <windows.h>
#include <stdio.h>

static void *(*OriginalMalloc)(size_t) = NULL;

void *HookedMalloc(size_t size) {
    void *ptr = OriginalMalloc(size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

// Patch IAT for a specific module
void HookIAT(HMODULE module, const char *targetDll,
             const char *funcName, void *hookFunc, void **original) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)module;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE *)module + dosHeader->e_lfanew);

    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(
        (BYTE *)module + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
    );

    while (importDesc->Name) {
        const char *dllName = (const char *)((BYTE *)module + importDesc->Name);
        if (_stricmp(dllName, targetDll) == 0) {
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE *)module + importDesc->FirstThunk);
            PIMAGE_THUNK_DATA origThunk = (PIMAGE_THUNK_DATA)((BYTE *)module + importDesc->OriginalFirstThunk);

            while (origThunk->u1.AddressOfData) {
                PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)(
                    (BYTE *)module + origThunk->u1.AddressOfData
                );
                if (strcmp(importByName->Name, funcName) == 0) {
                    DWORD oldProtect;
                    VirtualProtect(&thunk->u1.Function, sizeof(void *), PAGE_READWRITE, &oldProtect);
                    *original = (void *)thunk->u1.Function;
                    thunk->u1.Function = (ULONG_PTR)hookFunc;
                    VirtualProtect(&thunk->u1.Function, sizeof(void *), oldProtect, &oldProtect);
                    return;
                }
                thunk++;
                origThunk++;
            }
        }
        importDesc++;
    }
}
```

#### 3. Compile-Time Replacement (CRT-specific)

For MSVC, you can define your own CRT allocation functions:

```c
// Replace _malloc_dbg and related functions
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Custom allocation tracking
_CRT_ALLOC_HOOK oldHook = NULL;

int MyAllocHook(int allocType, void *userData, size_t size,
                int blockType, long requestNumber,
                const unsigned char *filename, int lineNumber) {
    if (allocType == _HOOK_ALLOC) {
        fprintf(stderr, "[HOOK] malloc(%zu) at %s:%d\n", size, filename, lineNumber);
    } else if (allocType == _HOOK_FREE) {
        fprintf(stderr, "[HOOK] free(%p)\n", userData);
    }
    return TRUE;  // Allow allocation
}

void InitHooks(void) {
    oldHook = _CrtSetAllocHook(MyAllocHook);
}
```

#### 4. MinGW/GCC on Windows

Similar to Linux LD_PRELOAD, but uses different mechanisms:

```c
// For MinGW, use weak symbols
void *__real_malloc(size_t size);
void __real_free(void *ptr);

void *__wrap_malloc(size_t size) {
    void *ptr = __real_malloc(size);
    fprintf(stderr, "[HOOK] malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

void __wrap_free(void *ptr) {
    fprintf(stderr, "[HOOK] free(%p)\n", ptr);
    __real_free(ptr);
}
```

**Build with:**
```bash
gcc -Wl,--wrap=malloc -Wl,--wrap=free -o program program.c
```

---

## This Experiment's Approach

This experiment uses **linker-level hooking** to capture all memory allocations in compiled Sindarin programs, including allocations from statically linked libraries like zlib.

### How It Works

The `runtime_malloc_hooks.c` file provides `__wrap_malloc`/`__wrap_free` functions that intercept **all** malloc/free calls in the executable:

```
[SN_ALLOC] malloc(65536) = 0x55555555b2a0
[SN_ALLOC] malloc(5952) = 0x55555555c2b0
[SN_ALLOC] free(0x55555555c2b0)
```

This is enabled via linker flags in `etc/sn.cfg`:
```
SN_LDFLAGS=-Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
```

The `--wrap` linker flag redirects all calls to `malloc` to `__wrap_malloc`, which logs the call and then invokes `__real_malloc` (the actual libc function).

### Critical: Static Linking Requirement

**The `--wrap` linker flag only works for statically linked code.**

Libraries that are dynamically linked (`.so`/`.dylib`/`.dll`) resolve their malloc calls directly to libc at runtime, bypassing the wrapper entirely.

To capture allocations from a library like zlib:

| Linking | SDK Directive | Hooks Capture |
|---------|---------------|---------------|
| Dynamic | `@link z` | Only Sindarin runtime allocations |
| **Static** | `@link :libz.a` | **All allocations including zlib** |

The experiment includes a modified `sdk/zlib.sn` that uses static linking:
```sindarin
@link :libz.a    # Static - hooks capture zlib allocations
# vs
@link z          # Dynamic - zlib allocations bypass hooks
```

### Platform Support

| Platform | Linker Wrapping | Notes |
|----------|-----------------|-------|
| Linux (GCC/Clang) | `--wrap` | Fully supported |
| Windows (MinGW/Clang) | `--wrap` | Fully supported |
| macOS | Not supported | Apple linker lacks `--wrap`; use DYLD_INSERT_LIBRARIES instead |

### Building the Experiment

```bash
cd experiments/malloc
make build              # Build compiler and runtime with hooks
make test               # Compile and run tests/test_zlib.sn
make run                # Compile and run samples/main.sn
make clean              # Clean artifacts
```

### Files Modified

- `src/runtime/runtime_malloc_hooks.h` - Linker wrapper declarations
- `src/runtime/runtime_malloc_hooks.c` - Linker-level `__wrap_*` functions
- `src/runtime/runtime_arena.c` - Simplified (uses standard malloc/free)
- `etc/sn.cfg` - Adds `--wrap` linker flags
- `sdk/zlib.sn` - Uses static linking (`@link :libz.a`)
- `Makefile` - Builds experimental compiler and runtime

### Expected Output

When hooks are enabled, you'll see allocation logs with caller function names:

```
[SN_ALLOC] malloc(72) = 0x5565282b12a0  [rt_arena_create_sized]
[SN_ALLOC] malloc(65560) = 0x5565282b12f0  [rt_arena_create_sized]
[SN_ALLOC] malloc(5952) = 0x5565282e2450  [deflateInit_]
[SN_ALLOC] malloc(65536) = 0x5565282e3ba0  [deflateInit_]
[SN_ALLOC] malloc(7160) = 0x5565282e2450  [inflateInit_]
[SN_ALLOC] free(0x5565282e2450)  [deflateEnd]
```

Caller function names clearly identify the source:
| Caller | Source |
|--------|--------|
| `rt_arena_create_sized` | Sindarin runtime |
| `rt_arena_new_block` | Sindarin runtime |
| `deflateInit_` | zlib compression |
| `inflateInit_` | zlib decompression |
| `deflateEnd` / `inflateEnd` | zlib cleanup |

Typical allocation sizes:
| Size | Source |
|------|--------|
| 72 | Sindarin arena struct |
| 65560 | Sindarin arena block (64KB + header) |
| 5952 | zlib deflate state |
| 7160 | zlib inflate state |
| 65536 | zlib internal buffers |
