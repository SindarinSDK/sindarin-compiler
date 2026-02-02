// src/runtime/runtime_thread_result.c
// Thread Result and Handle Functions

/* ============================================================================
 * Thread Result Functions
 * ============================================================================ */

/* Create a thread result structure in the given arena */
RtThreadResult *rt_thread_result_create(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_result_create: NULL arena\n");
        return NULL;
    }

    RtThreadResult *result = rt_arena_alloc(arena, sizeof(RtThreadResult));
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_create: allocation failed\n");
        exit(1);
    }

    result->value = NULL;
    result->value_size = 0;
    result->has_panic = false;
    result->panic_message = NULL;

    return result;
}

/* Set panic state on a thread result */
void rt_thread_result_set_panic(RtThreadResult *result, const char *message,
                                 RtArena *arena)
{
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_set_panic: NULL result\n");
        return;
    }

    result->has_panic = true;
    if (message != NULL && arena != NULL) {
        result->panic_message = rt_arena_strdup(arena, message);
    } else {
        result->panic_message = NULL;
    }
}

/* Set value on a thread result */
void rt_thread_result_set_value(RtThreadResult *result, void *value,
                                 size_t size, RtArena *arena)
{
    if (result == NULL) {
        fprintf(stderr, "rt_thread_result_set_value: NULL result\n");
        return;
    }

    if (value != NULL && size > 0 && arena != NULL) {
        /* Copy value to arena for safety */
        result->value = rt_arena_alloc(arena, size);
        if (result->value != NULL) {
            memcpy(result->value, value, size);
        }
    } else {
        result->value = value;
    }
    result->value_size = size;
}

/* ============================================================================
 * Thread Handle Functions
 * ============================================================================ */

/* Create a new thread handle in the given arena */
RtThreadHandle *rt_thread_handle_create(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_handle_create: NULL arena\n");
        return NULL;
    }

    RtThreadHandle *handle = rt_arena_alloc(arena, sizeof(RtThreadHandle));
    if (handle == NULL) {
        fprintf(stderr, "rt_thread_handle_create: allocation failed\n");
        exit(1);
    }

    handle->thread = 0;
    handle->result = NULL;
    handle->done = false;
    handle->synced = false;
    handle->thread_arena = NULL;
    handle->caller_arena = NULL;
    handle->result_type = -1;  /* -1 indicates void/unknown */
    handle->is_shared = false;
    handle->is_private = false;

    return handle;
}

/* Release a thread handle and its result back to the arena.
 * Marks them as dead so GC can reclaim the memory.
 * Safe to call even if handle is NULL. */
void rt_thread_handle_release(RtThreadHandle *handle, RtArena *arena)
{
    if (handle == NULL || arena == NULL) return;

    /* Release result->value if allocated */
    if (handle->result != NULL && handle->result->value != NULL) {
        rt_managed_release_pinned(arena, handle->result->value);
    }
    /* Release result struct */
    if (handle->result != NULL) {
        rt_managed_release_pinned(arena, handle->result);
    }
    /* Release handle struct */
    rt_managed_release_pinned(arena, handle);
}

/* ============================================================================
 * Thread Arguments Functions
 * ============================================================================ */

/* Create thread arguments structure in the given arena */
RtThreadArgs *rt_thread_args_create(RtArena *arena, void *func_ptr,
                                     void *args_data, size_t args_size)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_thread_args_create: NULL arena\n");
        return NULL;
    }

    RtThreadArgs *args = rt_arena_alloc(arena, sizeof(RtThreadArgs));
    if (args == NULL) {
        fprintf(stderr, "rt_thread_args_create: allocation failed\n");
        exit(1);
    }

    args->func_ptr = func_ptr;
    args->args_size = args_size;
    args->result = NULL;
    args->caller_arena = NULL;
    args->thread_arena = NULL;
    args->is_shared = false;
    args->is_private = false;

    /* Copy args_data if provided */
    if (args_data != NULL && args_size > 0) {
        args->args_data = rt_arena_alloc(arena, args_size);
        if (args->args_data == NULL) {
            fprintf(stderr, "rt_thread_args_create: args_data allocation failed\n");
            exit(1);
        }
        memcpy(args->args_data, args_data, args_size);
    } else {
        args->args_data = NULL;
    }

    return args;
}
