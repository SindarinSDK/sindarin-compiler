#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
    /* MinGW provides pthreads - but SN_USE_WIN32_THREADS forces native Win32 API */
    #include <pthread.h>
    #else
    /* Use Windows API compatibility layer for MSVC/clang-cl and packaged builds */
    #include "../platform/compat_pthread.h"
    #endif
#else
#include <pthread.h>
#endif
#include <setjmp.h>
#include "runtime_thread.h"
#include "runtime_array.h"

/* ============================================================================
 * Thread Implementation
 * ============================================================================
 *
 * This module provides threading support for the Sindarin runtime.
 * Threads are spawned using the & operator and synchronized using the ! operator.
 * Panic propagation occurs at synchronization time.
 */

/* ============================================================================
 * Thread-Local Panic Context
 * ============================================================================
 * Each thread has its own panic context for catching panics via setjmp/longjmp.
 */

RT_THREAD_LOCAL RtThreadPanicContext *rt_thread_panic_ctx = NULL;

/* ============================================================================
 * Global Thread Pool
 * ============================================================================
 * Tracks all active threads for cleanup on process exit.
 */

static RtThreadPool g_thread_pool = {
    .handles = NULL,
    .count = 0,
    .capacity = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

static bool g_thread_pool_initialized = false;

/* Initial capacity for thread pool */
#define RT_THREAD_POOL_INITIAL_CAPACITY 16

/* ============================================================================
 * Thread Pool Management
 * ============================================================================ */

/* Initialize the global thread pool */
void rt_thread_pool_init(void)
{
    if (g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);
    if (!g_thread_pool_initialized) {
        g_thread_pool.handles = malloc(RT_THREAD_POOL_INITIAL_CAPACITY * sizeof(RtThreadHandle *));
        if (g_thread_pool.handles == NULL) {
            fprintf(stderr, "rt_thread_pool_init: allocation failed\n");
            pthread_mutex_unlock(&g_thread_pool.mutex);
            exit(1);
        }
        g_thread_pool.capacity = RT_THREAD_POOL_INITIAL_CAPACITY;
        g_thread_pool.count = 0;
        g_thread_pool_initialized = true;
    }
    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Add a thread handle to the pool */
void rt_thread_pool_add(RtThreadHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (!g_thread_pool_initialized) {
        rt_thread_pool_init();
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Grow array if needed */
    if (g_thread_pool.count >= g_thread_pool.capacity) {
        size_t new_capacity = g_thread_pool.capacity * 2;
        RtThreadHandle **new_handles = realloc(g_thread_pool.handles,
                                                new_capacity * sizeof(RtThreadHandle *));
        if (new_handles == NULL) {
            fprintf(stderr, "rt_thread_pool_add: realloc failed\n");
            pthread_mutex_unlock(&g_thread_pool.mutex);
            exit(1);
        }
        g_thread_pool.handles = new_handles;
        g_thread_pool.capacity = new_capacity;
    }

    g_thread_pool.handles[g_thread_pool.count++] = handle;
    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Remove a thread handle from the pool */
void rt_thread_pool_remove(RtThreadHandle *handle)
{
    if (handle == NULL || !g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Find and remove the handle */
    for (size_t i = 0; i < g_thread_pool.count; i++) {
        if (g_thread_pool.handles[i] == handle) {
            /* Shift remaining elements */
            for (size_t j = i; j < g_thread_pool.count - 1; j++) {
                g_thread_pool.handles[j] = g_thread_pool.handles[j + 1];
            }
            g_thread_pool.count--;
            break;
        }
    }

    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Clean up all threads in the pool (called on process exit) */
void rt_thread_pool_cleanup(void)
{
    if (!g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Cancel all threads that haven't been synced */
    for (size_t i = 0; i < g_thread_pool.count; i++) {
        RtThreadHandle *handle = g_thread_pool.handles[i];
        if (handle != NULL && !handle->synced) {
            pthread_cancel(handle->thread);
        }
    }

    /* Free the handles array */
    free(g_thread_pool.handles);
    g_thread_pool.handles = NULL;
    g_thread_pool.count = 0;
    g_thread_pool.capacity = 0;
    g_thread_pool_initialized = false;

    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* ============================================================================
 * Thread Panic Context Functions
 * ============================================================================ */

/* Initialize a panic context for the current thread */
void rt_thread_panic_context_init(RtThreadPanicContext *ctx,
                                   RtThreadResult *result,
                                   RtArena *arena)
{
    if (ctx == NULL) {
        fprintf(stderr, "rt_thread_panic_context_init: NULL context\n");
        return;
    }

    ctx->is_active = true;
    ctx->result = result;
    ctx->arena = arena;

    /* Set as the current thread's panic context */
    rt_thread_panic_ctx = ctx;
}

/* Clear the panic context for the current thread */
void rt_thread_panic_context_clear(void)
{
    if (rt_thread_panic_ctx != NULL) {
        rt_thread_panic_ctx->is_active = false;
    }
    rt_thread_panic_ctx = NULL;
}

/* Check if the current thread has a panic context installed */
bool rt_thread_has_panic_context(void)
{
    return rt_thread_panic_ctx != NULL && rt_thread_panic_ctx->is_active;
}

/* Trigger a panic in the current thread
 * If a panic context is active, longjmp to the handler.
 * Otherwise, print message and exit(1).
 */
void rt_thread_panic(const char *message)
{
    if (rt_thread_has_panic_context()) {
        /* We're in a thread with a panic handler - capture and longjmp */
        RtThreadPanicContext *ctx = rt_thread_panic_ctx;

        /* Set panic state in the result */
        if (ctx->result != NULL) {
            rt_thread_result_set_panic(ctx->result, message, ctx->arena);
        }

        /* Jump back to the wrapper function's setjmp point */
        longjmp(ctx->jump_buffer, 1);
    } else {
        /* No panic handler - print and exit (main thread behavior) */
        if (message != NULL) {
            fprintf(stderr, "panic: %s\n", message);
        } else {
            fprintf(stderr, "panic: (no message)\n");
        }
        exit(1);
    }
}

/* ============================================================================
 * Thread Result Functions
 * ============================================================================ */

/* Create a thread result structure in the given arena */
RtThreadResult *rt_thread_result_create(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_result_create: NULL arena\n");
        return NULL;
    }

    RtThreadResult *result = rt_arena_alloc(arena, sizeof(RtThreadResult));
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_create: allocation failed\n");
        exit(1);
    }

    result->value = NULL;
    result->value_size = 0;
    result->has_panic = false;
    result->panic_message = NULL;

    return result;
}

/* Set panic state on a thread result */
void rt_thread_result_set_panic(RtThreadResult *result, const char *message,
                                 RtArena *arena)
{
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_set_panic: NULL result\n");
        return;
    }

    result->has_panic = true;
    if (message != NULL && arena != NULL) {
        result->panic_message = rt_arena_strdup(arena, message);
    } else {
        result->panic_message = NULL;
    }
}

/* Set value on a thread result */
void rt_thread_result_set_value(RtThreadResult *result, void *value,
                                 size_t size, RtArena *arena)
{
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_set_value: NULL result\n");
        return;
    }

    if (value != NULL && size > 0 && arena != NULL) {
        /* Copy value to arena for safety */
        result->value = rt_arena_alloc(arena, size);
        if (result->value != NULL) {
            memcpy(result->value, value, size);
        }
    } else {
        result->value = value;
    }
    result->value_size = size;
}

/* ============================================================================
 * Thread Handle Functions
 * ============================================================================ */

/* Create a new thread handle in the given arena */
RtThreadHandle *rt_thread_handle_create(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_handle_create: NULL arena\n");
        return NULL;
    }

    RtThreadHandle *handle = rt_arena_alloc(arena, sizeof(RtThreadHandle));
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_handle_create: allocation failed\n");
        exit(1);
    }

    handle->thread = 0;
    handle->result = NULL;
    handle->done = false;
    handle->synced = false;
    handle->thread_arena = NULL;
    handle->frozen_arena = NULL;
    handle->caller_arena = NULL;
    handle->result_type = -1;  /* -1 indicates void/unknown */
    handle->is_shared = false;
    handle->is_private = false;

    return handle;
}

/* ============================================================================
 * Thread Arguments Functions
 * ============================================================================ */

/* Create thread arguments structure in the given arena */
RtThreadArgs *rt_thread_args_create(RtArena *arena, void *func_ptr,
                                     void *args_data, size_t args_size)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_args_create: NULL arena\n");
        return NULL;
    }

    RtThreadArgs *args = rt_arena_alloc(arena, sizeof(RtThreadArgs));
    if (args == NULL) {
        fprintf(stderr, "rt_thread_args_create: allocation failed\n");
        exit(1);
    }

    args->func_ptr = func_ptr;
    args->args_size = args_size;
    args->result = NULL;
    args->caller_arena = NULL;
    args->thread_arena = NULL;
    args->is_shared = false;
    args->is_private = false;

    /* Copy args_data if provided */
    if (args_data != NULL && args_size > 0) {
        args->args_data = rt_arena_alloc(arena, args_size);
        if (args->args_data == NULL) {
            fprintf(stderr, "rt_thread_args_create: args_data allocation failed\n");
            exit(1);
        }
        memcpy(args->args_data, args_data, args_size);
    } else {
        args->args_data = NULL;
    }

    return args;
}

/* ============================================================================
 * Thread Spawn and Sync
 * ============================================================================ */

/* Spawn a new thread (implements & operator) */
RtThreadHandle *rt_thread_spawn(RtArena *arena, void *(*wrapper)(void *),
                                 RtThreadArgs *args)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_spawn: NULL arena\n");
        return NULL;
    }
    if (wrapper == NULL) {
        fprintf(stderr, "rt_thread_spawn: NULL wrapper function\n");
        return NULL;
    }
    if (args == NULL) {
        fprintf(stderr, "rt_thread_spawn: NULL args\n");
        return NULL;
    }

    /* Create thread handle */
    RtThreadHandle *handle = rt_thread_handle_create(arena);
    if (handle == NULL) {
        return NULL;
    }

    /* Create result structure for the thread to populate */
    handle->result = rt_thread_result_create(arena);
    if (handle->result == NULL) {
        return NULL;
    }

    /* Link result to args so thread wrapper can access it */
    args->result = handle->result;

    /* Copy mode flags from args to handle for use at sync time */
    handle->is_shared = args->is_shared;
    handle->is_private = args->is_private;
    handle->caller_arena = args->caller_arena;

    /* Create thread arena based on mode flags:
     * - shared mode (is_shared=true): thread_arena = caller_arena (reuse parent)
     * - private mode (is_private=true): thread_arena = rt_arena_create(NULL) (isolated)
     * - default mode (both false): thread_arena = rt_arena_create(caller_arena) (own with parent)
     */
    if (args->is_shared) {
        /* Shared mode: reuse caller's arena directly */
        args->thread_arena = args->caller_arena;
        handle->thread_arena = NULL;  /* Don't destroy - it's the caller's */
        /* Note: Arena freezing happens AFTER thread tracking to allow allocation */
        handle->frozen_arena = args->caller_arena;
    } else if (args->is_private) {
        /* Private mode: create isolated arena with no parent */
        args->thread_arena = rt_arena_create(NULL);
        if (args->thread_arena == NULL) {
            fprintf(stderr, "rt_thread_spawn: failed to create private thread arena\n");
            return NULL;
        }
        handle->thread_arena = args->thread_arena;
    } else {
        /* Default mode: create own arena with caller as parent for promotion */
        args->thread_arena = rt_arena_create(args->caller_arena);
        if (args->thread_arena == NULL) {
            fprintf(stderr, "rt_thread_spawn: failed to create thread arena\n");
            return NULL;
        }
        handle->thread_arena = args->thread_arena;
    }

    /* Create the pthread */
    int err = pthread_create(&handle->thread, NULL, wrapper, args);
    if (err != 0) {
        fprintf(stderr, "rt_thread_spawn: pthread_create failed with error %d\n", err);
        /* Clean up thread arena on failure */
        if (handle->thread_arena != NULL && !args->is_shared) {
            rt_arena_destroy(handle->thread_arena);
        }
        /* Unfreeze arena if it was frozen */
        if (handle->frozen_arena != NULL) {
            rt_arena_unfreeze(handle->frozen_arena);
            handle->frozen_arena = NULL;
        }
        return NULL;
    }

    /* Note: For shared mode, frozen_owner is set by the thread wrapper itself
     * using pthread_self() to avoid a race condition where the thread starts
     * before we can set frozen_owner here. */

    /* Track in global pool */
    rt_thread_pool_add(handle);

    /* Track in caller's arena so arena destruction auto-joins the thread */
    if (args->caller_arena != NULL) {
        rt_arena_track_thread(args->caller_arena, handle);
    }

    /* For shared mode, set frozen_owner to the spawned thread handle and freeze.
     * The thread wrapper also sets frozen_owner = pthread_self() as its first
     * action, but this may not happen before we freeze due to thread scheduling.
     * Setting it here with the real handle ensures it's set before the freeze.
     * On Windows, pthread_equal uses GetThreadId() which works for both real
     * handles and pseudo-handles (GetCurrentThread()), returning the same ID. */
    if (args->is_shared && handle->frozen_arena != NULL) {
        handle->frozen_arena->frozen_owner = handle->thread;
        rt_arena_freeze(handle->frozen_arena);
    }

    return handle;
}

/* Check if a thread has completed without blocking */
bool rt_thread_is_done(RtThreadHandle *handle)
{
    if (handle == NULL) {
        return true;  /* Treat NULL as done */
    }
    return handle->done;
}

/* Check if a result type is a primitive (does not need promotion) */
static bool rt_result_type_is_primitive(int result_type)
{
    return result_type == RT_TYPE_VOID ||
           result_type == RT_TYPE_INT ||
           result_type == RT_TYPE_LONG ||
           result_type == RT_TYPE_DOUBLE ||
           result_type == RT_TYPE_BOOL ||
           result_type == RT_TYPE_BYTE ||
           result_type == RT_TYPE_CHAR;
}

/* Join a thread and retrieve its result (low-level pthread_join wrapper)
 * This function waits for the thread to complete and returns the result value.
 * It handles both void and non-void return types:
 * - For void functions, returns NULL
 * - For non-void functions, returns pointer to the result value
 * The done flag is set to mark the thread as synchronized.
 * No double-join protection - caller is responsible for checking done flag.
 *
 * For default mode threads (not shared, not private):
 * - If result is non-primitive, promotes to caller arena before returning
 * - Thread arena is destroyed after promotion
 */
void *rt_thread_join(RtThreadHandle *handle)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_join: NULL handle\n");
        return NULL;
    }

    /* Wait for thread to complete using pthread_join */
    int err = pthread_join(handle->thread, NULL);
    if (err != 0) {
        fprintf(stderr, "rt_thread_join: pthread_join failed with error %d\n", err);
        return NULL;
    }

    /* Mark thread as done/synchronized */
    handle->done = true;
    handle->synced = true;

    /* Unfreeze caller arena if it was frozen for shared mode */
    if (handle->frozen_arena != NULL) {
        rt_arena_unfreeze(handle->frozen_arena);
        handle->frozen_arena = NULL;
    }

    /* Remove from global pool since thread has completed */
    rt_thread_pool_remove(handle);

    /* Get raw result value - caller is responsible for promotion and cleanup */
    void *result_value = NULL;
    if (handle->result != NULL) {
        result_value = handle->result->value;
    }

    return result_value;
}

/* Synchronize a thread handle (implements ! operator)
 * This is the high-level sync that also handles panic propagation.
 * Used for void syncs where no result value is returned.
 */
void rt_thread_sync(RtThreadHandle *handle)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_sync: NULL handle\n");
        return;
    }

    /* Already synced - return early */
    if (handle->synced) {
        return;
    }

    /* Join the thread to wait for completion */
    rt_thread_join(handle);

    /* Check for panic and propagate */
    if (handle->result != NULL && handle->result->has_panic) {
        /* Clean up thread arena before panicking */
        if (handle->thread_arena != NULL) {
            rt_arena_destroy(handle->thread_arena);
            handle->thread_arena = NULL;
        }
        /* Re-panic in the calling thread using rt_thread_panic
         * This will either longjmp if we're in a nested thread,
         * or exit(1) if we're in the main thread.
         */
        const char *msg = handle->result->panic_message;
        rt_thread_panic(msg);
    }

    /* Clean up thread arena for private and default modes
     * Shared mode has thread_arena == NULL, so no cleanup needed */
    if (handle->thread_arena != NULL) {
        rt_arena_destroy(handle->thread_arena);
        handle->thread_arena = NULL;
    }
}

/* Synchronize a thread handle and get promoted result
 * Promotes result to caller's arena before returning.
 * Handles panic propagation with message promotion.
 */
void *rt_thread_sync_with_result(RtThreadHandle *handle,
                                  RtArena *caller_arena,
                                  RtResultType result_type)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_sync_with_result: NULL handle\n");
        return NULL;
    }

    /* Already synced - return early */
    if (handle->synced) {
        /* If already synced, we can't get the result (it was already consumed) */
        return NULL;
    }

    /* Join the thread to wait for completion */
    rt_thread_join(handle);

    /* Check for panic and propagate with promoted message */
    if (handle->result != NULL && handle->result->has_panic) {
        /* Promote panic message to caller's arena before propagating */
        const char *msg = handle->result->panic_message;
        char *promoted_msg = NULL;
        if (msg != NULL && caller_arena != NULL) {
            promoted_msg = rt_arena_promote_string(caller_arena, msg);
        }

        /* Cleanup thread arena before panicking */
        if (handle->thread_arena != NULL) {
            rt_arena_destroy(handle->thread_arena);
            handle->thread_arena = NULL;
        }

        /* Re-panic in the calling thread */
        rt_thread_panic(promoted_msg != NULL ? promoted_msg : msg);

        /* Should not reach here - but return NULL just in case */
        return NULL;
    }

    /* Get result value */
    void *result_value = NULL;
    if (handle->result != NULL) {
        result_value = handle->result->value;
    }

    /* For shared mode (thread_arena == NULL), the result is already in
     * the caller's arena, so no promotion is needed.
     * However, result_value is a pointer to where the actual value is stored
     * (due to how rt_thread_result_set_value works), so we need to dereference
     * once for reference types (strings, arrays, etc.). For primitives, we
     * return the pointer as-is so caller can dereference to get the value.
     * For reference types, we dereference to get the actual pointer. */
    if (handle->thread_arena == NULL) {
        /* Shared mode - no promotion needed, but need to dereference for ref types */
        if (result_value != NULL && !rt_result_type_is_primitive(result_type)) {
            /* For reference types, dereference once to get actual pointer */
            return *(void **)result_value;
        }
        return result_value;
    }

    /* Default/private mode - promote result to caller's arena */
    void *promoted_result = NULL;
    if (result_value != NULL) {
        promoted_result = rt_thread_promote_result(
            caller_arena,
            handle->thread_arena,
            result_value,
            result_type
        );
    }

    /* Cleanup thread arena now that result is promoted */
    rt_arena_destroy(handle->thread_arena);
    handle->thread_arena = NULL;

    return promoted_result;
}

/* Synchronize multiple thread handles (implements [r1, r2, ...]! syntax) */
void rt_thread_sync_all(RtThreadHandle **handles, size_t count)
{
    if (handles == NULL || count == 0) {
        return;
    }

    /* Sync each thread in order */
    for (size_t i = 0; i < count; i++) {
        if (handles[i] != NULL) {
            rt_thread_sync(handles[i]);
        }
    }
}

/* ============================================================================
 * Thread Result Promotion
 * ============================================================================ */

/* Promote a thread result value to a destination arena
 * This handles all types appropriately:
 * - Primitives (int, long, double, bool, byte, char): copied by value
 * - Strings: promoted using rt_arena_promote_string
 * - Arrays: cloned using appropriate rt_array_clone_* function
 * - Files: promoted using rt_text_file_promote or rt_binary_file_promote
 */
void *rt_thread_promote_result(RtArena *dest, RtArena *src_arena,
                                void *value, RtResultType type)
{
    if (dest == NULL) {
        fprintf(stderr, "rt_thread_promote_result: NULL dest arena\n");
        return NULL;
    }

    if (value == NULL) {
        /* NULL values are valid for void and reference types */
        return NULL;
    }

    switch (type) {
        case RT_TYPE_VOID:
            /* Void has no value to promote */
            return NULL;

        /* Primitive types - copy by value
         * Note: Sindarin int is 64-bit, stored as long long in C code
         * (on Windows/MinGW, long is 32-bit, so we must use long long) */
        case RT_TYPE_INT:
        case RT_TYPE_LONG: {
            long long *promoted = rt_arena_alloc(dest, sizeof(long long));
            if (promoted != NULL) {
                *promoted = *(long long *)value;
            }
            return promoted;
        }

        case RT_TYPE_DOUBLE: {
            double *promoted = rt_arena_alloc(dest, sizeof(double));
            if (promoted != NULL) {
                *promoted = *(double *)value;
            }
            return promoted;
        }

        case RT_TYPE_BOOL: {
            int *promoted = rt_arena_alloc(dest, sizeof(int));
            if (promoted != NULL) {
                *promoted = *(int *)value;
            }
            return promoted;
        }

        case RT_TYPE_BYTE: {
            unsigned char *promoted = rt_arena_alloc(dest, sizeof(unsigned char));
            if (promoted != NULL) {
                *promoted = *(unsigned char *)value;
            }
            return promoted;
        }

        case RT_TYPE_CHAR: {
            char *promoted = rt_arena_alloc(dest, sizeof(char));
            if (promoted != NULL) {
                *promoted = *(char *)value;
            }
            return promoted;
        }

        /* String type - use arena string promotion
         * Note: value is a pointer to the char* (from rt_thread_result_set_value),
         * so we need to dereference once to get the actual string pointer. */
        case RT_TYPE_STRING: {
            const char *str = *(const char **)value;
            return rt_arena_promote_string(dest, str);
        }

        /* Array types - use appropriate clone function
         * Note: value is a pointer to the array pointer (from rt_thread_result_set_value),
         * so we need to dereference once to get the actual array pointer. */
        case RT_TYPE_ARRAY_INT: {
            /* int[] is stored as long long[] in runtime */
            long long *arr = *(long long **)value;
            return rt_array_clone_long(dest, arr);
        }

        case RT_TYPE_ARRAY_LONG: {
            long long *arr = *(long long **)value;
            return rt_array_clone_long(dest, arr);
        }

        case RT_TYPE_ARRAY_DOUBLE: {
            double *arr = *(double **)value;
            return rt_array_clone_double(dest, arr);
        }

        case RT_TYPE_ARRAY_BOOL: {
            int *arr = *(int **)value;
            return rt_array_clone_bool(dest, arr);
        }

        case RT_TYPE_ARRAY_BYTE: {
            unsigned char *arr = *(unsigned char **)value;
            return rt_array_clone_byte(dest, arr);
        }

        case RT_TYPE_ARRAY_CHAR: {
            char *arr = *(char **)value;
            return rt_array_clone_char(dest, arr);
        }

        case RT_TYPE_ARRAY_STRING: {
            char **arr = *(char ***)value;
            return rt_array_clone_string(dest, arr);
        }

        default:
            fprintf(stderr, "rt_thread_promote_result: unknown type %d\n", type);
            return NULL;
    }
}

/* ============================================================================
 * Sync Variable Lock Table
 * ============================================================================
 * Hash table mapping variable addresses to mutexes for lock blocks.
 * Uses a simple open-addressed hash table with linear probing.
 * ============================================================================ */

#define RT_SYNC_LOCK_TABLE_SIZE 256  /* Must be power of 2 */

typedef struct RtSyncLockEntry {
    void *addr;                      /* Variable address (NULL = empty slot) */
    pthread_mutex_t mutex;           /* Associated mutex */
    bool initialized;                /* True if mutex is initialized */
} RtSyncLockEntry;

static RtSyncLockEntry g_sync_lock_table[RT_SYNC_LOCK_TABLE_SIZE];
static pthread_mutex_t g_sync_lock_table_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_sync_lock_table_initialized = false;

/* Initialize the sync lock table */
void rt_sync_lock_table_init(void)
{
    if (g_sync_lock_table_initialized) {
        return;
    }

    pthread_mutex_lock(&g_sync_lock_table_mutex);
    if (!g_sync_lock_table_initialized) {
        for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
            g_sync_lock_table[i].addr = NULL;
            g_sync_lock_table[i].initialized = false;
        }
        g_sync_lock_table_initialized = true;
    }
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
}

/* Clean up all sync locks */
void rt_sync_lock_table_cleanup(void)
{
    if (!g_sync_lock_table_initialized) {
        return;
    }

    pthread_mutex_lock(&g_sync_lock_table_mutex);
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        if (g_sync_lock_table[i].initialized) {
            pthread_mutex_destroy(&g_sync_lock_table[i].mutex);
            g_sync_lock_table[i].initialized = false;
            g_sync_lock_table[i].addr = NULL;
        }
    }
    g_sync_lock_table_initialized = false;
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
}

/* Hash function for pointer addresses */
static unsigned int rt_sync_lock_hash(void *addr)
{
    /* Simple hash: use bits from the pointer, shift to avoid alignment patterns */
    uintptr_t val = (uintptr_t)addr;
    val = (val >> 3) ^ (val >> 7) ^ (val >> 11);
    return (unsigned int)(val & (RT_SYNC_LOCK_TABLE_SIZE - 1));
}

/* Find or create a mutex for an address */
static pthread_mutex_t *rt_sync_lock_get_mutex(void *addr)
{
    if (!g_sync_lock_table_initialized) {
        rt_sync_lock_table_init();
    }

    unsigned int hash = rt_sync_lock_hash(addr);

    /* Lock table for safe access */
    pthread_mutex_lock(&g_sync_lock_table_mutex);

    /* Linear probe to find existing or empty slot */
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        int idx = (hash + i) & (RT_SYNC_LOCK_TABLE_SIZE - 1);

        if (g_sync_lock_table[idx].addr == addr && g_sync_lock_table[idx].initialized) {
            /* Found existing entry */
            pthread_mutex_unlock(&g_sync_lock_table_mutex);
            return &g_sync_lock_table[idx].mutex;
        }

        if (g_sync_lock_table[idx].addr == NULL) {
            /* Empty slot - create new entry */
            g_sync_lock_table[idx].addr = addr;
            pthread_mutex_init(&g_sync_lock_table[idx].mutex, NULL);
            g_sync_lock_table[idx].initialized = true;
            pthread_mutex_unlock(&g_sync_lock_table_mutex);
            return &g_sync_lock_table[idx].mutex;
        }
    }

    /* Table is full - this shouldn't happen in practice */
    pthread_mutex_unlock(&g_sync_lock_table_mutex);
    fprintf(stderr, "rt_sync_lock_get_mutex: lock table full\n");
    return NULL;
}

/* Acquire a mutex lock for a sync variable */
void rt_sync_lock(void *addr)
{
    if (addr == NULL) {
        fprintf(stderr, "rt_sync_lock: NULL address\n");
        return;
    }

    pthread_mutex_t *mutex = rt_sync_lock_get_mutex(addr);
    if (mutex != NULL) {
        pthread_mutex_lock(mutex);
    }
}

/* Release a mutex lock for a sync variable */
void rt_sync_unlock(void *addr)
{
    if (addr == NULL) {
        fprintf(stderr, "rt_sync_unlock: NULL address\n");
        return;
    }

    /* Find the existing mutex */
    if (!g_sync_lock_table_initialized) {
        fprintf(stderr, "rt_sync_unlock: table not initialized\n");
        return;
    }

    unsigned int hash = rt_sync_lock_hash(addr);

    /* Linear probe to find existing entry */
    for (int i = 0; i < RT_SYNC_LOCK_TABLE_SIZE; i++) {
        int idx = (hash + i) & (RT_SYNC_LOCK_TABLE_SIZE - 1);

        if (g_sync_lock_table[idx].addr == addr && g_sync_lock_table[idx].initialized) {
            pthread_mutex_unlock(&g_sync_lock_table[idx].mutex);
            return;
        }

        if (g_sync_lock_table[idx].addr == NULL) {
            /* Empty slot - mutex was never created */
            fprintf(stderr, "rt_sync_unlock: no mutex found for address %p\n", addr);
            return;
        }
    }

    fprintf(stderr, "rt_sync_unlock: mutex not found for address %p\n", addr);
}
