// src/runtime/arena/managed_arena_block.c
// Internal: Block Management

static RtManagedBlock *managed_block_create(size_t size)
{
    size_t total = sizeof(RtManagedBlock) + size;
    RtManagedBlock *block = rt_arena_mmap(total);
    if (block == NULL) {
        fprintf(stderr, "managed_block_create: mmap failed\n");
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

static void managed_block_free(RtManagedBlock *block)
{
    rt_arena_munmap(block, sizeof(RtManagedBlock) + block->size);
}

static void managed_block_destroy(RtManagedBlock *block)
{
    while (block != NULL) {
        RtManagedBlock *next = block->next;
        managed_block_free(block);
        block = next;
    }
}

/* Align up to pointer size */
static inline size_t align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Lock-free bump allocation from current block.
 * Returns pointer on success, NULL if block is full. */
static void *block_try_alloc(RtManagedBlock *block, size_t aligned_size)
{
    size_t old_used = atomic_load_explicit(&block->used, memory_order_relaxed);
    while (old_used + aligned_size <= block->size) {
        if (atomic_compare_exchange_weak_explicit(&block->used, &old_used,
                                                   old_used + aligned_size,
                                                   memory_order_acquire,
                                                   memory_order_relaxed)) {
            return block->data + old_used;
        }
        /* CAS failed, old_used updated by CAS — retry */
    }
    return NULL;
}

/* Slow path: allocate a new block and bump from it.
 * Uses geometric growth (doubles block_size up to RT_MANAGED_BLOCK_MAX_SIZE).
 * Caller must hold alloc_mutex. */
static void *block_alloc_new(RtManagedArena *ma, size_t aligned_size)
{
    size_t new_size = ma->block_size;
    if (aligned_size > new_size) {
        new_size = aligned_size;
    }

    RtManagedBlock *new_block = managed_block_create(new_size);
    ma->total_allocated += sizeof(RtManagedBlock) + new_size;
    ma->current->next = new_block;
    ma->current = new_block;

    /* Geometric growth: double for next allocation, capped at max */
    if (ma->block_size < RT_MANAGED_BLOCK_MAX_SIZE) {
        ma->block_size = (ma->block_size * 2 <= RT_MANAGED_BLOCK_MAX_SIZE)
            ? ma->block_size * 2 : RT_MANAGED_BLOCK_MAX_SIZE;
    }

    atomic_store_explicit(&new_block->used, aligned_size, memory_order_relaxed);
    return new_block->data;
}
