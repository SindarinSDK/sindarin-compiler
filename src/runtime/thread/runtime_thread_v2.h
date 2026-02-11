/* ============================================================================
 * Thread V2 - Arena-First Threading for Sindarin
 * ============================================================================
 *
 * Design principles:
 * 1. Single structure - RtThread contains everything
 * 2. Arena ownership - t->arena is thread's own (NULL if shared mode)
 * 3. Atomic sync - waits, promotes, destroys in one call
 * 4. No type explosion - wrapper does type-aware promotion before done
 * 5. Handle-based results - RtHandleV2*, consistent with arena v2
 *
 * Memory model:
 * - Shared mode:  t->arena = NULL, allocations go to t->caller
 * - Default mode: t->arena = child of t->caller, promoted at sync
 * - Private mode: t->arena = isolated, destroyed at sync (void/primitive only)
 *
 * Mutex lifecycle:
 * - Created in rt_thread_v2_create
 * - Synced threads: destroyed in rt_thread_v2_sync
 * - Fire-and-forget: destroyed in rt_thread_v2_fire_and_forget_done
 *
 * ============================================================================ */

#ifndef RUNTIME_THREAD_V2_H
#define RUNTIME_THREAD_V2_H

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
    #define RT_THREAD_LOCAL_V2
#elif defined(_MSC_VER)
    #define RT_THREAD_LOCAL_V2 __declspec(thread)
#else
    #define RT_THREAD_LOCAL_V2 __thread
#endif

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

    RtArenaV2 *arena;           /* Thread's own arena (NULL if shared) */
    RtArenaV2 *caller;          /* Caller's arena (owns this struct) */
    RtThreadMode mode;          /* Thread mode for sync behavior */

    pthread_mutex_t mutex;      /* Destroyed by sync or cleanup callback */
    pthread_cond_t cond;
    bool done;

    RtHandleV2 *self_handle;    /* Handle to this RtThread in caller arena */
    RtHandleV2 *args;           /* Handle to packed args (in thread arena) */
    RtHandleV2 *result;         /* Result handle (NULL for void) */
    char *panic_msg;            /* NULL = success */
} RtThread;

/* ============================================================================
 * Spawn
 * ============================================================================ */

/* Create thread + arena. RtThread allocated in caller arena.
 * RT_THREAD_MODE_DEFAULT: t->arena = new child of caller (promotes on sync)
 * RT_THREAD_MODE_SHARED:  t->arena = NULL (uses caller's arena)
 * RT_THREAD_MODE_PRIVATE: t->arena = isolated (no parent, void/primitives only) */
RtThread *rt_thread_v2_create(RtArenaV2 *caller, RtThreadMode mode);

/* Start thread. Wrapper receives RtThread* as arg. */
void rt_thread_v2_start(RtThread *t, void *(*wrapper)(void *));

/* ============================================================================
 * Sync
 * ============================================================================ */

/* Wait, get result, destroy t->arena (if not shared), destroy mutex.
 * Panics if thread panicked.
 * Private mode: returns NULL (only void/primitives allowed by compiler). */
RtHandleV2 *rt_thread_v2_sync(RtThread *t);

/* Like sync, but does NOT destroy t->arena. Use when you need to promote
 * struct handle fields before arena destruction. Caller must destroy arena. */
RtHandleV2 *rt_thread_v2_sync_keep_arena(RtThread *t);

/* Sync multiple threads. Void-returning only (no result). */
void rt_thread_v2_sync_all(RtThread **threads, int count);

/* ============================================================================
 * Wrapper Helpers (called by generated wrapper code)
 * ============================================================================
 *
 * SPAWN SITE (codegen):
 *
 *   RtThread *t = rt_thread_v2_create(caller_arena, RT_THREAD_MODE_DEFAULT);
 *   RtArenaV2 *arena = rt_thread_v2_get_arena(t);
 *
 *   // Pack args in thread arena (safe - lives until sync)
 *   t->args = rt_arena_v2_alloc(arena, sizeof(MyFnArgs));
 *   MyFnArgs *args = rt_handle_v2_pin(t->args);
 *   args->x = x;
 *   args->y = rt_arena_v2_promote(arena, y_handle);  // Clone handles into thread arena
 *
 *   rt_thread_v2_start(t, my_fn_wrapper);
 *
 * WRAPPER FUNCTION (codegen):
 *
 *   void *my_fn_wrapper(void *arg) {
 *       RtThread *t = arg;
 *       rt_thread_v2_set_current(t);
 *       RtArenaV2 *arena = rt_thread_v2_get_arena(t);
 *
 *       // Unpack args
 *       MyFnArgs *a = rt_handle_v2_pin(t->args);
 *
 *       // Call actual function (result is in thread arena)
 *       RtHandleV2 *result = my_fn(arena, a->x, a->y);
 *
 *       // Store result - sync handles promotion to caller arena
 *       rt_thread_v2_set_result(t, result);
 *       rt_thread_v2_signal_done(t);
 *       rt_thread_v2_set_current(NULL);
 *       return NULL;
 *   }
 *
 * ============================================================================ */

/* Get arena for allocations: t->arena if set, else t->caller */
RtArenaV2 *rt_thread_v2_get_arena(RtThread *t);

/* Store result handle */
void rt_thread_v2_set_result(RtThread *t, RtHandleV2 *result);

/* Set panic and signal done */
void rt_thread_v2_set_panic(RtThread *t, const char *msg);

/* Signal completion */
void rt_thread_v2_signal_done(RtThread *t);

/* Fire-and-forget cleanup: signal done, destroy mutex/cond, condemn arena,
 * mark self_handle dead for GC. Called from wrapper epilogue when result
 * is discarded and no sync will ever happen. */
void rt_thread_v2_fire_and_forget_done(RtThread *t);

/* ============================================================================
 * Panic Integration
 * ============================================================================ */

/* TLS current thread context */
void rt_thread_v2_set_current(RtThread *t);
RtThread *rt_thread_v2_get_current(void);

/* Called by rt_panic - returns true if captured in thread context */
bool rt_thread_v2_capture_panic(const char *msg);

/* Unified panic function - captures in thread context or exits */
void rt_panic(const char *msg);

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

#endif /* RUNTIME_THREAD_V2_H */
