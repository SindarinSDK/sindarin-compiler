#include "test_framework.h"

/* ============================================================================
 * Simple deterministic PRNG (LCG) for reproducible test patterns
 * ============================================================================ */
static uint32_t prng_state = 0;

static void prng_seed(uint32_t seed)
{
    prng_state = seed;
}

static uint32_t prng_next(void)
{
    prng_state = prng_state * 1664525u + 1013904223u;
    return prng_state;
}

static uint32_t prng_range(uint32_t max)
{
    return prng_next() % max;
}

/* ============================================================================
 * Profile 1: Fragmentation Storm
 * Goal: Maximise fragmentation across many arenas to stress the compactor.
 * ============================================================================ */

static void test_profile_fragmentation_storm(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    prng_seed(42);

    #define FRAG_CHILDREN 8
    #define FRAG_HANDLES 200

    RtManagedArena *children[FRAG_CHILDREN];
    RtHandle handles[FRAG_CHILDREN][FRAG_HANDLES];

    /* Create children and initial allocations */
    for (int c = 0; c < FRAG_CHILDREN; c++) {
        children[c] = rt_managed_arena_create_child(root);
        for (int h = 0; h < FRAG_HANDLES; h++) {
            handles[c][h] = rt_managed_alloc(children[c], RT_HANDLE_NULL, 64);
            char *ptr = (char *)rt_managed_pin(children[c], handles[c][h]);
            snprintf(ptr, 64, "c%d-h%d", c, h);
            rt_managed_unpin(children[c], handles[c][h]);
        }
    }

    /* Rapid reassignment in random order to create checkerboard pattern */
    for (int round = 0; round < 3; round++) {
        for (int c = 0; c < FRAG_CHILDREN; c++) {
            for (int i = 0; i < FRAG_HANDLES / 2; i++) {
                int idx = (int)prng_range(FRAG_HANDLES);
                handles[c][idx] = rt_managed_alloc(children[c], handles[c][idx], 64);
                char *ptr = (char *)rt_managed_pin(children[c], handles[c][idx]);
                snprintf(ptr, 64, "c%d-h%d-r%d", c, idx, round);
                rt_managed_unpin(children[c], handles[c][idx]);
            }
        }
    }

    /* Wait for GC to process dead entries */
    rt_managed_gc_flush(root);

    /* Force compaction on root (walks children too) */
    rt_managed_compact(root);
    rt_managed_gc_flush(root);

    /* Verify: live data intact across all children */
    for (int c = 0; c < FRAG_CHILDREN; c++) {
        size_t live = rt_managed_live_count(children[c]);
        ASSERT_EQ(live, FRAG_HANDLES, "child live count preserved after compaction");

        /* Spot-check a few handles */
        for (int i = 0; i < 10; i++) {
            int idx = (int)prng_range(FRAG_HANDLES);
            char *ptr = (char *)rt_managed_pin(children[c], handles[c][idx]);
            ASSERT(ptr != NULL, "fragmented entry still accessible");
            rt_managed_unpin(children[c], handles[c][idx]);
        }
    }

    rt_managed_arena_destroy(root);

    #undef FRAG_CHILDREN
    #undef FRAG_HANDLES
}

/* ============================================================================
 * Profile 2: Mixed Scope Modes (Function Call Simulation)
 * Goal: Simulate a realistic call graph using default/private/shared modes.
 * ============================================================================ */

static void test_profile_mixed_scope_modes(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    prng_seed(123);

    RtHandle promoted_results[100];
    int promoted_count = 0;

    for (int call = 0; call < 50; call++) {
        uint32_t mode = prng_range(3);

        if (mode == 0) {
            /* Default mode: create child, allocate locals, promote 1-2 results */
            RtManagedArena *child = rt_managed_arena_create_child(root);

            /* Allocate "local" variables */
            for (int i = 0; i < 5; i++) {
                RtHandle local = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
                char *ptr = (char *)rt_managed_pin(child, local);
                snprintf(ptr, 32, "local-%d-%d", call, i);
                rt_managed_unpin(child, local);
            }

            /* Promote 1-2 results to parent */
            int num_promote = 1 + (int)prng_range(2);
            for (int p = 0; p < num_promote && promoted_count < 100; p++) {
                RtHandle result = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
                char *ptr = (char *)rt_managed_pin(child, result);
                snprintf(ptr, 64, "result-%d-%d", call, p);
                rt_managed_unpin(child, result);

                promoted_results[promoted_count] = rt_managed_promote(root, child, result);
                promoted_count++;
            }

            rt_managed_arena_destroy_child(child);
        }
        else if (mode == 1) {
            /* Private mode: create child, allocate locals, no promotion */
            RtManagedArena *child = rt_managed_arena_create_child(root);

            for (int i = 0; i < 8; i++) {
                RtHandle local = rt_managed_alloc(child, RT_HANDLE_NULL, 48);
                char *ptr = (char *)rt_managed_pin(child, local);
                snprintf(ptr, 48, "private-%d-%d", call, i);
                rt_managed_unpin(child, local);
            }

            rt_managed_arena_destroy_child(child);
        }
        else {
            /* Shared mode: reuse parent arena directly */
            RtHandle shared = rt_managed_alloc(root, RT_HANDLE_NULL, 32);
            char *ptr = (char *)rt_managed_pin(root, shared);
            snprintf(ptr, 32, "shared-%d", call);
            rt_managed_unpin(root, shared);
        }
    }

    /* Verify: root has promoted data intact */
    size_t live = rt_managed_live_count(root);
    ASSERT(live >= (size_t)promoted_count, "root has at least promoted_count live entries");

    /* Spot-check promoted results */
    for (int i = 0; i < promoted_count && i < 10; i++) {
        char *ptr = (char *)rt_managed_pin(root, promoted_results[i]);
        ASSERT(ptr != NULL, "promoted result accessible");
        ASSERT(strncmp(ptr, "result-", 7) == 0, "promoted result has correct prefix");
        rt_managed_unpin(root, promoted_results[i]);
    }

    /* Let GC reclaim dead memory from destroyed children */
    rt_managed_gc_flush(root);

    rt_managed_arena_destroy(root);
}

/* ============================================================================
 * Profile 3: Web Server Request Handling
 * Goal: Simulate short-lived request arenas with session data promotion.
 * ============================================================================ */

static void test_profile_web_server(void)
{
    RtManagedArena *server = rt_managed_arena_create();
    RtManagedArena *session = rt_managed_arena_create_child(server);
    prng_seed(777);

    RtHandle session_data[100];
    int session_count = 0;

    for (int req = 0; req < 100; req++) {
        /* Create request-child arena */
        RtManagedArena *request = rt_managed_arena_create_child(server);

        /* Allocate request data (headers, body strings) */
        int num_allocs = 5 + (int)prng_range(6); /* 5-10 allocs */
        RtHandle last_result = RT_HANDLE_NULL;

        for (int i = 0; i < num_allocs; i++) {
            RtHandle h = rt_managed_alloc(request, RT_HANDLE_NULL, 128);
            char *ptr = (char *)rt_managed_pin(request, h);
            snprintf(ptr, 128, "req%d-header%d: value%d", req, i, (int)prng_range(1000));
            rt_managed_unpin(request, h);
            last_result = h;
        }

        /* Promote 1 result string to session arena */
        if (last_result != RT_HANDLE_NULL && session_count < 100) {
            session_data[session_count] = rt_managed_promote(session, request, last_result);
            session_count++;
        }

        /* Destroy request child */
        rt_managed_arena_destroy_child(request);

        /* Every 20 requests, reset session (simulating session expiry) */
        if ((req + 1) % 20 == 0 && req < 99) {
            rt_managed_arena_reset(session);
            session_count = 0;
        }
    }

    /* Verify: session data correct for last batch */
    ASSERT(session_count > 0, "session has accumulated data");
    for (int i = 0; i < session_count; i++) {
        char *ptr = (char *)rt_managed_pin(session, session_data[i]);
        ASSERT(ptr != NULL, "session data accessible");
        ASSERT(strncmp(ptr, "req", 3) == 0, "session data has request prefix");
        rt_managed_unpin(session, session_data[i]);
    }

    /* Verify memory stays bounded - total shouldn't grow unboundedly */
    size_t total = rt_managed_total_allocated(server);
    ASSERT(total < 100 * 10 * 128 * 2, "memory bounded (not all requests retained)");

    rt_managed_arena_destroy(server);
}

/* ============================================================================
 * Profile 4: Recursive Tree Walk (Deep Nesting)
 * Goal: Simulate recursive AST processing with results bubbling up.
 * ============================================================================ */

static RtHandle recursive_tree_walk(RtManagedArena *parent, int depth, int max_depth)
{
    if (depth >= max_depth) {
        /* Leaf: allocate result directly in parent */
        RtHandle leaf = rt_managed_alloc(parent, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(parent, leaf);
        snprintf(ptr, 64, "leaf-%d", depth);
        rt_managed_unpin(parent, leaf);
        return leaf;
    }

    /* Create child arena for this level */
    RtManagedArena *child = rt_managed_arena_create_child(parent);

    /* Allocate intermediate entries at this level */
    for (int i = 0; i < 10; i++) {
        RtHandle tmp = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
        char *ptr = (char *)rt_managed_pin(child, tmp);
        snprintf(ptr, 32, "tmp-d%d-i%d", depth, i);
        rt_managed_unpin(child, tmp);
    }

    /* Recurse */
    RtHandle sub_result = recursive_tree_walk(child, depth + 1, max_depth);

    /* Promote result to parent */
    RtHandle promoted;
    if (depth + 1 >= max_depth) {
        /* sub_result was allocated directly in child (leaf case put it in parent=child) */
        promoted = rt_managed_promote(parent, child, sub_result);
    } else {
        /* sub_result is already in child (was promoted from deeper level) */
        promoted = rt_managed_promote(parent, child, sub_result);
    }

    /* Create our own result combining promoted data */
    RtHandle result = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
    char *ptr = (char *)rt_managed_pin(child, result);
    snprintf(ptr, 64, "node-%d", depth);
    rt_managed_unpin(child, result);

    RtHandle final_result = rt_managed_promote(parent, child, result);

    /* Destroy child - all intermediates die */
    rt_managed_arena_destroy_child(child);

    /* We return the node result; promoted sub_result stays alive in parent too */
    (void)promoted;
    return final_result;
}

static void test_profile_recursive_tree_walk(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    RtHandle result = recursive_tree_walk(root, 0, 20);
    ASSERT(result != RT_HANDLE_NULL, "recursive walk produced result");

    /* Verify the final result */
    char *ptr = (char *)rt_managed_pin(root, result);
    ASSERT(ptr != NULL, "final result accessible");
    ASSERT(strcmp(ptr, "node-0") == 0, "top-level node result correct");
    rt_managed_unpin(root, result);

    /* Root gets 2 entries: node-0 (final result) and node-1 (sub_result from depth 1) */
    size_t live = rt_managed_live_count(root);
    ASSERT(live == 2, "root has 2 promoted results (node-0 + node-1)");

    /* Let GC clean up any dead entries from promote operations */
    rt_managed_gc_flush(root);

    rt_managed_arena_destroy(root);
}

/* ============================================================================
 * Profile 5: Long-Running Event Loop with Periodic Reset
 * Goal: Simulate a game loop / event processor that resets periodically.
 * ============================================================================ */

static atomic_int event_cleanup_count;

static void event_cleanup_callback(void *data)
{
    (void)data;
    atomic_fetch_add(&event_cleanup_count, 1);
}

static void test_profile_event_loop_reset(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    atomic_store(&event_cleanup_count, 0);
    prng_seed(999);

    int expected_cleanups = 0;

    for (int tick = 0; tick < 10; tick++) {
        /* Allocate 50 event data entries */
        for (int e = 0; e < 50; e++) {
            RtHandle h = rt_managed_alloc(root, RT_HANDLE_NULL, 64);
            char *ptr = (char *)rt_managed_pin(root, h);
            snprintf(ptr, 64, "event-t%d-e%d-val%u", tick, e, prng_next());
            rt_managed_unpin(root, h);
        }

        /* Register 2 cleanup callbacks per tick */
        int cb_data_1 = tick * 2;
        int cb_data_2 = tick * 2 + 1;
        rt_managed_on_cleanup(root, (void *)(intptr_t)cb_data_1,
                              event_cleanup_callback, 50);
        rt_managed_on_cleanup(root, (void *)(intptr_t)cb_data_2,
                              event_cleanup_callback, 50);
        expected_cleanups += 2;

        /* Every 5 ticks: reset */
        if ((tick + 1) % 5 == 0) {
            rt_managed_arena_reset(root);

            /* After reset: all entries dead, cleanup fired */
            ASSERT_EQ(rt_managed_live_count(root), 0, "no live entries after reset");

            /* Wait for GC to reclaim */
            rt_managed_gc_flush(root);

            /* Verify cleanup count */
            int actual = atomic_load(&event_cleanup_count);
            ASSERT_EQ(actual, expected_cleanups,
                      "correct number of cleanups fired");
        }
    }

    /* Final state: cleanups should have all fired (2 resets x callbacks accumulated) */
    int final_cleanups = atomic_load(&event_cleanup_count);
    ASSERT_EQ(final_cleanups, expected_cleanups, "all cleanup callbacks fired");

    /* Re-allocate after final reset to verify arena is reusable */
    RtHandle h = rt_managed_strdup(root, RT_HANDLE_NULL, "post-reset-data");
    char *ptr = (char *)rt_managed_pin(root, h);
    ASSERT(strcmp(ptr, "post-reset-data") == 0, "arena reusable after resets");
    rt_managed_unpin(root, h);

    rt_managed_arena_destroy(root);
}

/* ============================================================================
 * Profile 6: Concurrent Multi-Arena Stress
 * Goal: Multiple threads operating on their own child arenas with promotion.
 * ============================================================================ */

typedef struct {
    RtManagedArena *root;
    RtManagedArena *child;
    int thread_id;
    int iterations;
    RtHandle *promoted;         /* Array to store promoted handles */
    int promoted_count;
    pthread_mutex_t *promote_mutex; /* Protects root promotion */
} WorkerArgs;

static void *worker_thread(void *arg)
{
    WorkerArgs *args = (WorkerArgs *)arg;
    RtManagedArena *child = args->child;
    RtHandle current = RT_HANDLE_NULL;

    for (int i = 0; i < args->iterations; i++) {
        /* Alloc in own child arena */
        current = rt_managed_alloc(child, current, 64);
        char *ptr = (char *)rt_managed_pin(child, current);
        snprintf(ptr, 64, "t%d-i%d", args->thread_id, i);
        rt_managed_unpin(child, current);

        /* Every 50 iterations: promote to root */
        if ((i + 1) % 50 == 0) {
            RtHandle to_promote = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
            ptr = (char *)rt_managed_pin(child, to_promote);
            snprintf(ptr, 64, "promoted-t%d-i%d", args->thread_id, i);
            rt_managed_unpin(child, to_promote);

            pthread_mutex_lock(args->promote_mutex);
            RtHandle ph = rt_managed_promote(args->root, child, to_promote);
            if (ph != RT_HANDLE_NULL && args->promoted_count < 100) {
                args->promoted[args->promoted_count++] = ph;
            }
            pthread_mutex_unlock(args->promote_mutex);
        }
    }

    return NULL;
}

static void test_profile_concurrent_multi_arena(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    pthread_mutex_t promote_mutex;
    pthread_mutex_init(&promote_mutex, NULL);

    #define NUM_WORKERS 4
    #define WORKER_ITERS 500

    RtManagedArena *children[NUM_WORKERS];
    pthread_t threads[NUM_WORKERS];
    WorkerArgs args[NUM_WORKERS];
    RtHandle promoted_handles[NUM_WORKERS][100];
    int promoted_counts[NUM_WORKERS];

    memset(promoted_counts, 0, sizeof(promoted_counts));

    /* Create child arenas and launch workers */
    for (int i = 0; i < NUM_WORKERS; i++) {
        children[i] = rt_managed_arena_create_child(root);
        args[i] = (WorkerArgs){
            .root = root,
            .child = children[i],
            .thread_id = i,
            .iterations = WORKER_ITERS,
            .promoted = promoted_handles[i],
            .promoted_count = 0,
            .promote_mutex = &promote_mutex
        };
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    /* Main thread: periodically trigger compaction on root */
    for (int c = 0; c < 5; c++) {
        usleep(50000);
        rt_managed_compact(root);
    }

    /* Wait for all workers */
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
        promoted_counts[i] = args[i].promoted_count;
    }

    /* Verify: promoted data intact in root */
    int total_promoted = 0;
    for (int i = 0; i < NUM_WORKERS; i++) {
        total_promoted += promoted_counts[i];
        for (int j = 0; j < promoted_counts[i]; j++) {
            char *ptr = (char *)rt_managed_pin(root, promoted_handles[i][j]);
            ASSERT(ptr != NULL, "promoted handle accessible after threads join");
            ASSERT(strncmp(ptr, "promoted-t", 10) == 0, "promoted data has correct prefix");
            rt_managed_unpin(root, promoted_handles[i][j]);
        }
    }

    /* Each worker promotes every 50 iterations: 500/50 = 10 per worker */
    ASSERT_EQ(total_promoted, NUM_WORKERS * (WORKER_ITERS / 50),
              "correct number of promotions");

    /* Verify root data correct after all operations */
    size_t root_live = rt_managed_live_count(root);
    ASSERT(root_live >= (size_t)total_promoted, "root has at least promoted entries");

    /* Destroy children */
    for (int i = 0; i < NUM_WORKERS; i++) {
        rt_managed_arena_destroy_child(children[i]);
    }

    pthread_mutex_destroy(&promote_mutex);
    rt_managed_arena_destroy(root);

    #undef NUM_WORKERS
    #undef WORKER_ITERS
}

/* ============================================================================
 * Profile 7: Compaction Benchmark (Direct timing of rt_managed_compact)
 * Goal: Measure compaction cost with large handle tables and many blocks.
 *       No GC flush (avoids sleep noise). Exercises the table-scan + block-walk
 *       path directly.
 * ============================================================================ */

static void test_profile_compaction_bench(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    #define COMPACT_ENTRIES 10000
    #define COMPACT_REASSIGN 8000
    #define COMPACT_ENTRY_SIZE 128

    RtHandle handles[COMPACT_ENTRIES];

    /* Phase 1: Allocate entries to fill multiple blocks.
     * 10000 entries x 128 bytes = 1.28MB → ~20 blocks at 64KB each */
    for (int i = 0; i < COMPACT_ENTRIES; i++) {
        handles[i] = rt_managed_alloc(root, RT_HANDLE_NULL, COMPACT_ENTRY_SIZE);
        char *ptr = (char *)rt_managed_pin(root, handles[i]);
        snprintf(ptr, COMPACT_ENTRY_SIZE, "entry-%05d-initial", i);
        rt_managed_unpin(root, handles[i]);
    }

    /* Phase 2: Reassign most entries (marks old as dead, allocates new).
     * Creates heavy fragmentation: dead entries scattered across old blocks,
     * new live entries in newer blocks. */
    for (int i = 0; i < COMPACT_REASSIGN; i++) {
        handles[i] = rt_managed_alloc(root, handles[i], COMPACT_ENTRY_SIZE);
        char *ptr = (char *)rt_managed_pin(root, handles[i]);
        snprintf(ptr, COMPACT_ENTRY_SIZE, "entry-%05d-updated", i);
        rt_managed_unpin(root, handles[i]);
    }

    /* Phase 3: Compact directly (this is what we're benchmarking).
     * Old code: 3 table scans, inner loop walks all blocks per dead entry.
     * New code: single pass, no inner block search. */
    double t0 = test_timer_now();
    rt_managed_compact(root);
    double compact_ms = (test_timer_now() - t0) * 1000.0;

    /* Print compaction time as a sub-metric */
    printf("\n    compact: %.3fms (%d entries, %d dead) ",
           compact_ms, COMPACT_ENTRIES + COMPACT_REASSIGN, COMPACT_REASSIGN);

    /* Verify: all live entries still accessible and correct */
    for (int i = 0; i < COMPACT_ENTRIES; i++) {
        char *ptr = (char *)rt_managed_pin(root, handles[i]);
        ASSERT(ptr != NULL, "live entry accessible after compaction");
        if (i < COMPACT_REASSIGN) {
            ASSERT(strncmp(ptr, "entry-", 6) == 0, "updated entry has correct prefix");
        } else {
            ASSERT(strncmp(ptr, "entry-", 6) == 0, "initial entry has correct prefix");
        }
        rt_managed_unpin(root, handles[i]);
    }

    rt_managed_arena_destroy(root);

    #undef COMPACT_ENTRIES
    #undef COMPACT_REASSIGN
    #undef COMPACT_ENTRY_SIZE
}

/* ============================================================================
 * Runner
 * ============================================================================ */

void test_stress_run(void)
{
    printf("\n-- Stress Profiles --\n");
    TEST_RUN("fragmentation storm (8 arenas x 200 handles)", test_profile_fragmentation_storm);
    TEST_RUN("mixed scope modes (50 function calls)", test_profile_mixed_scope_modes);
    TEST_RUN("web server (100 requests + session)", test_profile_web_server);
    TEST_RUN("recursive tree walk (depth 20)", test_profile_recursive_tree_walk);
    TEST_RUN("event loop with periodic reset", test_profile_event_loop_reset);
    TEST_RUN("concurrent multi-arena (4 threads x 500)", test_profile_concurrent_multi_arena);
    TEST_RUN("compaction bench (10k entries, 8k dead)", test_profile_compaction_bench);
}
