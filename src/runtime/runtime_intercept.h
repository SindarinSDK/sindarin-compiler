/**
 * runtime_intercept.h - Function interceptor infrastructure for Sindarin
 *
 * Provides the ability to intercept user-defined function calls for debugging,
 * profiling, mocking, and AOP-style programming patterns.
 */

#ifndef RUNTIME_INTERCEPT_H
#define RUNTIME_INTERCEPT_H

#include "runtime_any.h"
#include <stdbool.h>

/* RtArena is defined via runtime_any.h -> runtime_arena.h */

/**
 * Continue callback type - calls the original function (or next interceptor in chain)
 * with the current args array values.
 */
typedef RtAny (*RtContinueFn)(void);

/**
 * Closure type for continue callback - matches __Closure__ in generated code.
 * The fn pointer uses closure calling convention: fn(closure_ptr) -> RtAny
 */
typedef struct RtClosure {
    void *fn;       // Function pointer (code address, not arena-allocated)
    RtArenaV2 *arena; // Arena for allocations (may be NULL for continue callbacks)
} RtClosure;

/**
 * Interceptor handler function type.
 * Matches the Sindarin signature: fn(name: str, args: any[], continue_fn: fn(): any): any
 * Note: arg_count is not passed separately since args is a Sindarin array with .length
 *
 * @param arena     Arena for memory allocations (from caller's context)
 * @param name      The name of the function being called
 * @param args      Array of boxed arguments (Sindarin array with length header)
 * @param continue_fn Closure to invoke the original function (call via continue_fn->fn)
 * @return The result to return to the caller (can substitute the real result)
 */
typedef RtAny (*RtInterceptHandler)(
    RtArenaV2 *arena,
    RtHandleV2 *name,
    RtHandleV2 *args,
    RtClosure *continue_fn
);

/**
 * Pattern-matching interceptor entry.
 */
typedef struct RtInterceptorEntry
{
    RtInterceptHandler handler;
    const char *pattern; // NULL for "match all", or a pattern like "get*", "*User", "get*Name"
} RtInterceptorEntry;

/**
 * Global interceptor count for fast check at call sites.
 * When zero, function calls bypass interception entirely.
 */
extern volatile int __rt_interceptor_count;

/**
 * Per-thread interception depth for recursion detection.
 * Used by Interceptor.isActive() to detect if we're inside an interceptor.
 */
#ifdef _MSC_VER
extern __declspec(thread) int __rt_intercept_depth;
#elif defined(__TINYC__)
/* TinyCC: use pthread TLS for thread-safety */
#include <pthread.h>

/* Maximum args must match runtime_intercept.c */
#define __RT_MAX_INTERCEPT_ARGS 32

/* Buffer size for wrapped args - must be >= sizeof(RtArrayMetadataV2) + MAX_ARGS * sizeof(RtAny)
 * RtArrayMetadataV2 is 24 bytes on 64-bit, RtAny is ~24 bytes, so 32 + 32*32 = 1056 is plenty */
#define __RT_WRAPPED_ARGS_BUFFER_SIZE 1056

/* Thread-local storage struct for interceptor state */
typedef struct RtInterceptTLS {
    int intercept_depth;
    RtAny *thunk_args;
    RtArenaV2 *thunk_arena;
    char wrapped_args_buffer[__RT_WRAPPED_ARGS_BUFFER_SIZE];
    void *current_context;  /* InterceptContext* - void* to avoid forward decl issues */
} RtInterceptTLS;

/* Get thread-local interceptor state (lazily initialized) */
RtInterceptTLS *__rt_get_intercept_tls(void);

/* Accessor macros that look like variables */
#define __rt_intercept_depth (__rt_get_intercept_tls()->intercept_depth)
#define __rt_thunk_args (__rt_get_intercept_tls()->thunk_args)
#define __rt_thunk_arena (__rt_get_intercept_tls()->thunk_arena)
#define __rt_wrapped_args_buffer (__rt_get_intercept_tls()->wrapped_args_buffer)
#define __rt_current_context ((InterceptContext *)(__rt_get_intercept_tls()->current_context))
#define __rt_current_context_ptr (&(__rt_get_intercept_tls()->current_context))
#else
extern __thread int __rt_intercept_depth;
#endif

/**
 * Per-thread arguments array for thunk functions.
 * Used to pass boxed arguments to file-scope thunk functions without
 * requiring nested functions (which are a GCC extension).
 */
#ifdef _MSC_VER
extern __declspec(thread) RtAny *__rt_thunk_args;
extern __declspec(thread) RtArenaV2 *__rt_thunk_arena;
#elif !defined(__TINYC__)
extern __thread RtAny *__rt_thunk_args;
extern __thread RtArenaV2 *__rt_thunk_arena;
#endif

/**
 * Register an interceptor for all user-defined functions.
 *
 * @param handler The interceptor function to register
 */
void rt_interceptor_register(RtInterceptHandler handler);

/**
 * Register an interceptor with pattern matching.
 * Patterns support wildcard (*) at start, middle, or end.
 *
 * @param handler The interceptor function to register
 * @param pattern Pattern to match function names (e.g., "get*", "*User", "get*Name")
 */
void rt_interceptor_register_where(RtInterceptHandler handler, const char *pattern);

/**
 * Clear all registered interceptors.
 */
void rt_interceptor_clear_all(void);

/**
 * Get the current count of registered interceptors.
 *
 * @return Number of registered interceptors
 */
int rt_interceptor_count(void);

/**
 * Get the list of registered interceptor handlers.
 *
 * @param out_count Output parameter for the number of handlers
 * @return Array of interceptor entries (owned by runtime, do not free)
 */
RtInterceptorEntry *rt_interceptor_list(int *out_count);

/**
 * Check if currently inside an interceptor call.
 * Useful for preventing infinite recursion in interceptors.
 *
 * @return true if inside an interceptor, false otherwise
 */
bool rt_interceptor_is_active(void);

/**
 * Call a function through the interceptor chain.
 * This is called by generated code for user-defined function calls.
 *
 * @param name         The function name
 * @param args         Array of boxed arguments
 * @param arg_count    Number of arguments
 * @param original_fn  The original function to call if no interception
 * @return The (possibly intercepted) result
 */
RtAny rt_call_intercepted(
    const char *name,
    RtAny *args,
    int arg_count,
    RtContinueFn original_fn
);

/**
 * Check if a function name matches a pattern.
 * Patterns can contain wildcards (*) at start, middle, or end.
 *
 * @param name    The function name to check
 * @param pattern The pattern to match against
 * @return true if the name matches the pattern
 */
bool rt_pattern_matches(const char *name, const char *pattern);

#endif // RUNTIME_INTERCEPT_H
