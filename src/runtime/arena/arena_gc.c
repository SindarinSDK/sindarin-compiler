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
#include <malloc.h>
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

/* Free all handles in an arena */
static void gc_free_arena_handles(RtArenaV2 *arena)
{
    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;
        if (!(handle->flags & RT_HANDLE_FLAG_EXTERN)) {
            free(handle->ptr);
        }
        free(handle);
        handle = next;
    }
    arena->handles_head = NULL;
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
 * Must be called AFTER gc_compact_all so that compact can still read
 * handle structs from dead arenas for reference counting.
 * Iterates via condemned_next (intrusive) — save next before destroying. */
static size_t gc_sweep_finalize(RtArenaV2 *dead_list)
{
    size_t total_bytes = 0;
    RtArenaV2 *a = dead_list;
    while (a != NULL) {
        RtArenaV2 *next = a->condemned_next;
        /* Count bytes in arena's handles before freeing */
        for (RtHandleV2 *h = a->handles_head; h != NULL; h = h->next)
            total_bytes += h->size + sizeof(RtHandleV2);
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

/* Dead handle list — intrusive via handle->next
 * ================================================
 * Dead handles are collected into a singly-linked list by repurposing the
 * handle's own `next` pointer. This is safe because:
 *
 *   1. rt_handle_v2_unlink() removes the handle from the arena's doubly-linked
 *      list, leaving next/prev stale — we overwrite next with the dead list link.
 *
 *   2. handle->arena still points to the owning arena (never cleared), so the
 *      rescue pass can re-link handles without a separate arena field.
 *
 *   3. On rescue, rt_handle_v2_link() overwrites next/prev to re-insert into
 *      the arena list, restoring the handle to its normal state.
 *
 * Scope: handle->next is repurposed ONLY between gc_collect_and_build_sets
 * (which unlinks dead handles and chains them via next) and the end of
 * gc_compact_all (which frees them). Outside this window, handle->next has
 * its normal arena-list meaning.
 *
 * This eliminates one malloc + one free per dead handle per GC cycle. */

/* Info about a live handle's data pointer for flat refcount scanning */
typedef struct {
    void *ptr;
    size_t size;
} LiveHandleInfo;

typedef struct {
    LiveHandleInfo *items;
    size_t count;
    size_t capacity;
} LiveHandleList;

/* ============================================================================
 * Reference Count Hash Table
 * ============================================================================
 * Simple open-addressing hash table mapping handle addresses to reference
 * counts. Used during GC to determine if a dead handle's children are
 * shared with other handles (live or dead).
 * ============================================================================ */

#define RC_TABLE_INITIAL_CAP 256

typedef struct {
    void *key;          /* Handle address */
    size_t count;       /* Number of handles referencing this address */
} RcEntry;

typedef struct {
    RcEntry *entries;
    size_t capacity;
    size_t size;
} RcTable;

static void rc_table_init(RcTable *t)
{
    t->capacity = RC_TABLE_INITIAL_CAP;
    t->size = 0;
    t->entries = calloc(t->capacity, sizeof(RcEntry));
}

static void rc_table_free(RcTable *t)
{
    free(t->entries);
    t->entries = NULL;
    t->capacity = 0;
    t->size = 0;
}

static void rc_table_grow(RcTable *t)
{
    size_t old_cap = t->capacity;
    RcEntry *old = t->entries;
    t->capacity *= 2;
    t->entries = calloc(t->capacity, sizeof(RcEntry));
    t->size = 0;

    for (size_t i = 0; i < old_cap; i++) {
        if (old[i].key != NULL) {
            /* Re-insert */
            size_t idx = ((uintptr_t)old[i].key >> 3) % t->capacity;
            while (t->entries[idx].key != NULL) {
                idx = (idx + 1) % t->capacity;
            }
            t->entries[idx] = old[i];
            t->size++;
        }
    }
    free(old);
}

static void rc_table_inc(RcTable *t, void *key)
{
    if (key == NULL) return;
    if (t->size * 2 >= t->capacity) rc_table_grow(t);

    size_t idx = ((uintptr_t)key >> 3) % t->capacity;
    while (t->entries[idx].key != NULL) {
        if (t->entries[idx].key == key) {
            t->entries[idx].count++;
            return;
        }
        idx = (idx + 1) % t->capacity;
    }
    t->entries[idx].key = key;
    t->entries[idx].count = 1;
    t->size++;
}


static size_t rc_table_get(RcTable *t, void *key)
{
    if (key == NULL || t->size == 0) return 0;

    size_t idx = ((uintptr_t)key >> 3) % t->capacity;
    while (t->entries[idx].key != NULL) {
        if (t->entries[idx].key == key) {
            return t->entries[idx].count;
        }
        idx = (idx + 1) % t->capacity;
    }
    return 0;
}

/* ============================================================================
 * Bloom Filter — fast rejection of non-handle pointers
 * ============================================================================
 * Most pointer-sized slots in handle data are NOT handle addresses (they're
 * string data, integers, etc.). The bloom filter rejects these in a few
 * bitwise ops instead of a hash table probe. Only bloom filter hits fall
 * through to the real rc_table_get.
 *
 * Uses 2 hash functions and ~8 bits per element for <1% false positive rate.
 * The filter array fits in L1 cache for typical handle counts.
 * ============================================================================ */

typedef struct {
    uint64_t *bits;
    size_t nbits;
} BloomFilter;

static inline size_t bloom_hash1(uintptr_t key) { return key * 0x9E3779B97F4A7C15ULL; }
static inline size_t bloom_hash2(uintptr_t key) { return key * 0x517CC1B727220A95ULL; }

static void bloom_init(BloomFilter *bf, size_t element_count)
{
    /* 8 bits per element, minimum 512 bits */
    bf->nbits = element_count * 8;
    if (bf->nbits < 512) bf->nbits = 512;
    size_t nwords = (bf->nbits + 63) / 64;
    bf->bits = calloc(nwords, sizeof(uint64_t));
}

static void bloom_add(BloomFilter *bf, void *key)
{
    uintptr_t k = (uintptr_t)key;
    size_t b1 = bloom_hash1(k) % bf->nbits;
    size_t b2 = bloom_hash2(k) % bf->nbits;
    bf->bits[b1 / 64] |= (1ULL << (b1 % 64));
    bf->bits[b2 / 64] |= (1ULL << (b2 % 64));
}

static inline bool bloom_maybe_contains(BloomFilter *bf, void *key)
{
    uintptr_t k = (uintptr_t)key;
    size_t b1 = bloom_hash1(k) % bf->nbits;
    size_t b2 = bloom_hash2(k) % bf->nbits;
    return (bf->bits[b1 / 64] & (1ULL << (b1 % 64)))
        && (bf->bits[b2 / 64] & (1ULL << (b2 % 64)));
}

static void bloom_free(BloomFilter *bf)
{
    free(bf->bits);
    bf->bits = NULL;
    bf->nbits = 0;
}

/* Build bloom filter from handle_set keys */
static void bloom_populate_from_rc_table(BloomFilter *bf, RcTable *handle_set)
{
    for (size_t i = 0; i < handle_set->capacity; i++) {
        if (handle_set->entries[i].key != NULL) {
            bloom_add(bf, handle_set->entries[i].key);
        }
    }
}

/* Build reference count table from a flat list of live handle data pointers.
 * Replaces the tree-walking gc_build_refcount_table — live handle data is
 * collected during gc_collect_and_build_sets so no additional tree walk needed.
 * Uses bloom filter for fast rejection of non-handle pointers. */
static void gc_build_refcount_from_list(LiveHandleList *live, RcTable *rctable,
                                        RcTable *handle_set, BloomFilter *bf)
{
    for (size_t i = 0; i < live->count; i++) {
        void **scan = (void **)live->items[i].ptr;
        size_t slots = live->items[i].size / sizeof(void *);
        for (size_t j = 0; j < slots; j++) {
            if (scan[j] != NULL
                && bloom_maybe_contains(bf, scan[j])
                && rc_table_get(handle_set, scan[j]) > 0) {
                rc_table_inc(rctable, scan[j]);
            }
        }
    }
}

/* Single-pass recursive walk: collect dead handles, build handle address set,
 * and gather live handle data pointers for refcount scanning.
 * Merges gc_collect_all_handles + gc_build_handle_set + live handle data
 * collection into one tree walk, reducing mutex lock/unlock and children
 * snapshot malloc/free from 3x to 1x per arena. */

static void gc_collect_and_build_sets(RtArenaV2 *arena, RtHandleV2 **dead_list,
                                       RcTable *handle_set, LiveHandleList *live_handles,
                                       RtArenaGCResult *result)
{
    if (arena == NULL) return;

    /* No mutex needed: called during STW, all mutators are parked. */
    RtHandleV2 *handle = arena->handles_head;
    while (handle != NULL) {
        RtHandleV2 *next = handle->next;

        /* Add ALL handle addresses to the handle set (live + dead) */
        rc_table_inc(handle_set, (void *)handle);

        if (handle->flags & RT_HANDLE_FLAG_DEAD) {
            /* Dead handle: unlink from arena and chain into dead list.
             * After unlink, handle->next is stale — repurpose it as
             * the intrusive dead list link (see comment above). */
            rt_handle_v2_unlink(arena, handle);
            result->bytes_freed += handle->size;
            result->handles_freed++;

            handle->next = *dead_list;
            *dead_list = handle;
        } else if (handle->ptr != NULL) {
            /* Live handle with data: record for refcount scanning */
            if (live_handles->count >= live_handles->capacity) {
                size_t new_cap = live_handles->capacity ? live_handles->capacity * 2 : 256;
                live_handles->items = realloc(live_handles->items, new_cap * sizeof(LiveHandleInfo));
                live_handles->capacity = new_cap;
            }
            live_handles->items[live_handles->count].ptr = handle->ptr;
            live_handles->items[live_handles->count].size = handle->size;
            live_handles->count++;
        }

        handle = next;
    }

    /* Recurse into children. No mutex or snapshot needed under STW —
     * no mutators can modify the child list concurrently. */
    for (RtArenaV2 *c = arena->first_child; c != NULL; c = c->next_sibling) {
        gc_collect_and_build_sets(c, dead_list, handle_set, live_handles, result);
    }
}

/* GC all arenas using reference counting for rescue decisions.
 * Dead handles with refcount > 0 (referenced by live handle data) are rescued.
 * Dead handles with refcount 0 are freed.
 *
 * NOTE: We intentionally do NOT cascade into dead handle data to kill children.
 * Conservative pointer scanning of dead handle data causes false positives:
 * stale pointer values in dead handle data can match live handle addresses,
 * and if those live handles have refcount 0 (only on thread stacks), the
 * cascade incorrectly kills them — causing use-after-free / SIGSEGV.
 * The compiler generates explicit rt_arena_v2_free() calls for all struct
 * fields via __release_*_inline__ / __free_*_inline__, so children of dead
 * handles are already marked DEAD by codegen before GC runs. */
static void gc_compact_all(RtArenaV2 *arena, RtArenaGCResult *result)
{
    if (arena == NULL) return;

    /* Single-pass tree walk: collect dead handles, build handle address set,
     * and gather live handle data pointers — all in one recursive traversal.
     * This reduces mutex lock/unlock and children snapshot malloc/free from
     * 3x to 1x per arena compared to three separate walks. */
    RtHandleV2 *dead_handles = NULL;
    RcTable handle_set;
    rc_table_init(&handle_set);
    LiveHandleList live_handles = {NULL, 0, 0};
    gc_collect_and_build_sets(arena, &dead_handles, &handle_set, &live_handles, result);

    if (dead_handles == NULL) {
        rc_table_free(&handle_set);
        free(live_handles.items);
        return;
    }

    /* Build bloom filter from handle_set for fast rejection in refcount scan */
    BloomFilter bf;
    bloom_init(&bf, handle_set.size);
    bloom_populate_from_rc_table(&bf, &handle_set);

    /* Build reference count table from the flat list of live handle data
     * pointers collected during the single walk. Bloom filter rejects
     * non-handle pointers cheaply; only hits probe the hash table. */
    RcTable rctable;
    rc_table_init(&rctable);
    gc_build_refcount_from_list(&live_handles, &rctable, &handle_set, &bf);

    bloom_free(&bf);

    /* Live handle list no longer needed after refcount table is built */
    free(live_handles.items);

    /* Rescue pass: Dead handles still referenced by live handles are rescued.
     * This prevents use-after-free when scope cleanup marks handles as DEAD
     * but they're still stored in live data structures (e.g., struct fields
     * pushed into arrays). Re-link rescued handles into their arenas and
     * clear the DEAD flag so they stay alive.
     *
     * Iterates the intrusive dead list via handle->next. On rescue,
     * rt_handle_v2_link() overwrites next/prev to re-insert the handle into
     * its arena — we save next before the call so iteration is safe. */
    RtHandleV2 *truly_dead = NULL;
    RtHandleV2 *h = dead_handles;
    while (h != NULL) {
        RtHandleV2 *next = h->next;
        if (rc_table_get(&rctable, (void *)h) > 0) {
            /* Handle is referenced by a live handle -- rescue it.
             * h->arena is the original owning arena (never cleared). */
            GC_DEBUG_LOG("  rescue: h=%p ptr=%p (refcount > 0)", (void *)h, h->ptr);
            h->flags &= ~RT_HANDLE_FLAG_DEAD;
            rt_handle_v2_link(h->arena, h);
            result->handles_freed--;  /* Undo the count from collection */
            result->bytes_freed -= h->size;
        } else {
            /* Truly dead -- chain into truly_dead list */
            h->next = truly_dead;
            truly_dead = h;
        }
        h = next;
    }
    dead_handles = truly_dead;

    GC_DEBUG_LOG("gc_compact_all: freeing dead handle data + structs");

    /* Pass 2: Free all dead handle data + structs.
     * No wrapper nodes to free — handles ARE the list nodes (intrusive).
     *
     * Before freeing, remove any cleanup callback that references this handle
     * from its owning arena. Without this, the cleanup would hold a dangling
     * pointer and fire on freed memory when the arena is later condemned.
     * STW guarantees no concurrent access, so we walk the cleanup list
     * without the arena mutex. */
    RtHandleV2 *dh = dead_handles;
    while (dh != NULL) {
        RtHandleV2 *next = dh->next;
        GC_DEBUG_LOG("  free: h=%p ptr=%p", (void *)dh, dh->ptr);

        /* Run and remove any cleanup callback for this handle before freeing.
         * The callback (e.g., sn_json_cleanup) may release external resources
         * like json-c references. If we just removed it, those would leak.
         * STW guarantees no concurrent access, so no arena mutex needed. */
        if (dh->arena != NULL) {
            RtCleanupNodeV2 **pp = &dh->arena->cleanups;
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

        if (!(dh->flags & RT_HANDLE_FLAG_EXTERN)) {
            free(dh->ptr);
        }
        free(dh);
        dh = next;
    }

    rc_table_free(&rctable);
    rc_table_free(&handle_set);
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

    rt_safepoint_request_stw();

    /* Temporarily clear the malloc redirect handler to prevent GC's internal
     * allocations (RcTable, LiveHandleList, etc.) from being redirected into
     * the arena being collected. On Windows (MinHook inline hooking), ALL
     * malloc/free calls are intercepted including GC internals. If these
     * allocations go to the arena, the GC's reference counting scan sees
     * pointers to dead handles in its own bookkeeping data and falsely
     * rescues them, causing GC to report 0 collected instead of the correct
     * count. Arena-managed data (handle->ptr) was allocated via orig_malloc
     * (hook guard was set), so orig_free from the cleared handler is correct. */
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

    /* Phase 2: Collect dead handles in all live arenas */
    gc_compact_all(arena, &result);

    /* Phase 3: Now safe to free dead arena handle structs and arena structs.
     * All cleanup callbacks and compact have completed. */
    size_t phase3_bytes = gc_sweep_finalize(dead_arenas);

    /* Fill in arena-level results */
    result.arenas_freed = dead_arena_count;
    result.arena_bytes_freed = phase3_bytes;

    /* Restore malloc redirect handler */
    if (saved_handler) rt_malloc_hooks_set_handler(saved_handler);

    rt_safepoint_release_stw();

    /* Record GC results after STW release — recompute_stats walks the
     * arena tree with mutex locks, which is expensive. Doing it outside
     * the STW window avoids extending the pause for all threads. */
    result.gc_calls = _gc_call_count;
    result.gc_skips = _gc_skip_count;
    rt_arena_stats_record_gc(arena, &result);

    pthread_mutex_unlock(&_gc_global_mutex);

    /* Throttle malloc_trim to avoid expensive syscall on every GC cycle */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long long elapsed_ns = (now.tv_sec - _last_trim_time.tv_sec) * 1000000000LL
                         + (now.tv_nsec - _last_trim_time.tv_nsec);
    if (elapsed_ns >= GC_MALLOC_TRIM_INTERVAL_NS || _last_trim_time.tv_sec == 0) {
        malloc_trim(0);
        _last_trim_time = now;
    }

    return result.handles_freed;
}
