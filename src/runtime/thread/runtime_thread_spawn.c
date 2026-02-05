// src/runtime/runtime_thread_spawn.c
// Thread Spawn and Basic Sync

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
        /* Shared mode: reuse caller's arena directly.
         * The managed arena is thread-safe (lock-free bump + mutex fallback),
         * so no freezing is needed. */
        args->thread_arena = args->caller_arena;
        handle->thread_arena = NULL;  /* Don't destroy - it's the caller's */
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

    /* Link handle to args BEFORE creating thread to avoid race condition.
     * The thread wrapper needs args->handle to release the handle on completion.
     * If we set this after pthread_create, a fast thread could complete before
     * args->handle is set, leaving it NULL and causing a memory leak. */
    args->handle = handle;

    /* Create the pthread */
    int err = pthread_create(&handle->thread, NULL, wrapper, args);
    if (err != 0) {
        fprintf(stderr, "rt_thread_spawn: pthread_create failed with error %d\n", err);
        /* Clean up thread arena on failure */
        if (handle->thread_arena != NULL && !args->is_shared) {
            rt_arena_destroy(handle->thread_arena);
        }
        args->handle = NULL;  /* Clear on failure */
        return NULL;
    }

    /* Detach the thread so OS auto-cleans resources when thread exits.
     * We use condition variables for synchronization instead of pthread_join. */
    pthread_detach(handle->thread);

    /* Track in global pool */
    rt_thread_pool_add(handle);

    /* Track in caller's arena so arena destruction auto-joins the thread */
    if (args->caller_arena != NULL) {
        rt_arena_on_cleanup(args->caller_arena, handle,
                            rt_thread_cleanup, RT_CLEANUP_PRIORITY_HIGH);
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

/* Signal that thread has completed (called by thread wrapper before returning).
 * This wakes up any thread waiting in rt_thread_join.
 * Thread must be detached so pthread resources are auto-cleaned on return. */
void rt_thread_signal_completion(RtThreadHandle *handle)
{
    if (handle == NULL) return;

    pthread_mutex_lock(&handle->completion_mutex);
    handle->done = true;
    pthread_cond_broadcast(&handle->completion_cond);
    pthread_mutex_unlock(&handle->completion_mutex);
}

/* Join a thread and retrieve its result.
 * Waits for the thread to complete using condition variable (not pthread_join
 * because threads are detached for automatic resource cleanup).
 * It handles both void and non-void return types:
 * - For void functions, returns NULL
 * - For non-void functions, returns pointer to the result value
 * The synced flag is set to mark the thread as synchronized.
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

    /* Wait for thread to complete using condition variable.
     * Threads are detached so we can't use pthread_join. */
    pthread_mutex_lock(&handle->completion_mutex);
    while (!handle->done) {
        pthread_cond_wait(&handle->completion_cond, &handle->completion_mutex);
    }
    pthread_mutex_unlock(&handle->completion_mutex);

    /* Mark thread as synchronized */
    handle->synced = true;

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

    /* Release handle and result back to caller arena for GC reclamation */
    rt_thread_handle_release(handle, handle->caller_arena);
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
