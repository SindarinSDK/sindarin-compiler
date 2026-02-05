// src/runtime/arena/managed_arena_lifecycle.c
// Public API: Lifecycle

/* Initialize common arena fields (shared between root and child creation) */
static void arena_init_common(RtManagedArena *ma)
{
    /* Backing store */
    ma->block_size = RT_MANAGED_BLOCK_SIZE;
    ma->first = managed_block_create(ma->block_size);
    ma->current = ma->first;
    ma->total_allocated = sizeof(RtManagedBlock) + ma->block_size;
    ma->retired_list = NULL;

    /* Handle table (paged) — index 0 is reserved as RT_HANDLE_NULL */
    ma->pages_capacity = RT_HANDLE_DIR_INIT_CAP;
    ma->pages = calloc(ma->pages_capacity, sizeof(RtHandleEntry *));
    if (ma->pages == NULL) {
        fprintf(stderr, "arena_init_common: page directory allocation failed\n");
        exit(1);
    }
    ma->pages[0] = table_alloc_page();
    ma->pages_count = 1;
    ma->table_count = 1; /* Skip index 0 (null handle) */
    ma->retired_pages = NULL;

    /* Free list */
    ma->free_capacity = RT_HANDLE_PAGE_SIZE;
    ma->free_list = malloc(ma->free_capacity * sizeof(uint32_t));
    if (ma->free_list == NULL) {
        fprintf(stderr, "arena_init_common: free list allocation failed\n");
        exit(1);
    }
    ma->free_count = 0;

    /* Stats */
    atomic_init(&ma->live_bytes, 0);
    atomic_init(&ma->dead_bytes, 0);

    /* Synchronization */
    pthread_mutex_init(&ma->alloc_mutex, NULL);
    pthread_mutex_init(&ma->pin_mutex, NULL);
    atomic_init(&ma->block_epoch, 0);

    /* Tree linkage defaults */
    ma->parent = NULL;
    ma->first_child = NULL;
    ma->next_sibling = NULL;
    pthread_mutex_init(&ma->children_mutex, NULL);
    atomic_init(&ma->gc_processing, 0);
    atomic_init(&ma->destroying, false);
    atomic_init(&ma->gc_cleaner_epoch, 0);
    atomic_init(&ma->gc_compactor_epoch, 0);

    /* Cleanup callbacks */
    ma->cleanup_list = NULL;
}

RtManagedArena *rt_managed_arena_create(void)
{
    RtManagedArena *ma = calloc(1, sizeof(RtManagedArena));
    if (ma == NULL) {
        fprintf(stderr, "rt_managed_arena_create: allocation failed\n");
        exit(1);
    }

    arena_init_common(ma);
    ma->is_root = true;

    /* Start background threads (root only) */
    atomic_init(&ma->running, true);
    ma->retired_arenas = NULL;
    pthread_create(&ma->cleaner_thread, NULL, rt_managed_cleaner_thread, ma);
    pthread_create(&ma->compactor_thread, NULL, rt_managed_compactor_thread, ma);

    return ma;
}

RtManagedArena *rt_managed_arena_create_child(RtManagedArena *parent)
{
    if (parent == NULL) return NULL;

    RtManagedArena *child = calloc(1, sizeof(RtManagedArena));
    if (child == NULL) {
        fprintf(stderr, "rt_managed_arena_create_child: allocation failed\n");
        exit(1);
    }

    arena_init_common(child);
    child->is_root = false;
    child->parent = parent;

    /* Start child's handle indices at an offset to avoid collision with parent.
     * This ensures that handles allocated in the child arena won't have the same
     * index as existing entries in the parent, preventing incorrect lookups when
     * parameters come from different arenas.
     *
     * We inherit the parent's current table_count as our starting index.
     * The index_offset tracks the starting point; table_count will grow from there.
     * Pages are only allocated for indices >= index_offset as needed. */
    child->index_offset = parent->table_count;
    child->table_count = parent->table_count;

    /* Link into parent's child list */
    pthread_mutex_lock(&parent->children_mutex);
    child->next_sibling = parent->first_child;
    parent->first_child = child;
    pthread_mutex_unlock(&parent->children_mutex);

    return child;
}

RtManagedArena *rt_managed_arena_root(RtManagedArena *ma)
{
    if (ma == NULL) return NULL;
    while (ma->parent != NULL) {
        ma = ma->parent;
    }
    return ma;
}

void rt_managed_arena_destroy_child(RtManagedArena *child)
{
    if (child == NULL || child->is_root) return;

    RtManagedArena *parent = child->parent;

    /* Invoke cleanup callbacks */
    invoke_cleanup_list(child);

    /* Mark all live entries as dead.
     * Only iterate entries this arena actually allocated (from index_offset onwards).
     * Entries at indices < index_offset don't exist in this arena's page table. */
    pthread_mutex_lock(&child->alloc_mutex);
    uint32_t start_idx = child->index_offset > 0 ? child->index_offset : 1;
    for (uint32_t i = start_idx; i < child->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(child, i);
        if (!entry->dead && entry->ptr != NULL) {
            entry->dead = true;
            atomic_fetch_add(&child->dead_bytes, entry->size);
            atomic_fetch_sub(&child->live_bytes, entry->size);
        }
    }
    pthread_mutex_unlock(&child->alloc_mutex);

    /* Signal GC threads to skip this arena */
    atomic_store(&child->destroying, true);

    /* Unlink from parent's child list */
    if (parent != NULL) {
        pthread_mutex_lock(&parent->children_mutex);
        RtManagedArena **prev = &parent->first_child;
        while (*prev != NULL) {
            if (*prev == child) {
                *prev = child->next_sibling;
                break;
            }
            prev = &(*prev)->next_sibling;
        }
        pthread_mutex_unlock(&parent->children_mutex);
    }

    /* Recursively destroy any children of this child. */
    while (child->first_child != NULL) {
        RtManagedArena *grandchild = child->first_child;
        child->first_child = grandchild->next_sibling;
        /* Don't clear parent yet - destroy_child needs it to find the root */
        rt_managed_arena_destroy_child(grandchild);
    }

    /* Wait for GC threads to finish any current processing of this arena.
     * The destroying flag ensures they won't start new work on it. */
    int max_wait = 1000;
    while (atomic_load(&child->gc_processing) > 0 && max_wait-- > 0) {
        rt_arena_sleep_ms(1);
    }

    /* Force-release all leases - the child arena is being destroyed and the
     * caller guarantees no outstanding accesses (ensured by lexical scoping).
     * Legacy API (rt_arena_alloc) creates permanent pins that would otherwise
     * never drain, making child arena destruction hang.
     * Only iterate entries this arena actually allocated (from index_offset onwards). */
    pthread_mutex_lock(&child->pin_mutex);
    for (uint32_t i = start_idx; i < child->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(child, i);
        entry->leased = 0;
    }
    pthread_mutex_unlock(&child->pin_mutex);

    /* Free blocks and table — but NOT the arena struct itself.
     * GC threads may still hold a stale snapshot reference to this arena.
     * They check the 'destroying' flag before accessing table/blocks,
     * so accessing the struct (for gc_processing/destroying) is safe.
     * The struct is freed when the root arena is destroyed. */
    managed_block_destroy(child->first);
    managed_block_destroy(child->retired_list);
    child->first = NULL;
    child->current = NULL;
    child->retired_list = NULL;

    for (uint32_t p = 0; p < child->pages_count; p++) {
        free(child->pages[p]);
    }
    free(child->pages);
    child->pages = NULL;
    child->pages_count = 0;
    child->table_count = 0;

    free(child->free_list);
    child->free_list = NULL;

    /* Free retired page directories (from table growth) */
    RtRetiredPagesNode *retired_dir = child->retired_pages;
    while (retired_dir != NULL) {
        RtRetiredPagesNode *next = retired_dir->next;
        free(retired_dir->pages);
        free(retired_dir);
        retired_dir = next;
    }
    child->retired_pages = NULL;

    pthread_mutex_destroy(&child->alloc_mutex);
    pthread_mutex_destroy(&child->pin_mutex);
    pthread_mutex_destroy(&child->children_mutex);

    /* Add to root's retired arenas list for epoch-based safe freeing.
     * Record the current compactor epoch so we know when it's safe to free.
     * After 2 GC epochs have passed, no GC thread can have a stale reference. */
    RtManagedArena *root = parent ? rt_managed_arena_root(parent) : NULL;
    if (root && root->is_root) {
        /* Record the epoch when this arena was destroyed */
        child->destroyed_at_epoch = atomic_load(&root->gc_compactor_epoch);

        pthread_mutex_lock(&root->children_mutex);
        child->next_sibling = root->retired_arenas;
        root->retired_arenas = child;
        pthread_mutex_unlock(&root->children_mutex);
    } else {
        /* No root available (e.g., during root destroy) — free immediately */
        free(child);
    }
}

void rt_managed_arena_destroy(RtManagedArena *ma)
{
    if (ma == NULL) return;

    /* Stop background threads (root only) */
    if (ma->is_root) {
        atomic_store(&ma->running, false);
        pthread_join(ma->cleaner_thread, NULL);
        pthread_join(ma->compactor_thread, NULL);
    }

    /* Invoke cleanup callbacks */
    invoke_cleanup_list(ma);

    /* Recursively destroy all children */
    while (ma->first_child != NULL) {
        RtManagedArena *child = ma->first_child;
        ma->first_child = child->next_sibling;
        child->parent = NULL; /* Prevent re-unlinking */
        rt_managed_arena_destroy_child(child);
    }

    /* Free retired blocks */
    managed_block_destroy(ma->retired_list);

    /* Free active blocks */
    managed_block_destroy(ma->first);

    /* Free handle table pages and free list */
    for (uint32_t p = 0; p < ma->pages_count; p++) {
        free(ma->pages[p]);
    }
    free(ma->pages);
    free(ma->free_list);

    /* Free retired page directories (from table growth) */
    RtRetiredPagesNode *retired_dir = ma->retired_pages;
    while (retired_dir != NULL) {
        RtRetiredPagesNode *next = retired_dir->next;
        free(retired_dir->pages);
        free(retired_dir);
        retired_dir = next;
    }

    /* Free retired arena structs (deferred from destroy_child) */
    RtManagedArena *retired = ma->retired_arenas;
    while (retired != NULL) {
        RtManagedArena *next = retired->next_sibling;
        free(retired);
        retired = next;
    }

    pthread_mutex_destroy(&ma->alloc_mutex);
    pthread_mutex_destroy(&ma->pin_mutex);
    pthread_mutex_destroy(&ma->children_mutex);
    free(ma);
}
