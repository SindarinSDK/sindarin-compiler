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
#include "array/runtime_array.h"
#include "array/runtime_array_h.h"

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

/* Forward declaration for cleanup callback used by spawn module */
static void rt_thread_cleanup(void *data);

/* Include split modules */
#include "runtime_thread_pool.c"
#include "runtime_thread_panic.c"
#include "runtime_thread_result.c"
#include "runtime_thread_spawn.c"
#include "runtime_thread_sync.c"
#include "runtime_thread_promote.c"
#include "runtime_thread_lock.c"

/* ============================================================================
 * Thread Cleanup Callback for Arena
 * ============================================================================
 * Called when an arena is destroyed to auto-join any tracked threads.
 */

static void rt_thread_cleanup(void *data)
{
    RtThreadHandle *handle = (RtThreadHandle *)data;
    if (handle != NULL && !handle->synced) {
        rt_thread_sync(handle);
    }
}
