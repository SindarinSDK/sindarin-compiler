#ifndef RUNTIME_ARENA_H
#define RUNTIME_ARENA_H

#include <stddef.h>
#include <stdbool.h>
#include "arena/managed_arena.h"

/* ============================================================================
 * Arena Memory Management — Handle-Based Managed Arena
 * ============================================================================
 * RtArena is now an alias for RtManagedArena. All allocations go through the
 * managed arena's handle table with lock-free bump allocation and background
 * GC (cleaner + compactor threads).
 *
 * The legacy rt_arena_* API is preserved as a bridge layer for internal runtime
 * use (diagnostics, format buffers, etc.). These use permanent pins and cannot
 * be compacted but are short-lived.
 *
 * Generated code uses the handle-based API directly:
 *   RtHandle h = rt_managed_alloc(arena, old, size);
 *   char *p = (char *)rt_managed_pin(arena, h);
 *   // use p
 *   rt_managed_unpin(arena, h);
 * ============================================================================ */

/* RtArena is now RtManagedArena */
typedef RtManagedArena RtArena;

/* Cleanup callback function type (matches RtManagedCleanupFn) */
typedef RtManagedCleanupFn RtCleanupFn;

/* Cleanup priority - lower values are cleaned up first */
typedef enum {
    RT_CLEANUP_PRIORITY_HIGH = 0,         /* Threads synced first (highest priority) */
    RT_CLEANUP_PRIORITY_MEDIUM = 10,      /* Files closed after threads */
    RT_CLEANUP_PRIORITY_DEFAULT = 50      /* Default priority for user resources */
} RtCleanupPriority;

/* Cleanup node — typedef of RtManagedCleanupNode */
typedef RtManagedCleanupNode RtCleanupNode;

/* ============================================================================
 * Legacy Arena API (Bridge to Managed Arena)
 * ============================================================================
 * These functions delegate to the managed arena. Allocations use permanent
 * pins (no unpin) so pointers remain valid for the arena's lifetime.
 * This is appropriate for internal runtime allocations that don't participate
 * in GC (format buffers, diagnostic strings, etc.).
 * ============================================================================ */

/* Create a new arena, optionally with a parent */
RtArena *rt_arena_create(RtArena *parent);

/* Allocate memory from arena (permanently pinned — not compactable) */
void *rt_arena_alloc(RtArena *arena, size_t size);

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size);

/* Duplicate a string into arena */
char *rt_arena_strdup(RtArena *arena, const char *str);

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n);

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena);

/* Reset arena for reuse */
void rt_arena_reset(RtArena *arena);

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size);

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src);

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena);

/* ============================================================================
 * Cleanup Callback Registration (delegates to managed arena)
 * ============================================================================ */

/* Register a cleanup callback for arena destruction/reset. */
RtCleanupNode *rt_arena_on_cleanup(RtArena *arena, void *data,
                                    RtCleanupFn cleanup_fn, int priority);

/* Remove a cleanup callback by data pointer. */
void rt_arena_remove_cleanup(RtArena *arena, void *data);

#endif /* RUNTIME_ARENA_H */
