// src/runtime/arena/managed_arena_table.c
// Internal: Handle Table Management (paged)

/* Allocate a new page of handle entries */
static RtHandleEntry *table_alloc_page(void)
{
    RtHandleEntry *page = calloc(RT_HANDLE_PAGE_SIZE, sizeof(RtHandleEntry));
    if (page == NULL) {
        fprintf(stderr, "table_alloc_page: allocation failed\n");
        exit(1);
    }
    return page;
}

/* Add a new page to the table. Grows the page directory if needed.
 * Uses atomic store for thread-safe directory growth. Old directories
 * are deferred for freeing to avoid use-after-free in concurrent readers. */
static void table_add_page(RtManagedArena *ma)
{
    if (ma->pages_count >= ma->pages_capacity) {
        uint32_t new_cap = ma->pages_capacity * 2;
        RtHandleEntry **old_dir = ma->pages;

        /* Allocate new directory and copy existing pointers */
        RtHandleEntry **new_dir = malloc(new_cap * sizeof(RtHandleEntry *));
        if (new_dir == NULL) {
            fprintf(stderr, "table_add_page: directory malloc failed\n");
            exit(1);
        }
        memcpy(new_dir, old_dir, ma->pages_count * sizeof(RtHandleEntry *));

        /* Atomically publish new directory */
        __atomic_store_n(&ma->pages, new_dir, __ATOMIC_RELEASE);
        ma->pages_capacity = new_cap;

        /* Defer freeing old directory (readers may still be using it) */
        RtRetiredPagesNode *node = malloc(sizeof(RtRetiredPagesNode));
        if (node != NULL) {
            node->pages = old_dir;
            node->next = ma->retired_pages;
            ma->retired_pages = node;
        }
        /* If malloc fails, we leak the old directory - acceptable tradeoff */
    }
    ma->pages[ma->pages_count++] = table_alloc_page();
}

/* Get next available handle index */
static uint32_t next_handle(RtManagedArena *ma)
{
    /* Try free list first */
    if (ma->free_count > 0) {
        return ma->free_list[--ma->free_count];
    }

    /* Add pages until we have enough to cover the next index.
     * Child arenas may start with table_count > 0 (inherited from parent's
     * table_count as index_offset), so we may need multiple pages. */
    while (ma->table_count >= ma->pages_count * RT_HANDLE_PAGE_SIZE) {
        table_add_page(ma);
    }

    return ma->table_count++;
}
