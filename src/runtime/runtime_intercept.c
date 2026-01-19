/**
 * runtime_intercept.c - Function interceptor implementation
 */

#include "runtime_intercept.h"
#include "runtime.h"
#include "runtime_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Maximum number of interceptors that can be registered
#define MAX_INTERCEPTORS 64

// Global interceptor count (for fast check at call sites)
volatile int __rt_interceptor_count = 0;

#ifdef __TINYC__
/* TinyCC pthread-based TLS implementation */
#include <pthread.h>
#include <stdlib.h>

static pthread_key_t __rt_intercept_tls_key;
static pthread_once_t __rt_intercept_tls_once = PTHREAD_ONCE_INIT;

static void __rt_intercept_tls_init(void) {
    pthread_key_create(&__rt_intercept_tls_key, free);
}

RtInterceptTLS *__rt_get_intercept_tls(void) {
    pthread_once(&__rt_intercept_tls_once, __rt_intercept_tls_init);
    RtInterceptTLS *tls = (RtInterceptTLS *)pthread_getspecific(__rt_intercept_tls_key);
    if (tls == NULL) {
        tls = (RtInterceptTLS *)calloc(1, sizeof(RtInterceptTLS));
        pthread_setspecific(__rt_intercept_tls_key, tls);
    }
    return tls;
}
#else
// Per-thread interception depth
#ifdef _WIN32
__declspec(thread) int __rt_intercept_depth = 0;
#else
__thread int __rt_intercept_depth = 0;
#endif

// Per-thread arguments array and arena for thunk functions
#ifdef _WIN32
__declspec(thread) RtAny *__rt_thunk_args = NULL;
__declspec(thread) void *__rt_thunk_arena = NULL;
#else
__thread RtAny *__rt_thunk_args = NULL;
__thread void *__rt_thunk_arena = NULL;
#endif
#endif /* __TINYC__ */

// Interceptor registry
static RtInterceptorEntry interceptor_registry[MAX_INTERCEPTORS];
static int interceptor_registry_count = 0;

/* Maximum number of arguments for intercepted functions.
 * Functions with more arguments than this cannot use args.length in interceptors. */
#define MAX_INTERCEPT_ARGS 32

/* Static thread-local buffer for wrapped args (avoids allocations).
 * Layout: [RtArrayMetadata][RtAny data...] */
#ifdef _WIN32
__declspec(thread) char __rt_wrapped_args_buffer[sizeof(RtArrayMetadata) + MAX_INTERCEPT_ARGS * sizeof(RtAny)];
#elif !defined(__TINYC__)
/* TCC uses __rt_wrapped_args_buffer macro from header pointing to TLS struct */
__thread char __rt_wrapped_args_buffer[sizeof(RtArrayMetadata) + MAX_INTERCEPT_ARGS * sizeof(RtAny)];
#endif

/* Wrap raw RtAny* args into a proper Sindarin array with metadata.
 * Uses a static thread-local buffer to avoid allocations. */
static RtAny *wrap_args_as_sindarin_array(RtAny *raw_args, int arg_count)
{
    /* If too many args, fall back to raw args (no length support) */
    if (arg_count > MAX_INTERCEPT_ARGS)
    {
        return raw_args;
    }

    /* Point to the data area after the metadata */
    RtAny *wrapped_args = (RtAny *)(__rt_wrapped_args_buffer + sizeof(RtArrayMetadata));

    /* Set up the metadata */
    RtArrayMetadata *meta = ((RtArrayMetadata *)wrapped_args) - 1;
    meta->arena = NULL;  /* Not arena-managed */
    meta->size = arg_count;
    meta->capacity = MAX_INTERCEPT_ARGS;

    /* Copy the args data */
    if (arg_count > 0 && raw_args != NULL)
    {
        memcpy(wrapped_args, raw_args, arg_count * sizeof(RtAny));
    }

    return wrapped_args;
}

// Simple mutex for thread safety (platform-specific)
#ifdef _WIN32
#include <windows.h>
static CRITICAL_SECTION interceptor_mutex;
static int mutex_initialized = 0;

static void ensure_mutex_initialized(void)
{
    if (!mutex_initialized)
    {
        InitializeCriticalSection(&interceptor_mutex);
        mutex_initialized = 1;
    }
}

static void lock_registry(void)
{
    ensure_mutex_initialized();
    EnterCriticalSection(&interceptor_mutex);
}

static void unlock_registry(void)
{
    LeaveCriticalSection(&interceptor_mutex);
}
#else
#include <pthread.h>
static pthread_mutex_t interceptor_mutex = PTHREAD_MUTEX_INITIALIZER;

static void lock_registry(void)
{
    pthread_mutex_lock(&interceptor_mutex);
}

static void unlock_registry(void)
{
    pthread_mutex_unlock(&interceptor_mutex);
}
#endif

void rt_interceptor_register(RtInterceptHandler handler)
{
    rt_interceptor_register_where(handler, NULL);
}

void rt_interceptor_register_where(RtInterceptHandler handler, const char *pattern)
{
    lock_registry();

    if (interceptor_registry_count >= MAX_INTERCEPTORS)
    {
        unlock_registry();
        fprintf(stderr, "Error: Maximum interceptor count (%d) exceeded\n", MAX_INTERCEPTORS);
        exit(1);
    }

    RtInterceptorEntry *entry = &interceptor_registry[interceptor_registry_count];
    entry->handler = handler;
    entry->pattern = pattern; // Pattern is owned by caller (typically a string literal)

    interceptor_registry_count++;
    __rt_interceptor_count = interceptor_registry_count;

    unlock_registry();
}

void rt_interceptor_clear_all(void)
{
    lock_registry();

    interceptor_registry_count = 0;
    __rt_interceptor_count = 0;

    unlock_registry();
}

int rt_interceptor_count(void)
{
    return __rt_interceptor_count;
}

RtInterceptorEntry *rt_interceptor_list(int *out_count)
{
    if (out_count)
    {
        *out_count = interceptor_registry_count;
    }
    return interceptor_registry;
}

bool rt_interceptor_is_active(void)
{
    return __rt_intercept_depth > 0;
}

bool rt_pattern_matches(const char *name, const char *pattern)
{
    if (pattern == NULL || strcmp(pattern, "*") == 0)
    {
        return true; // NULL or "*" matches everything
    }

    size_t pattern_len = strlen(pattern);
    size_t name_len = strlen(name);

    // Find wildcard position
    const char *wildcard = strchr(pattern, '*');

    if (wildcard == NULL)
    {
        // No wildcard - exact match required
        return strcmp(name, pattern) == 0;
    }

    size_t prefix_len = wildcard - pattern;
    size_t suffix_len = pattern_len - prefix_len - 1;

    // Check if name is long enough
    if (name_len < prefix_len + suffix_len)
    {
        return false;
    }

    // Check prefix
    if (prefix_len > 0 && strncmp(name, pattern, prefix_len) != 0)
    {
        return false;
    }

    // Check suffix
    if (suffix_len > 0)
    {
        const char *suffix = wildcard + 1;
        const char *name_suffix = name + name_len - suffix_len;
        if (strcmp(name_suffix, suffix) != 0)
        {
            return false;
        }
    }

    return true;
}

// Context for chained interceptor calls
typedef struct InterceptContext
{
    const char *name;
    RtAny *args;
    int arg_count;
    RtContinueFn original_fn;
    int current_interceptor;
    int matching_count;
    int *matching_indices;
} InterceptContext;

// Thread-local context for the continue callback
#ifdef _WIN32
static __declspec(thread) InterceptContext *current_context = NULL;
#elif defined(__TINYC__)
/* TCC: use pthread TLS via macro - cast void* to InterceptContext** for lvalue access */
#define current_context (*((InterceptContext **)__rt_current_context_ptr))
#else
static __thread InterceptContext *current_context = NULL;
#endif

// Forward declarations
static RtAny call_next_interceptor(void);
static RtAny call_next_interceptor_closure(void *closure);

/**
 * Closure-compatible wrapper for call_next_interceptor.
 * This function uses the Sindarin closure calling convention where the first
 * parameter is the closure pointer (which we ignore since we use thread-local context).
 */
static RtAny call_next_interceptor_closure(void *closure)
{
    (void)closure; // Unused - context is in thread-local storage
    return call_next_interceptor();
}

static RtAny call_next_interceptor(void)
{
    InterceptContext *ctx = current_context;

    if (ctx->current_interceptor >= ctx->matching_count)
    {
        // No more interceptors - call the original function
        return ctx->original_fn();
    }

    // Get the next matching interceptor
    int idx = ctx->matching_indices[ctx->current_interceptor];
    RtInterceptorEntry *entry = &interceptor_registry[idx];
    ctx->current_interceptor++;

    // Increment depth before calling handler
    __rt_intercept_depth++;

    // Wrap args into a proper Sindarin array so handlers can use args.length
    RtAny *wrapped_args = wrap_args_as_sindarin_array(ctx->args, ctx->arg_count);

    // Create a closure wrapper for the continue callback
    // This matches the __Closure__ type expected by generated Sindarin code
    RtClosure continue_closure = {
        .fn = (void *)call_next_interceptor_closure,
        .arena = (RtArena *)__rt_thunk_arena
    };

    // Call the interceptor with wrapped args and the closure-wrapped continue callback
    // Note: arg_count is not passed - the wrapped_args array has length info in its header
    RtAny result = entry->handler((RtArena *)__rt_thunk_arena, ctx->name, wrapped_args, &continue_closure);

    // Copy any modifications back to the original args array
    if (wrapped_args != ctx->args && ctx->arg_count > 0)
    {
        memcpy(ctx->args, wrapped_args, ctx->arg_count * sizeof(RtAny));
    }

    // Decrement depth after handler returns
    __rt_intercept_depth--;

    return result;
}

RtAny rt_call_intercepted(
    const char *name,
    RtAny *args,
    int arg_count,
    RtContinueFn original_fn)
{
    // Fast path: no interceptors registered
    if (__rt_interceptor_count == 0)
    {
        return original_fn();
    }

    // Find all interceptors that match this function name
    int matching_indices[MAX_INTERCEPTORS];
    int matching_count = 0;

    lock_registry();
    for (int i = 0; i < interceptor_registry_count; i++)
    {
        if (rt_pattern_matches(name, interceptor_registry[i].pattern))
        {
            matching_indices[matching_count++] = i;
        }
    }
    unlock_registry();

    // No matching interceptors
    if (matching_count == 0)
    {
        return original_fn();
    }

    // Set up context for chained calls
    InterceptContext ctx = {
        .name = name,
        .args = args,
        .arg_count = arg_count,
        .original_fn = original_fn,
        .current_interceptor = 0,
        .matching_count = matching_count,
        .matching_indices = matching_indices};

    // Save and set thread-local context
    InterceptContext *prev_context = current_context;
    current_context = &ctx;

    // Start the interceptor chain
    RtAny result = call_next_interceptor();

    // Restore previous context
    current_context = prev_context;

    return result;
}
