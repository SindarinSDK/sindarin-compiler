// runtime_malloc_redirect_state.c
// Redirect state management and statistics

/* ============================================================================
 * Redirect State Management
 * ============================================================================ */

bool rt_malloc_redirect_push(RtArena *arena, const RtRedirectConfig *config)
{
    if (!arena) return false;

    /* Allocate new state using system malloc */
    RtRedirectState *state;
    if (orig_malloc) {
        state = orig_malloc(sizeof(RtRedirectState));
    } else {
        state = malloc(sizeof(RtRedirectState));
    }
    if (!state) return false;

    memset(state, 0, sizeof(RtRedirectState));

    state->active = true;
    state->arena = arena;
    state->prev = tls_redirect_state;

    /* Apply config */
    if (config) {
        state->config = *config;
    } else {
        RtRedirectConfig default_config = RT_REDIRECT_CONFIG_DEFAULT;
        state->config = default_config;
    }

    /* Create hash set for allocation tracking */
    state->alloc_set = rt_alloc_hash_set_create(256);
    if (!state->alloc_set) {
        if (orig_free) orig_free(state);
        else free(state);
        return false;
    }

    /* Create mutex if thread-safe mode requested */
    if (state->config.thread_safe) {
        if (orig_malloc) {
            state->mutex = orig_malloc(sizeof(pthread_mutex_t));
        } else {
            state->mutex = malloc(sizeof(pthread_mutex_t));
        }
        if (state->mutex) {
            pthread_mutex_init(state->mutex, NULL);
        }
    }

    /* Push onto stack */
    tls_redirect_state = state;

    return true;
}

bool rt_malloc_redirect_pop(void)
{
    RtRedirectState *state = tls_redirect_state;
    if (!state) return false;

    /* Pop from stack */
    tls_redirect_state = state->prev;

    /* Clean up */
    if (state->alloc_set) {
        rt_alloc_hash_set_destroy(state->alloc_set);
    }

    if (state->mutex) {
        pthread_mutex_destroy(state->mutex);
        if (orig_free) orig_free(state->mutex);
        else free(state->mutex);
    }

    /* Free tracking entries */
    RtAllocTrackEntry *track = state->track_head;
    while (track) {
        RtAllocTrackEntry *next = track->next;
        if (orig_free) orig_free(track);
        else free(track);
        track = next;
    }

    if (orig_free) orig_free(state);
    else free(state);

    return true;
}

bool rt_malloc_redirect_is_active(void)
{
    return tls_redirect_state != NULL && tls_redirect_state->active;
}

RtArena *rt_malloc_redirect_arena(void)
{
    if (!tls_redirect_state) return NULL;
    return tls_redirect_state->arena;
}

int rt_malloc_redirect_depth(void)
{
    int depth = 0;
    for (RtRedirectState *s = tls_redirect_state; s; s = s->prev) {
        depth++;
    }
    return depth;
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

bool rt_malloc_redirect_get_stats(RtRedirectStats *stats)
{
    if (!stats || !tls_redirect_state) return false;

    RtRedirectState *state = tls_redirect_state;
    stats->alloc_count = state->alloc_count;
    stats->free_count = state->free_count;
    stats->realloc_count = state->realloc_count;
    stats->total_requested = state->total_requested;
    stats->total_allocated = state->total_allocated;
    stats->fallback_count = state->fallback_count;
    stats->current_live = state->current_live;
    stats->peak_live = state->peak_live;
    stats->hash_set_entries = state->alloc_set ? state->alloc_set->entry_count : 0;

    /* Count tracking entries */
    stats->track_entries = 0;
    for (RtAllocTrackEntry *e = state->track_head; e; e = e->next) {
        stats->track_entries++;
    }

    return true;
}

void rt_malloc_redirect_reset_stats(void)
{
    if (!tls_redirect_state) return;

    RtRedirectState *state = tls_redirect_state;
    state->alloc_count = 0;
    state->free_count = 0;
    state->realloc_count = 0;
    state->total_requested = 0;
    state->total_allocated = 0;
    state->fallback_count = 0;
    /* Don't reset current_live or peak_live - they track actual state */
}

void rt_malloc_redirect_print_stats(void)
{
    RtRedirectStats stats;
    if (!rt_malloc_redirect_get_stats(&stats)) {
        fprintf(stderr, "[REDIRECT] Not active\n");
        return;
    }

    fprintf(stderr, "[REDIRECT] Statistics:\n");
    fprintf(stderr, "  Allocations:   %zu\n", stats.alloc_count);
    fprintf(stderr, "  Frees:         %zu\n", stats.free_count);
    fprintf(stderr, "  Reallocs:      %zu\n", stats.realloc_count);
    fprintf(stderr, "  Requested:     %zu bytes\n", stats.total_requested);
    fprintf(stderr, "  Allocated:     %zu bytes (with headers)\n", stats.total_allocated);
    fprintf(stderr, "  Fallbacks:     %zu\n", stats.fallback_count);
    fprintf(stderr, "  Current live:  %zu\n", stats.current_live);
    fprintf(stderr, "  Peak live:     %zu\n", stats.peak_live);
    fprintf(stderr, "  Hash entries:  %zu\n", stats.hash_set_entries);
    if (stats.track_entries > 0) {
        fprintf(stderr, "  Track entries: %zu\n", stats.track_entries);
    }
}
