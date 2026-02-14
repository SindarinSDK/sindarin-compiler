/* ============================================================================
 * Thread V2 Implementation - Arena-First Threading for Sindarin
 * ============================================================================ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include "runtime_thread_v2.h"
#include "runtime/arena/arena_id.h"  /* Thread ID system */
#include "runtime/array/runtime_array_v2.h"  /* For array promotion functions */

/* ============================================================================
 * Thread-Local State
 * ============================================================================ */

RT_THREAD_LOCAL_V2 RtThread *rt_current_thread = NULL;

/* ============================================================================
 * Spawn Implementation
 * ============================================================================ */

RtThread *rt_thread_v2_create(RtArenaV2 *caller, RtThreadMode mode)
{
    if (caller == NULL) {
        fprintf(stderr, "rt_thread_create: NULL caller arena\n");
        return NULL;
    }

    /* Allocate RtThread in caller arena (survives until sync/cleanup) */
    RtHandleV2 *t_h = rt_arena_v2_alloc(caller, sizeof(RtThread));
    rt_handle_begin_transaction(t_h);
    RtThread *t = (RtThread *)t_h->ptr;
    if (t == NULL) {
        fprintf(stderr, "rt_thread_create: allocation failed\n");
        return NULL;
    }

    /* Initialize fields */
    t->pthread = 0;
    t->thread_id = rt_arena_alloc_thread_id();
    t->self_handle = t_h;
    t->caller = caller;
    t->mode = mode;
    t->done = false;
    t->args = NULL;
    t->result = NULL;
    t->panic_msg = NULL;

    /* Initialize synchronization primitives */
    pthread_mutex_init(&t->mutex, NULL);
    pthread_cond_init(&t->cond, NULL);

    /* Create thread arena based on mode */
    switch (mode) {
        case RT_THREAD_MODE_SHARED:
            /* Shared: use caller's arena directly */
            t->arena = NULL;
            break;

        case RT_THREAD_MODE_DEFAULT:
            /* Default: own arena as child of caller (for hierarchy/promotion) */
            t->arena = rt_arena_v2_create(caller, RT_ARENA_MODE_DEFAULT, "thread");
            if (t->arena == NULL) {
                fprintf(stderr, "rt_thread_create: failed to create thread arena\n");
                pthread_mutex_destroy(&t->mutex);
                pthread_cond_destroy(&t->cond);
                rt_handle_end_transaction(t_h);
                return NULL;
            }
            break;

        case RT_THREAD_MODE_PRIVATE:
            /* Private: isolated arena with no parent */
            t->arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_PRIVATE, "thread_private");
            if (t->arena == NULL) {
                fprintf(stderr, "rt_thread_create: failed to create private thread arena\n");
                pthread_mutex_destroy(&t->mutex);
                pthread_cond_destroy(&t->cond);
                rt_handle_end_transaction(t_h);
                return NULL;
            }
            break;
    }

    /* End transaction - initialization complete */
    rt_handle_end_transaction(t_h);

    return t;
}

void rt_thread_v2_start(RtThread *t, void *(*wrapper)(void *))
{
    if (t == NULL) {
        fprintf(stderr, "rt_thread_start: NULL thread\n");
        return;
    }
    if (wrapper == NULL) {
        fprintf(stderr, "rt_thread_start: NULL wrapper\n");
        return;
    }

    int err = pthread_create(&t->pthread, NULL, wrapper, t);
    if (err != 0) {
        fprintf(stderr, "rt_thread_start: pthread_create failed with error %d\n", err);
        /* Set panic so sync will report the error */
        t->panic_msg = "failed to create thread";
        t->done = true;
        return;
    }

    /* Detach thread - we use condition variables for sync, not pthread_join */
    pthread_detach(t->pthread);
}

/* ============================================================================
 * Sync Implementation
 * ============================================================================ */

RtHandleV2 *rt_thread_v2_sync(RtThread *t)
{
    if (t == NULL) {
        fprintf(stderr, "rt_thread_sync: NULL thread\n");
        return NULL;
    }

    /* Wait for completion */
    pthread_mutex_lock(&t->mutex);
    while (!t->done) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    pthread_mutex_unlock(&t->mutex);

    /* Capture result and panic state */
    RtHandleV2 *result = t->result;
    char *panic_msg = t->panic_msg;

    /* Promote panic message to caller arena if needed */
    char *promoted_panic = NULL;
    if (panic_msg != NULL && t->arena != NULL) {
        RtHandleV2 *_h = rt_arena_v2_strdup(t->caller, panic_msg);
        rt_handle_begin_transaction(_h);
        promoted_panic = (char *)_h->ptr;
        rt_handle_end_transaction(_h);
    } else {
        promoted_panic = panic_msg;  /* Already in caller arena or NULL */
    }

    /* Handle result based on mode */
    switch (t->mode) {
        case RT_THREAD_MODE_SHARED:
            /* Shared: result already in caller's arena, nothing to promote/destroy */
            break;

        case RT_THREAD_MODE_DEFAULT:
            /* Default: promote result to caller arena, then destroy thread arena.
             * Note: For types needing deep promotion (2D/3D arrays), use
             * rt_thread_v2_sync_keep_arena + explicit promotion + cleanup. */
            if (result != NULL) {
                result = rt_arena_v2_promote(t->caller, result);
            }
            rt_arena_v2_condemn(t->arena);
            t->arena = NULL;
            break;

        case RT_THREAD_MODE_PRIVATE:
            /* Private: only primitives allowed (compiler enforces). */
            if (result != NULL) {
                result = rt_arena_v2_promote(t->caller, result);
            }
            rt_arena_v2_condemn(t->arena);
            t->arena = NULL;
            break;
    }

    /* Destroy synchronization primitives */
    pthread_mutex_destroy(&t->mutex);
    pthread_cond_destroy(&t->cond);

    /* Re-raise panic in caller context */
    if (promoted_panic != NULL) {
        fprintf(stderr, "panic: %s\n", promoted_panic);
        exit(1);
    }

    return result;
}

RtHandleV2 *rt_thread_v2_sync_keep_arena(RtThread *t)
{
    if (t == NULL) {
        fprintf(stderr, "rt_thread_v2_sync_keep_arena: NULL thread\n");
        return NULL;
    }

    /* Wait for completion */
    pthread_mutex_lock(&t->mutex);
    while (!t->done) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    pthread_mutex_unlock(&t->mutex);

    /* Capture result and panic state */
    RtHandleV2 *result = t->result;
    char *panic_msg = t->panic_msg;

    /* Promote panic message to caller arena if needed */
    char *promoted_panic = NULL;
    if (panic_msg != NULL && t->arena != NULL) {
        RtHandleV2 *_h = rt_arena_v2_strdup(t->caller, panic_msg);
        rt_handle_begin_transaction(_h);
        promoted_panic = (char *)_h->ptr;
        rt_handle_end_transaction(_h);
    } else {
        promoted_panic = panic_msg;
    }

    /* Do NOT promote result or destroy arena - caller handles that.
     * This is used for:
     * 1. Structs with handle fields that need field-by-field promotion
     * 2. 2D/3D arrays that need deep promotion
     * Caller must call appropriate promotion function then rt_arena_v2_condemn(t->arena) */

    /* Destroy synchronization primitives */
    pthread_mutex_destroy(&t->mutex);
    pthread_cond_destroy(&t->cond);

    /* Re-raise panic in caller context */
    if (promoted_panic != NULL) {
        fprintf(stderr, "panic: %s\n", promoted_panic);
        exit(1);
    }

    return result;
}

void rt_thread_v2_sync_all(RtThread **threads, int count)
{
    if (threads == NULL || count <= 0) return;

    /* Sync each thread - all must be void-returning */
    for (int i = 0; i < count; i++) {
        if (threads[i] != NULL) {
            rt_thread_v2_sync(threads[i]);
        }
    }
}

/* ============================================================================
 * Fire-and-Forget Cleanup
 * ============================================================================ */

void rt_thread_v2_fire_and_forget_done(RtThread *t)
{
    if (t == NULL) return;

    /* Signal completion (no-op for fire-and-forget since nobody waits,
     * but keeps the done flag consistent for safety) */
    pthread_mutex_lock(&t->mutex);
    t->done = true;
    pthread_mutex_unlock(&t->mutex);

    /* Destroy synchronization primitives - safe because nobody will
     * ever call sync on a fire-and-forget thread */
    pthread_mutex_destroy(&t->mutex);
    pthread_cond_destroy(&t->cond);

    /* Condemn thread arena for GC destruction */
    if (t->arena != NULL) {
        rt_arena_v2_condemn(t->arena);
    }

    /* Unpin and mark self_handle dead so GC reclaims the RtThread from caller arena */
    rt_handle_end_transaction(t->self_handle);
    rt_arena_v2_free(t->self_handle);
}

/* ============================================================================
 * Wrapper Helpers
 * ============================================================================ */

RtArenaV2 *rt_thread_v2_get_arena(RtThread *t)
{
    if (t == NULL) return NULL;
    return t->arena ? t->arena : t->caller;
}

void rt_thread_v2_set_result(RtThread *t, RtHandleV2 *result)
{
    if (t == NULL) return;
    t->result = result;
}


void rt_thread_v2_set_panic(RtThread *t, const char *msg)
{
    if (t == NULL) return;

    /* Store panic message in thread arena (or caller if shared) */
    RtArenaV2 *arena = rt_thread_v2_get_arena(t);
    if (msg != NULL && arena != NULL) {
        RtHandleV2 *_h = rt_arena_v2_strdup(arena, msg);
        rt_handle_begin_transaction(_h);
        t->panic_msg = (char *)_h->ptr;
        rt_handle_end_transaction(_h);
    } else {
        t->panic_msg = (char *)msg;
    }

    /* Signal done */
    rt_thread_v2_signal_done(t);
}

void rt_thread_v2_signal_done(RtThread *t)
{
    if (t == NULL) return;

    pthread_mutex_lock(&t->mutex);
    t->done = true;
    pthread_cond_broadcast(&t->cond);
    pthread_mutex_unlock(&t->mutex);
}

/* ============================================================================
 * Panic Integration
 * ============================================================================ */

void rt_tls_thread_set(RtThread *t)
{
    rt_current_thread = t;
    if (t != NULL) {
        rt_arena_set_thread_id(t->thread_id);
    }
}

RtThread *rt_tls_thread_get(void)
{
    return rt_current_thread;
}

uint64_t rt_thread_get_id(void)
{
    return rt_arena_get_thread_id();
}

bool rt_thread_v2_capture_panic(const char *msg)
{
    RtThread *t = rt_tls_thread_get();
    if (t == NULL) {
        return false;  /* Not in thread context, let caller handle */
    }

    /* Store panic in thread */
    RtArenaV2 *arena = rt_thread_v2_get_arena(t);
    if (msg != NULL && arena != NULL) {
        RtHandleV2 *_h = rt_arena_v2_strdup(arena, msg);
        rt_handle_begin_transaction(_h);
        t->panic_msg = (char *)_h->ptr;
        rt_handle_end_transaction(_h);
    } else {
        t->panic_msg = (char *)msg;
    }

    return true;  /* Panic captured */
}

void rt_panic(const char *msg)
{
    /* Try to capture in V2 thread context first */
    if (rt_thread_v2_capture_panic(msg)) {
        /* Signal thread completion and exit thread */
        RtThread *t = rt_tls_thread_get();
        rt_thread_v2_signal_done(t);
        rt_tls_thread_set(NULL);
        pthread_exit(NULL);
        /* Not reached */
    }

    /* Not in thread context - print and exit */
    if (msg != NULL) {
        fprintf(stderr, "panic: %s\n", msg);
    } else {
        fprintf(stderr, "panic: (no message)\n");
    }
    exit(1);
}

/* ============================================================================
 * Sync Lock Table
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
