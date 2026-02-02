#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_arena.h"

/* ============================================================================
 * Legacy Arena API Bridge
 * ============================================================================
 * These functions delegate to the managed arena. Allocations use permanent
 * pins (no unpin) so pointers remain valid for the arena's lifetime.
 * This is appropriate for internal runtime allocations that don't participate
 * in GC (format buffers, diagnostic strings, temporary conversions, etc.).
 * ============================================================================ */

/* Create a new arena, optionally with a parent */
RtArena *rt_arena_create(RtArena *parent)
{
    if (parent != NULL) {
        return rt_managed_arena_create_child(parent);
    } else {
        return rt_managed_arena_create();
    }
}

/* Allocate memory from arena (permanently pinned — not compactable) */
void *rt_arena_alloc(RtArena *arena, size_t size)
{
    if (arena == NULL || size == 0) return NULL;

    /* Use pinned allocation — the entry is marked as permanently unmovable
     * so the compactor will never relocate it. This is necessary because
     * rt_arena_alloc returns a raw pointer that callers expect to remain
     * stable for the arena's lifetime. */
    RtHandle h = rt_managed_alloc_pinned(arena, RT_HANDLE_NULL, size);
    if (h == RT_HANDLE_NULL) return NULL;

    /* Get the raw pointer. We still pin to get the pointer, but the entry
     * is already permanently pinned so this is just for the pointer lookup. */
    return rt_managed_pin(arena, h);
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

/* Duplicate a string into arena */
char *rt_arena_strdup(RtArena *arena, const char *str)
{
    if (arena == NULL || str == NULL) return NULL;

    size_t len = strlen(str) + 1;
    RtHandle h = rt_managed_alloc_pinned(arena, RT_HANDLE_NULL, len);
    if (h == RT_HANDLE_NULL) return NULL;

    /* Get pointer (entry is already permanently pinned) */
    char *ptr = (char *)rt_managed_pin(arena, h);
    if (ptr != NULL) {
        memcpy(ptr, str, len);
    }
    return ptr;
}

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n)
{
    if (arena == NULL || str == NULL) return NULL;

    size_t len = strlen(str);
    if (n < len) len = n;

    RtHandle h = rt_managed_alloc_pinned(arena, RT_HANDLE_NULL, len + 1);
    if (h == RT_HANDLE_NULL) return NULL;

    /* Get pointer (entry is already permanently pinned) */
    char *ptr = (char *)rt_managed_pin(arena, h);
    if (ptr != NULL) {
        memcpy(ptr, str, len);
        ptr[len] = '\0';
    }
    return ptr;
}

/* Release a pinned allocation by pointer.
 * Marks the entry as dead and decrements pinned_count so GC can free the block.
 * Used for thread handles/results that need to be released when thread completes. */
void rt_arena_release(RtArena *arena, void *ptr)
{
    rt_managed_release_pinned(arena, ptr);
}

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena)
{
    if (arena == NULL) return;

    if (arena->is_root) {
        rt_managed_arena_destroy(arena);
    } else {
        rt_managed_arena_destroy_child(arena);
    }
}

/* Reset arena for reuse */
void rt_arena_reset(RtArena *arena)
{
    rt_managed_arena_reset(arena);
}

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0) return NULL;

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

/* Get total bytes allocated by arena (sum of used bytes across all blocks) */
size_t rt_arena_total_allocated(RtArena *arena)
{
    return rt_managed_arena_used(arena);
}

/* ============================================================================
 * Cleanup Callback Bridge
 * ============================================================================ */

/* Register a cleanup callback for arena destruction/reset */
RtCleanupNode *rt_arena_on_cleanup(RtArena *arena, void *data,
                                    RtCleanupFn cleanup_fn, int priority)
{
    return rt_managed_on_cleanup(arena, data, cleanup_fn, priority);
}

/* Remove a cleanup callback by data pointer */
void rt_arena_remove_cleanup(RtArena *arena, void *data)
{
    rt_managed_remove_cleanup(arena, data);
}
