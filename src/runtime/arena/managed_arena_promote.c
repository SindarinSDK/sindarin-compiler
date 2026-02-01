// src/runtime/arena/managed_arena_promote.c
// Public API: Promotion (child → parent)

RtHandle rt_managed_promote(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    /* Invalid handle - out of range for source arena */
    if (h >= src->table_count) return RT_HANDLE_NULL;

    /* Pin source to get stable pointer */
    RtManagedArena *src_root = rt_managed_arena_root(src);
    RtHandleEntry *src_entry = rt_handle_get(src, h);

    pthread_mutex_lock(&src_root->pin_mutex);
    src_entry->leased++;
    if (src_entry->block != NULL) {
        src_entry->block->lease_count++;
    }
    pthread_mutex_unlock(&src_root->pin_mutex);

    if (src_entry->ptr == NULL || src_entry->dead) {
        pthread_mutex_lock(&src_root->pin_mutex);
        if (src_entry->block != NULL) {
            src_entry->block->lease_count--;
        }
        src_entry->leased--;
        pthread_mutex_unlock(&src_root->pin_mutex);
        /* Entry is dead or empty - nothing to promote */
        return RT_HANDLE_NULL;
    }

    size_t size = src_entry->size;

    /* Allocate in destination arena */
    size_t aligned_size = align_up(size, sizeof(void *));
    unsigned epoch_before = atomic_load_explicit(&dest->block_epoch, memory_order_acquire);
    RtManagedBlock *alloc_block = dest->current;
    void *new_ptr = block_try_alloc(alloc_block, aligned_size);
    pthread_mutex_lock(&dest->alloc_mutex);
    if (new_ptr != NULL && atomic_load_explicit(&dest->block_epoch, memory_order_relaxed) != epoch_before) {
        new_ptr = NULL;
    }
    if (new_ptr == NULL) {
        new_ptr = block_alloc_new(dest, aligned_size);
        alloc_block = dest->current;
    }
    uint32_t new_index = next_handle(dest);
    RtHandleEntry *dest_entry = rt_handle_get(dest, new_index);
    dest_entry->ptr = new_ptr;
    dest_entry->block = alloc_block;
    dest_entry->size = size;
    dest_entry->leased = 0;
    dest_entry->dead = false;
    dest_entry->pinned = false;
    atomic_fetch_add(&dest->live_bytes, size);
    pthread_mutex_unlock(&dest->alloc_mutex);

    /* Copy data */
    memcpy(new_ptr, src_entry->ptr, size);

    /* Unpin source and mark dead */
    pthread_mutex_lock(&src_root->pin_mutex);
    if (src_entry->block != NULL) {
        src_entry->block->lease_count--;
    }
    src_entry->leased--;
    pthread_mutex_unlock(&src_root->pin_mutex);

    pthread_mutex_lock(&src->alloc_mutex);
    if (!src_entry->dead) {
        src_entry->dead = true;
        atomic_fetch_add(&src->dead_bytes, size);
        atomic_fetch_sub(&src->live_bytes, size);
    }
    pthread_mutex_unlock(&src->alloc_mutex);

    return (RtHandle)new_index;
}

RtHandle rt_managed_clone(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    if (h >= src->table_count) return RT_HANDLE_NULL;

    /* Pin source to get stable pointer */
    RtManagedArena *src_root = rt_managed_arena_root(src);
    RtHandleEntry *src_entry = rt_handle_get(src, h);

    pthread_mutex_lock(&src_root->pin_mutex);
    src_entry->leased++;
    if (src_entry->block != NULL) {
        src_entry->block->lease_count++;
    }
    pthread_mutex_unlock(&src_root->pin_mutex);

    if (src_entry->ptr == NULL || src_entry->dead) {
        pthread_mutex_lock(&src_root->pin_mutex);
        if (src_entry->block != NULL) {
            src_entry->block->lease_count--;
        }
        src_entry->leased--;
        pthread_mutex_unlock(&src_root->pin_mutex);
        return RT_HANDLE_NULL;
    }

    size_t size = src_entry->size;

    /* Allocate in destination arena (relaxed epoch load, x86-64 TSO safe) */
    size_t aligned_size = align_up(size, sizeof(void *));
    unsigned epoch_before = atomic_load_explicit(&dest->block_epoch, memory_order_relaxed);
    RtManagedBlock *alloc_block = dest->current;
    void *new_ptr = block_try_alloc(alloc_block, aligned_size);
    pthread_mutex_lock(&dest->alloc_mutex);
    if (new_ptr != NULL && atomic_load_explicit(&dest->block_epoch, memory_order_relaxed) != epoch_before) {
        new_ptr = NULL;
    }
    if (new_ptr == NULL) {
        new_ptr = block_alloc_new(dest, aligned_size);
        alloc_block = dest->current;
    }
    uint32_t new_index = next_handle(dest);
    RtHandleEntry *dest_entry = rt_handle_get(dest, new_index);
    dest_entry->ptr = new_ptr;
    dest_entry->block = alloc_block;
    dest_entry->size = size;
    dest_entry->leased = 0;
    dest_entry->dead = false;
    dest_entry->pinned = false;
    atomic_fetch_add(&dest->live_bytes, size);
    pthread_mutex_unlock(&dest->alloc_mutex);

    /* Copy data */
    memcpy(new_ptr, src_entry->ptr, size);

    /* Unpin source (do NOT mark dead — source remains valid) */
    pthread_mutex_lock(&src_root->pin_mutex);
    if (src_entry->block != NULL) {
        src_entry->block->lease_count--;
    }
    src_entry->leased--;
    pthread_mutex_unlock(&src_root->pin_mutex);

    return (RtHandle)new_index;
}

/* Helper to check if a handle is valid in an arena (has non-null, non-dead entry) */
static bool is_handle_valid_in_arena(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) return false;
    /* For child arenas with index_offset, reject indices below the offset.
     * Those indices don't exist in this arena's page table - they belong to parents. */
    if (h < ma->index_offset) return false;
    RtHandleEntry *entry = rt_handle_get(ma, h);
    return entry != NULL && entry->ptr != NULL && !entry->dead;
}

/* Clone from any arena in the tree (self, parents, or root).
 * Like rt_managed_pin_any, walks up the parent chain to find the source handle.
 * Used when handles may come from parent scopes (e.g., loop parent arena).
 * Important: Verifies the handle is actually valid in each arena before cloning,
 * to avoid incorrectly matching an index that exists in the wrong arena. */
RtHandle rt_managed_clone_any(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* Try current arena first (most common case) - verify entry is valid */
    if (is_handle_valid_in_arena(src, h)) {
        return rt_managed_clone(dest, src, h);
    }

    /* Walk up parent chain to find the arena containing this handle */
    RtManagedArena *search = src->parent;
    while (search != NULL) {
        if (is_handle_valid_in_arena(search, h)) {
            return rt_managed_clone(dest, search, h);
        }
        search = search->parent;
    }

    return RT_HANDLE_NULL;
}

/* Clone from parent arenas only, skipping the immediate source arena.
 * Used for cloning function parameters where the handle likely came from a
 * parent scope (not the immediate caller arena). This avoids index collisions
 * when the caller arena has a different entry at the same index. */
RtHandle rt_managed_clone_from_parent(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* Skip the immediate source arena - start searching from its parent.
     * This is because parameters typically come from scopes OUTSIDE the
     * immediate caller, and index collisions can occur in child arenas. */
    RtManagedArena *search = src->parent;
    while (search != NULL) {
        if (is_handle_valid_in_arena(search, h)) {
            return rt_managed_clone(dest, search, h);
        }
        search = search->parent;
    }

    /* Fallback: try the source arena itself (in case it's the root) */
    if (src->parent == NULL && is_handle_valid_in_arena(src, h)) {
        return rt_managed_clone(dest, src, h);
    }

    return RT_HANDLE_NULL;
}

/* Clone preferring parent arenas over the immediate source arena.
 * Used for function parameter cloning where handles typically come from scopes
 * OUTSIDE the immediate caller. If both src and src->parent have valid entries
 * at the same index, prefers the parent's entry (which was allocated first).
 *
 * This handles the common case of child arenas that create new entries at low
 * indices that collide with handles from the parent scope.
 *
 * Search order: parent chain first (if parent has valid entry), then src itself. */
RtHandle rt_managed_clone_prefer_parent(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;

    /* First check parent chain - parent entries predate child entries */
    RtManagedArena *search = src->parent;
    while (search != NULL) {
        if (is_handle_valid_in_arena(search, h)) {
            return rt_managed_clone(dest, search, h);
        }
        search = search->parent;
    }

    /* No parent has this handle - check the source arena itself */
    if (is_handle_valid_in_arena(src, h)) {
        return rt_managed_clone(dest, src, h);
    }

    return RT_HANDLE_NULL;
}
