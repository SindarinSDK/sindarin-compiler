// src/runtime/runtime_thread_sync.c
// Thread Sync with Result Functions

/* ============================================================================
 * Thread Sync with Result
 * ============================================================================ */

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
     * result_value is a pointer to where the actual value is stored.
     * With handle-based arenas, all types (primitives and handles) are
     * stored by value and the caller dereferences to get the result. */
    if (handle->thread_arena == NULL) {
        return result_value;
    }

    /* Default/private mode - promote result to caller's arena */
    void *promoted_result = NULL;
    if (result_value != NULL) {
        size_t result_size = handle->result ? handle->result->value_size : 0;
        promoted_result = rt_thread_promote_result(
            caller_arena,
            handle->thread_arena,
            result_value,
            result_type,
            result_size
        );
    }

    /* Cleanup thread arena now that result is promoted */
    rt_arena_destroy(handle->thread_arena);
    handle->thread_arena = NULL;

    return promoted_result;
}

/* Synchronize a thread handle and get promoted result WITHOUT destroying thread arena.
 * Used for structs with handle fields that need field-by-field promotion.
 * The caller MUST call rt_thread_cleanup_arena(handle) after field promotion.
 */
void *rt_thread_sync_with_result_keep_arena(RtThreadHandle *handle,
                                             RtArena *caller_arena,
                                             RtResultType result_type)
{
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_sync_with_result_keep_arena: NULL handle\n");
        return NULL;
    }

    /* Already synced - return early */
    if (handle->synced) {
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
        return NULL;
    }

    /* Get result value */
    void *result_value = NULL;
    if (handle->result != NULL) {
        result_value = handle->result->value;
    }

    /* For shared mode (thread_arena == NULL), return as-is */
    if (handle->thread_arena == NULL) {
        return result_value;
    }

    /* Default/private mode - promote result to caller's arena
     * but DO NOT destroy thread arena - caller will do field promotion first */
    void *promoted_result = NULL;
    if (result_value != NULL) {
        size_t result_size = handle->result ? handle->result->value_size : 0;
        promoted_result = rt_thread_promote_result(
            caller_arena,
            handle->thread_arena,
            result_value,
            result_type,
            result_size
        );
    }

    /* NOTE: Thread arena is NOT destroyed here - caller must call rt_thread_cleanup_arena */
    return promoted_result;
}

/* Destroy the thread arena after struct field promotion is complete */
void rt_thread_cleanup_arena(RtThreadHandle *handle)
{
    if (handle == NULL) return;

    if (handle->thread_arena != NULL) {
        rt_arena_destroy(handle->thread_arena);
        handle->thread_arena = NULL;
    }
}
