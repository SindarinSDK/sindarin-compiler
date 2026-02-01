// runtime_malloc_redirect_track.c
// Pointer queries and allocation tracking

/* ============================================================================
 * Pointer Queries
 * ============================================================================ */

bool rt_malloc_redirect_is_arena_ptr(void *ptr)
{
    if (!ptr || !tls_redirect_state) return false;
    return rt_alloc_hash_set_contains(tls_redirect_state->alloc_set, ptr);
}

size_t rt_malloc_redirect_ptr_size(void *ptr)
{
    if (!ptr || !tls_redirect_state) return 0;
    return rt_alloc_hash_set_get_size(tls_redirect_state->alloc_set, ptr);
}

/* ============================================================================
 * Allocation Tracking
 * ============================================================================ */

/* Prevent compiler from reordering or eliminating tracking code */
#if defined(__GNUC__) || defined(__clang__)
#define COMPILER_BARRIER() __asm__ volatile("" ::: "memory")
#define NOINLINE __attribute__((noinline))
#else
#define COMPILER_BARRIER() do {} while(0)
#define NOINLINE
#endif

static NOINLINE void track_allocation(RtRedirectState *state, void *ptr, size_t size, void *caller)
{
    /* Force memory read - prevent compiler from optimizing away config check */
    COMPILER_BARRIER();
    bool should_track = state->config.track_allocations;
    COMPILER_BARRIER();
    if (!should_track) return;

    RtAllocTrackEntry *entry;
    if (orig_malloc) {
        entry = orig_malloc(sizeof(RtAllocTrackEntry));
    } else {
        entry = malloc(sizeof(RtAllocTrackEntry));
    }
    if (!entry) return;

    entry->ptr = ptr;
    entry->size = size;
    entry->caller = caller;
    entry->freed = false;

    /* Memory barrier to ensure proper ordering of linked list update */
    COMPILER_BARRIER();
    entry->next = state->track_head;
    COMPILER_BARRIER();
    state->track_head = entry;
    COMPILER_BARRIER();
}

static NOINLINE void track_free(RtRedirectState *state, void *ptr)
{
    if (!state->config.track_allocations) return;

    for (RtAllocTrackEntry *e = state->track_head; e; e = e->next) {
        if (e->ptr == ptr && !e->freed) {
            e->freed = true;
            return;
        }
    }
}

size_t rt_malloc_redirect_track_iterate(RtAllocTrackCallback callback, void *user_data)
{
    if (!callback || !tls_redirect_state) return 0;

    size_t count = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        callback(e->ptr, e->size, e->freed, e->caller, user_data);
        count++;
    }

    return count;
}

size_t rt_malloc_redirect_track_leaks(void **ptrs, size_t *sizes, size_t max_count)
{
    if (!tls_redirect_state) return 0;

    size_t count = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        if (!e->freed) {
            if (count < max_count) {
                if (ptrs) ptrs[count] = e->ptr;
                if (sizes) sizes[count] = e->size;
            }
            count++;
        }
    }

    return count;
}

void rt_malloc_redirect_track_print(void)
{
    if (!tls_redirect_state || !tls_redirect_state->config.track_allocations) {
        fprintf(stderr, "[REDIRECT] Tracking not enabled\n");
        return;
    }

    fprintf(stderr, "[REDIRECT] Tracked allocations:\n");
    size_t live = 0, freed = 0;
    for (RtAllocTrackEntry *e = tls_redirect_state->track_head; e; e = e->next) {
        fprintf(stderr, "  %p: %zu bytes %s", e->ptr, e->size,
                e->freed ? "[freed]" : "[live]");
        if (e->caller) {
            fprintf(stderr, " (caller: %p)", e->caller);
        }
        fprintf(stderr, "\n");

        if (e->freed) freed++;
        else live++;
    }
    fprintf(stderr, "  Total: %zu live, %zu freed\n", live, freed);
}
