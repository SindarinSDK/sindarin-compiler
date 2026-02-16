/* ============================================================================
 * Thread V3 - Clean Lifecycle Threading for Sindarin
 * ============================================================================
 *
 * Design principles:
 * 1. RtHandleV2 is first-class - all thread access via handle
 * 2. Clean lifecycle - create, start, sync, dispose (4 public functions)
 * 3. All promotion in runtime - copy_callback handles deep copy
 * 4. RtThread arena-allocated - transactions protect pthread primitives from GC
 *
 * Public API:
 *   rt_thread_v3_create  - Create thread handle
 *   rt_thread_v3_start   - Start the thread
 *   rt_thread_v3_sync    - Wait, promote result, cleanup
 *   rt_thread_v3_dispose - Explicit cleanup (for fire-and-forget)
 *
 * ============================================================================ */

#ifndef RUNTIME_THREAD_V3_H
#define RUNTIME_THREAD_V3_H

#include <stdbool.h>
#include "runtime/arena/arena_v2.h"

/* Platform-specific pthread include */
#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        #include <pthread.h>
    #else
        #include "platform/compat_pthread.h"
    #endif
#else
    #include <pthread.h>
#endif

/* Thread-local storage compatibility */
#ifdef __TINYC__
    #define RT_THREAD_LOCAL
#elif defined(_MSC_VER)
    #define RT_THREAD_LOCAL __declspec(thread)
#else
    #define RT_THREAD_LOCAL __thread
#endif

/* Backwards compat for any code still using RT_THREAD_LOCAL_V2 */
#define RT_THREAD_LOCAL_V2 RT_THREAD_LOCAL

/* ============================================================================
 * Thread Mode - Matches Function Arena Modes
 * ============================================================================
 * Default: Thread gets own arena (child of caller), result promoted at sync
 * Shared:  Thread uses caller's arena, no promotion needed
 * Private: Thread gets isolated arena, only void/primitives returned
 * ============================================================================ */

typedef enum {
    RT_THREAD_MODE_DEFAULT,     /* Own arena (child of caller), promote on sync */
    RT_THREAD_MODE_SHARED,      /* Use caller's arena directly */
    RT_THREAD_MODE_PRIVATE      /* Isolated arena, void/primitives only */
} RtThreadMode;

/* ============================================================================
 * RtThread - Single Structure
 * ============================================================================
 * Allocated in CALLER arena (survives until sync or arena cleanup).
 * t->arena is thread's working arena (NULL if shared mode).
 * ============================================================================ */

typedef struct RtThread {
    pthread_t pthread;
    uint64_t thread_id;         /* Unique runtime thread ID */

    RtArenaV2 *arena;           /* Thread's own arena (NULL if shared) */
    RtArenaV2 *caller;          /* Caller's arena (owns this struct) */
    RtThreadMode mode;          /* Thread mode for sync behavior */

    pthread_mutex_t mutex;      /* Destroyed by sync or dispose */
    pthread_cond_t cond;
    bool done;
    bool disposed;              /* Dispose tracking */

    RtHandleV2 *self_handle;    /* Handle to this RtThread in caller arena */
    RtHandleV2 *args;           /* Handle to packed args (in thread arena) */
    RtHandleV2 *result;         /* Result handle (NULL for void) */
    char *panic_msg;            /* NULL = success */
} RtThread;

/* ============================================================================
 * Deep Copy via copy_callback
 * ============================================================================
 * All deep copying is handled by RtHandleV2CopyCallback set on each handle.
 * Codegen sets appropriate callbacks at allocation time:
 *
 *   - Primitives/primitive arrays: no callback (shallow copy is sufficient)
 *   - str[]: callback promotes each string element
 *   - T[][]: callback promotes each inner array (recursively)
 *   - Structs with handles: callback promotes handle fields
 *   - Arrays of structs: callback iterates and promotes each struct's fields
 *
 * rt_arena_v2_clone() calls the callback automatically after shallow copy.
 * This means rt_thread_v3_sync() just calls rt_arena_v2_promote() and
 * everything works.
 * ============================================================================ */

/* ============================================================================
 * Public API - Thread Lifecycle
 * ============================================================================ */

/* Create thread. Returns handle to RtThread (allocated in caller arena).
 *
 * Usage:
 *   RtHandleV2 *th = rt_thread_v3_create(caller_arena, mode);
 *   rt_handle_begin_transaction(th);
 *   RtThread *t = (RtThread *)th->ptr;
 *   // setup t->args, etc.
 *   rt_handle_end_transaction(th);
 */
RtHandleV2 *rt_thread_v3_create(RtArenaV2 *caller, RtThreadMode mode);

/* Start thread execution.
 * wrapper receives RtHandleV2* (NOT RtThread*) as argument.
 *
 * Usage:
 *   rt_thread_v3_start(thread_handle, my_wrapper);
 */
void rt_thread_v3_start(RtHandleV2 *thread_handle, void *(*wrapper)(void *));

/* Sync: Wait for completion, promote result, cleanup.
 *
 * Parameters:
 *   thread_handle - Handle to RtThread
 *
 * Returns: Promoted result handle in caller arena (NULL for void/primitives)
 *
 * The result handle's copy_callback (set by codegen) is invoked automatically
 * during promotion for deep copying.
 *
 * After sync:
 *   - Thread arena is condemned (unless shared mode)
 *   - Mutex/cond are destroyed
 *   - Handle is marked dead
 *   - If thread panicked, re-raises panic in caller context
 */
RtHandleV2 *rt_thread_v3_sync(RtHandleV2 *thread_handle);

/* Dispose: Cleanup without waiting (for fire-and-forget from worker thread).
 * Called by worker wrapper when no sync will ever happen.
 *
 * After dispose:
 *   - Thread arena is condemned
 *   - Mutex/cond are destroyed
 *   - Handle is marked dead
 */
void rt_thread_v3_dispose(RtHandleV2 *thread_handle);

/* Sync multiple void-returning threads. */
void rt_thread_v3_sync_all(RtHandleV2 **thread_handles, int count);


/* ============================================================================
 * Wrapper Helpers (called by generated wrapper code)
 * ============================================================================ */

/* Get arena for allocations: t->arena if set, else t->caller. */
RtArenaV2 *rt_thread_v3_get_arena(RtHandleV2 *thread_handle);

/* Store result handle. */
void rt_thread_v3_set_result(RtHandleV2 *thread_handle, RtHandleV2 *result);

/* Signal completion (sets done flag, broadcasts cond). */
void rt_thread_v3_signal_done(RtHandleV2 *thread_handle);

/* ============================================================================
 * Panic Integration
 * ============================================================================ */

/* TLS current thread context (stores handle, not raw pointer) */
void rt_tls_thread_set_v3(RtHandleV2 *thread_handle);
RtHandleV2 *rt_tls_thread_get_v3(void);

/* Panic - stores message in thread (if in thread context), signals done,
 * and exits thread. If not in thread context, prints and exits process. */
void rt_panic(const char *msg);

/* ============================================================================
 * Args Helpers (used by codegen for accessing thread args via handle)
 * ============================================================================ */

/* Get the args handle from a thread handle (transaction-safe). */
RtHandleV2 *rt_thread_v3_get_args(RtHandleV2 *thread_handle);

/* Set the args handle on a thread handle (transaction-safe). */
void rt_thread_v3_set_args(RtHandleV2 *thread_handle, RtHandleV2 *args);

/* ============================================================================
 * Sync Lock Table
 * ============================================================================
 * These functions provide mutex-based synchronization for sync variables
 * when using lock(var) => { ... } blocks.
 * ============================================================================ */

/* Acquire a mutex lock for a sync variable (by handle)
 * Creates mutex on first use. Thread-safe. */
void rt_sync_lock(RtHandleV2 *handle);

/* Release a mutex lock for a sync variable (by handle)
 * Must be called after rt_sync_lock with the same handle. */
void rt_sync_unlock(RtHandleV2 *handle);

/* Initialize the sync lock table (called automatically if needed) */
void rt_sync_lock_table_init(void);

/* Clean up all sync locks (called on process exit) */
void rt_sync_lock_table_cleanup(void);

#endif /* RUNTIME_THREAD_V3_H */
