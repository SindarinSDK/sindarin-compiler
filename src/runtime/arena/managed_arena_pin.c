// src/runtime/arena/managed_arena_pin.c
// Public API: Pin / Unpin / Mark Dead / String Helpers

/* ============================================================================
 * Public API: Pin / Unpin
 * ============================================================================
 * Pin functions walk the arena parent chain to find handles.
 * This simplifies code generation - no need to track which arena owns a handle.
 * ============================================================================ */

/* Direct pin from a specific arena (no parent walk). Used internally.
 * Uses pin_mutex to safely increment lease counts, preventing races
 * with block freeing. The mutex is only held during the increment,
 * not for the duration of the pin. */
static void *rt_managed_pin_direct(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) {
        return NULL;
    }

    RtHandleEntry *entry = rt_handle_get(ma, h);
    RtManagedArena *root = rt_managed_arena_root(ma);

    /* Lock pin_mutex to safely increment lease counts */
    pthread_mutex_lock(&root->pin_mutex);
    entry->leased++;
    if (entry->block != NULL) {
        entry->block->lease_count++;
    }
    void *ptr = entry->ptr;
    pthread_mutex_unlock(&root->pin_mutex);

    return ptr;
}

/* Pin a handle, searching the arena tree (self, parents, root) to find it.
 * This is the primary pin function - handles can come from any parent scope. */
void *rt_managed_pin(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL) {
        return NULL;
    }

    /* Try current arena first (most common case) - verify entry is valid */
    if (is_handle_valid_in_arena(ma, h)) {
        return rt_managed_pin_direct(ma, h);
    }

    /* Walk up parent chain to find the arena containing this handle */
    RtManagedArena *search = ma->parent;
    while (search != NULL) {
        if (is_handle_valid_in_arena(search, h)) {
            return rt_managed_pin_direct(search, h);
        }
        search = search->parent;
    }

    return NULL;
}

void rt_managed_unpin(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) {
        return;
    }

    RtHandleEntry *entry = rt_handle_get(ma, h);
    RtManagedArena *root = rt_managed_arena_root(ma);

    /* Lock pin_mutex to safely decrement lease counts */
    pthread_mutex_lock(&root->pin_mutex);
    if (entry->block != NULL) {
        entry->block->lease_count--;
    }
    entry->leased--;
    pthread_mutex_unlock(&root->pin_mutex);
}

/* Legacy alias for rt_managed_pin (both now walk the parent chain) */
void *rt_managed_pin_any(RtManagedArena *ma, RtHandle h)
{
    return rt_managed_pin(ma, h);
}

/* ============================================================================
 * Public API: Mark Dead
 * ============================================================================ */

void rt_managed_mark_dead(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) return;

    pthread_mutex_lock(&ma->alloc_mutex);
    RtHandleEntry *entry = rt_handle_get(ma, h);
    if (!entry->dead && entry->ptr != NULL) {
        entry->dead = true;
        atomic_fetch_add(&ma->dead_bytes, entry->size);
        atomic_fetch_sub(&ma->live_bytes, entry->size);
    }
    pthread_mutex_unlock(&ma->alloc_mutex);
}

/* ============================================================================
 * Public API: String Helpers
 * ============================================================================ */

RtHandle rt_managed_strdup(RtManagedArena *ma, RtHandle old, const char *str)
{
    if (ma == NULL || str == NULL) return RT_HANDLE_NULL;

    size_t len = strlen(str);
    RtHandle h = rt_managed_alloc(ma, old, len + 1);
    if (h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    char *ptr = (char *)rt_managed_pin(ma, h);
    memcpy(ptr, str, len + 1);
    rt_managed_unpin(ma, h);

    return h;
}

RtHandle rt_managed_strndup(RtManagedArena *ma, RtHandle old, const char *str, size_t n)
{
    if (ma == NULL || str == NULL) return RT_HANDLE_NULL;

    size_t len = strnlen(str, n);

    RtHandle h = rt_managed_alloc(ma, old, len + 1);
    if (h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    char *ptr = (char *)rt_managed_pin(ma, h);
    memcpy(ptr, str, len);
    ptr[len] = '\0';
    rt_managed_unpin(ma, h);

    return h;
}

RtHandle rt_managed_promote_string(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    return rt_managed_promote(dest, src, h);
}
