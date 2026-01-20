#ifndef RUNTIME_MALLOC_HOOKS_H
#define RUNTIME_MALLOC_HOOKS_H

/*
 * Memory allocation hooks for Sindarin compiled programs.
 *
 * Platform-specific mechanisms:
 *
 * Linux/Windows (MinGW):
 *   Uses linker-level wrapping (--wrap=malloc, etc.)
 *   Link with: -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
 *   The __real_* functions are provided by the linker and call libc.
 *
 * macOS:
 *   Uses Facebook's fishhook library for runtime symbol rebinding.
 *   No special linker flags required - hooks are installed at startup.
 */

#include <stddef.h>

#ifdef SN_MALLOC_HOOKS

#ifndef __APPLE__
/*
 * Linux/Windows: Linker-provided wrapper declarations
 * These are only valid when using the --wrap linker flag
 */

/* Declarations for the real libc functions (provided by linker --wrap) */
extern void *__real_malloc(size_t size);
extern void  __real_free(void *ptr);
extern void *__real_calloc(size_t count, size_t size);
extern void *__real_realloc(void *ptr, size_t size);

/* Our wrapper functions that intercept all calls */
void *__wrap_malloc(size_t size);
void  __wrap_free(void *ptr);
void *__wrap_calloc(size_t count, size_t size);
void *__wrap_realloc(void *ptr, size_t size);

#endif /* !__APPLE__ */

/*
 * macOS: Hooks are installed at runtime via fishhook constructor.
 * No declarations needed here - see runtime_malloc_hooks.c
 */

#endif /* SN_MALLOC_HOOKS */

#endif /* RUNTIME_MALLOC_HOOKS_H */
