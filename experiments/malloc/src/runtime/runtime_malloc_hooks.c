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

/* Use dladdr to resolve caller function names (Linux/macOS) */
#ifndef _WIN32
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

/* Thread-local guard to prevent recursive hook calls (fprintf may call malloc) */
static __thread int sn_malloc_hook_guard = 0;

/* Get caller's function name using dladdr */
static const char *get_caller_name(void *addr)
{
#ifndef _WIN32
    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname) {
        return info.dli_sname;
    }
#else
    (void)addr;
#endif
    return "???";
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
