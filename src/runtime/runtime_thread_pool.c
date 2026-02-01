// src/runtime/runtime_thread_pool.c
// Thread Pool Management

/* ============================================================================
 * Thread Pool Management
 * ============================================================================ */

/* Initialize the global thread pool */
void rt_thread_pool_init(void)
{
    if (g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);
    if (!g_thread_pool_initialized) {
        g_thread_pool.handles = malloc(RT_THREAD_POOL_INITIAL_CAPACITY * sizeof(RtThreadHandle *));
        if (g_thread_pool.handles == NULL) {
            fprintf(stderr, "rt_thread_pool_init: allocation failed\n");
            pthread_mutex_unlock(&g_thread_pool.mutex);
            exit(1);
        }
        g_thread_pool.capacity = RT_THREAD_POOL_INITIAL_CAPACITY;
        g_thread_pool.count = 0;
        g_thread_pool_initialized = true;
    }
    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Add a thread handle to the pool */
void rt_thread_pool_add(RtThreadHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (!g_thread_pool_initialized) {
        rt_thread_pool_init();
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Grow array if needed */
    if (g_thread_pool.count >= g_thread_pool.capacity) {
        size_t new_capacity = g_thread_pool.capacity * 2;
        RtThreadHandle **new_handles = realloc(g_thread_pool.handles,
                                                new_capacity * sizeof(RtThreadHandle *));
        if (new_handles == NULL) {
            fprintf(stderr, "rt_thread_pool_add: realloc failed\n");
            pthread_mutex_unlock(&g_thread_pool.mutex);
            exit(1);
        }
        g_thread_pool.handles = new_handles;
        g_thread_pool.capacity = new_capacity;
    }

    g_thread_pool.handles[g_thread_pool.count++] = handle;
    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Remove a thread handle from the pool */
void rt_thread_pool_remove(RtThreadHandle *handle)
{
    if (handle == NULL || !g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Find and remove the handle */
    for (size_t i = 0; i < g_thread_pool.count; i++) {
        if (g_thread_pool.handles[i] == handle) {
            /* Shift remaining elements */
            for (size_t j = i; j < g_thread_pool.count - 1; j++) {
                g_thread_pool.handles[j] = g_thread_pool.handles[j + 1];
            }
            g_thread_pool.count--;
            break;
        }
    }

    pthread_mutex_unlock(&g_thread_pool.mutex);
}

/* Clean up all threads in the pool (called on process exit) */
void rt_thread_pool_cleanup(void)
{
    if (!g_thread_pool_initialized) {
        return;
    }

    pthread_mutex_lock(&g_thread_pool.mutex);

    /* Cancel all threads that haven't been synced */
    for (size_t i = 0; i < g_thread_pool.count; i++) {
        RtThreadHandle *handle = g_thread_pool.handles[i];
        if (handle != NULL && !handle->synced) {
            pthread_cancel(handle->thread);
        }
    }

    /* Free the handles array */
    free(g_thread_pool.handles);
    g_thread_pool.handles = NULL;
    g_thread_pool.count = 0;
    g_thread_pool.capacity = 0;
    g_thread_pool_initialized = false;

    pthread_mutex_unlock(&g_thread_pool.mutex);
}
