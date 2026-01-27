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
    block->lease_count = 0;   /* Plain int, protected by pin_mutex */
    block->pinned_count = 0;  /* Plain int, protected by pin_mutex */
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
    RtManagedArena *root = rt_managed_arena_root(ma);

    /* Single lock acquisition to process all dead+unleased entries.
     * Previous implementation locked/unlocked per entry (2x per entry)
     * and performed an unnecessary memset on dead memory. */
    pthread_mutex_lock(&ma->alloc_mutex);
    pthread_mutex_lock(&root->pin_mutex);

    /* For child arenas, indices below index_offset don't have pages allocated.
     * Start from index_offset (or 1 for root arenas where index_offset is 0). */
    uint32_t start = (ma->index_offset > 1) ? ma->index_offset : 1;
    uint32_t count = ma->table_count;
    for (uint32_t i = start; i < count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (!entry->dead || entry->ptr == NULL || entry->leased != 0) {
            continue;
        }
        /* Entry is dead and unleased — recycle immediately.
         * Only ptr and dead need clearing (block/size overwritten on reuse). */
        atomic_fetch_sub(&ma->dead_bytes, entry->size);
        entry->ptr = NULL;
        entry->dead = false;
        recycle_handle_gc(ma, i);
        did_work = true;
    }

    pthread_mutex_unlock(&root->pin_mutex);
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
    if (root == NULL) return NULL;

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

    RtManagedArena *root = rt_managed_arena_root(ma);
    pthread_mutex_lock(&ma->alloc_mutex);
    pthread_mutex_lock(&root->pin_mutex);

    /* Single pass: copy live entries to new block chain.
     * Previous implementation did a counting pass first to pre-size the block,
     * but block overflow is already handled by allocating new blocks on demand.
     * This eliminates one full table traversal. */
    RtManagedBlock *new_first = managed_block_create_compact(ma->block_size);
    RtManagedBlock *new_current = new_first;
    uint32_t live_count = 0;

    RtManagedBlock *old_first = ma->first;
    uint32_t table_count = ma->table_count;

    /* For child arenas, indices below index_offset don't have pages allocated.
     * Start from index_offset (or 1 for root arenas where index_offset is 0). */
    uint32_t start = (ma->index_offset > 1) ? ma->index_offset : 1;
    for (uint32_t i = start; i < table_count; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);

        if (entry->dead || entry->ptr == NULL) {
            /* Recycle dead + unleased entries in the same pass.
             * Since we hold alloc_mutex and pin_mutex, no state changes can occur.
             * All dead entries point to old blocks (live ones are being moved).
             * This replaces the O(entries * blocks) third scan that previously
             * searched the old block chain per entry. */
            if (entry->dead && entry->ptr != NULL && entry->leased == 0) {
                atomic_fetch_sub(&ma->dead_bytes, entry->size);
                entry->ptr = NULL;
                entry->dead = false;
                recycle_handle_gc(ma, i);
            }
            continue;
        }

        /* Skip permanently pinned entries — they must never be moved.
         * These contain pthread_mutex_t, pthread_cond_t, or other OS resources. */
        if (entry->pinned) continue;

        /* Check if entry is leased — if so, skip it.
         * With pin_mutex held, we have exclusive access to leased field. */
        if (entry->leased > 0) {
            /* Entry is temporarily leased — skip, best effort.
             * Leased data stays in old block (which remains alive until drained). */
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

        live_count++;
    }

    pthread_mutex_unlock(&root->pin_mutex);

    /* If no live entries were moved, free the unused new block */
    if (live_count == 0) {
        managed_block_free_gc(new_first);
        pthread_mutex_unlock(&ma->alloc_mutex);
        return;
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

    /* Reset dead bytes counter (recycled entries already decremented) */
    atomic_store(&ma->dead_bytes, 0);

    pthread_mutex_unlock(&ma->alloc_mutex);
}

/* ============================================================================
 * Compactor Thread
 * ============================================================================
 * Walks the arena tree. For each arena, checks fragmentation and compacts
 * if threshold is exceeded. Also retires drained blocks.
 * ============================================================================ */

/* Check block utilization: live_bytes / total_block_capacity.
 * Returns true if utilization is below threshold and we have enough blocks
 * to make compaction worthwhile. This catches cases where the cleaner
 * recycles handles faster than dead_bytes can accumulate. */
static bool should_compact_low_utilization(RtManagedArena *ma)
{
    size_t live = atomic_load(&ma->live_bytes);
    size_t total_capacity = 0;
    int block_count = 0;

    /* Sum capacity across all active blocks */
    for (RtManagedBlock *b = ma->first; b != NULL; b = b->next) {
        total_capacity += b->size;
        block_count++;
    }

    /* Need at least minimum blocks to consider utilization-based compaction */
    if (block_count < RT_MANAGED_UTILIZATION_MIN_BLOCKS) {
        return false;
    }

    /* Avoid division by zero */
    if (total_capacity == 0) {
        return false;
    }

    double utilization = (double)live / (double)total_capacity;

    return utilization < RT_MANAGED_UTILIZATION_THRESHOLD;
}

/* Check if block has any leased or pinned entries — O(1) via counters.
 * Caller must hold pin_mutex. */
static bool block_has_active_entries(RtManagedBlock *block)
{
    /* Block cannot be freed if it has temporarily leased OR permanently pinned entries */
    return block->lease_count > 0 || block->pinned_count > 0;
}

/* Rescue orphaned entries from a block about to be freed.
 * These are entries that were skipped during compaction (because they were leased)
 * and have since been unpinned. Their data must be copied to a live block before
 * the retired block can be freed.
 * Caller must hold both alloc_mutex and pin_mutex. */
static void rescue_orphaned_entries(RtManagedArena *ma, RtManagedBlock *block)
{
    uint32_t start = (ma->index_offset > 1) ? ma->index_offset : 1;
    uint32_t end = ma->table_count;

    for (uint32_t i = start; i < end; i++) {
        RtHandleEntry *entry = rt_handle_get(ma, i);
        if (entry->block != block) continue;
        if (entry->ptr == NULL || entry->dead) continue;

        /* Found a live entry pointing to this block - rescue it */
        size_t aligned = align_up_compact(entry->size, sizeof(void *));

        /* Try current block first */
        void *new_ptr = NULL;
        size_t cur_used = atomic_load_explicit(&ma->current->used, memory_order_relaxed);
        if (cur_used + aligned <= ma->current->size) {
            if (atomic_compare_exchange_strong_explicit(&ma->current->used, &cur_used,
                                                        cur_used + aligned,
                                                        memory_order_acquire,
                                                        memory_order_relaxed)) {
                new_ptr = ma->current->data + cur_used;
            }
        }

        /* Need a new block */
        if (new_ptr == NULL) {
            size_t new_size = ma->block_size;
            if (aligned > new_size) new_size = aligned;

            RtManagedBlock *new_block = managed_block_create_compact(new_size);
            ma->total_allocated += sizeof(RtManagedBlock) + new_size;
            ma->current->next = new_block;
            ma->current = new_block;

            atomic_store_explicit(&new_block->used, aligned, memory_order_relaxed);
            new_ptr = new_block->data;
        }

        /* Copy data and update entry */
        memcpy(new_ptr, entry->ptr, entry->size);
        entry->ptr = new_ptr;
        entry->block = ma->current;
    }
}

/* Try to free retired blocks whose entries are all unleased.
 * Uses pin_mutex to safely check lease_count and pinned_count.
 * Before freeing, rescues any orphaned entries (live entries that were
 * skipped during compaction because they were leased). */
static void retire_drained_blocks(RtManagedArena *ma)
{
    RtManagedArena *root = rt_managed_arena_root(ma);

    pthread_mutex_lock(&ma->alloc_mutex);
    pthread_mutex_lock(&root->pin_mutex);

    RtManagedBlock **prev = &ma->retired_list;
    RtManagedBlock *block = ma->retired_list;

    while (block != NULL) {
        RtManagedBlock *next = block->next;
        if (!block_has_active_entries(block)) {
            /* Before freeing, rescue any live entries still pointing to this block.
             * These are entries that were skipped during compaction (leased at the time)
             * and have since been unpinned. */
            rescue_orphaned_entries(ma, block);
            *prev = next;
            managed_block_free_gc(block);
        } else {
            prev = &block->next;
        }
        block = next;
    }

    pthread_mutex_unlock(&root->pin_mutex);
    pthread_mutex_unlock(&ma->alloc_mutex);
}

void *rt_managed_compactor_thread(void *arg)
{
    RtManagedArena *root = (RtManagedArena *)arg;
    if (root == NULL) return NULL;

    while (atomic_load(&root->running)) {
        /* Snapshot the tree */
        RtManagedArena *arenas[MAX_ARENA_SNAPSHOT];
        int arena_count = snapshot_arena_tree(root, arenas, MAX_ARENA_SNAPSHOT);

        /* FIRST PASS: Free retired blocks from PREVIOUS iteration.
         * This ensures at least one sleep interval between retiring blocks
         * (when compaction installs new blocks) and freeing them, giving
         * in-flight lock-free allocators time to complete and check epochs. */
        for (int a = 0; a < arena_count; a++) {
            if (!atomic_load(&root->running)) break;

            RtManagedArena *ma = arenas[a];
            atomic_fetch_add(&ma->gc_processing, 1);
            if (atomic_load(&ma->destroying)) {
                atomic_fetch_sub(&ma->gc_processing, 1);
                continue;
            }

            retire_drained_blocks(ma);

            atomic_fetch_sub(&ma->gc_processing, 1);
        }

        /* SECOND PASS: Check fragmentation and compact each arena.
         * This may retire more blocks, which will be freed in the next iteration. */
        for (int a = 0; a < arena_count; a++) {
            if (!atomic_load(&root->running)) break;

            RtManagedArena *ma = arenas[a];
            atomic_fetch_add(&ma->gc_processing, 1);
            if (atomic_load(&ma->destroying)) {
                atomic_fetch_sub(&ma->gc_processing, 1);
                continue;
            }

            /* Compact if EITHER condition is met:
             * 1. High fragmentation: many dead handles waiting (existing check)
             * 2. Low block utilization: blocks mostly empty after cleaner recycled handles */
            double frag = rt_managed_fragmentation(ma);
            bool high_frag = (frag >= RT_MANAGED_COMPACT_THRESHOLD);
            bool low_util = should_compact_low_utilization(ma);

            if (high_frag || low_util) {
                rt_managed_compact(ma);
            }

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
