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

/* Thread-local guard to prevent recursive hook calls (fprintf may call malloc) */
static __thread int sn_malloc_hook_guard = 0;

/* Original function pointers - populated by hooking libraries */
static void *(*orig_malloc)(size_t) = NULL;
static void (*orig_free)(void *) = NULL;
static void *(*orig_calloc)(size_t, size_t) = NULL;
static void *(*orig_realloc)(void *, size_t) = NULL;

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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
    void *new_ptr;

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

    if (sn_malloc_hook_guard == 0) {
        sn_malloc_hook_guard = 1;
        void *caller = __builtin_return_address(0);
        fprintf(stderr, "[SN_ALLOC] realloc(%p, %zu) = %p  [%s]\n",
                old_ptr, size, new_ptr, get_caller_name(caller));
        sn_malloc_hook_guard = 0;
    }

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

static plthook_t *sn_plthook = NULL;

/* Install hooks at program startup using constructor attribute */
__attribute__((constructor))
static void sn_install_malloc_hooks(void)
{
    int rv;

    /* Open the main executable for PLT hooking */
    rv = plthook_open(&sn_plthook, NULL);
    if (rv != 0) {
        fprintf(stderr, "[SN_ALLOC] Warning: plthook_open failed: %s\n", plthook_error());
        return;
    }

    /* Hook malloc */
    rv = plthook_replace(sn_plthook, "malloc", (void *)hooked_malloc, (void **)&orig_malloc);
    if (rv != 0 && rv != PLTHOOK_FUNCTION_NOT_FOUND) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook malloc: %s\n", plthook_error());
    }

    /* Hook free */
    rv = plthook_replace(sn_plthook, "free", (void *)hooked_free, (void **)&orig_free);
    if (rv != 0 && rv != PLTHOOK_FUNCTION_NOT_FOUND) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook free: %s\n", plthook_error());
    }

    /* Hook calloc */
    rv = plthook_replace(sn_plthook, "calloc", (void *)hooked_calloc, (void **)&orig_calloc);
    if (rv != 0 && rv != PLTHOOK_FUNCTION_NOT_FOUND) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook calloc: %s\n", plthook_error());
    }

    /* Hook realloc */
    rv = plthook_replace(sn_plthook, "realloc", (void *)hooked_realloc, (void **)&orig_realloc);
    if (rv != 0 && rv != PLTHOOK_FUNCTION_NOT_FOUND) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook realloc: %s\n", plthook_error());
    }
}

__attribute__((destructor))
static void sn_uninstall_malloc_hooks(void)
{
    if (sn_plthook != NULL) {
        plthook_close(sn_plthook);
        sn_plthook = NULL;
    }
}

/* ============================================================================
 * Windows: MinHook-based hooking
 * ============================================================================ */

#elif defined(_WIN32)

#include "minhook/MinHook.h"

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

    /* Hook malloc */
    status = MH_CreateHook(&malloc, hooked_malloc, (void **)&orig_malloc);
    if (status != MH_OK) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook malloc: %d\n", status);
    }

    /* Hook free */
    status = MH_CreateHook(&free, hooked_free, (void **)&orig_free);
    if (status != MH_OK) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook free: %d\n", status);
    }

    /* Hook calloc */
    status = MH_CreateHook(&calloc, hooked_calloc, (void **)&orig_calloc);
    if (status != MH_OK) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook calloc: %d\n", status);
    }

    /* Hook realloc */
    status = MH_CreateHook(&realloc, hooked_realloc, (void **)&orig_realloc);
    if (status != MH_OK) {
        fprintf(stderr, "[SN_ALLOC] Warning: failed to hook realloc: %d\n", status);
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
