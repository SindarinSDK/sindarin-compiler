#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "managed_arena.h"

/* ============================================================================
 * Internal: Utility
 * ============================================================================ */

static inline size_t align_up_compact(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static RtManagedBlock *managed_block_create_compact(size_t size)
{
    size_t total = sizeof(RtManagedBlock) + size;
    RtManagedBlock *block = rt_arena_mmap(total);
    if (block == NULL) {
        fprintf(stderr, "managed_block_create_compact: mmap failed\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    atomic_init(&block->used, 0);
    atomic_init(&block->lease_count, 0);
    block->retired = false;
    return block;
}

static void managed_block_free_gc(RtManagedBlock *block)
{
    rt_arena_munmap(block, sizeof(RtManagedBlock) + block->size);
}

/* Recycle a handle index back to the free list (caller must hold alloc_mutex) */
static void recycle_handle_gc(RtManagedArena *ma, uint32_t index)
{
    if (ma->free_count >= ma->free_capacity) {
        uint32_t new_cap = ma->free_capacity * 2;
        uint32_t *new_list = realloc(ma->free_list, new_cap * sizeof(uint32_t));
        if (new_list == NULL) return; /* Best effort — skip if alloc fails */
        ma->free_list = new_list;
        ma->free_capacity = new_cap;
    }
    ma->free_list[ma->free_count++] = index;
}

/* ============================================================================
 * Tree Traversal
 * ============================================================================
 * The GC threads live on the root arena but must process the entire tree.
 * We snapshot child lists under children_mutex to avoid holding locks during
 * the actual GC work.
 * ============================================================================ */

#define MAX_ARENA_SNAPSHOT 64

/* Collect all arenas in the tree into a flat array for safe iteration.
 * Returns the number of arenas collected. */
static int snapshot_arena_tree(RtManagedArena *root, RtManagedArena **out, int max)
{
    if (root == NULL || max <= 0) return 0;

    int count = 0;
    out[count++] = root;

    /* BFS using the output array as the queue */
    for (int qi = 0; qi < count && count < max; qi++) {
        RtManagedArena *arena = out[qi];

        pthread_mutex_lock(&arena->children_mutex);
        RtManagedArena *child = arena->first_child;
        while (child != NULL && count < max) {
            out[count++] = child;
            child = child->next_sibling;
        }
        pthread_mutex_unlock(&arena->children_mutex);
    }

    return count;
}

/* ============================================================================
 * Cleaner: Per-Arena Pass
 * ============================================================================
 * Scans one arena's handle table for dead + unleased entries.
 * Zeros their memory and recycles the handle slot.
 * ============================================================================ */

static bool clean_arena(RtManagedArena *ma)
{
    bool did_work = false;

    /* Single lock acquisition to process all dead+unleased entries.
     * Previous implementation locked/unlocked per entry (2x per entry)
     * and performed an unnecessary memset on dead memory. */
    pthread_mutex_lock(&ma->alloc_mutex);

    uint32_t count = ma->table_count;
    for (uint32_t i = 1; i < count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (!entry->dead || entry->ptr == NULL || atomic_load(&entry->leased) != 0) {
            continue;
        }
        /* Entry is dead and unleased — recycle immediately */
        atomic_fetch_sub(&ma->dead_bytes, entry->size);
        entry->ptr = NULL;
        entry->block = NULL;
        entry->size = 0;
        entry->dead = false;
        recycle_handle_gc(ma, i);
        did_work = true;
    }

    pthread_mutex_unlock(&ma->alloc_mutex);

    return did_work;
}

/* ============================================================================
 * Cleaner Thread
 * ============================================================================
 * Walks the entire arena tree, cleaning dead entries in each arena.
 * ============================================================================ */

void *rt_managed_cleaner_thread(void *arg)
{
    RtManagedArena *root = (RtManagedArena *)arg;

    while (atomic_load(&root->running)) {
        bool did_work = false;

        /* Snapshot the tree */
        RtManagedArena *arenas[MAX_ARENA_SNAPSHOT];
        int arena_count = snapshot_arena_tree(root, arenas, MAX_ARENA_SNAPSHOT);

        /* Clean each arena */
        for (int a = 0; a < arena_count; a++) {
            if (!atomic_load(&root->running)) break;
            RtManagedArena *ma = arenas[a];
            atomic_fetch_add(&ma->gc_processing, 1);
            if (atomic_load(&ma->destroying)) {
                atomic_fetch_sub(&ma->gc_processing, 1);
                continue;
            }
            if (clean_arena(ma)) {
                did_work = true;
            }
            atomic_fetch_sub(&ma->gc_processing, 1);
        }

        /* Mark iteration complete */
        atomic_fetch_add(&root->gc_cleaner_epoch, 1);

        /* Sleep longer if no work was done */
        if (!did_work) {
            rt_arena_sleep_ms(RT_MANAGED_GC_INTERVAL_MS);
        }
    }

    return NULL;
}

/* ============================================================================
 * Compaction: Hot Swap (per-arena)
 * ============================================================================ */

void rt_managed_compact(RtManagedArena *ma)
{
    if (ma == NULL) return;

    pthread_mutex_lock(&ma->alloc_mutex);

    /* Count live entries to estimate new arena size */
    size_t live_total = 0;
    uint32_t live_count = 0;
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (!entry->dead && entry->ptr != NULL) {
            live_total += align_up_compact(entry->size, sizeof(void *));
            live_count++;
        }
    }

    /* Nothing to compact */
    if (live_count == 0) {
        pthread_mutex_unlock(&ma->alloc_mutex);
        return;
    }

    /* Allocate new block chain — single block if possible */
    size_t new_block_size = live_total > ma->block_size ? live_total : ma->block_size;
    RtManagedBlock *new_first = managed_block_create_compact(new_block_size);

    /* Copy live entries to new block */
    RtManagedBlock *new_current = new_first;
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (entry->dead || entry->ptr == NULL) continue;

        /* Try to acquire entry for moving — CAS leased: 0 → MOVING */
        int expected = 0;
        if (!atomic_compare_exchange_strong(&entry->leased, &expected, RT_LEASE_MOVING)) {
            /* Entry is pinned — skip, best effort.
             * Pinned data stays in old block (which remains alive until drained). */
            continue;
        }

        /* Copy data to new block */
        size_t aligned = align_up_compact(entry->size, sizeof(void *));
        size_t cur_used = atomic_load_explicit(&new_current->used, memory_order_relaxed);
        if (cur_used + aligned > new_current->size) {
            /* Need another block */
            RtManagedBlock *next = managed_block_create_compact(ma->block_size);
            new_current->next = next;
            new_current = next;
            cur_used = 0;
        }
        void *new_ptr = new_current->data + cur_used;
        memcpy(new_ptr, entry->ptr, entry->size);
        atomic_store_explicit(&new_current->used, cur_used + aligned, memory_order_relaxed);

        /* Update handle table pointer and block reference */
        entry->ptr = new_ptr;
        entry->block = new_current;

        /* Release moving lock */
        atomic_store(&entry->leased, 0);
    }

    /* Null out dead + unleased entries that point into old blocks */
    RtManagedBlock *old_first = ma->first;
    for (uint32_t i = 1; i < ma->table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (entry->dead && entry->ptr != NULL && atomic_load(&entry->leased) == 0) {
            /* Check if ptr is in any old block */
            for (RtManagedBlock *b = old_first; b != NULL; b = b->next) {
                char *p = (char *)entry->ptr;
                if (p >= b->data && p < b->data + b->size) {
                    entry->ptr = NULL;
                    entry->block = NULL;
                    entry->size = 0;
                    entry->dead = false;
                    recycle_handle_gc(ma, i);
                    break;
                }
            }
        }
    }

    /* Retire old blocks */
    for (RtManagedBlock *b = old_first; b != NULL; b = b->next) {
        b->retired = true;
    }

    /* Link old blocks to retired list */
    RtManagedBlock *old_tail = old_first;
    while (old_tail->next != NULL) old_tail = old_tail->next;
    old_tail->next = ma->retired_list;
    ma->retired_list = old_first;

    /* Install new blocks and bump epoch to invalidate any in-flight
     * lock-free bumps that targeted the old blocks */
    ma->first = new_first;
    ma->current = new_current;
    atomic_fetch_add_explicit(&ma->block_epoch, 1, memory_order_release);

    /* Update total_allocated to reflect new state */
    size_t new_total = 0;
    for (RtManagedBlock *b = new_first; b != NULL; b = b->next) {
        new_total += sizeof(RtManagedBlock) + b->size;
    }
    ma->total_allocated = new_total;

    /* Reset dead bytes counter (all dead entries were in old blocks) */
    atomic_store(&ma->dead_bytes, 0);

    pthread_mutex_unlock(&ma->alloc_mutex);
}

/* ============================================================================
 * Compactor Thread
 * ============================================================================
 * Walks the arena tree. For each arena, checks fragmentation and compacts
 * if threshold is exceeded. Also retires drained blocks.
 * ============================================================================ */

/* Check if any leased entry points into this block — O(1) via atomic counter */
static bool block_has_leased_entries(RtManagedArena *ma, RtManagedBlock *block)
{
    (void)ma;
    return atomic_load_explicit(&block->lease_count, memory_order_acquire) > 0;
}

/* Try to free retired blocks whose entries are all unleased */
static void retire_drained_blocks(RtManagedArena *ma)
{
    pthread_mutex_lock(&ma->alloc_mutex);

    RtManagedBlock **prev = &ma->retired_list;
    RtManagedBlock *block = ma->retired_list;

    while (block != NULL) {
        RtManagedBlock *next = block->next;
        if (!block_has_leased_entries(ma, block)) {
            *prev = next;
            managed_block_free_gc(block);
        } else {
            prev = &block->next;
        }
        block = next;
    }

    pthread_mutex_unlock(&ma->alloc_mutex);
}

void *rt_managed_compactor_thread(void *arg)
{
    RtManagedArena *root = (RtManagedArena *)arg;

    while (atomic_load(&root->running)) {
        /* Snapshot the tree */
        RtManagedArena *arenas[MAX_ARENA_SNAPSHOT];
        int arena_count = snapshot_arena_tree(root, arenas, MAX_ARENA_SNAPSHOT);

        /* Check fragmentation and compact each arena */
        for (int a = 0; a < arena_count; a++) {
            if (!atomic_load(&root->running)) break;

            RtManagedArena *ma = arenas[a];
            atomic_fetch_add(&ma->gc_processing, 1);
            if (atomic_load(&ma->destroying)) {
                atomic_fetch_sub(&ma->gc_processing, 1);
                continue;
            }

            double frag = rt_managed_fragmentation(ma);
            if (frag >= RT_MANAGED_COMPACT_THRESHOLD) {
                rt_managed_compact(ma);
            }

            /* Try to free fully-drained retired blocks */
            retire_drained_blocks(ma);

            atomic_fetch_sub(&ma->gc_processing, 1);
        }

        /* Mark iteration complete */
        atomic_fetch_add(&root->gc_compactor_epoch, 1);

        rt_arena_sleep_ms(RT_MANAGED_GC_INTERVAL_MS * 10);
    }

    /* Final cleanup of retired blocks across all arenas */
    {
        RtManagedArena *final_arenas[MAX_ARENA_SNAPSHOT];
        int final_count = snapshot_arena_tree(root, final_arenas, MAX_ARENA_SNAPSHOT);
        for (int a = 0; a < final_count; a++) {
            atomic_fetch_add(&final_arenas[a]->gc_processing, 1);
            retire_drained_blocks(final_arenas[a]);
            atomic_fetch_sub(&final_arenas[a]->gc_processing, 1);
        }
    }

    return NULL;
}
