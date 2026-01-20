/*
 * Memory allocation hooks for Sindarin compiled programs.
 *
 * Platform-specific hooking mechanisms:
 *
 * Linux/Windows (MinGW):
 *   Uses linker-level wrapping (--wrap=malloc, etc.)
 *   Link executables with: -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
 *
 * macOS:
 *   Uses Facebook's fishhook library for runtime symbol rebinding.
 *   No special linker flags required - hooks are installed at runtime.
 *
 * Build the runtime with: -DSN_MALLOC_HOOKS
 */

#include <stdio.h>
#include <stddef.h>
#include "runtime_malloc_hooks.h"

#ifdef SN_MALLOC_HOOKS

/* Thread-local guard to prevent recursive hook calls (fprintf may call malloc) */
static __thread int sn_malloc_hook_guard = 0;

/* ============================================================================
 * Platform-specific symbol resolution for caller identification
 * ============================================================================ */

#ifdef _WIN32
/* Windows: Use DbgHelp for symbol resolution */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static int sn_symbols_initialized = 0;

static void sn_init_symbols(void)
{
    if (!sn_symbols_initialized) {
        /* Get the path to the current executable */
        char exe_path[MAX_PATH];
        GetModuleFileNameA(NULL, exe_path, MAX_PATH);

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_DEBUG);
        SymInitialize(GetCurrentProcess(), NULL, TRUE);

        /* Load symbols for the main executable */
        SymLoadModuleEx(GetCurrentProcess(), NULL, exe_path, NULL, 0, 0, NULL, 0);

        sn_symbols_initialized = 1;
    }
}

static const char *get_caller_name(void *addr)
{
    static __thread char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    static __thread char name_buffer[256];

    sn_init_symbols();

    SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    if (SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &displacement, symbol)) {
        snprintf(name_buffer, sizeof(name_buffer), "%s+0x%llx", symbol->Name, (unsigned long long)displacement);
        return name_buffer;
    }
    /* Fallback: return address for use with addr2line */
    snprintf(name_buffer, sizeof(name_buffer), "@%p", addr);
    return name_buffer;
}

#else
/* Linux/macOS: Use dladdr for symbol resolution */
#define _GNU_SOURCE
#include <dlfcn.h>

static const char *get_caller_name(void *addr)
{
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        return info.dli_sname;
    }
    return "???";
}
#endif

/* ============================================================================
 * macOS: fishhook-based hooking
 * ============================================================================ */

#ifdef __APPLE__

#include "fishhook.h"

/* Original function pointers - fishhook will populate these */
static void *(*orig_malloc)(size_t) = NULL;
static void (*orig_free)(void *) = NULL;
static void *(*orig_calloc)(size_t, size_t) = NULL;
static void *(*orig_realloc)(void *, size_t) = NULL;

static void *hooked_malloc(size_t size)
{
    void *ptr = orig_malloc(size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] malloc(%zu) = %p  [%s]\n",
                size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return ptr;
}

static void hooked_free(void *ptr)
{
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] free(%p)  [%s]\n",
                ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    orig_free(ptr);
}

static void *hooked_calloc(size_t count, size_t size)
{
    void *ptr = orig_calloc(count, size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] calloc(%zu, %zu) = %p  [%s]\n",
                count, size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return ptr;
}

static void *hooked_realloc(void *ptr, size_t size)
{
    void *old_ptr = ptr;
    void *new_ptr = orig_realloc(ptr, size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] realloc(%p, %zu) = %p  [%s]\n",
                old_ptr, size, new_ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return new_ptr;
}

/* Install hooks at program startup using constructor attribute */
__attribute__((constructor))
static void sn_install_malloc_hooks(void)
{
    struct rebinding rebindings[] = {
        {"malloc", hooked_malloc, (void **)&orig_malloc},
        {"free", hooked_free, (void **)&orig_free},
        {"calloc", hooked_calloc, (void **)&orig_calloc},
        {"realloc", hooked_realloc, (void **)&orig_realloc},
    };
    rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));
}

#else
/* ============================================================================
 * Linux/Windows: Linker-based wrapping (--wrap)
 * ============================================================================ */

void *__wrap_malloc(size_t size)
{
    void *ptr = __real_malloc(size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] malloc(%zu) = %p  [%s]\n",
                size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return ptr;
}

void __wrap_free(void *ptr)
{
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] free(%p)  [%s]\n",
                ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    __real_free(ptr);
}

void *__wrap_calloc(size_t count, size_t size)
{
    void *ptr = __real_calloc(count, size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] calloc(%zu, %zu) = %p  [%s]\n",
                count, size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return ptr;
}

void *__wrap_realloc(void *ptr, size_t size)
{
    void *old_ptr = ptr;
    void *new_ptr = __real_realloc(ptr, size);

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] realloc(%p, %zu) = %p  [%s]\n",
                old_ptr, size, new_ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

    return new_ptr;
}

#endif /* __APPLE__ */

#endif /* SN_MALLOC_HOOKS */
