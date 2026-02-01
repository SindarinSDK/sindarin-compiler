// src/runtime/arena/managed_arena_diag.c
// Public API: Diagnostics and GC Flush

/* ============================================================================
 * Public API: Diagnostics
 * ============================================================================ */

size_t rt_managed_total_allocated(RtManagedArena *ma)
{
    if (ma == NULL) return 0;
    return ma->total_allocated;
}

size_t rt_managed_live_count(RtManagedArena *ma)
{
    if (ma == NULL) return 0;
    size_t count = 0;
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (!entry->dead && entry->ptr != NULL) {
            count++;
        }
    }
    return count;
}

size_t rt_managed_dead_count(RtManagedArena *ma)
{
    if (ma == NULL) return 0;
    size_t count = 0;
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (entry->dead && entry->ptr != NULL) {
            count++;
        }
    }
    return count;
}

double rt_managed_fragmentation(RtManagedArena *ma)
{
    if (ma == NULL) return 0.0;
    size_t live = atomic_load(&ma->live_bytes);
    size_t dead = atomic_load(&ma->dead_bytes);
    size_t total = live + dead;
    if (total == 0) return 0.0;
    return (double)dead / (double)total;
}

size_t rt_managed_arena_used(RtManagedArena *ma)
{
    if (ma == NULL) return 0;
    size_t used = 0;
    for (RtManagedBlock *b = ma->first; b != NULL; b = b->next) {
        used += atomic_load_explicit(&b->used, memory_order_relaxed);
    }
    return used;
}

/* ============================================================================
 * Public API: GC Flush (deterministic synchronization)
 * ============================================================================ */

void rt_managed_gc_flush(RtManagedArena *ma)
{
    if (ma == NULL) return;

    /* Navigate to root */
    RtManagedArena *root = rt_managed_arena_root(ma);
    if (root == NULL || !root->is_root) return;
    if (!atomic_load(&root->running)) return;

    /* Read current epochs */
    unsigned cleaner_start = atomic_load(&root->gc_cleaner_epoch);
    unsigned compactor_start = atomic_load(&root->gc_compactor_epoch);

    /* Spin-wait until both advance (max 500ms safety bound) */
    int max_wait_ms = 500;
    int waited = 0;
    while (waited < max_wait_ms) {
        unsigned cleaner_now = atomic_load(&root->gc_cleaner_epoch);
        unsigned compactor_now = atomic_load(&root->gc_compactor_epoch);
        if (cleaner_now > cleaner_start && compactor_now > compactor_start) {
            return;
        }
        if (!atomic_load(&root->running)) return;
        rt_arena_sleep_ms(1);
        waited++;
    }
}
