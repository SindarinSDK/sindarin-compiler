/*
 * Arena GC - Garbage Collection Implementation
 * =============================================
 * Stop-the-world garbage collection for arena memory management.
 * Two-pass algorithm:
 * 1. Dead arena sweep - destroy arenas in the condemned queue
 * 2. Handle collection - collect dead handles, free data via free()
 */

#include "arena_gc.h"
#include "arena_v2.h"
#include "arena_handle.h"
#include "arena_id.h"
#include "arena_stats.h"
#include "arena_safepoint.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef __linux__
#include <malloc.h>
#endif
#include "arena_compat.h"
#include "../malloc/runtime_malloc_hooks.h"

/* Debug logging (shared with arena_v2.c) */
extern int arena_debug_initialized;
extern int arena_debug_enabled;
void arena_debug_init(void);

#define GC_DEBUG_LOG(fmt, ...) do { \
    if (!arena_debug_initialized) arena_debug_init(); \
    if (arena_debug_enabled) { \
        fprintf(stderr, "[GC:T%lu] " fmt "\n", (unsigned long)(uintptr_t)pthread_self(), ##__VA_ARGS__); \
        fflush(stderr); \
    } \
} while(0)

/* ============================================================================
 * Core Helpers
 * ============================================================================ */

/* Free all handles in an arena (block-based) */
static void gc_free_arena_handles(RtArenaV2 *arena)
{
    RtHandleBlock *block = arena->blocks_head;
    while (block != NULL) {
        RtHandleBlock *next = block->next;
        for (uint32_t i = 0; i < block->count; i++) {
            RtHandleV2 *h = &block->slots[i];
            /* Skip free/recycled slots (arena == NULL) */
            if (h->arena == NULL) continue;
            if (h->ptr != NULL && !(h->flags & RT_HANDLE_FLAG_EXTERN)) {
                free(h->ptr);
            }
        }
        free(block);
        block = next;
    }
    arena->blocks_head = NULL;
    arena->current_block = NULL;
    arena->free_list_head_ptr = NULL;
    arena->free_list_count = 0;
}

/* Run cleanup callbacks for an arena */
static void gc_run_cleanup_callbacks(RtArenaV2 *arena)
{
    RtCleanupNodeV2 *cleanup = arena->cleanups;
    arena->cleanups = NULL;
    while (cleanup != NULL) {
        RtCleanupNodeV2 *next = cleanup->next;
        if (cleanup->fn) {
            cleanup->fn(cleanup->data);
        }
        free(cleanup);
        cleanup = next;
    }
}

/* Destroy arena struct (mutex and memory) */
static void gc_destroy_arena_struct(RtArenaV2 *arena)
{
    pthread_mutex_destroy(&arena->mutex);
    free(arena);
}

/* Forward declaration for recursive call */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent);

/* Destroy an arena and all its handles */
void rt_arena_v2_destroy(RtArenaV2 *arena, bool unlink_from_parent)
{
    if (arena == NULL) return;

    /* Run cleanup callbacks FIRST (before destroying children).
     * This is critical because thread cleanup callbacks need to join threads
     * that may still be using their child arenas. */
    gc_run_cleanup_callbacks(arena);

    pthread_mutex_lock(&arena->mutex);

    /* Destroy children (recursive) - children don't need to unlink */
    RtArenaV2 *child = arena->first_child;
    arena->first_child = NULL;
    while (child != NULL) {
        RtArenaV2 *next = child->next_sibling;
        child->parent = NULL;
        rt_arena_v2_destroy(child, false);
        child = next;
    }

    /* Free all handles in this arena */
    gc_free_arena_handles(arena);

    /* Unlink from parent (only if requested and parent exists) */
    if (unlink_from_parent && arena->parent != NULL) {
        pthread_mutex_lock(&arena->parent->mutex);
        RtArenaV2 **pp = &arena->parent->first_child;
        while (*pp != NULL) {
            if (*pp == arena) {
                *pp = arena->next_sibling;
                break;
            }
            pp = &(*pp)->next_sibling;
        }
        pthread_mutex_unlock(&arena->parent->mutex);
    }

    pthread_mutex_unlock(&arena->mutex);
    gc_destroy_arena_struct(arena);
}

/* ============================================================================
 * Pass 1: Dead Arena Sweep (Two-Phase Collection)
 * ============================================================================
 * To avoid use-after-free when arenas reference handles in other arenas,
 * we collect all dead arenas first, then destroy them in two passes:
 * 1. Call cleanup callbacks (handles still valid across all arenas)
 * 2. Free all handles and arena structs
 * ============================================================================ */

/* Drain the root's condemned queue — intrusive via condemned_next.
 * Returns the head of the condemned chain. The arenas are already linked
 * via condemned_next from rt_arena_v2_condemn's lock-free CAS push.
 * Children are NOT destroyed — they'll be condemned individually by
 * compiler-generated cleanup code and pushed to the queue on their own. */
static RtArenaV2 *gc_drain_condemned_queue(RtArenaV2 *root)
{
    if (root == NULL) return NULL;
    return __sync_lock_test_and_set(&root->condemned_head, NULL);
}

/* Call cleanup callbacks for a condemned arena. */
static void gc_call_all_arena_callbacks(RtArenaV2 *arena)
{
    gc_run_cleanup_callbacks(arena);
}

/* Free all handles and destroy arena struct (non-recursive).
 * Condemns any remaining children so GC cleans them up on the next cycle.
 * After promotion, copy callbacks update s->__arena__ to the destination,
 * so these children are unreferenced orphans safe to condemn. */
static void gc_destroy_arena_fully(RtArenaV2 *arena)
{
    /* Condemn children — push them to the root's condemned queue.
     * No mutex needed: called during STW, all mutators are parked. */
    RtArenaV2 *child = arena->first_child;
    arena->first_child = NULL;

    while (child != NULL) {
        RtArenaV2 *next = child->next_sibling;
        child->parent = NULL;
        child->next_sibling = NULL;
        rt_arena_v2_condemn(child);
        child = next;
    }

    gc_free_arena_handles(arena);
    gc_destroy_arena_struct(arena);
}

/* Sweep dead arenas - phase 1: drain the condemned queue and call
 * cleanup callbacks. Returns the intrusive list head (handle structs valid).
 * Caller must pass the returned list to gc_sweep_finalize() to free memory. */
static RtArenaV2 *gc_sweep_dead_arenas_callbacks(RtArenaV2 *arena)
{
    if (arena == NULL) return NULL;

    /* Drain the condemned queue — O(condemned) not O(tree) */
    RtArenaV2 *dead_list = gc_drain_condemned_queue(arena);
    if (dead_list == NULL) return NULL;

    /* Call all callbacks on all dead arenas (handle structs still valid).
     * This must happen before any handles are freed to avoid use-after-free
     * when callbacks reference handles across arena boundaries. */
    for (RtArenaV2 *a = dead_list; a != NULL; a = a->condemned_next) {
        gc_call_all_arena_callbacks(a);
    }

    return dead_list;
}

/* Sweep dead arenas - phase 2: free all handle data and arena structs.
 * Each arena is freed non-recursively (descendants already in the list).
 * Iterates via condemned_next (intrusive) — save next before destroying. */
static size_t gc_sweep_finalize(RtArenaV2 *dead_list)
{
    size_t total_bytes = 0;
    RtArenaV2 *a = dead_list;
    while (a != NULL) {
        RtArenaV2 *next = a->condemned_next;
        /* Count bytes in arena's handles before freeing (block iteration) */
        for (RtHandleBlock *block = a->blocks_head; block != NULL; block = block->next) {
            total_bytes += sizeof(RtHandleBlock);
            for (uint32_t i = 0; i < block->count; i++) {
                RtHandleV2 *h = &block->slots[i];
                if (h->arena != NULL) {
                    total_bytes += h->size;
                }
            }
        }
        total_bytes += sizeof(RtArenaV2); /* arena struct itself */
        gc_destroy_arena_fully(a);
        a = next;
    }
    return total_bytes;
}

/* ============================================================================
 * Pass 2: Handle Collection (Two-Phase)
 * ============================================================================
 * Similar to dead arena sweep, we collect all dead handles first, then
 * destroy them in two passes to avoid use-after-free when callbacks
 * reference other dead handles.
 * ============================================================================ */

/* Dead handle list — flat array
 * ================================
 * Dead handles are collected into a growable flat array during block iteration.
 * This replaces the former intrusive linked list (via handle->next) since
 * handles no longer have next/prev pointers. The array is allocated once per
 * GC cycle and freed at the end.
 *
 * On rescue: just clear the DEAD flag (handle stays in its block slot).
 * On free: clear slot fields, push onto arena's free list for recycling. */

typedef struct {
    RtHandleV2 **items;
    size_t count;
    size_t capacity;
} DeadHandleList;

static void dead_list_add(DeadHandleList *list, RtHandleV2 *handle)
{
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 256;
        list->items = realloc(list->items, new_cap * sizeof(RtHandleV2 *));
        list->capacity = new_cap;
    }
    list->items[list->count++] = handle;
}

/* Timing helper — returns nanoseconds (monotonic clock) */
static inline long long gc_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* ============================================================================
 * Persistent GC Workspace
 * ============================================================================
 * The dead handle array is kept persistent — just reset count each cycle.
 * No hash tables, bloom filters, or refcount scanning needed: the compiler
 * generates explicit rt_arena_v2_free() for all struct fields via
 * __release_*_inline__ / __free_*_inline__, so dead handles are always
 * truly dead (no live references point to them). This was verified by
 * benchmark instrumentation: zero rescues across thousands of GC cycles.
 * ============================================================================ */

static struct {
    DeadHandleList dead_handles;/* Flat array of dead handle pointers */
    size_t last_handle_count;   /* Handle count from previous cycle (for logging) */
} _gc_ws = {0};

/* Recursive walk: collect dead handles from all arenas into flat array.
 * Iterates contiguous handle blocks (cache-friendly sequential scan). */
static void gc_collect_dead_handles(RtArenaV2 *arena, DeadHandleList *dead_list,
                                     RtArenaGCResult *result, size_t *total_handles)
{
    if (arena == NULL) return;

    /* No mutex needed: called during STW, all mutators are parked. */
    for (RtHandleBlock *block = arena->blocks_head; block != NULL; block = block->next) {
        for (uint32_t i = 0; i < block->count; i++) {
            RtHandleV2 *handle = &block->slots[i];

            /* Skip free/recycled slots */
            if (handle->arena == NULL) continue;

            (*total_handles)++;

            if (handle->flags & RT_HANDLE_FLAG_DEAD) {
                dead_list_add(dead_list, handle);
                result->bytes_freed += handle->size;
                result->handles_freed++;
            }
        }
    }

    /* Recurse into children */
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling) {
        gc_collect_dead_handles(c, dead_list, result, total_handles);
    }
}

/* Compare function for qsort/bsearch on pointer values */
static int ptr_compare(const void *a, const void *b)
{
    uintptr_t pa = *(const uintptr_t *)a;
    uintptr_t pb = *(const uintptr_t *)b;
    return (pa > pb) - (pa < pb);
}

/* Recursive scan: walk arena tree scanning live handle data for dead handle refs */
static void gc_scan_live_for_dead_refs(RtArenaV2 *arena,
                                        const uintptr_t *dead_addrs, size_t dead_count,
                                        RtArenaGCResult *result)
{
    if (arena == NULL) return;

    for (RtHandleBlock *block = arena->blocks_head; block != NULL; block = block->next) {
        for (uint32_t i = 0; i < block->count; i++) {
            RtHandleV2 *h = &block->slots[i];
            if (h->arena == NULL) continue;
            if (h->flags & RT_HANDLE_FLAG_DEAD) continue;
            if (h->ptr == NULL || h->size < sizeof(void *)) continue;

            uintptr_t *words = (uintptr_t *)h->ptr;
            size_t word_count = h->size / sizeof(void *);
            for (size_t w = 0; w < word_count; w++) {
                uintptr_t val = words[w];
                if (val == 0) continue;
                if (bsearch(&val, dead_addrs, dead_count, sizeof(uintptr_t), ptr_compare)) {
                    RtHandleV2 *rescued = (RtHandleV2 *)val;
                    rescued->flags &= ~RT_HANDLE_FLAG_DEAD;
                    result->bytes_freed -= rescued->size;
                    result->handles_freed--;
                }
            }
        }
    }

    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling) {
        gc_scan_live_for_dead_refs(c, dead_addrs, dead_count, result);
    }
}

/* Rescue: scan live handle data for references to dead handles.
 * Dead handles whose addresses appear in live handle data are "rescued" —
 * their DEAD flag is cleared and they're removed from the dead list.
 *
 * This handles the case where codegen marks a handle dead (e.g. temp freed
 * after method call) but the handle is still referenced by live data (e.g.
 * stored in a struct field inside an array). The rescue prevents premature
 * collection that would cause use-after-free during later promotion. */
static void gc_rescue_referenced_dead_handles(RtArenaV2 *arena,
                                                DeadHandleList *dead_list,
                                                RtArenaGCResult *result)
{
    if (dead_list->count == 0) return;

    /* Build sorted array of dead handle addresses for binary search */
    size_t dead_count = dead_list->count;
    uintptr_t *dead_addrs = (uintptr_t *)malloc(dead_count * sizeof(uintptr_t));
    if (!dead_addrs) return;

    for (size_t i = 0; i < dead_count; i++) {
        dead_addrs[i] = (uintptr_t)dead_list->items[i];
    }
    qsort(dead_addrs, dead_count, sizeof(uintptr_t), ptr_compare);

    /* Scan all live handle data for references to dead handles.
     * Uses recursive walk (matching gc_collect_dead_handles) to avoid
     * stack overflow with deep arena trees (e.g. many threads). */
    gc_scan_live_for_dead_refs(arena, dead_addrs, dead_count, result);


    free(dead_addrs);

    /* Compact dead list: remove rescued handles */
    size_t write = 0;
    for (size_t read = 0; read < dead_list->count; read++) {
        if (dead_list->items[read]->flags & RT_HANDLE_FLAG_DEAD) {
            dead_list->items[write++] = dead_list->items[read];
        }
    }
    dead_list->count = write;
}

/* Collect dead handles and free them. Includes a lightweight rescue pass
 * that checks whether any dead handle's address appears in live handle data.
 * This prevents premature collection of handles that codegen marked dead
 * but are still referenced (e.g. struct fields stored in arrays). */
static void gc_compact_all(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    _gc_ws.dead_handles.count = 0;
    size_t total_handles = 0;

    /* Collect all dead handles */
    gc_collect_dead_handles(arena, &_gc_ws.dead_handles, result, &total_handles);

    _gc_ws.last_handle_count = total_handles;

    if (_gc_ws.dead_handles.count == 0) return;

    /* Rescue dead handles that are still referenced by live handle data */
    gc_rescue_referenced_dead_handles(arena, &_gc_ws.dead_handles, result);

    if (_gc_ws.dead_handles.count == 0) return;

    GC_DEBUG_LOG("gc_compact_all: freeing %zu dead handles", _gc_ws.dead_handles.count);

    /* Free confirmed-dead handles */
    for (size_t i = 0; i < _gc_ws.dead_handles.count; i++) {
        RtHandleV2 *dh = _gc_ws.dead_handles.items[i];

        /* Run and remove any cleanup callback for this handle.
         * STW guarantees no concurrent access, so no mutex needed. */
        RtArenaV2 *slot_arena = dh->arena;
        if (slot_arena != NULL) {
            RtCleanupNodeV2 **pp = &slot_arena->cleanups;
            while (*pp != NULL) {
                if ((*pp)->data == dh) {
                    RtCleanupNodeV2 *node = *pp;
                    *pp = node->next;
                    if (node->fn) {
                        node->fn(node->data);
                    }
                    free(node);
                    break;
                }
                pp = &(*pp)->next;
            }
        }

        /* Free data */
        if (!(dh->flags & RT_HANDLE_FLAG_EXTERN)) {
            free(dh->ptr);
        }

        /* Recycle slot: clear fields and push onto arena's free list.
         * The ptr field is repurposed as the free list link pointer. */
        dh->flags = 0;
        dh->size = 0;
        dh->copy_callback = NULL;
        dh->arena = NULL;  /* Marks slot as free/recycled */
        dh->ptr = (void *)slot_arena->free_list_head_ptr;
        slot_arena->free_list_head_ptr = dh;
        slot_arena->free_list_count++;
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

/* Main GC entry point - three-phase collection.
 * Phase 1: Collect dead arenas, call their callbacks (handle structs stay valid)
 * Phase 2: Collect dead handles in live arenas, free data via free()
 * Phase 3: Free dead arena handle structs and arena structs */
/* GC call/skip counters for diagnostics */
static volatile size_t _gc_call_count = 0;
static volatile size_t _gc_skip_count = 0;

/* Global GC mutex — serializes all GC operations across threads.
 * pthread_mutex_trylock ensures at most one GC runs at a time;
 * other callers skip rather than block. This prevents the TOCTOU race
 * where two threads both read gc_running==false before either sets it. */
static pthread_mutex_t _gc_global_mutex = PTHREAD_MUTEX_INITIALIZER;

/* malloc_trim throttle — only call once every 5 seconds */
#define GC_MALLOC_TRIM_INTERVAL_NS (5LL * 1000000000LL)
static struct timespec _last_trim_time = {0, 0};

size_t rt_arena_v2_gc(RtArenaV2 *arena)
{
    __sync_add_and_fetch(&_gc_call_count, 1);

    /* Try to acquire the global GC lock. If another thread is already
     * running GC, skip this invocation rather than blocking. */
    if (pthread_mutex_trylock(&_gc_global_mutex) != 0) {
        __sync_add_and_fetch(&_gc_skip_count, 1);
        return 0;
    }

    long long t0 = gc_now_ns();

    rt_safepoint_request_stw();

    long long t1 = gc_now_ns();  /* STW acquired */

    /* Temporarily clear the malloc redirect handler to prevent GC's internal
     * allocations (RcTable, LiveHandleList, etc.) from being redirected into
     * the arena being collected. */
    RtMallocHandler *saved_handler = rt_malloc_hooks_get_handler();
    if (saved_handler) rt_malloc_hooks_clear_handler();

    /* Initialize result */
    RtArenaGCResult result = {0};

    /* Phase 1: Sweep dead arenas - call cleanup callbacks but keep handle
     * structs alive. Handle structs must remain valid so that compact's
     * reference counting can still scan handle data in dead arenas. */
    RtArenaV2 *dead_arenas = gc_sweep_dead_arenas_callbacks(arena);

    /* Count dead arenas for debug */
    size_t dead_arena_count = 0;
    for (RtArenaV2 *a = dead_arenas; a != NULL; a = a->condemned_next) {
        dead_arena_count++;
    }

    long long t2 = gc_now_ns();  /* Phase 1 done */

    /* Phase 2: Collect dead handles in all live arenas */
    gc_compact_all(arena, &result);

    long long t3 = gc_now_ns();  /* Phase 2 done */

    /* Phase 3: Now safe to free dead arena handle structs and arena structs.
     * All cleanup callbacks and compact have completed. */
    size_t phase3_bytes = gc_sweep_finalize(dead_arenas);

    long long t4 = gc_now_ns();  /* Phase 3 done */

    /* Fill in arena-level results */
    result.arenas_freed = dead_arena_count;
    result.arena_bytes_freed = phase3_bytes;

    /* Restore malloc redirect handler */
    if (saved_handler) rt_malloc_hooks_set_handler(saved_handler);

    rt_safepoint_release_stw();

    /* Record GC results after STW release */
    result.gc_calls = _gc_call_count;
    result.gc_skips = _gc_skip_count;
    rt_arena_stats_record_gc(arena, &result);

    /* Log phase timing (periodic to avoid flooding) */
    static volatile size_t _gc_timing_cycle = 0;
    size_t cycle = __sync_add_and_fetch(&_gc_timing_cycle, 1);
    if (arena->gc_log_enabled && (cycle <= 5 || cycle % 100 == 0)) {
        fprintf(stderr,
                "[GC-TIMING #%zu] stw_wait=%.2fms phase1=%.2fms phase2=%.2fms "
                "phase3=%.2fms stw_total=%.2fms | "
                "handles=%zu dead=%zu arenas=%zu\n",
                cycle,
                (t1 - t0) / 1e6,
                (t2 - t1) / 1e6,
                (t3 - t2) / 1e6,
                (t4 - t3) / 1e6,
                (t4 - t1) / 1e6,
                _gc_ws.last_handle_count,
                result.handles_freed,
                dead_arena_count);
        fflush(stderr);
    }

    pthread_mutex_unlock(&_gc_global_mutex);

#ifdef __linux__
    /* Throttle malloc_trim to avoid expensive syscall on every GC cycle */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long long elapsed_ns = (now.tv_sec - _last_trim_time.tv_sec) * 1000000000LL
                         + (now.tv_nsec - _last_trim_time.tv_nsec);
    if (elapsed_ns >= GC_MALLOC_TRIM_INTERVAL_NS || _last_trim_time.tv_sec == 0) {
        malloc_trim(0);
        _last_trim_time = now;
    }
#endif

    return result.handles_freed;
}
