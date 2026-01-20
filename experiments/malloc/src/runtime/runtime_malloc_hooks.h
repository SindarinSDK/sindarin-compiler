#ifndef RUNTIME_MALLOC_HOOKS_H
#define RUNTIME_MALLOC_HOOKS_H

/*
 * Linker-level malloc/free hooks for Sindarin compiled programs.
 *
 * When linked with -Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc
 * these functions intercept ALL malloc/free calls in the executable,
 * including those from linked libraries like zlib.
 *
 * The __real_* functions are provided by the linker and call the actual
 * libc implementations.
 */

#include <stddef.h>

#ifdef SN_MALLOC_HOOKS

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

#endif /* SN_MALLOC_HOOKS */

#endif /* RUNTIME_MALLOC_HOOKS_H */
