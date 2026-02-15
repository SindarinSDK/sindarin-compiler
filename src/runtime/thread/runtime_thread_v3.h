/* ============================================================================
 * Thread V3 - Clean Lifecycle Threading for Sindarin
 * ============================================================================
 *
 * Design principles:
 * 1. RtHandleV2 is first-class - all thread access via handle, always in transaction
 * 2. Clean lifecycle - create, start, sync, dispose (4 public functions)
 * 3. All promotion in runtime - rt_thread_v3_promote handles everything
 * 4. Transaction safety - all t->field access within begin/end transaction
 * 5. RtThread arena-allocated - transactions protect pthread primitives from GC
 *
 * Public API:
 *   rt_thread_v3_create  - Create thread handle
 *   rt_thread_v3_start   - Start the thread
 *   rt_thread_v3_sync    - Wait, promote result, cleanup
 *   rt_thread_v3_dispose - Explicit cleanup (for fire-and-forget)
 *
 * Internal (used by sync):
 *   rt_thread_v3_promote - Promotes result based on type info
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

/* ============================================================================
 * Thread Mode
 * ============================================================================ */

typedef enum {
    RT_THREAD_MODE_DEFAULT,     /* Own arena (child of caller), promote on sync */
    RT_THREAD_MODE_SHARED,      /* Use caller's arena directly */
    RT_THREAD_MODE_PRIVATE      /* Isolated arena, void/primitives only */
} RtThreadMode;

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
 * RtThread - Core Structure (Arena-Allocated)
 * ============================================================================
 * RtThread is allocated in the CALLER's arena. All access to fields
 * (including pthread primitives) MUST be within a transaction. This ensures
 * GC cannot move the memory while pthread operations are in progress.
 * ============================================================================ */

typedef struct RtThread {
    /* pthread primitives - protected by transaction during use */
    pthread_t pthread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    /* Thread identity */
    uint64_t thread_id;

    /* Arena management */
    RtArenaV2 *arena;           /* Thread's own arena (NULL if shared) */
    RtArenaV2 *caller;          /* Caller's arena */
    RtThreadMode mode;

    /* State */
    bool done;
    bool disposed;

    /* Result */
    RtHandleV2 *args;           /* Handle to packed args (in thread arena) */
    RtHandleV2 *result;         /* Result handle (NULL for void/primitives) */

    /* Error state */
    char *panic_msg;            /* NULL = success, else error message */
} RtThread;

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
void rt_panic_v3(const char *msg);

#endif /* RUNTIME_THREAD_V3_H */
