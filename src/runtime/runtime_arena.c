#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_arena.h"

/* Forward declaration for thread join function to avoid circular include */
extern void rt_thread_sync(RtThreadHandle *handle);

/* ============================================================================
 * Arena Memory Management Implementation
 * ============================================================================ */

/* Allocate a new arena block */
static RtArenaBlock *rt_arena_new_block(size_t size)
{
    RtArenaBlock *block = malloc(sizeof(RtArenaBlock) + size);
    if (block == NULL) {
        fprintf(stderr, "rt_arena_new_block: allocation failed\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

/* Create a new arena with custom block size */
RtArena *rt_arena_create_sized(RtArena *parent, size_t block_size)
{
    if (block_size == 0) {
        block_size = RT_ARENA_DEFAULT_BLOCK_SIZE;
    }

    RtArena *arena = malloc(sizeof(RtArena));
    if (arena == NULL) {
        fprintf(stderr, "rt_arena_create_sized: allocation failed\n");
        exit(1);
    }

    arena->parent = parent;
    arena->default_block_size = block_size;
    arena->total_allocated = 0;
    arena->open_files = NULL;       /* Initialize file handle list to empty */
    arena->active_threads = NULL;   /* Initialize thread handle list to empty */
    arena->frozen = false;          /* Initialize frozen flag to false */

    /* Create initial block */
    arena->first = rt_arena_new_block(block_size);
    arena->current = arena->first;
    arena->total_allocated += sizeof(RtArenaBlock) + block_size;

    return arena;
}

/* Create a new arena with default block size */
RtArena *rt_arena_create(RtArena *parent)
{
    return rt_arena_create_sized(parent, RT_ARENA_DEFAULT_BLOCK_SIZE);
}

/* Align a pointer up to the given alignment */
static inline size_t rt_align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Allocate aligned memory from arena */
void *rt_arena_alloc_aligned(RtArena *arena, size_t size, size_t alignment)
{
    if (arena == NULL || size == 0) {
        return NULL;
    }

    /* Check if arena is frozen (shared thread mode)
     * Only block allocations from non-owner threads */
    if (arena->frozen) {
        pthread_t current_thread = pthread_self();
        if (!pthread_equal(current_thread, arena->frozen_owner)) {
            fprintf(stderr, "panic: cannot allocate from frozen arena (shared thread executing)\n");
            exit(1);
        }
    }

    /* Ensure alignment is at least sizeof(void*) and a power of 2 */
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    RtArenaBlock *block = arena->current;

    /* Calculate the actual address where we'd allocate next */
    char *current_ptr = block->data + block->used;

    /* Calculate alignment padding needed for the actual memory address */
    size_t addr = (size_t)current_ptr;
    size_t aligned_addr = rt_align_up(addr, alignment);
    size_t padding = aligned_addr - addr;

    /* Calculate total space needed */
    size_t end_offset = block->used + padding + size;

    /* Check if allocation fits in current block */
    if (end_offset <= block->size) {
        block->used = end_offset;
        return (char *)aligned_addr;
    }

    /* Need a new block - calculate size needed */
    size_t needed = size + alignment;  /* Extra for alignment */
    size_t new_block_size = arena->default_block_size;
    if (needed > new_block_size) {
        new_block_size = needed;
    }

    /* Allocate new block */
    RtArenaBlock *new_block = rt_arena_new_block(new_block_size);
    arena->total_allocated += sizeof(RtArenaBlock) + new_block_size;

    /* Link new block */
    block->next = new_block;
    arena->current = new_block;

    /* Allocate from new block with proper alignment */
    current_ptr = new_block->data;
    addr = (size_t)current_ptr;
    aligned_addr = rt_align_up(addr, alignment);
    padding = aligned_addr - addr;

    new_block->used = padding + size;
    return (char *)aligned_addr;
}

/* Allocate memory from arena (uninitialized) */
void *rt_arena_alloc(RtArena *arena, size_t size)
{
    return rt_arena_alloc_aligned(arena, size, sizeof(void *));
}

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = rt_arena_alloc(arena, total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/* Duplicate a string into arena.
 * IMMUTABLE STRING: Creates a simple arena-allocated copy with NO metadata.
 * This function can remain unchanged - it creates immutable strings that are
 * compatible with all existing code. Use rt_string_from() if you need a
 * mutable string with metadata that can be efficiently appended to. */
char *rt_arena_strdup(RtArena *arena, const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    if (n < len) {
        len = n;
    }
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Join all tracked threads first to prevent orphaned threads
     * Panics are silently discarded since arena destruction implies
     * fire-and-forget semantics for unsynced threads */
    RtThreadTrackingNode *tn = arena->active_threads;
    while (tn != NULL) {
        if (tn->handle != NULL) {
            /* Sync the thread - this will join and propagate panics */
            rt_thread_sync(tn->handle);
        }
        tn = tn->next;
    }
    arena->active_threads = NULL;

    /* Close all tracked file handles */
    RtFileHandle *fh = arena->open_files;
    while (fh != NULL) {
        if (fh->is_open && fh->fp != NULL) {
            fclose((FILE *)fh->fp);
            fh->is_open = false;
        }
        fh = fh->next;
    }

    /* Free all blocks */
    RtArenaBlock *block = arena->first;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

/* Reset arena for reuse (keeps first block, frees rest) */
void rt_arena_reset(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Join all tracked threads first */
    RtThreadTrackingNode *tn = arena->active_threads;
    while (tn != NULL) {
        if (tn->handle != NULL) {
            rt_thread_sync(tn->handle);
        }
        tn = tn->next;
    }
    arena->active_threads = NULL;

    /* Close all tracked file handles */
    RtFileHandle *fh = arena->open_files;
    while (fh != NULL) {
        if (fh->is_open && fh->fp != NULL) {
            fclose((FILE *)fh->fp);
            fh->is_open = false;
        }
        fh = fh->next;
    }
    arena->open_files = NULL;

    /* Free all blocks except the first */
    RtArenaBlock *block = arena->first->next;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    /* Reset first block */
    arena->first->next = NULL;
    arena->first->used = 0;
    arena->current = arena->first;
    arena->total_allocated = sizeof(RtArenaBlock) + arena->first->size;
}

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return NULL;
    }
    void *copy = rt_arena_alloc(dest, size);
    if (copy != NULL) {
        memcpy(copy, src, size);
    }
    return copy;
}

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src)
{
    return rt_arena_strdup(dest, src);
}

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena)
{
    if (arena == NULL) {
        return 0;
    }
    return arena->total_allocated;
}

/* ============================================================================
 * File Handle Tracking Implementation
 * ============================================================================ */

/* Track a file handle in an arena */
RtFileHandle *rt_arena_track_file(RtArena *arena, void *fp, const char *path, bool is_text)
{
    if (arena == NULL || fp == NULL) {
        return NULL;
    }

    /* Allocate handle from arena */
    RtFileHandle *handle = rt_arena_alloc(arena, sizeof(RtFileHandle));
    if (handle == NULL) {
        return NULL;
    }

    handle->fp = fp;
    handle->path = rt_arena_strdup(arena, path);
    handle->is_open = true;
    handle->is_text = is_text;

    /* Add to front of arena's file list */
    handle->next = arena->open_files;
    arena->open_files = handle;

    return handle;
}

/* Untrack a file handle from an arena (removes from list but doesn't close) */
void rt_arena_untrack_file(RtArena *arena, RtFileHandle *handle)
{
    if (arena == NULL || handle == NULL) {
        return;
    }

    /* Find and remove from list */
    RtFileHandle **curr = &arena->open_files;
    while (*curr != NULL) {
        if (*curr == handle) {
            *curr = handle->next;
            handle->next = NULL;
            return;
        }
        curr = &(*curr)->next;
    }
}

/* ============================================================================
 * Thread Handle Tracking Implementation
 * ============================================================================ */

/* Track a thread handle in an arena */
RtThreadTrackingNode *rt_arena_track_thread(RtArena *arena, RtThreadHandle *handle)
{
    if (arena == NULL || handle == NULL) {
        return NULL;
    }

    /* Allocate tracking node from arena */
    RtThreadTrackingNode *node = rt_arena_alloc(arena, sizeof(RtThreadTrackingNode));
    if (node == NULL) {
        return NULL;
    }

    node->handle = handle;

    /* Add to front of arena's thread list */
    node->next = arena->active_threads;
    arena->active_threads = node;

    return node;
}

/* Untrack a thread handle from an arena (removes from list but doesn't join) */
void rt_arena_untrack_thread(RtArena *arena, RtThreadHandle *handle)
{
    if (arena == NULL || handle == NULL) {
        return;
    }

    /* Find and remove from list */
    RtThreadTrackingNode **curr = &arena->active_threads;
    while (*curr != NULL) {
        if ((*curr)->handle == handle) {
            RtThreadTrackingNode *to_remove = *curr;
            *curr = to_remove->next;
            to_remove->next = NULL;
            to_remove->handle = NULL;
            return;
        }
        curr = &(*curr)->next;
    }
}

/* ============================================================================
 * Arena Freezing Implementation (for shared thread mode)
 * ============================================================================ */

/* Freeze an arena to prevent allocations (used during shared thread execution) */
void rt_arena_freeze(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }
    arena->frozen = true;
}

/* Unfreeze an arena to allow allocations again (called after thread sync) */
void rt_arena_unfreeze(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }
    arena->frozen = false;
}

/* Check if an arena is frozen */
bool rt_arena_is_frozen(RtArena *arena)
{
    if (arena == NULL) {
        return false;
    }
    return arena->frozen;
}
