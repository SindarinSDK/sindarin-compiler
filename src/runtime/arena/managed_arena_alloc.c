// src/runtime/arena/managed_arena_alloc.c
// Public API: Allocation

RtHandle rt_managed_alloc(RtManagedArena *ma, RtHandle old, size_t size)
{
    if (ma == NULL || size == 0) return RT_HANDLE_NULL;

    size_t aligned_size = align_up(size, sizeof(void *));

    /* Fast path: try lock-free bump on current block.
     * Read block_epoch before and after to detect if the compactor
     * swapped blocks (which would invalidate our pointer).
     * Relaxed ordering is sufficient on x86-64 (TSO guarantees load ordering). */
    unsigned epoch_before = atomic_load_explicit(&ma->block_epoch, memory_order_relaxed);
    RtManagedBlock *alloc_block = ma->current;
    void *ptr = block_try_alloc(alloc_block, aligned_size);

    /* Lock for handle table operations (and slow-path block alloc if needed) */
    pthread_mutex_lock(&ma->alloc_mutex);

    /* Discard bump result if compactor ran (block may have been retired) */
    if (ptr != NULL && atomic_load_explicit(&ma->block_epoch, memory_order_relaxed) != epoch_before) {
        ptr = NULL;
    }

    /* Slow path: block was full or compactor invalidated the bump */
    if (ptr == NULL) {
        ptr = block_alloc_new(ma, aligned_size);
        alloc_block = ma->current;
    }

    /* Mark old allocation as dead */
    if (old != RT_HANDLE_NULL && old < ma->table_count) {
        RtHandleEntry *old_entry = rt_handle_get(ma, old);
        if (!old_entry->dead) {
            old_entry->dead = true;
            atomic_fetch_add(&ma->dead_bytes, old_entry->size);
            atomic_fetch_sub(&ma->live_bytes, old_entry->size);
        }
    }

    /* Create handle entry */
    uint32_t index = next_handle(ma);
    RtHandleEntry *entry = rt_handle_get(ma, index);
    entry->ptr = ptr;
    entry->block = alloc_block;
    entry->size = size;
    entry->leased = 0;  /* Plain int, protected by pin_mutex when accessed */
    entry->dead = false;
    entry->pinned = false;

    atomic_fetch_add(&ma->live_bytes, size);

    pthread_mutex_unlock(&ma->alloc_mutex);

    return (RtHandle)index;
}

RtHandle rt_managed_alloc_pinned(RtManagedArena *ma, RtHandle old, size_t size)
{
    if (ma == NULL || size == 0) return RT_HANDLE_NULL;

    size_t aligned_size = align_up(size, sizeof(void *));

    /* For pinned allocations, we always use the slow path with mutex held.
     * This ensures the allocation is properly tracked and won't be moved. */
    pthread_mutex_lock(&ma->alloc_mutex);

    void *ptr = block_alloc_new(ma, aligned_size);
    RtManagedBlock *alloc_block = ma->current;

    /* Mark old allocation as dead */
    if (old != RT_HANDLE_NULL && old < ma->table_count) {
        RtHandleEntry *old_entry = rt_handle_get(ma, old);
        if (!old_entry->dead) {
            old_entry->dead = true;
            atomic_fetch_add(&ma->dead_bytes, old_entry->size);
            atomic_fetch_sub(&ma->live_bytes, old_entry->size);
        }
    }

    /* Create handle entry with pinned flag set */
    uint32_t index = next_handle(ma);
    RtHandleEntry *entry = rt_handle_get(ma, index);
    entry->ptr = ptr;
    entry->block = alloc_block;
    entry->size = size;
    entry->leased = 0;  /* Plain int, protected by pin_mutex when accessed */
    entry->dead = false;
    entry->pinned = true;  /* Permanently pinned - compactor will never move */

    /* Increment the block's pinned count to prevent it from being freed */
    pthread_mutex_lock(&ma->pin_mutex);
    alloc_block->pinned_count++;
    pthread_mutex_unlock(&ma->pin_mutex);

    pthread_mutex_unlock(&ma->alloc_mutex);

    /* Update stats outside critical section (atomic ops are thread-safe) */
    atomic_fetch_add(&ma->live_bytes, size);

    return (RtHandle)index;
}
