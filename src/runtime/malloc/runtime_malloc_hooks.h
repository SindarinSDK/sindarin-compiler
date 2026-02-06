#ifndef RUNTIME_MALLOC_HOOKS_H
#define RUNTIME_MALLOC_HOOKS_H

/*
 * Memory allocation hooks for Sindarin compiled programs.
 *
 * Platform-specific runtime hooking mechanisms:
 *
 * Linux:
 *   Uses plthook library to modify PLT/GOT entries at runtime.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * macOS:
 *   Uses Facebook's fishhook library for runtime symbol rebinding.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * Windows:
 *   Uses MinHook library for inline function hooking via trampolines.
 *   Hooks are installed via constructor, no special linker flags needed.
 *
 * All platforms now use runtime hooking, which:
 *   - Catches allocations from both static and dynamic libraries
 *   - Requires no special linker flags (--wrap removed)
 *   - Provides consistent behavior across all platforms
 *
 * Handler Registration:
 *   Arenas can register thread-local handlers to intercept allocations.
 *   When a handler is set, malloc/free/etc calls are routed to it.
 *   Each thread can have its own handler (thread-local storage).
 */

#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Handler Function Types
 * ============================================================================
 * Each handler function receives user_data and sets *handled to indicate
 * whether it processed the call:
 *   - *handled = true, return ptr: Handler allocated, use returned pointer
 *   - *handled = false: Handler declined, fall through to system malloc
 */

typedef void *(*RtHookMallocFn)(size_t size, bool *handled, void *user_data);
typedef void (*RtHookFreeFn)(void *ptr, bool *handled, void *user_data);
typedef void *(*RtHookReallocFn)(void *ptr, size_t size, bool *handled, void *user_data);

/* ============================================================================
 * Handler Structure
 * ============================================================================
 * Bundle of handler functions. Set functions to NULL to pass through.
 */

typedef struct {
    RtHookMallocFn malloc_fn;
    RtHookFreeFn free_fn;
    RtHookReallocFn realloc_fn;   /* Also used for calloc */
    void *user_data;
} RtMallocHandler;

/* ============================================================================
 * API: Handler Registration (Thread-Local)
 * ============================================================================
 */

/* Set the malloc handler for the current thread.
 * The handler pointer must remain valid until cleared.
 * Only one handler can be active per thread at a time. */
void rt_malloc_hooks_set_handler(RtMallocHandler *handler);

/* Clear the malloc handler for the current thread. */
void rt_malloc_hooks_clear_handler(void);

/* Get the current handler for this thread (NULL if none). */
RtMallocHandler *rt_malloc_hooks_get_handler(void);

/* ============================================================================
 * API: Original Functions (for handlers to pass through)
 * ============================================================================
 */

void *rt_malloc_hooks_orig_malloc(size_t size);
void rt_malloc_hooks_orig_free(void *ptr);
void *rt_malloc_hooks_orig_realloc(void *ptr, size_t size);

#endif /* RUNTIME_MALLOC_HOOKS_H */
