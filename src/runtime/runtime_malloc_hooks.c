/*
 * Memory allocation hooks for Sindarin compiled programs.
 *
 * Platform-specific runtime hooking mechanisms:
 *
 * Linux:
 *   Uses plthook library to modify PLT/GOT entries at runtime.
 *   Intercepts all malloc/free calls including from dynamic libraries.
 *
 * macOS:
 *   Uses Facebook's fishhook library for runtime symbol rebinding.
 *   Intercepts all malloc/free calls via Mach-O symbol pointer modification.
 *
 * Windows:
 *   Uses MinHook library for inline function hooking via trampolines.
 *   Intercepts all malloc/free calls via code patching.
 *
 * Build the runtime with: -DSN_MALLOC_HOOKS
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "runtime_malloc_hooks.h"

#ifdef SN_MALLOC_HOOKS

/* Linux/macOS: Need dlfcn.h for dlsym bootstrap in hooked functions */
#ifndef _WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#endif

/* Original function pointers - populated by hooking libraries */
static void *(*orig_malloc)(size_t) = NULL;
static void (*orig_free)(void *) = NULL;
static void *(*orig_calloc)(size_t, size_t) = NULL;
static void *(*orig_realloc)(void *, size_t) = NULL;

/* ============================================================================
 * Platform-specific symbol resolution for caller identification
 * Only compiled when verbose output is enabled.
 * ============================================================================ */

#ifdef SN_MALLOC_HOOKS_VERBOSE

/* Thread-local guard to prevent recursive hook calls (fprintf may call malloc) */
static __thread int sn_malloc_hook_guard = 0;

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
/* Linux/macOS: Use dladdr for symbol resolution (dlfcn.h already included above) */

static const char *get_caller_name(void *addr)
{
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        return info.dli_sname;
    }
    return "???";
}
#endif /* _WIN32 */

#endif /* SN_MALLOC_HOOKS_VERBOSE */

/* ============================================================================
 * Common hooked function implementations
 * ============================================================================ */

static void *hooked_malloc(size_t size)
{
    void *ptr;

    /* Bootstrap: if hooks not yet installed, use libc directly */
    if (orig_malloc == NULL) {
#ifdef _WIN32
        ptr = malloc(size);
#else
        ptr = ((void *(*)(size_t))dlsym(RTLD_NEXT, "malloc"))(size);
#endif
    } else {
        ptr = orig_malloc(size);
    }

#ifdef SN_MALLOC_HOOKS_VERBOSE
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] malloc(%zu) = %p  [%s]\n",
                size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }
#endif

    return ptr;
}

static void hooked_free(void *ptr)
{
#ifdef SN_MALLOC_HOOKS_VERBOSE
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] free(%p)  [%s]\n",
                ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }
#endif

    /* Bootstrap: if hooks not yet installed, use libc directly */
    if (orig_free == NULL) {
#ifdef _WIN32
        free(ptr);
#else
        ((void (*)(void *))dlsym(RTLD_NEXT, "free"))(ptr);
#endif
    } else {
        orig_free(ptr);
    }
}

static void *hooked_calloc(size_t count, size_t size)
{
    void *ptr;

    /* Bootstrap: if hooks not yet installed, use libc directly */
    if (orig_calloc == NULL) {
#ifdef _WIN32
        ptr = calloc(count, size);
#else
        ptr = ((void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc"))(count, size);
#endif
    } else {
        ptr = orig_calloc(count, size);
    }

#ifdef SN_MALLOC_HOOKS_VERBOSE
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] calloc(%zu, %zu) = %p  [%s]\n",
                count, size, ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }
#endif

    return ptr;
}

static void *hooked_realloc(void *ptr, size_t size)
{
    void *new_ptr;
#ifdef SN_MALLOC_HOOKS_VERBOSE
    void *old_ptr = ptr;
#endif

    /* Bootstrap: if hooks not yet installed, use libc directly */
    if (orig_realloc == NULL) {
#ifdef _WIN32
        new_ptr = realloc(ptr, size);
#else
        new_ptr = ((void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc"))(ptr, size);
#endif
    } else {
        new_ptr = orig_realloc(ptr, size);
    }

#ifdef SN_MALLOC_HOOKS_VERBOSE
    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] realloc(%p, %zu) = %p  [%s]\n",
                old_ptr, size, new_ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }
#endif

    return new_ptr;
}

/* ============================================================================
 * macOS: fishhook-based hooking
 * ============================================================================ */

#ifdef __APPLE__

#include "fishhook.h"

/* Install hooks at program startup using constructor attribute */
__attribute__((constructor))
static void sn_install_malloc_hooks(void)
{
    struct rebinding rebindings[] = {
        {"malloc", (void *)hooked_malloc, (void **)&orig_malloc},
        {"free", (void *)hooked_free, (void **)&orig_free},
        {"calloc", (void *)hooked_calloc, (void **)&orig_calloc},
        {"realloc", (void *)hooked_realloc, (void **)&orig_realloc},
    };
    rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));
}

/* ============================================================================
 * Linux: plthook-based hooking
 * ============================================================================ */

#elif defined(__linux__)

#include "plthook.h"
#include <link.h>

/* Track all hooked libraries for cleanup */
#define MAX_HOOKED_LIBS 64
static plthook_t *sn_plthooks[MAX_HOOKED_LIBS];
static int sn_plthook_count = 0;

/* Hook a single library's PLT */
static void hook_library(plthook_t *ph)
{
    /* Hook malloc - only save orig_malloc from first successful hook */
    void *tmp_orig = NULL;
    int rv = plthook_replace(ph, "malloc", (void *)hooked_malloc, &tmp_orig);
    if (rv == 0 && orig_malloc == NULL && tmp_orig != NULL) {
        orig_malloc = tmp_orig;
    }

    /* Hook free */
    tmp_orig = NULL;
    rv = plthook_replace(ph, "free", (void *)hooked_free, &tmp_orig);
    if (rv == 0 && orig_free == NULL && tmp_orig != NULL) {
        orig_free = tmp_orig;
    }

    /* Hook calloc */
    tmp_orig = NULL;
    rv = plthook_replace(ph, "calloc", (void *)hooked_calloc, &tmp_orig);
    if (rv == 0 && orig_calloc == NULL && tmp_orig != NULL) {
        orig_calloc = tmp_orig;
    }

    /* Hook realloc */
    tmp_orig = NULL;
    rv = plthook_replace(ph, "realloc", (void *)hooked_realloc, &tmp_orig);
    if (rv == 0 && orig_realloc == NULL && tmp_orig != NULL) {
        orig_realloc = tmp_orig;
    }
}

/* Callback for dl_iterate_phdr - hooks each loaded shared object */
static int hook_library_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;

    if (sn_plthook_count >= MAX_HOOKED_LIBS) {
        return 0; /* Stop iteration if we've reached the limit */
    }

    plthook_t *ph = NULL;
    int rv;

    /* Open by filename (info->dlpi_name), or NULL for main executable */
    const char *name = info->dlpi_name;
    if (name == NULL || name[0] == '\0') {
        rv = plthook_open(&ph, NULL); /* Main executable */
    } else {
        rv = plthook_open(&ph, name);
    }

    if (rv != 0) {
        return 0; /* Continue to next library */
    }

    /* Hook this library */
    hook_library(ph);

    /* Store for cleanup */
    sn_plthooks[sn_plthook_count++] = ph;

    return 0; /* Continue iteration */
}

/* Install hooks at program startup using constructor attribute */
__attribute__((constructor))
static void sn_install_malloc_hooks(void)
{
    /* Iterate over all loaded shared objects and hook each one */
    dl_iterate_phdr(hook_library_callback, NULL);
}

__attribute__((destructor))
static void sn_uninstall_malloc_hooks(void)
{
    for (int i = 0; i < sn_plthook_count; i++) {
        if (sn_plthooks[i] != NULL) {
            plthook_close(sn_plthooks[i]);
            sn_plthooks[i] = NULL;
        }
    }
    sn_plthook_count = 0;
}

/* ============================================================================
 * Windows: MinHook-based hooking
 * ============================================================================ */

#elif defined(_WIN32)

#include "minhook/MinHook.h"

/* Get the actual CRT function address from ucrtbase.dll or msvcrt.dll */
static void *get_crt_function(const char *name)
{
    /* Try Universal CRT first (Windows 10+) */
    HMODULE hCRT = GetModuleHandleA("ucrtbase.dll");
    if (!hCRT) {
        /* Fall back to legacy MSVCRT */
        hCRT = GetModuleHandleA("msvcrt.dll");
    }
    if (!hCRT) {
        /* Try api-ms-win-crt-heap (Windows 8+) */
        hCRT = GetModuleHandleA("api-ms-win-crt-heap-l1-1-0.dll");
    }
    if (hCRT) {
        return (void *)GetProcAddress(hCRT, name);
    }
    return NULL;
}

/* Install hooks at program startup using constructor attribute */
__attribute__((constructor))
static void sn_install_malloc_hooks(void)
{
    MH_STATUS status;

    /* Initialize MinHook */
    status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        fprintf(stderr, "[SN_ALLOC] Warning: MH_Initialize failed: %d\n", status);
        return;
    }

    /* Get actual CRT function addresses to hook all callers (including DLLs) */
    void *crt_malloc = get_crt_function("malloc");
    void *crt_free = get_crt_function("free");
    void *crt_calloc = get_crt_function("calloc");
    void *crt_realloc = get_crt_function("realloc");

    /* Hook malloc */
    if (crt_malloc) {
        status = MH_CreateHook(crt_malloc, hooked_malloc, (void **)&orig_malloc);
        if (status != MH_OK) {
            fprintf(stderr, "[SN_ALLOC] Warning: failed to hook malloc: %d\n", status);
        }
    }

    /* Hook free */
    if (crt_free) {
        status = MH_CreateHook(crt_free, hooked_free, (void **)&orig_free);
        if (status != MH_OK) {
            fprintf(stderr, "[SN_ALLOC] Warning: failed to hook free: %d\n", status);
        }
    }

    /* Hook calloc */
    if (crt_calloc) {
        status = MH_CreateHook(crt_calloc, hooked_calloc, (void **)&orig_calloc);
        if (status != MH_OK) {
            fprintf(stderr, "[SN_ALLOC] Warning: failed to hook calloc: %d\n", status);
        }
    }

    /* Hook realloc */
    if (crt_realloc) {
        status = MH_CreateHook(crt_realloc, hooked_realloc, (void **)&orig_realloc);
        if (status != MH_OK) {
            fprintf(stderr, "[SN_ALLOC] Warning: failed to hook realloc: %d\n", status);
        }
    }

    /* Enable all hooks */
    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        fprintf(stderr, "[SN_ALLOC] Warning: MH_EnableHook failed: %d\n", status);
    }
}

__attribute__((destructor))
static void sn_uninstall_malloc_hooks(void)
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

#else
#error "Unsupported platform for malloc hooks"
#endif

#endif /* SN_MALLOC_HOOKS */
