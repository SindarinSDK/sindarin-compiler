#ifndef RUNTIME_THREAD_H
#define RUNTIME_THREAD_H

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
#include <stdbool.h>
#include <setjmp.h>
#include "runtime_arena.h"

/* Thread-local storage compatibility:
 * - MSVC: use __declspec(thread)
 * - GCC/Clang: use __thread
 * - TinyCC: no TLS support, falls back to global (NOT thread-safe)
 */
#ifdef __TINYC__
#define RT_THREAD_LOCAL /* TinyCC: no TLS, threading features limited */
#elif defined(_MSC_VER)
#define RT_THREAD_LOCAL __declspec(thread)
#else
#define RT_THREAD_LOCAL __thread
#endif

/* ============================================================================
 * Thread Types - Core Structures
 * ============================================================================
 * Sindarin threads use the & operator for spawning and ! for synchronization.
 * These structures support the runtime implementation of threaded execution.
 * ============================================================================ */

/* Forward declarations */
typedef struct RtThreadHandle RtThreadHandle;
typedef struct RtThreadArgs RtThreadArgs;
typedef struct RtThreadResult RtThreadResult;

/* ============================================================================
 * Thread Result Structure
 * ============================================================================
 * Captures the return value from a thread and any panic state.
 * Used to propagate both successful results and errors on synchronization.
 * ============================================================================ */

typedef struct RtThreadResult {
    void *value;              /* Pointer to result value (type-specific) */
    size_t value_size;        /* Size of the result value in bytes */
    bool has_panic;           /* True if thread panicked */
    char *panic_message;      /* Panic message (if has_panic is true) */
} RtThreadResult;

/* ============================================================================
 * Thread Handle Structure
 * ============================================================================
 * Represents a spawned thread. The handle tracks the pthread, result state,
 * and synchronization status. Created by & operator, consumed by ! operator.
 * ============================================================================ */

typedef struct RtThreadHandle {
    pthread_t thread;         /* Underlying POSIX thread */
    RtThreadResult *result;   /* Result from thread execution */
    bool done;                /* True when thread has completed */
    bool synced;              /* True when ! operator has been applied */
    RtArena *thread_arena;    /* Arena used by the thread (for cleanup) */
    RtArena *frozen_arena;    /* Arena that was frozen for shared mode (NULL if none) */
    RtArena *caller_arena;    /* Caller's arena (for default mode promotion) */
    int result_type;          /* RtResultType for result promotion (-1 if void) */
    bool is_shared;           /* True if function uses shared arena semantics */
    bool is_private;          /* True if function uses private arena semantics */
} RtThreadHandle;

/* ============================================================================
 * Thread Arguments Structure
 * ============================================================================
 * Packages arguments for passing to a thread wrapper function.
 * Includes the function pointer, captured arguments, and result destination.
 * ============================================================================ */

typedef struct RtThreadArgs {
    void *func_ptr;           /* Pointer to the function to execute */
    void *args_data;          /* Packed argument data */
    size_t args_size;         /* Size of args_data in bytes */
    RtThreadResult *result;   /* Where to store the result */
    RtArena *caller_arena;    /* Caller's arena (for shared functions) */
    RtArena *thread_arena;    /* Thread's own arena */
    bool is_shared;           /* True if function uses shared arena semantics */
    bool is_private;          /* True if function uses private arena semantics */
} RtThreadArgs;

/* ============================================================================
 * Thread Handle Pool (for tracking all active threads)
 * ============================================================================
 * Maintains a list of all active thread handles for cleanup on process exit.
 * Fire-and-forget threads are tracked here to ensure proper termination.
 * ============================================================================ */

typedef struct RtThreadPool {
    RtThreadHandle **handles; /* Array of active thread handles */
    size_t count;             /* Number of active handles */
    size_t capacity;          /* Capacity of handles array */
    pthread_mutex_t mutex;    /* Mutex for thread-safe access */
} RtThreadPool;

/* ============================================================================
 * Thread Panic Context
 * ============================================================================
 * Used by thread wrappers to catch panics (setjmp/longjmp) and store them
 * in the thread result for propagation on synchronization.
 * ============================================================================ */

typedef struct RtThreadPanicContext {
    jmp_buf jump_buffer;          /* setjmp buffer for panic recovery */
    bool is_active;               /* True if panic handler is installed */
    RtThreadResult *result;       /* Where to store panic state */
    RtArena *arena;               /* Arena for panic message allocation */
} RtThreadPanicContext;

/* Thread-local panic context for the current thread */
extern RT_THREAD_LOCAL RtThreadPanicContext *rt_thread_panic_ctx;

/* Initialize a panic context for the current thread */
void rt_thread_panic_context_init(RtThreadPanicContext *ctx,
                                   RtThreadResult *result,
                                   RtArena *arena);

/* Clear the panic context for the current thread */
void rt_thread_panic_context_clear(void);

/* Trigger a panic in the current thread
 * If a panic context is active, longjmp to the handler.
 * Otherwise, print message and exit(1).
 */
void rt_thread_panic(const char *message);

/* Check if the current thread has a panic context installed */
bool rt_thread_has_panic_context(void);

/* ============================================================================
 * Thread Function Declarations
 * ============================================================================ */

/* Create a new thread handle in the given arena */
RtThreadHandle *rt_thread_handle_create(RtArena *arena);

/* Create thread arguments structure in the given arena */
RtThreadArgs *rt_thread_args_create(RtArena *arena, void *func_ptr,
                                     void *args_data, size_t args_size);

/* Create a thread result structure in the given arena */
RtThreadResult *rt_thread_result_create(RtArena *arena);

/* Spawn a new thread (implements & operator) */
RtThreadHandle *rt_thread_spawn(RtArena *arena, void *(*wrapper)(void *),
                                 RtThreadArgs *args);

/* Join a thread and retrieve its result (low-level pthread_join wrapper) */
void *rt_thread_join(RtThreadHandle *handle);

/* Synchronize a thread handle (implements ! operator) */
void rt_thread_sync(RtThreadHandle *handle);

/* Synchronize multiple thread handles (implements [r1, r2, ...]! syntax) */
void rt_thread_sync_all(RtThreadHandle **handles, size_t count);

/* Check if a thread has completed without blocking */
bool rt_thread_is_done(RtThreadHandle *handle);

/* Set panic state on a thread result */
void rt_thread_result_set_panic(RtThreadResult *result, const char *message,
                                 RtArena *arena);

/* Set value on a thread result */
void rt_thread_result_set_value(RtThreadResult *result, void *value,
                                 size_t size, RtArena *arena);

/* ============================================================================
 * Result Type Identifiers
 * ============================================================================
 * Used by rt_thread_promote_result to determine how to promote values.
 * ============================================================================ */

typedef enum {
    RT_TYPE_VOID = 0,           /* void (no value) */
    RT_TYPE_INT,                /* int (32-bit) */
    RT_TYPE_LONG,               /* long (64-bit) */
    RT_TYPE_DOUBLE,             /* double */
    RT_TYPE_BOOL,               /* bool (int) */
    RT_TYPE_BYTE,               /* byte (unsigned char) */
    RT_TYPE_CHAR,               /* char */
    RT_TYPE_STRING,             /* str (char *) */
    RT_TYPE_ARRAY_INT,          /* int[] */
    RT_TYPE_ARRAY_LONG,         /* long[] */
    RT_TYPE_ARRAY_DOUBLE,       /* double[] */
    RT_TYPE_ARRAY_BOOL,         /* bool[] */
    RT_TYPE_ARRAY_BYTE,         /* byte[] */
    RT_TYPE_ARRAY_CHAR,         /* char[] */
    RT_TYPE_ARRAY_STRING        /* str[] */
} RtResultType;

/* ============================================================================
 * Thread Result Promotion
 * ============================================================================
 * Promote thread results from source arena to destination arena.
 * ============================================================================ */

/* Promote a thread result value to a destination arena
 * This handles all types appropriately:
 * - Primitives (int, long, double, bool, byte, char): copied by value
 * - Strings: promoted using rt_arena_promote_string
 * - Arrays: cloned using appropriate rt_array_clone_* function
 */
void *rt_thread_promote_result(RtArena *dest, RtArena *src_arena,
                                void *value, RtResultType type);

/* Synchronize a thread handle and get promoted result
 * Promotes result to caller's arena before returning.
 * Handles panic propagation with message promotion.
 */
void *rt_thread_sync_with_result(RtThreadHandle *handle,
                                  RtArena *caller_arena,
                                  RtResultType result_type);

/* ============================================================================
 * Thread Pool Management
 * ============================================================================ */

/* Initialize the global thread pool */
void rt_thread_pool_init(void);

/* Add a thread handle to the pool */
void rt_thread_pool_add(RtThreadHandle *handle);

/* Remove a thread handle from the pool */
void rt_thread_pool_remove(RtThreadHandle *handle);

/* Clean up all threads in the pool (called on process exit) */
void rt_thread_pool_cleanup(void);

/* ============================================================================
 * Sync Variable Lock/Unlock for lock blocks
 * ============================================================================
 * These functions provide mutex-based synchronization for sync variables
 * when using lock(var) => { ... } blocks.
 * ============================================================================ */

/* Acquire a mutex lock for a sync variable (by address)
 * Creates mutex on first use. Thread-safe.
 */
void rt_sync_lock(void *addr);

/* Release a mutex lock for a sync variable (by address)
 * Must be called after rt_sync_lock with the same address.
 */
void rt_sync_unlock(void *addr);

/* Initialize the sync lock table (called automatically if needed) */
void rt_sync_lock_table_init(void);

/* Clean up all sync locks (called on process exit) */
void rt_sync_lock_table_cleanup(void);

#endif /* RUNTIME_THREAD_H */
