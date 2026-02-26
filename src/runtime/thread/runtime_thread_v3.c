/* ============================================================================
 * Thread V3 Implementation
 * ============================================================================
 * All access to RtThread fields goes through handle transactions as designed.
 *
 * For sync: transaction is released BEFORE pthread_cond_wait (never hold a
 * block spinlock across a blocking call) and re-acquired after to read results.
 * ============================================================================ */

#include "runtime_thread_v3.h"
#include "runtime/arena/arena_handle.h"
#include "runtime/arena/arena_id.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Thread-Local Storage
 * ============================================================================ */

static RT_THREAD_LOCAL RtHandleV2 *rt_current_thread_handle = NULL;

void rt_tls_thread_set_v3(RtHandleV2 *thread_handle)
{
    rt_current_thread_handle = thread_handle;
    if (thread_handle != NULL) {
        rt_handle_begin_transaction(thread_handle);
        RtThread *t = (RtThread *)thread_handle->ptr;
        uint64_t tid = t->thread_id;
        rt_handle_end_transaction(thread_handle);
        rt_arena_set_thread_id(tid);
    }
}

RtHandleV2 *rt_tls_thread_get_v3(void)
{
    return rt_current_thread_handle;
}

/* ============================================================================
 * Thread Callbacks
 * ============================================================================ */

/* Copy callback: RtThread contains pthread primitives and handles that need
 * proper deep copying. Called within a transaction by rt_arena_v2_clone. */
static void rt_thread_copy_callback(RtArenaV2 *dest, RtHandleV2 *new_handle)
{
    RtThread *t = (RtThread *)new_handle->ptr;

    /* Reinitialize pthread primitives - memcpy'd values are invalid */
    pthread_mutex_init(&t->mutex, NULL);
    pthread_cond_init(&t->cond, NULL);

    /* Get new thread ID */
    t->thread_id = rt_arena_alloc_thread_id();

    /* Update caller to destination arena */
    t->caller = dest;

    /* Create new thread arena based on mode */
    switch (t->mode) {
        case RT_THREAD_MODE_SHARED:
            t->arena = NULL;
            break;
        case RT_THREAD_MODE_DEFAULT:
            t->arena = rt_arena_v2_create(dest, RT_ARENA_MODE_DEFAULT, "thread");
            break;
        case RT_THREAD_MODE_PRIVATE:
            t->arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_PRIVATE, "thread_private");
            break;
    }

    /* Reset thread state */
    t->pthread = 0;
    t->done = false;
    t->disposed = false;

    /* Promote handles to new arena */
    RtArenaV2 *target = t->arena ? t->arena : t->caller;
    if (t->args != NULL) {
        t->args = rt_arena_v2_promote(target, t->args);
    }
    if (t->result != NULL) {
        t->result = rt_arena_v2_promote(target, t->result);
    }

    /* Copy panic message */
    if (t->panic_msg != NULL) {
        t->panic_msg = strdup(t->panic_msg);
    }
}

/* Forward declaration for use as cleanup callback */
void rt_thread_v3_dispose(RtHandleV2 *thread_handle);

/* ============================================================================
 * Thread Creation
 * ============================================================================ */

RtHandleV2 *rt_thread_v3_create(RtArenaV2 *caller, RtThreadMode mode)
{
    if (caller == NULL) {
        fprintf(stderr, "rt_thread_v3_create: NULL caller arena\n");
        return NULL;
    }

    /* Allocate RtThread in caller arena */
    RtHandleV2 *handle = rt_arena_v2_alloc(caller, sizeof(RtThread));
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_v3_create: allocation failed\n");
        return NULL;
    }

    rt_handle_begin_transaction(handle);
    RtThread *t = (RtThread *)handle->ptr;

    /* Set copy callback for pthread primitive safety */
    rt_handle_set_copy_callback(handle, rt_thread_copy_callback);

    /* Register cleanup callback so thread is disposed when caller arena is destroyed */
    rt_arena_v2_on_cleanup(caller, handle, rt_thread_v3_dispose, 0);

    /* Initialize pthread primitives */
    pthread_mutex_init(&t->mutex, NULL);
    pthread_cond_init(&t->cond, NULL);

    /* Initialize fields */
    t->pthread = 0;
    t->thread_id = rt_arena_alloc_thread_id();
    t->caller = caller;
    t->mode = mode;
    t->done = false;
    t->disposed = false;
    t->self_handle = handle;
    t->args = NULL;
    t->result = NULL;
    t->panic_msg = NULL;

    /* Create thread arena based on mode */
    switch (mode) {
        case RT_THREAD_MODE_SHARED:
            t->arena = NULL;
            break;

        case RT_THREAD_MODE_DEFAULT:
            t->arena = rt_arena_v2_create(caller, RT_ARENA_MODE_DEFAULT, "thread");
            if (t->arena == NULL) {
                fprintf(stderr, "rt_thread_v3_create: failed to create thread arena\n");
                rt_handle_end_transaction(handle);
                rt_thread_v3_dispose(handle);
                return NULL;
            }
            break;

        case RT_THREAD_MODE_PRIVATE:
            t->arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_PRIVATE, "thread_private");
            if (t->arena == NULL) {
                fprintf(stderr, "rt_thread_v3_create: failed to create private arena\n");
                rt_handle_end_transaction(handle);
                rt_thread_v3_dispose(handle);
                return NULL;
            }
            break;
    }

    rt_handle_end_transaction(handle);
    return handle;
}

/* ============================================================================
 * Thread Start
 * ============================================================================ */

void rt_thread_v3_start(RtHandleV2 *thread_handle, void *(*wrapper)(void *))
{
    if (thread_handle == NULL) {
        fprintf(stderr, "rt_thread_v3_start: NULL handle\n");
        return;
    }
    if (wrapper == NULL) {
        fprintf(stderr, "rt_thread_v3_start: NULL wrapper\n");
        return;
    }

    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;

    /* Create pthread, passing the HANDLE (not RtThread*) */
    int err = pthread_create(&t->pthread, NULL, wrapper, thread_handle);
    rt_handle_end_transaction(thread_handle);

    if (err != 0) {
        fprintf(stderr, "rt_thread_v3_start: pthread_create failed: %d\n", err);
        rt_thread_v3_dispose(thread_handle);
        return;
    }
}

/* ============================================================================
 * Thread Sync
 * ============================================================================ */

RtHandleV2 *rt_thread_v3_sync(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) {
        fprintf(stderr, "rt_thread_v3_sync: NULL handle\n");
        return NULL;
    }

    /* Transaction to get mutex/cond pointers and check state */
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;

    if (t == NULL) {
        rt_handle_end_transaction(thread_handle);
        fprintf(stderr, "rt_thread_v3_sync: NULL thread\n");
        return NULL;
    }

    /* Capture pthread primitives for the blocking wait.
     * These are embedded in the RtThread struct which is stable for the
     * lifetime of the handle. */
    pthread_mutex_t *mutex = &t->mutex;
    pthread_cond_t *cond = &t->cond;
    pthread_t pthread = t->pthread;

    /* Release transaction BEFORE blocking - never hold a block spinlock
     * across pthread_cond_wait. */
    rt_handle_end_transaction(thread_handle);

    /* Wait for completion (no transaction held) */
    pthread_mutex_lock(mutex);
    while (!t->done) {
        pthread_cond_wait(cond, mutex);
    }
    pthread_mutex_unlock(mutex);

    /* Join the pthread (no transaction held) */
    pthread_join(pthread, NULL);

    /* Re-acquire transaction to read results - worker has exited,
     * we are the sole accessor. */
    rt_handle_begin_transaction(thread_handle);
    char *panic_msg = t->panic_msg;
    t->panic_msg = NULL;  /* Prevent dispose from freeing it */

    /* Capture result info */
    RtHandleV2 *thread_result = t->result;
    RtThreadMode mode = t->mode;
    RtArenaV2 *caller = t->caller;
    rt_handle_end_transaction(thread_handle);

    /* Promote result (copy_callback handles deep copy automatically) */
    RtHandleV2 *result = NULL;
    if (thread_result != NULL) {
        if (mode == RT_THREAD_MODE_SHARED) {
            result = thread_result;  /* Already in caller arena */
        } else {
            result = rt_arena_v2_promote(caller, thread_result);
        }
    }

    /* Dispose the thread handle and its arena */
    rt_thread_v3_dispose(thread_handle);

    /* Re-raise panic if needed */
    if (panic_msg != NULL) {
        fprintf(stderr, "panic: %s\n", panic_msg);
        free(panic_msg);
        exit(1);
    }

    return result;
}

/* ============================================================================
 * Thread Dispose (Fire-and-Forget Cleanup)
 * ============================================================================ */

void rt_thread_v3_dispose(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) {
        return;
    }

    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;

    if (t == NULL || t->disposed) {
        rt_handle_end_transaction(thread_handle);
        return;
    }

    t->disposed = true;

    /* Signal done (in case anyone is somehow waiting) */
    pthread_mutex_lock(&t->mutex);
    t->done = true;
    pthread_cond_broadcast(&t->cond);
    pthread_mutex_unlock(&t->mutex);

    /* Condemn thread arena */
    if (t->arena != NULL) {
        rt_arena_v2_condemn(t->arena);
        t->arena = NULL;
    }

    /* Destroy pthread primitives */
    pthread_mutex_destroy(&t->mutex);
    pthread_cond_destroy(&t->cond);

    /* Free malloc'd panic message if any */
    if (t->panic_msg != NULL) {
        free(t->panic_msg);
        t->panic_msg = NULL;
    }

    rt_handle_end_transaction(thread_handle);

    /* Mark handle as dead so GC can collect it */
    rt_arena_v2_free(thread_handle);

}

/* ============================================================================
 * Sync All
 * ============================================================================ */

void rt_thread_v3_sync_all(RtHandleV2 **thread_handles, int count)
{
    for (int i = 0; i < count; i++) {
        rt_thread_v3_sync(thread_handles[i]);
    }
}

/* ============================================================================
 * Wrapper Helpers
 * ============================================================================ */

RtArenaV2 *rt_thread_v3_get_arena(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) return NULL;
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;
    RtArenaV2 *arena = t->arena ? t->arena : t->caller;
    rt_handle_end_transaction(thread_handle);
    return arena;
}

void rt_thread_v3_set_result(RtHandleV2 *thread_handle, RtHandleV2 *result)
{
    if (thread_handle == NULL) return;
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;
    t->result = result;
    rt_handle_end_transaction(thread_handle);
}

void rt_thread_v3_signal_done(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) return;
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;

    pthread_mutex_lock(&t->mutex);
    t->done = true;
    pthread_cond_broadcast(&t->cond);
    pthread_mutex_unlock(&t->mutex);

    rt_handle_end_transaction(thread_handle);
}

/* ============================================================================
 * Panic Integration
 * ============================================================================ */

void rt_panic(const char *msg)
{
    RtHandleV2 *th = rt_tls_thread_get_v3();

    if (th != NULL) {
        /* In thread context - store message, signal done, exit thread */
        rt_handle_begin_transaction(th);
        RtThread *t = (RtThread *)th->ptr;
        t->panic_msg = msg ? strdup(msg) : NULL;
        rt_handle_end_transaction(th);

        rt_thread_v3_signal_done(th);

        rt_tls_thread_set_v3(NULL);
        pthread_exit(NULL);
        /* Not reached */
    }

    /* Not in thread context - print and exit process */
    fprintf(stderr, "panic: %s\n", msg ? msg : "(no message)");
    exit(1);
}

/* ============================================================================
 * Args Helpers
 * ============================================================================ */

RtHandleV2 *rt_thread_v3_get_args(RtHandleV2 *thread_handle)
{
    if (thread_handle == NULL) return NULL;
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;
    RtHandleV2 *args = t->args;
    rt_handle_end_transaction(thread_handle);
    return args;
}

void rt_thread_v3_set_args(RtHandleV2 *thread_handle, RtHandleV2 *args)
{
    if (thread_handle == NULL) return;
    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;
    t->args = args;
    rt_handle_end_transaction(thread_handle);
}

/* ============================================================================
 * Sync Lock Table
 * ============================================================================
 * Hash table mapping variable addresses to mutexes for lock blocks.
 * Uses a simple open-addressed hash table with linear probing.
 * ============================================================================ */

#include <stdint.h>
#include <stdatomic.h>

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
void rt_sync_lock(RtHandleV2 *handle)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_sync_lock: NULL handle\n");
        return;
    }

    pthread_mutex_t *mutex = rt_sync_lock_get_mutex((void *)handle);
    if (mutex != NULL) {
        pthread_mutex_lock(mutex);
    }
}

/* Release a mutex lock for a sync variable */
void rt_sync_unlock(RtHandleV2 *handle)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_sync_unlock: NULL handle\n");
        return;
    }

    void *addr = (void *)handle;

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
