/*
 * Linker-level malloc/free hooks for Sindarin compiled programs.
 *
 * These wrapper functions intercept ALL malloc/free/calloc/realloc calls
 * in the compiled executable, including calls from linked libraries like zlib.
 *
 * Build the runtime with: -DSN_MALLOC_HOOKS
 * Link executables with:  -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
 */

#include <stdio.h>
#include <stddef.h>
#include "runtime_malloc_hooks.h"

#ifdef SN_MALLOC_HOOKS

#ifdef _WIN32
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

#else
/* Linux/macOS: use dladdr */
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

/* Thread-local guard to prevent recursive hook calls (fprintf may call malloc) */
static __thread int sn_malloc_hook_guard = 0;

/* Get caller's function name */
static const char *get_caller_name(void *addr)
{
#ifdef _WIN32
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
#else
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        return info.dli_sname;
    }
    return "???";
#endif
}

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

#endif /* SN_MALLOC_HOOKS */
