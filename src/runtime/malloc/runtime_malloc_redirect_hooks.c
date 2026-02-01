// runtime_malloc_redirect_hooks.c
// Platform-specific hook installation

/* ============================================================================
 * Platform-specific hook installation
 * ============================================================================ */

bool rt_malloc_redirect_hooks_installed(void)
{
    return hooks_installed;
}

/* ============================================================================
 * macOS: fishhook-based hooking
 * ============================================================================ */

#ifdef __APPLE__

#include "fishhook.h"

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    struct rebinding rebindings[] = {
        {"malloc", (void *)hooked_malloc, (void **)&orig_malloc},
        {"free", (void *)hooked_free, (void **)&orig_free},
        {"calloc", (void *)hooked_calloc, (void **)&orig_calloc},
        {"realloc", (void *)hooked_realloc, (void **)&orig_realloc},
    };
    rebind_symbols(rebindings, sizeof(rebindings) / sizeof(rebindings[0]));
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    /* fishhook doesn't support unhooking */
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

/* ============================================================================
 * Linux: plthook-based hooking
 * ============================================================================ */

#elif defined(__linux__)

#include "plthook.h"
#include <link.h>

#define MAX_HOOKED_LIBS 64
static plthook_t *plthooks[MAX_HOOKED_LIBS];
static int plthook_count = 0;

static void hook_library(plthook_t *ph)
{
    void *tmp_orig = NULL;
    int rv;

    rv = plthook_replace(ph, "malloc", (void *)hooked_malloc, &tmp_orig);
    if (rv == 0 && orig_malloc == NULL && tmp_orig != NULL) {
        orig_malloc = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "free", (void *)hooked_free, &tmp_orig);
    if (rv == 0 && orig_free == NULL && tmp_orig != NULL) {
        orig_free = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "calloc", (void *)hooked_calloc, &tmp_orig);
    if (rv == 0 && orig_calloc == NULL && tmp_orig != NULL) {
        orig_calloc = tmp_orig;
    }

    tmp_orig = NULL;
    rv = plthook_replace(ph, "realloc", (void *)hooked_realloc, &tmp_orig);
    if (rv == 0 && orig_realloc == NULL && tmp_orig != NULL) {
        orig_realloc = tmp_orig;
    }
}

static int hook_library_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;

    if (plthook_count >= MAX_HOOKED_LIBS) return 0;

    plthook_t *ph = NULL;
    const char *name = info->dlpi_name;
    int rv;

    if (name == NULL || name[0] == '\0') {
        rv = plthook_open(&ph, NULL);
    } else {
        rv = plthook_open(&ph, name);
    }

    if (rv != 0) return 0;

    hook_library(ph);
    plthooks[plthook_count++] = ph;

    return 0;
}

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    dl_iterate_phdr(hook_library_callback, NULL);
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    for (int i = 0; i < plthook_count; i++) {
        if (plthooks[i] != NULL) {
            plthook_close(plthooks[i]);
            plthooks[i] = NULL;
        }
    }
    plthook_count = 0;
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

__attribute__((destructor))
static void auto_uninstall_hooks(void)
{
    rt_malloc_redirect_uninstall_hooks();
}

/* ============================================================================
 * Windows: MinHook-based hooking
 * ============================================================================ */

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "minhook/MinHook.h"

static void *get_crt_function(const char *name)
{
    HMODULE hCRT = GetModuleHandleA("ucrtbase.dll");
    if (!hCRT) hCRT = GetModuleHandleA("msvcrt.dll");
    if (!hCRT) hCRT = GetModuleHandleA("api-ms-win-crt-heap-l1-1-0.dll");
    if (hCRT) return (void *)GetProcAddress(hCRT, name);
    return NULL;
}

void rt_malloc_redirect_install_hooks(void)
{
    if (hooks_installed) return;

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return;

    void *crt_malloc = get_crt_function("malloc");
    void *crt_free = get_crt_function("free");
    void *crt_calloc = get_crt_function("calloc");
    void *crt_realloc = get_crt_function("realloc");

    if (crt_malloc) {
        MH_CreateHook(crt_malloc, hooked_malloc, (void **)&orig_malloc);
    }
    if (crt_free) {
        MH_CreateHook(crt_free, hooked_free, (void **)&orig_free);
    }
    if (crt_calloc) {
        MH_CreateHook(crt_calloc, hooked_calloc, (void **)&orig_calloc);
    }
    if (crt_realloc) {
        MH_CreateHook(crt_realloc, hooked_realloc, (void **)&orig_realloc);
    }

    MH_EnableHook(MH_ALL_HOOKS);
    hooks_installed = true;
}

void rt_malloc_redirect_uninstall_hooks(void)
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    hooks_installed = false;
}

__attribute__((constructor))
static void auto_install_hooks(void)
{
    rt_malloc_redirect_install_hooks();
}

__attribute__((destructor))
static void auto_uninstall_hooks(void)
{
    rt_malloc_redirect_uninstall_hooks();
}

#else
#error "Unsupported platform for malloc redirect"
#endif
