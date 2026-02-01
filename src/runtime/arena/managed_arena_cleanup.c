// src/runtime/arena/managed_arena_cleanup.c
// Public API: Cleanup Callbacks and Reset

/* ============================================================================
 * Public API: Cleanup Callbacks
 * ============================================================================ */

RtManagedCleanupNode *rt_managed_on_cleanup(RtManagedArena *ma, void *data,
                                             RtManagedCleanupFn fn, int priority)
{
    if (ma == NULL || fn == NULL) return NULL;

    RtManagedCleanupNode *node = malloc(sizeof(RtManagedCleanupNode));
    if (node == NULL) return NULL;

    node->data = data;
    node->fn = fn;
    node->priority = priority;
    node->next = NULL;

    /* Insert in sorted order by priority (lower values first) */
    pthread_mutex_lock(&ma->alloc_mutex);
    RtManagedCleanupNode **curr = &ma->cleanup_list;
    while (*curr != NULL && (*curr)->priority <= priority) {
        curr = &(*curr)->next;
    }
    node->next = *curr;
    *curr = node;
    pthread_mutex_unlock(&ma->alloc_mutex);

    return node;
}

void rt_managed_remove_cleanup(RtManagedArena *ma, void *data)
{
    if (ma == NULL || data == NULL) return;

    pthread_mutex_lock(&ma->alloc_mutex);
    RtManagedCleanupNode **curr = &ma->cleanup_list;
    while (*curr != NULL) {
        if ((*curr)->data == data) {
            RtManagedCleanupNode *to_remove = *curr;
            *curr = to_remove->next;
            free(to_remove);
            pthread_mutex_unlock(&ma->alloc_mutex);
            return;
        }
        curr = &(*curr)->next;
    }
    pthread_mutex_unlock(&ma->alloc_mutex);
}

/* Invoke and free all cleanup callbacks */
static void invoke_cleanup_list(RtManagedArena *ma)
{
    RtManagedCleanupNode *node = ma->cleanup_list;
    while (node != NULL) {
        RtManagedCleanupNode *next = node->next;
        if (node->fn != NULL) {
            node->fn(node->data);
        }
        free(node);
        node = next;
    }
    ma->cleanup_list = NULL;
}

/* ============================================================================
 * Public API: Reset
 * ============================================================================ */

void rt_managed_arena_reset(RtManagedArena *ma)
{
    if (ma == NULL) return;

    /* Invoke cleanup callbacks */
    pthread_mutex_lock(&ma->alloc_mutex);
    invoke_cleanup_list(ma);

    /* Mark all live entries as dead */
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (!entry->dead && entry->ptr != NULL) {
            entry->dead = true;
            atomic_fetch_add(&ma->dead_bytes, entry->size);
            atomic_fetch_sub(&ma->live_bytes, entry->size);
        }
    }
    pthread_mutex_unlock(&ma->alloc_mutex);
}
