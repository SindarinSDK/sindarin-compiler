/* ============================================================================
 * Thread V3 Implementation
 * ============================================================================ */

#include "runtime_thread_v3.h"
#include "runtime/arena/arena_handle.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Thread-Local Storage
 * ============================================================================ */

static RT_THREAD_LOCAL_V2 RtHandleV2 *rt_current_thread_handle = NULL;

void rt_tls_thread_set_v3(RtHandleV2 *thread_handle)
{
    rt_current_thread_handle = thread_handle;
    if (thread_handle != NULL) {
        rt_handle_begin_transaction(thread_handle);
        RtThread *t = (RtThread *)thread_handle->ptr;
        rt_arena_set_thread_id(t->thread_id);
        rt_handle_end_transaction(thread_handle);
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
 * proper deep copying. */
static void rt_thread_copy_callback(RtArenaV2 *dest, void *ptr)
{
    RtThread *t = (RtThread *)ptr;

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

/* Forward declaration for use as free_callback */
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

    /* Initialize within transaction */
    rt_handle_begin_transaction(handle);
    RtThread *t = (RtThread *)handle->ptr;

    /* Set callbacks for pthread primitive safety */
    rt_handle_set_copy_callback(handle, rt_thread_copy_callback);
    rt_handle_set_free_callback(handle, rt_thread_v3_dispose);

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
    if (err != 0) {
        fprintf(stderr, "rt_thread_v3_start: pthread_create failed: %d\n", err);
        rt_handle_end_transaction(thread_handle);
        rt_thread_v3_dispose(thread_handle);
        return;
    }

    rt_handle_end_transaction(thread_handle);
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

    rt_handle_begin_transaction(thread_handle);
    RtThread *t = (RtThread *)thread_handle->ptr;

    if (t == NULL) {
        fprintf(stderr, "rt_thread_v3_sync: NULL thread\n");
        rt_handle_end_transaction(thread_handle);
        return NULL;
    }

    /* Wait for completion */
    pthread_mutex_lock(&t->mutex);
    while (!t->done) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    pthread_mutex_unlock(&t->mutex);

    /* Join the pthread */
    pthread_join(t->pthread, NULL);

    /* Capture panic message before dispose (it's malloc'd, not arena) */
    char *panic_msg = t->panic_msg;

    /* Promote result (copy_callback handles deep copy automatically) */
    RtHandleV2 *result = NULL;
    if (t->result != NULL) {
        if (t->mode == RT_THREAD_MODE_SHARED) {
            result = t->result;  /* Already in caller arena */
        } else {
            result = rt_arena_v2_promote(t->caller, t->result);
        }
    }

    /* End our transaction, then dispose handles cleanup */
    rt_handle_end_transaction(thread_handle);
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

    /* Check disposed before transaction - fast path for already-disposed */
    RtThread *t = (RtThread *)thread_handle->ptr;
    if (t == NULL || t->disposed) {
        return;
    }

    rt_handle_begin_transaction(thread_handle);

    /* Re-check under transaction (another thread may have disposed) */
    if (t->disposed) {
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

    /* Only free handle if not already being destroyed by GC.
     * When GC calls us via free_callback, handle has DEAD flag set. */
    if (!(thread_handle->flags & RT_HANDLE_FLAG_DEAD)) {
        rt_arena_v2_free(thread_handle);
    }
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

void rt_panic_v3(const char *msg)
{
    RtHandleV2 *th = rt_tls_thread_get_v3();

    if (th != NULL) {
        /* In thread context - store message, signal done, exit thread */
        rt_handle_begin_transaction(th);
        RtThread *t = (RtThread *)th->ptr;

        /* Simple malloc copy - no arena complexity */
        t->panic_msg = msg ? strdup(msg) : NULL;

        rt_thread_v3_signal_done(th);
        rt_handle_end_transaction(th);

        rt_tls_thread_set_v3(NULL);
        pthread_exit(NULL);
        /* Not reached */
    }

    /* Not in thread context - print and exit process */
    fprintf(stderr, "panic: %s\n", msg ? msg : "(no message)");
    exit(1);
}
