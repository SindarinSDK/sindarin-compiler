#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "managed_arena.h"

/* Forward declarations */
static void invoke_cleanup_list(RtManagedArena *ma);

/* ============================================================================
 * Internal: Block Management
 * ============================================================================ */

static RtManagedBlock *managed_block_create(size_t size)
{
    RtManagedBlock *block = malloc(sizeof(RtManagedBlock) + size);
    if (block == NULL) {
        fprintf(stderr, "managed_block_create: allocation failed\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    block->retired = false;
    return block;
}

static void managed_block_destroy(RtManagedBlock *block)
{
    while (block != NULL) {
        RtManagedBlock *next = block->next;
        free(block);
        block = next;
    }
}

/* Align up to pointer size */
static inline size_t align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Allocate raw bytes from the backing block chain */
static void *block_alloc(RtManagedArena *ma, size_t size)
{
    size_t aligned_size = align_up(size, sizeof(void *));
    RtManagedBlock *block = ma->current;

    /* Try current block */
    if (block->used + aligned_size <= block->size) {
        void *ptr = block->data + block->used;
        block->used += aligned_size;
        return ptr;
    }

    /* Need a new block */
    size_t new_size = ma->block_size;
    if (aligned_size > new_size) {
        new_size = aligned_size;
    }

    RtManagedBlock *new_block = managed_block_create(new_size);
    ma->total_allocated += sizeof(RtManagedBlock) + new_size;
    block->next = new_block;
    ma->current = new_block;

    void *ptr = new_block->data;
    new_block->used = aligned_size;
    return ptr;
}


/* ============================================================================
 * Internal: Handle Table Management
 * ============================================================================ */

static void table_grow(RtManagedArena *ma)
{
    uint32_t new_cap = ma->table_capacity * 2;
    RtHandleEntry *new_table = realloc(ma->table, new_cap * sizeof(RtHandleEntry));
    if (new_table == NULL) {
        fprintf(stderr, "table_grow: allocation failed\n");
        exit(1);
    }
    /* Zero new entries */
    memset(&new_table[ma->table_capacity], 0,
           (new_cap - ma->table_capacity) * sizeof(RtHandleEntry));
    ma->table = new_table;
    ma->table_capacity = new_cap;
}

/* Get next available handle index */
static uint32_t next_handle(RtManagedArena *ma)
{
    /* Try free list first */
    if (ma->free_count > 0) {
        return ma->free_list[--ma->free_count];
    }

    /* Grow table if needed */
    if (ma->table_count >= ma->table_capacity) {
        table_grow(ma);
    }

    return ma->table_count++;
}

/* ============================================================================
 * Internal: GC Thread Forward Declarations
 * ============================================================================ */

extern void *rt_managed_cleaner_thread(void *arg);
extern void *rt_managed_compactor_thread(void *arg);

/* ============================================================================
 * Public API: Lifecycle
 * ============================================================================ */

/* Initialize common arena fields (shared between root and child creation) */
static void arena_init_common(RtManagedArena *ma)
{
    /* Backing store */
    ma->block_size = RT_MANAGED_BLOCK_SIZE;
    ma->first = managed_block_create(ma->block_size);
    ma->current = ma->first;
    ma->total_allocated = sizeof(RtManagedBlock) + ma->block_size;
    ma->retired_list = NULL;

    /* Handle table — index 0 is reserved as RT_HANDLE_NULL */
    ma->table_capacity = RT_MANAGED_TABLE_INIT_CAP;
    ma->table = calloc(ma->table_capacity, sizeof(RtHandleEntry));
    if (ma->table == NULL) {
        fprintf(stderr, "arena_init_common: table allocation failed\n");
        exit(1);
    }
    ma->table_count = 1; /* Skip index 0 (null handle) */

    /* Free list */
    ma->free_capacity = RT_MANAGED_TABLE_INIT_CAP;
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

    /* Mark all live entries as dead */
    pthread_mutex_lock(&child->alloc_mutex);
    for (uint32_t i = 1; i < child->table_count; i++) {
        RtHandleEntry *entry = &child->table[i];
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

    /* Recursively destroy any children of this child */
    while (child->first_child != NULL) {
        RtManagedArena *grandchild = child->first_child;
        child->first_child = grandchild->next_sibling;
        grandchild->parent = NULL; /* Prevent re-unlinking */
        rt_managed_arena_destroy_child(grandchild);
    }

    /* Wait for GC threads to finish any current processing of this arena.
     * The destroying flag ensures they won't start new work on it. */
    int max_wait = 1000;
    while (atomic_load(&child->gc_processing) > 0 && max_wait-- > 0) {
        rt_arena_sleep_ms(1);
    }

    /* Wait for all leases to drain, then free blocks */
    bool has_leases = true;
    max_wait = 1000;
    while (has_leases && max_wait-- > 0) {
        has_leases = false;
        for (uint32_t i = 1; i < child->table_count; i++) {
            if (atomic_load(&child->table[i].leased) > 0) {
                has_leases = true;
                break;
            }
        }
        if (has_leases) {
            rt_arena_sleep_ms(1);
        }
    }

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

    free(child->table);
    child->table = NULL;
    child->table_count = 0;

    free(child->free_list);
    child->free_list = NULL;

    pthread_mutex_destroy(&child->alloc_mutex);
    pthread_mutex_destroy(&child->children_mutex);

    /* Add to root's retired arenas list for deferred free */
    RtManagedArena *root = parent ? rt_managed_arena_root(parent) : NULL;
    if (root && root->is_root) {
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

    /* Free handle table and free list */
    free(ma->table);
    free(ma->free_list);

    /* Free retired arena structs (deferred from destroy_child) */
    RtManagedArena *retired = ma->retired_arenas;
    while (retired != NULL) {
        RtManagedArena *next = retired->next_sibling;
        free(retired);
        retired = next;
    }

    pthread_mutex_destroy(&ma->alloc_mutex);
    pthread_mutex_destroy(&ma->children_mutex);
    free(ma);
}

/* ============================================================================
 * Public API: Allocation
 * ============================================================================ */

RtHandle rt_managed_alloc(RtManagedArena *ma, RtHandle old, size_t size)
{
    if (ma == NULL || size == 0) return RT_HANDLE_NULL;

    pthread_mutex_lock(&ma->alloc_mutex);

    /* Mark old allocation as dead */
    if (old != RT_HANDLE_NULL && old < ma->table_count) {
        RtHandleEntry *old_entry = &ma->table[old];
        if (!old_entry->dead) {
            old_entry->dead = true;
            atomic_fetch_add(&ma->dead_bytes, old_entry->size);
            atomic_fetch_sub(&ma->live_bytes, old_entry->size);
        }
    }

    /* Allocate from backing store */
    void *ptr = block_alloc(ma, size);

    /* Create handle entry */
    uint32_t index = next_handle(ma);
    RtHandleEntry *entry = &ma->table[index];
    entry->ptr = ptr;
    entry->size = size;
    atomic_init(&entry->leased, 0);
    entry->dead = false;

    atomic_fetch_add(&ma->live_bytes, size);

    pthread_mutex_unlock(&ma->alloc_mutex);

    return (RtHandle)index;
}

/* ============================================================================
 * Public API: Promotion (child → parent)
 * ============================================================================ */

RtHandle rt_managed_promote(RtManagedArena *dest, RtManagedArena *src, RtHandle h)
{
    if (dest == NULL || src == NULL || h == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    if (h >= src->table_count) return RT_HANDLE_NULL;

    /* Pin source to get stable pointer */
    RtHandleEntry *src_entry = &src->table[h];
    atomic_fetch_add(&src_entry->leased, 1);

    if (src_entry->ptr == NULL || src_entry->dead) {
        atomic_fetch_sub(&src_entry->leased, 1);
        return RT_HANDLE_NULL;
    }

    size_t size = src_entry->size;

    /* Allocate in destination arena */
    pthread_mutex_lock(&dest->alloc_mutex);
    void *new_ptr = block_alloc(dest, size);
    uint32_t new_index = next_handle(dest);
    RtHandleEntry *dest_entry = &dest->table[new_index];
    dest_entry->ptr = new_ptr;
    dest_entry->size = size;
    atomic_init(&dest_entry->leased, 0);
    dest_entry->dead = false;
    atomic_fetch_add(&dest->live_bytes, size);
    pthread_mutex_unlock(&dest->alloc_mutex);

    /* Copy data */
    memcpy(new_ptr, src_entry->ptr, size);

    /* Unpin source and mark dead */
    atomic_fetch_sub(&src_entry->leased, 1);

    pthread_mutex_lock(&src->alloc_mutex);
    if (!src_entry->dead) {
        src_entry->dead = true;
        atomic_fetch_add(&src->dead_bytes, size);
        atomic_fetch_sub(&src->live_bytes, size);
    }
    pthread_mutex_unlock(&src->alloc_mutex);

    return (RtHandle)new_index;
}

/* ============================================================================
 * Public API: Pin / Unpin
 * ============================================================================ */

void *rt_managed_pin(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) {
        return NULL;
    }

    RtHandleEntry *entry = &ma->table[h];

    /* Increment lease — pointer is now safe from compaction */
    atomic_fetch_add(&entry->leased, 1);

    return entry->ptr;
}

void rt_managed_unpin(RtManagedArena *ma, RtHandle h)
{
    if (ma == NULL || h == RT_HANDLE_NULL || h >= ma->table_count) {
        return;
    }

    RtHandleEntry *entry = &ma->table[h];

    /* Decrement lease */
    atomic_fetch_sub(&entry->leased, 1);
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

    size_t len = strlen(str);
    if (n < len) len = n;

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
        RtHandleEntry *entry = &ma->table[i];
        if (!entry->dead && entry->ptr != NULL) {
            entry->dead = true;
            atomic_fetch_add(&ma->dead_bytes, entry->size);
            atomic_fetch_sub(&ma->live_bytes, entry->size);
        }
    }
    pthread_mutex_unlock(&ma->alloc_mutex);
}

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
        if (!ma->table[i].dead && ma->table[i].ptr != NULL) {
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
        if (ma->table[i].dead && ma->table[i].ptr != NULL) {
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
        used += b->used;
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
