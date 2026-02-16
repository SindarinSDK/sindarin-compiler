// runtime_malloc_redirect_alloc.c
// Hooked allocation functions implementation

/* ============================================================================
 * Hooked Functions Implementation
 * ============================================================================ */

static NOINLINE void *redirected_malloc(size_t size)
{
    RtRedirectState *state = tls_redirect_state;

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_malloc) return orig_malloc(size);
        return malloc(size);
    }

    /* Check max size limit */
    if (state->config.max_arena_size > 0) {
        size_t current = state->total_allocated;
        if (current + size + sizeof(RtAllocHeader) > state->config.max_arena_size) {
            switch (state->config.overflow_policy) {
            case RT_REDIRECT_OVERFLOW_GROW:
                /* Continue anyway */
                break;
            case RT_REDIRECT_OVERFLOW_FALLBACK:
                state->fallback_count++;
                if (orig_malloc) return orig_malloc(size);
                return malloc(size);
            case RT_REDIRECT_OVERFLOW_FAIL:
                return NULL;
            case RT_REDIRECT_OVERFLOW_PANIC:
                if (state->config.on_overflow) {
                    state->config.on_overflow(state->arena, size,
                                               state->config.callback_user_data);
                }
                fprintf(stderr, "[REDIRECT] Arena overflow: requested %zu, "
                        "current %zu, max %zu\n",
                        size, current, state->config.max_arena_size);
                abort();
            }
        }
    }

    /* IMPORTANT: Set hook guard BEFORE any operations that might call malloc,
     * including pthread_mutex_lock which may allocate internally.
     * This prevents infinite recursion when pthread or arena calls malloc. */
    tls_hook_guard = 1;

    /* Lock if thread-safe */
    if (state->mutex) {
        pthread_mutex_lock(state->mutex);
    }

    /* Allocate with header */
    size_t total_size = sizeof(RtAllocHeader) + size;
    RtHandleV2 *raw_h = rt_arena_v2_alloc(state->arena, total_size);
    void *raw = raw_h ? (raw_h->ptr) : NULL;

    if (state->mutex) {
        pthread_mutex_unlock(state->mutex);
    }

    tls_hook_guard = 0;

    if (!raw) {
        /* Arena allocation failed */
        if (state->config.overflow_policy == RT_REDIRECT_OVERFLOW_FALLBACK) {
            state->fallback_count++;
            if (orig_malloc) return orig_malloc(size);
            return malloc(size);
        }
        return NULL;
    }

    /* Fill in header */
    RtAllocHeader *header = (RtAllocHeader *)raw;
    header->size = size;
    header->magic = RT_ALLOC_MAGIC;
    header->flags = 0;

    void *user_ptr = header + 1;  /* Pointer to user data */

    /* Add to hash set */
    rt_alloc_hash_set_insert(state->alloc_set, user_ptr, size);

    /* Update stats */
    state->alloc_count++;
    state->total_requested += size;
    state->total_allocated += total_size;
    state->current_live++;
    if (state->current_live > state->peak_live) {
        state->peak_live = state->current_live;
    }

    /* Track allocation */
    void *caller = NULL;
#if defined(__GNUC__) || defined(__clang__)
    caller = __builtin_return_address(0);
#endif
    track_allocation(state, user_ptr, size, caller);

    /* Callback */
    if (state->config.on_alloc) {
        tls_hook_guard = 1;
        state->config.on_alloc(user_ptr, size, state->config.callback_user_data);
        tls_hook_guard = 0;
    }

    return user_ptr;
}

static void redirected_free(void *ptr)
{
    if (!ptr) return;

    RtRedirectState *state = tls_redirect_state;

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_free) orig_free(ptr);
        else free(ptr);
        return;
    }

    /* Check if this is an arena pointer */
    if (!rt_alloc_hash_set_contains(state->alloc_set, ptr)) {
        /* Not our pointer - pass through to real free */
        if (orig_free) orig_free(ptr);
        else free(ptr);
        return;
    }

    /* Get size from header for stats/callbacks */
    RtAllocHeader *header = ((RtAllocHeader *)ptr) - 1;
    size_t size = 0;
    if (header->magic == RT_ALLOC_MAGIC) {
        size = header->size;
    }

    /* Apply free policy */
    switch (state->config.free_policy) {
    case RT_REDIRECT_FREE_IGNORE:
        /* Do nothing */
        break;

    case RT_REDIRECT_FREE_TRACK:
        track_free(state, ptr);
        break;

    case RT_REDIRECT_FREE_WARN:
        tls_hook_guard = 1;
        fprintf(stderr, "[REDIRECT] Warning: free(%p) called on arena memory "
                "(size=%zu)\n", ptr, size);
        tls_hook_guard = 0;
        break;

    case RT_REDIRECT_FREE_ERROR:
        tls_hook_guard = 1;
        fprintf(stderr, "[REDIRECT] Error: free(%p) called on arena memory\n", ptr);
        tls_hook_guard = 0;
        abort();
    }

    /* Zero memory if requested */
    if (state->config.zero_on_free && size > 0) {
        memset(ptr, 0, size);
    }

    /* Remove from hash set */
    rt_alloc_hash_set_remove(state->alloc_set, ptr);

    /* Update stats */
    state->free_count++;
    if (state->current_live > 0) {
        state->current_live--;
    }

    /* Callback */
    if (state->config.on_free) {
        tls_hook_guard = 1;
        state->config.on_free(ptr, size, state->config.callback_user_data);
        tls_hook_guard = 0;
    }

    /* Note: Memory is NOT actually freed - it remains in the arena
     * and will be freed when the arena is destroyed */
}

static void *redirected_calloc(size_t count, size_t size)
{
    size_t total = count * size;

    /* Check for overflow */
    if (size != 0 && total / size != count) {
        return NULL;
    }

    void *ptr = redirected_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

static void *redirected_realloc(void *ptr, size_t new_size)
{
    RtRedirectState *state = tls_redirect_state;

    /* Case 1: ptr is NULL - equivalent to malloc */
    if (ptr == NULL) {
        return redirected_malloc(new_size);
    }

    /* Case 2: new_size is 0 - equivalent to free */
    if (new_size == 0) {
        redirected_free(ptr);
        return NULL;
    }

    /* Not redirecting - use original */
    if (!state || !state->active || tls_hook_guard) {
        if (orig_realloc) return orig_realloc(ptr, new_size);
        return realloc(ptr, new_size);
    }

    /* Case 3: Check if ptr is from arena */
    if (!rt_alloc_hash_set_contains(state->alloc_set, ptr)) {
        /* Not our pointer - use real realloc */
        if (orig_realloc) return orig_realloc(ptr, new_size);
        return realloc(ptr, new_size);
    }

    state->realloc_count++;

    /* Get original size from header */
    RtAllocHeader *header = ((RtAllocHeader *)ptr) - 1;
    size_t old_size = 0;
    if (header->magic == RT_ALLOC_MAGIC) {
        old_size = header->size;
    } else {
        /* Fallback: try hash set */
        old_size = rt_alloc_hash_set_get_size(state->alloc_set, ptr);
    }

    /* Case 4: Shrinking - just update size metadata */
    if (new_size <= old_size) {
        header->size = new_size;
        rt_alloc_hash_set_insert(state->alloc_set, ptr, new_size);
        return ptr;
    }

    /* Case 5: Growing - allocate new and copy */
    void *new_ptr = redirected_malloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* Copy old data */
    memcpy(new_ptr, ptr, old_size);

    /* Zero old memory if requested */
    if (state->config.zero_on_free) {
        memset(ptr, 0, old_size);
    }

    /* Remove old from hash set (don't update stats - not a real free) */
    rt_alloc_hash_set_remove(state->alloc_set, ptr);

    /* Track the "free" of old pointer */
    track_free(state, ptr);

    return new_ptr;
}

/* ============================================================================
 * Wrapper functions that call the redirected implementations
 * ============================================================================ */

static void *hooked_malloc(size_t size)
{
    return redirected_malloc(size);
}

static void hooked_free(void *ptr)
{
    redirected_free(ptr);
}

static void *hooked_calloc(size_t count, size_t size)
{
    return redirected_calloc(count, size);
}

static void *hooked_realloc(void *ptr, size_t size)
{
    return redirected_realloc(ptr, size);
}
