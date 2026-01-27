#include "test_framework.h"

/* ============================================================================
 * Race Detection Stress Tests
 * ============================================================================
 * These tests are designed to expose race conditions by:
 * - Using higher thread counts than normal tests
 * - Running operations for longer durations
 * - Mixing operations that stress synchronization boundaries
 * - Using barriers to maximize timing sensitivity
 * ============================================================================ */

/* Simple barrier implementation for coordinating thread start */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int threshold;
    int generation;
} Barrier;

static void barrier_init(Barrier *b, int count)
{
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->count = 0;
    b->threshold = count;
    b->generation = 0;
}

static void barrier_wait(Barrier *b)
{
    pthread_mutex_lock(&b->mutex);
    int gen = b->generation;
    b->count++;
    if (b->count >= b->threshold) {
        b->generation++;
        b->count = 0;
        pthread_cond_broadcast(&b->cond);
    } else {
        while (gen == b->generation) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }
    pthread_mutex_unlock(&b->mutex);
}

static void barrier_destroy(Barrier *b)
{
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

/* ============================================================================
 * Thread Scaling Stress Test
 * Goal: Run with progressively higher thread counts to expose races that
 *       only manifest under high concurrency.
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    int thread_id;
    int iterations;
    Barrier *start_barrier;
    atomic_bool *stop;
    atomic_int *error_count;
    int alloc_size;
} ScalingWorkerArgs;

static void *scaling_worker(void *arg)
{
    ScalingWorkerArgs *args = (ScalingWorkerArgs *)arg;
    RtManagedArena *arena = args->arena;
    int tid = args->thread_id;

    /* Wait for all threads to be ready */
    barrier_wait(args->start_barrier);

    RtHandle current = RT_HANDLE_NULL;
    char expected[128];

    for (int i = 0; i < args->iterations && !atomic_load(args->stop); i++) {
        /* Allocate with reassignment */
        current = rt_managed_alloc(arena, current, args->alloc_size);
        if (current == RT_HANDLE_NULL) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }

        /* Write data */
        char *ptr = (char *)rt_managed_pin(arena, current);
        if (ptr == NULL) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }
        snprintf(ptr, args->alloc_size, "t%d-i%d-magic%d", tid, i, tid * 1000 + i);
        rt_managed_unpin(arena, current);

        /* Verify data integrity by re-pinning */
        ptr = (char *)rt_managed_pin(arena, current);
        if (ptr == NULL) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }
        snprintf(expected, sizeof(expected), "t%d-i%d-magic%d", tid, i, tid * 1000 + i);
        if (strcmp(ptr, expected) != 0) {
            fprintf(stderr, "  DATA CORRUPTION: thread %d iter %d: expected '%s', got '%s'\n",
                    tid, i, expected, ptr);
            atomic_fetch_add(args->error_count, 1);
        }
        rt_managed_unpin(arena, current);
    }

    return NULL;
}

static void run_scaling_test(int num_threads, int iterations, const char *desc)
{
    RtManagedArena *arena = rt_managed_arena_create();
    Barrier barrier;
    atomic_bool stop = false;
    atomic_int error_count = 0;

    barrier_init(&barrier, num_threads);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ScalingWorkerArgs *args = malloc(num_threads * sizeof(ScalingWorkerArgs));

    for (int i = 0; i < num_threads; i++) {
        args[i] = (ScalingWorkerArgs){
            .arena = arena,
            .thread_id = i,
            .iterations = iterations,
            .start_barrier = &barrier,
            .stop = &stop,
            .error_count = &error_count,
            .alloc_size = 64
        };
        pthread_create(&threads[i], NULL, scaling_worker, &args[i]);
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    int errors = atomic_load(&error_count);

    barrier_destroy(&barrier);
    free(threads);
    free(args);
    rt_managed_arena_destroy(arena);

    ASSERT_EQ(errors, 0, desc);
}

static void test_scaling_2_threads(void)
{
    run_scaling_test(2, 2000, "2 threads: no data corruption");
}

static void test_scaling_4_threads(void)
{
    run_scaling_test(4, 2000, "4 threads: no data corruption");
}

static void test_scaling_8_threads(void)
{
    run_scaling_test(8, 1000, "8 threads: no data corruption");
}

static void test_scaling_16_threads(void)
{
    run_scaling_test(16, 500, "16 threads: no data corruption");
}

static void test_scaling_32_threads(void)
{
    run_scaling_test(32, 250, "32 threads: no data corruption");
}

/* ============================================================================
 * Mixed Operations Scaling
 * Goal: Each thread performs a mix of operations (alloc, pin, unpin, strdup)
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    int thread_id;
    int iterations;
    Barrier *start_barrier;
    atomic_int *error_count;
} MixedOpsArgs;

static void *mixed_ops_worker(void *arg)
{
    MixedOpsArgs *args = (MixedOpsArgs *)arg;
    RtManagedArena *arena = args->arena;
    int tid = args->thread_id;

    barrier_wait(args->start_barrier);

    RtHandle handles[10] = {0};
    char buf[128];

    for (int i = 0; i < args->iterations; i++) {
        int op = i % 5;
        int slot = i % 10;

        switch (op) {
        case 0: /* Alloc new */
            handles[slot] = rt_managed_alloc(arena, handles[slot], 64);
            if (handles[slot] != RT_HANDLE_NULL) {
                char *ptr = (char *)rt_managed_pin(arena, handles[slot]);
                if (ptr) {
                    snprintf(ptr, 64, "t%d-slot%d-i%d", tid, slot, i);
                    rt_managed_unpin(arena, handles[slot]);
                }
            }
            break;

        case 1: /* Strdup */
            snprintf(buf, sizeof(buf), "strdup-t%d-i%d", tid, i);
            handles[slot] = rt_managed_strdup(arena, handles[slot], buf);
            break;

        case 2: /* Pin and verify */
            if (handles[slot] != RT_HANDLE_NULL) {
                char *ptr = (char *)rt_managed_pin(arena, handles[slot]);
                if (ptr) {
                    /* Just read to verify no crash */
                    volatile char c = ptr[0];
                    (void)c;
                    rt_managed_unpin(arena, handles[slot]);
                }
            }
            break;

        case 3: /* Double pin/unpin */
            if (handles[slot] != RT_HANDLE_NULL) {
                char *ptr1 = (char *)rt_managed_pin(arena, handles[slot]);
                char *ptr2 = (char *)rt_managed_pin(arena, handles[slot]);
                if (ptr1 && ptr2 && ptr1 != ptr2) {
                    /* Double pin should return same pointer */
                    atomic_fetch_add(args->error_count, 1);
                }
                if (ptr2) rt_managed_unpin(arena, handles[slot]);
                if (ptr1) rt_managed_unpin(arena, handles[slot]);
            }
            break;

        case 4: /* Reassign with size change */
            handles[slot] = rt_managed_alloc(arena, handles[slot], 32 + (i % 64));
            break;
        }
    }

    return NULL;
}

static void test_mixed_ops_scaling(void)
{
    int num_threads = 8;
    int iterations = 1000;

    RtManagedArena *arena = rt_managed_arena_create();
    Barrier barrier;
    atomic_int error_count = 0;

    barrier_init(&barrier, num_threads);

    pthread_t threads[8];
    MixedOpsArgs args[8];

    for (int i = 0; i < num_threads; i++) {
        args[i] = (MixedOpsArgs){
            .arena = arena,
            .thread_id = i,
            .iterations = iterations,
            .start_barrier = &barrier,
            .error_count = &error_count
        };
        pthread_create(&threads[i], NULL, mixed_ops_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&barrier);
    rt_managed_arena_destroy(arena);

    ASSERT_EQ(atomic_load(&error_count), 0, "mixed ops: no errors");
}

/* ============================================================================
 * Rapid Arena Lifecycle Churn
 * Goal: Stress the gc_processing/destroying coordination by rapidly
 *       creating and destroying child arenas while GC is running.
 * ============================================================================ */

typedef struct {
    RtManagedArena *root;
    int thread_id;
    int iterations;
    Barrier *start_barrier;
    atomic_int *error_count;
    atomic_bool *stop;
} LifecycleArgs;

static void *lifecycle_worker(void *arg)
{
    LifecycleArgs *args = (LifecycleArgs *)arg;
    int tid = args->thread_id;

    barrier_wait(args->start_barrier);

    for (int i = 0; i < args->iterations && !atomic_load(args->stop); i++) {
        /* Create child arena */
        RtManagedArena *child = rt_managed_arena_create_child(args->root);
        if (child == NULL) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }

        /* Allocate some data */
        RtHandle h = RT_HANDLE_NULL;
        for (int j = 0; j < 10; j++) {
            h = rt_managed_alloc(child, h, 64);
            if (h != RT_HANDLE_NULL) {
                char *ptr = (char *)rt_managed_pin(child, h);
                if (ptr) {
                    snprintf(ptr, 64, "t%d-i%d-j%d", tid, i, j);
                    rt_managed_unpin(child, h);
                }
            }
        }

        /* Destroy child - this must coordinate with GC */
        rt_managed_arena_destroy_child(child);
    }

    return NULL;
}

static void test_rapid_lifecycle_4_threads(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    Barrier barrier;
    atomic_int error_count = 0;
    atomic_bool stop = false;

    int num_threads = 4;
    int iterations = 200;

    barrier_init(&barrier, num_threads);

    pthread_t threads[4];
    LifecycleArgs args[4];

    for (int i = 0; i < num_threads; i++) {
        args[i] = (LifecycleArgs){
            .root = root,
            .thread_id = i,
            .iterations = iterations,
            .start_barrier = &barrier,
            .error_count = &error_count,
            .stop = &stop
        };
        pthread_create(&threads[i], NULL, lifecycle_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&barrier);
    rt_managed_arena_destroy(root);

    ASSERT_EQ(atomic_load(&error_count), 0, "rapid lifecycle: no errors");
}

static void test_rapid_lifecycle_8_threads(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    Barrier barrier;
    atomic_int error_count = 0;
    atomic_bool stop = false;

    int num_threads = 8;
    int iterations = 100;

    barrier_init(&barrier, num_threads);

    pthread_t threads[8];
    LifecycleArgs args[8];

    for (int i = 0; i < num_threads; i++) {
        args[i] = (LifecycleArgs){
            .root = root,
            .thread_id = i,
            .iterations = iterations,
            .start_barrier = &barrier,
            .error_count = &error_count,
            .stop = &stop
        };
        pthread_create(&threads[i], NULL, lifecycle_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    barrier_destroy(&barrier);
    rt_managed_arena_destroy(root);

    ASSERT_EQ(atomic_load(&error_count), 0, "rapid lifecycle 8t: no errors");
}

/* ============================================================================
 * Compaction During Allocation Storm
 * Goal: Main thread triggers compaction while workers allocate aggressively
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    int thread_id;
    atomic_bool *stop;
    atomic_int *alloc_count;
    atomic_int *error_count;
} CompactStormArgs;

static void *compact_storm_worker(void *arg)
{
    CompactStormArgs *args = (CompactStormArgs *)arg;
    RtManagedArena *arena = args->arena;
    int tid = args->thread_id;

    RtHandle current = RT_HANDLE_NULL;
    int local_count = 0;

    while (!atomic_load(args->stop)) {
        current = rt_managed_alloc(arena, current, 128);
        if (current == RT_HANDLE_NULL) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }

        char *ptr = (char *)rt_managed_pin(arena, current);
        if (ptr) {
            snprintf(ptr, 128, "storm-t%d-n%d", tid, local_count);
            rt_managed_unpin(arena, current);
            local_count++;
        }
    }

    atomic_fetch_add(args->alloc_count, local_count);
    return NULL;
}

static void test_compaction_during_storm(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int alloc_count = 0;
    atomic_int error_count = 0;

    int num_workers = 4;
    pthread_t workers[4];
    CompactStormArgs args[4];

    /* Start allocation storm */
    for (int i = 0; i < num_workers; i++) {
        args[i] = (CompactStormArgs){
            .arena = arena,
            .thread_id = i,
            .stop = &stop,
            .alloc_count = &alloc_count,
            .error_count = &error_count
        };
        pthread_create(&workers[i], NULL, compact_storm_worker, &args[i]);
    }

    /* Main thread triggers compaction repeatedly */
    for (int c = 0; c < 20; c++) {
        usleep(10000);  /* 10ms between compactions */
        rt_managed_compact(arena);
    }

    /* Stop workers */
    atomic_store(&stop, true);
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    int total_allocs = atomic_load(&alloc_count);
    int errors = atomic_load(&error_count);

    TEST_STATS("%d allocs, %d errors", total_allocs, errors);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(errors, 0, "compact storm: no errors");
    ASSERT(total_allocs > 1000, "compact storm: sufficient allocations occurred");
}

/* ============================================================================
 * Promotion Contention
 * Goal: Multiple threads promoting to the same parent arena
 * ============================================================================ */

typedef struct {
    RtManagedArena *parent;
    int thread_id;
    int iterations;
    Barrier *start_barrier;
    RtHandle *results;
    int *result_count;
    pthread_mutex_t *result_mutex;
    atomic_int *error_count;
} PromoteContentionArgs;

static void *promote_contention_worker(void *arg)
{
    PromoteContentionArgs *args = (PromoteContentionArgs *)arg;
    int tid = args->thread_id;

    barrier_wait(args->start_barrier);

    for (int i = 0; i < args->iterations; i++) {
        /* Create ephemeral child */
        RtManagedArena *child = rt_managed_arena_create_child(args->parent);
        if (!child) {
            atomic_fetch_add(args->error_count, 1);
            continue;
        }

        /* Allocate in child */
        RtHandle ch = rt_managed_alloc(child, RT_HANDLE_NULL, 64);
        if (ch == RT_HANDLE_NULL) {
            rt_managed_arena_destroy_child(child);
            continue;
        }

        char *ptr = (char *)rt_managed_pin(child, ch);
        if (ptr) {
            snprintf(ptr, 64, "promoted-t%d-i%d", tid, i);
            rt_managed_unpin(child, ch);
        }

        /* Promote to parent (contended operation) */
        RtHandle promoted = rt_managed_promote(args->parent, child, ch);

        /* Track result */
        if (promoted != RT_HANDLE_NULL) {
            pthread_mutex_lock(args->result_mutex);
            if (*args->result_count < 1000) {
                args->results[(*args->result_count)++] = promoted;
            }
            pthread_mutex_unlock(args->result_mutex);
        }

        rt_managed_arena_destroy_child(child);
    }

    return NULL;
}

static void test_promotion_contention(void)
{
    RtManagedArena *parent = rt_managed_arena_create();
    Barrier barrier;
    atomic_int error_count = 0;
    pthread_mutex_t result_mutex;

    int num_threads = 8;
    int iterations = 100;

    RtHandle results[1000];
    int result_count = 0;

    barrier_init(&barrier, num_threads);
    pthread_mutex_init(&result_mutex, NULL);

    pthread_t threads[8];
    PromoteContentionArgs args[8];

    for (int i = 0; i < num_threads; i++) {
        args[i] = (PromoteContentionArgs){
            .parent = parent,
            .thread_id = i,
            .iterations = iterations,
            .start_barrier = &barrier,
            .results = results,
            .result_count = &result_count,
            .result_mutex = &result_mutex,
            .error_count = &error_count
        };
        pthread_create(&threads[i], NULL, promote_contention_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Verify all promoted handles are valid */
    int valid_count = 0;
    for (int i = 0; i < result_count; i++) {
        char *ptr = (char *)rt_managed_pin(parent, results[i]);
        if (ptr && strncmp(ptr, "promoted-t", 10) == 0) {
            valid_count++;
        }
        if (ptr) rt_managed_unpin(parent, results[i]);
    }

    TEST_STATS("%d/%d promoted handles valid", valid_count, result_count);

    barrier_destroy(&barrier);
    pthread_mutex_destroy(&result_mutex);
    rt_managed_arena_destroy(parent);

    ASSERT_EQ(atomic_load(&error_count), 0, "promotion contention: no errors");
    ASSERT_EQ(valid_count, result_count, "promotion contention: all promoted handles valid");
}

/* ============================================================================
 * Handle Table Growth Race
 * Goal: Force the paged handle table to grow while threads are reading
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    RtHandle *handles;
    int handle_count;
    atomic_bool *stop;
    atomic_int *error_count;
} TableGrowthReaderArgs;

static void *table_growth_reader(void *arg)
{
    TableGrowthReaderArgs *args = (TableGrowthReaderArgs *)arg;

    while (!atomic_load(args->stop)) {
        /* Randomly read handles that exist */
        for (int i = 0; i < args->handle_count && !atomic_load(args->stop); i++) {
            RtHandle h = args->handles[i];
            if (h == RT_HANDLE_NULL) continue;

            char *ptr = (char *)rt_managed_pin(args->arena, h);
            if (ptr) {
                volatile char c = ptr[0];
                (void)c;
                rt_managed_unpin(args->arena, h);
            }
        }
    }

    return NULL;
}

static void test_handle_table_growth_race(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int error_count = 0;

    /* Pre-allocate some handles */
    #define INITIAL_HANDLES 100
    #define GROWTH_HANDLES 2000  /* Enough to force multiple page additions */

    RtHandle handles[GROWTH_HANDLES];
    for (int i = 0; i < INITIAL_HANDLES; i++) {
        handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 32);
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        if (ptr) {
            snprintf(ptr, 32, "initial-%d", i);
            rt_managed_unpin(arena, handles[i]);
        }
    }

    /* Start reader threads */
    int num_readers = 4;
    pthread_t readers[4];
    TableGrowthReaderArgs reader_args[4];

    for (int i = 0; i < num_readers; i++) {
        reader_args[i] = (TableGrowthReaderArgs){
            .arena = arena,
            .handles = handles,
            .handle_count = INITIAL_HANDLES,
            .stop = &stop,
            .error_count = &error_count
        };
        pthread_create(&readers[i], NULL, table_growth_reader, &reader_args[i]);
    }

    /* Main thread forces table growth by allocating many more handles */
    for (int i = INITIAL_HANDLES; i < GROWTH_HANDLES; i++) {
        handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 32);
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        if (ptr) {
            snprintf(ptr, 32, "growth-%d", i);
            rt_managed_unpin(arena, handles[i]);
        }
    }

    /* Let readers run a bit more */
    usleep(50000);

    /* Stop readers */
    atomic_store(&stop, true);
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    /* Verify all handles still valid */
    int valid = 0;
    for (int i = 0; i < GROWTH_HANDLES; i++) {
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        if (ptr) {
            valid++;
            rt_managed_unpin(arena, handles[i]);
        }
    }

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(valid, GROWTH_HANDLES, "all handles valid after table growth");
    ASSERT_EQ(atomic_load(&error_count), 0, "table growth: no reader errors");

    #undef INITIAL_HANDLES
    #undef GROWTH_HANDLES
}

/* ============================================================================
 * Long-Running Stability Test
 * Goal: Run mixed operations for extended duration to catch rare races
 * ============================================================================ */

typedef struct {
    RtManagedArena *root;
    int thread_id;
    atomic_bool *stop;
    atomic_int *op_count;
    atomic_int *error_count;
} StabilityArgs;

static void *stability_worker(void *arg)
{
    StabilityArgs *args = (StabilityArgs *)arg;
    int tid = args->thread_id;
    int local_ops = 0;

    /* Each thread maintains its own child arena */
    RtManagedArena *my_arena = rt_managed_arena_create_child(args->root);
    RtHandle handles[20] = {0};

    while (!atomic_load(args->stop)) {
        int op = local_ops % 10;
        int slot = local_ops % 20;

        switch (op) {
        case 0: case 1: case 2: /* Alloc in own arena */
            handles[slot] = rt_managed_alloc(my_arena, handles[slot], 64);
            if (handles[slot] != RT_HANDLE_NULL) {
                char *ptr = (char *)rt_managed_pin(my_arena, handles[slot]);
                if (ptr) {
                    snprintf(ptr, 64, "stab-t%d-op%d", tid, local_ops);
                    rt_managed_unpin(my_arena, handles[slot]);
                }
            }
            break;

        case 3: /* Promote to root */
            if (handles[slot] != RT_HANDLE_NULL) {
                RtHandle promoted = rt_managed_promote(args->root, my_arena, handles[slot]);
                (void)promoted;  /* Don't track, let GC reclaim */
                handles[slot] = RT_HANDLE_NULL;
            }
            break;

        case 4: /* Pin and verify */
            if (handles[slot] != RT_HANDLE_NULL) {
                char *ptr = (char *)rt_managed_pin(my_arena, handles[slot]);
                if (ptr) {
                    if (strncmp(ptr, "stab-t", 6) != 0) {
                        atomic_fetch_add(args->error_count, 1);
                    }
                    rt_managed_unpin(my_arena, handles[slot]);
                }
            }
            break;

        case 5: /* Strdup */
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "str-t%d-op%d", tid, local_ops);
                handles[slot] = rt_managed_strdup(my_arena, handles[slot], buf);
            }
            break;

        case 6: case 7: /* More allocs */
            handles[slot] = rt_managed_alloc(my_arena, handles[slot], 32 + (local_ops % 96));
            break;

        case 8: /* Create and immediately destroy nested child */
            {
                RtManagedArena *temp = rt_managed_arena_create_child(my_arena);
                RtHandle th = rt_managed_alloc(temp, RT_HANDLE_NULL, 32);
                (void)th;
                rt_managed_arena_destroy_child(temp);
            }
            break;

        case 9: /* Mark dead explicitly */
            if (handles[slot] != RT_HANDLE_NULL) {
                rt_managed_mark_dead(my_arena, handles[slot]);
                handles[slot] = RT_HANDLE_NULL;
            }
            break;
        }

        local_ops++;
    }

    rt_managed_arena_destroy_child(my_arena);
    atomic_fetch_add(args->op_count, local_ops);

    return NULL;
}

static void test_long_running_stability(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int op_count = 0;
    atomic_int error_count = 0;

    int num_threads = 8;
    pthread_t threads[8];
    StabilityArgs args[8];

    for (int i = 0; i < num_threads; i++) {
        args[i] = (StabilityArgs){
            .root = root,
            .thread_id = i,
            .stop = &stop,
            .op_count = &op_count,
            .error_count = &error_count
        };
        pthread_create(&threads[i], NULL, stability_worker, &args[i]);
    }

    /* Run for 500ms */
    usleep(500000);

    atomic_store(&stop, true);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    int total_ops = atomic_load(&op_count);
    int errors = atomic_load(&error_count);

    TEST_STATS("%d ops, %d errors", total_ops, errors);

    rt_managed_arena_destroy(root);

    ASSERT_EQ(errors, 0, "stability: no data corruption");
    ASSERT(total_ops > 10000, "stability: sufficient operations performed");
}

/* ============================================================================
 * Pin Duration Variance
 * Goal: Mix long-held pins with rapid pin/unpin to stress compactor skip logic
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    RtHandle *handles;
    int handle_count;
    int thread_id;
    atomic_bool *stop;
    atomic_int *error_count;
    int hold_time_ms;  /* 0 = rapid, >0 = hold for this duration */
} PinVarianceArgs;

static void *pin_variance_worker(void *arg)
{
    PinVarianceArgs *args = (PinVarianceArgs *)arg;
    int tid = args->thread_id;

    while (!atomic_load(args->stop)) {
        for (int i = 0; i < args->handle_count && !atomic_load(args->stop); i++) {
            RtHandle h = args->handles[i];
            if (h == RT_HANDLE_NULL) continue;

            char *ptr = (char *)rt_managed_pin(args->arena, h);
            if (ptr == NULL) continue;

            /* Verify data starts with expected prefix */
            if (ptr[0] != 'v') {
                atomic_fetch_add(args->error_count, 1);
            }

            if (args->hold_time_ms > 0) {
                /* Long hold - simulates I/O or processing */
                usleep(args->hold_time_ms * 1000);
            }
            /* else: rapid pin/unpin */

            rt_managed_unpin(args->arena, h);
        }
    }

    (void)tid;
    return NULL;
}

static void test_pin_duration_variance(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int error_count = 0;

    #define PIN_VAR_HANDLES 50

    RtHandle handles[PIN_VAR_HANDLES];
    for (int i = 0; i < PIN_VAR_HANDLES; i++) {
        handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        snprintf(ptr, 64, "val-%d", i);
        rt_managed_unpin(arena, handles[i]);
    }

    /* 2 threads with long holds, 4 threads with rapid pin/unpin */
    pthread_t threads[6];
    PinVarianceArgs args[6];

    for (int i = 0; i < 6; i++) {
        args[i] = (PinVarianceArgs){
            .arena = arena,
            .handles = handles,
            .handle_count = PIN_VAR_HANDLES,
            .thread_id = i,
            .stop = &stop,
            .error_count = &error_count,
            .hold_time_ms = (i < 2) ? 10 : 0  /* First 2 hold for 10ms */
        };
        pthread_create(&threads[i], NULL, pin_variance_worker, &args[i]);
    }

    /* Main thread triggers compaction repeatedly */
    for (int c = 0; c < 30; c++) {
        usleep(20000);
        rt_managed_compact(arena);
    }

    atomic_store(&stop, true);
    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Verify all data intact */
    int valid = 0;
    for (int i = 0; i < PIN_VAR_HANDLES; i++) {
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        if (ptr && ptr[0] == 'v') valid++;
        if (ptr) rt_managed_unpin(arena, handles[i]);
    }

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(atomic_load(&error_count), 0, "pin variance: no data corruption");
    ASSERT_EQ(valid, PIN_VAR_HANDLES, "pin variance: all handles valid");

    #undef PIN_VAR_HANDLES
}

/* ============================================================================
 * Deep Hierarchy Stress
 * Goal: 20+ levels of nested arenas with concurrent operations
 * ============================================================================ */

typedef struct {
    RtManagedArena **arenas;
    int depth;
    atomic_bool *stop;
    atomic_int *error_count;
    int thread_id;
} DeepHierarchyArgs;

static void *deep_hierarchy_worker(void *arg)
{
    DeepHierarchyArgs *args = (DeepHierarchyArgs *)arg;
    int tid = args->thread_id;
    int ops = 0;

    while (!atomic_load(args->stop)) {
        /* Pick a random level and do operations there */
        int level = ops % args->depth;
        RtManagedArena *arena = args->arenas[level];

        /* Allocate */
        RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, 32);
        if (h != RT_HANDLE_NULL) {
            char *ptr = (char *)rt_managed_pin(arena, h);
            if (ptr) {
                snprintf(ptr, 32, "t%d-L%d-op%d", tid, level, ops);
                rt_managed_unpin(arena, h);
            }

            /* Sometimes promote up one level */
            if (level > 0 && (ops % 5) == 0) {
                RtManagedArena *parent = args->arenas[level - 1];
                RtHandle promoted = rt_managed_promote(parent, arena, h);
                (void)promoted;
            }

            /* Mark dead */
            rt_managed_mark_dead(arena, h);
        }

        ops++;
    }

    return NULL;
}

static void test_deep_hierarchy_stress(void)
{
    #define DEEP_LEVELS 20
    #define DEEP_THREADS 4

    RtManagedArena *arenas[DEEP_LEVELS];
    arenas[0] = rt_managed_arena_create();

    for (int i = 1; i < DEEP_LEVELS; i++) {
        arenas[i] = rt_managed_arena_create_child(arenas[i - 1]);
        ASSERT(arenas[i] != NULL, "deep hierarchy: child creation succeeded");
    }

    atomic_bool stop = false;
    atomic_int error_count = 0;

    pthread_t threads[DEEP_THREADS];
    DeepHierarchyArgs args[DEEP_THREADS];

    for (int i = 0; i < DEEP_THREADS; i++) {
        args[i] = (DeepHierarchyArgs){
            .arenas = arenas,
            .depth = DEEP_LEVELS,
            .stop = &stop,
            .error_count = &error_count,
            .thread_id = i
        };
        pthread_create(&threads[i], NULL, deep_hierarchy_worker, &args[i]);
    }

    /* Let it run */
    usleep(300000);

    atomic_store(&stop, true);
    for (int i = 0; i < DEEP_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Destroy from deepest to root (normal pattern) */
    for (int i = DEEP_LEVELS - 1; i >= 1; i--) {
        rt_managed_arena_destroy_child(arenas[i]);
    }
    rt_managed_arena_destroy(arenas[0]);

    ASSERT_EQ(atomic_load(&error_count), 0, "deep hierarchy: no errors");

    #undef DEEP_LEVELS
    #undef DEEP_THREADS
}

/* ============================================================================
 * Concurrent Destroy + Promote Race
 * Goal: Race between destroying a child and promoting from it
 * ============================================================================ */

typedef struct {
    RtManagedArena *parent;
    RtManagedArena **child_ptr;
    pthread_mutex_t *child_mutex;
    atomic_bool *stop;
    atomic_int *promote_count;
    atomic_int *destroy_count;
    int is_destroyer;
} DestroyPromoteArgs;

static void *destroy_promote_worker(void *arg)
{
    DestroyPromoteArgs *args = (DestroyPromoteArgs *)arg;

    while (!atomic_load(args->stop)) {
        pthread_mutex_lock(args->child_mutex);
        RtManagedArena *child = *args->child_ptr;

        if (args->is_destroyer) {
            if (child != NULL) {
                *args->child_ptr = NULL;
                pthread_mutex_unlock(args->child_mutex);

                rt_managed_arena_destroy_child(child);
                atomic_fetch_add(args->destroy_count, 1);

                /* Recreate for next round */
                pthread_mutex_lock(args->child_mutex);
                if (*args->child_ptr == NULL) {
                    *args->child_ptr = rt_managed_arena_create_child(args->parent);
                }
                pthread_mutex_unlock(args->child_mutex);
            } else {
                pthread_mutex_unlock(args->child_mutex);
            }
        } else {
            /* Promoter */
            if (child != NULL) {
                /* Allocate in child */
                RtHandle h = rt_managed_alloc(child, RT_HANDLE_NULL, 32);
                if (h != RT_HANDLE_NULL) {
                    char *ptr = (char *)rt_managed_pin(child, h);
                    if (ptr) {
                        strcpy(ptr, "promote-me");
                        rt_managed_unpin(child, h);
                    }

                    /* Try to promote - may fail if child is being destroyed */
                    RtHandle promoted = rt_managed_promote(args->parent, child, h);
                    if (promoted != RT_HANDLE_NULL) {
                        atomic_fetch_add(args->promote_count, 1);
                    }
                }
            }
            pthread_mutex_unlock(args->child_mutex);
        }

        usleep(100);  /* Small delay to increase interleaving */
    }

    return NULL;
}

static void test_concurrent_destroy_promote(void)
{
    RtManagedArena *parent = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(parent);
    pthread_mutex_t child_mutex;
    pthread_mutex_init(&child_mutex, NULL);

    atomic_bool stop = false;
    atomic_int promote_count = 0;
    atomic_int destroy_count = 0;

    /* 1 destroyer, 3 promoters */
    pthread_t threads[4];
    DestroyPromoteArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i] = (DestroyPromoteArgs){
            .parent = parent,
            .child_ptr = &child,
            .child_mutex = &child_mutex,
            .stop = &stop,
            .promote_count = &promote_count,
            .destroy_count = &destroy_count,
            .is_destroyer = (i == 0)
        };
        pthread_create(&threads[i], NULL, destroy_promote_worker, &args[i]);
    }

    usleep(300000);

    atomic_store(&stop, true);
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    int promotes = atomic_load(&promote_count);
    int destroys = atomic_load(&destroy_count);

    TEST_STATS("%d promotes, %d destroys", promotes, destroys);

    /* Cleanup */
    pthread_mutex_lock(&child_mutex);
    if (child != NULL) {
        rt_managed_arena_destroy_child(child);
    }
    pthread_mutex_unlock(&child_mutex);

    pthread_mutex_destroy(&child_mutex);
    rt_managed_arena_destroy(parent);

    /* If we got here without crashing, the test passed */
    ASSERT(destroys > 0, "destroy/promote: some destroys occurred");
}

/* ============================================================================
 * Reset Under Active Use
 * Goal: Call reset while other threads are pinning/allocating
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    atomic_bool *stop;
    atomic_int *error_count;
    atomic_int *op_count;
} ResetStressArgs;

static void *reset_stress_worker(void *arg)
{
    ResetStressArgs *args = (ResetStressArgs *)arg;
    RtHandle current = RT_HANDLE_NULL;

    while (!atomic_load(args->stop)) {
        /* Try to allocate - may get null handle after reset */
        RtHandle h = rt_managed_alloc(args->arena, current, 64);
        if (h != RT_HANDLE_NULL) {
            char *ptr = (char *)rt_managed_pin(args->arena, h);
            if (ptr) {
                strcpy(ptr, "data");
                rt_managed_unpin(args->arena, h);
                current = h;
            }
        } else {
            current = RT_HANDLE_NULL;
        }
        atomic_fetch_add(args->op_count, 1);
    }

    return NULL;
}

static void test_reset_under_load(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int error_count = 0;
    atomic_int op_count = 0;

    pthread_t threads[4];
    ResetStressArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i] = (ResetStressArgs){
            .arena = arena,
            .stop = &stop,
            .error_count = &error_count,
            .op_count = &op_count
        };
        pthread_create(&threads[i], NULL, reset_stress_worker, &args[i]);
    }

    /* Main thread calls reset repeatedly */
    for (int r = 0; r < 20; r++) {
        usleep(15000);
        rt_managed_arena_reset(arena);
    }

    atomic_store(&stop, true);
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    int ops = atomic_load(&op_count);
    TEST_STATS("%d ops", ops);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(atomic_load(&error_count), 0, "reset stress: no errors");
    ASSERT(ops > 1000, "reset stress: sufficient operations");
}

/* ============================================================================
 * Pinned Allocation Stress
 * Goal: Stress rt_managed_alloc_pinned which creates permanently pinned entries
 * ============================================================================ */

static void test_pinned_allocation_stress(void)
{
    RtManagedArena *arena = rt_managed_arena_create();

    #define PINNED_COUNT 100

    RtHandle pinned_handles[PINNED_COUNT];
    RtHandle normal_handles[PINNED_COUNT];

    /* Interleave pinned and normal allocations */
    for (int i = 0; i < PINNED_COUNT; i++) {
        pinned_handles[i] = rt_managed_alloc_pinned(arena, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(arena, pinned_handles[i]);
        snprintf(ptr, 64, "pinned-%d", i);
        rt_managed_unpin(arena, pinned_handles[i]);

        normal_handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 64);
        ptr = (char *)rt_managed_pin(arena, normal_handles[i]);
        snprintf(ptr, 64, "normal-%d", i);
        rt_managed_unpin(arena, normal_handles[i]);
    }

    /* Get pointers to pinned entries (they should never move) */
    void *pinned_ptrs[PINNED_COUNT];
    for (int i = 0; i < PINNED_COUNT; i++) {
        pinned_ptrs[i] = rt_managed_pin(arena, pinned_handles[i]);
    }

    /* Trigger compaction multiple times */
    for (int c = 0; c < 10; c++) {
        /* Mark normal handles dead to create fragmentation */
        for (int i = 0; i < PINNED_COUNT; i++) {
            normal_handles[i] = rt_managed_alloc(arena, normal_handles[i], 64);
        }
        rt_managed_compact(arena);
    }

    /* Verify pinned entries haven't moved */
    int moved = 0;
    for (int i = 0; i < PINNED_COUNT; i++) {
        void *current_ptr = rt_managed_pin(arena, pinned_handles[i]);
        if (current_ptr != pinned_ptrs[i]) {
            moved++;
        }
        /* Verify data intact */
        char expected[64];
        snprintf(expected, sizeof(expected), "pinned-%d", i);
        if (current_ptr && strcmp((char *)current_ptr, expected) != 0) {
            moved++;  /* Count as error */
        }
        rt_managed_unpin(arena, pinned_handles[i]);
    }

    /* Unpin original pins */
    for (int i = 0; i < PINNED_COUNT; i++) {
        rt_managed_unpin(arena, pinned_handles[i]);
    }

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(moved, 0, "pinned alloc: no pinned entries moved");

    #undef PINNED_COUNT
}

/* ============================================================================
 * Free List Recycling Stress
 * Goal: Maximize handle recycling through the free list
 * ============================================================================ */

static void test_free_list_recycling_stress(void)
{
    RtManagedArena *arena = rt_managed_arena_create();

    #define RECYCLE_ITERS 1000
    #define RECYCLE_BATCH 50

    RtHandle handles[RECYCLE_BATCH];

    for (int iter = 0; iter < RECYCLE_ITERS; iter++) {
        /* Allocate a batch */
        for (int i = 0; i < RECYCLE_BATCH; i++) {
            handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 32);
            char *ptr = (char *)rt_managed_pin(arena, handles[i]);
            if (ptr) {
                snprintf(ptr, 32, "iter%d-h%d", iter, i);
                rt_managed_unpin(arena, handles[i]);
            }
        }

        /* Mark all dead */
        for (int i = 0; i < RECYCLE_BATCH; i++) {
            rt_managed_mark_dead(arena, handles[i]);
        }

        /* Flush GC to recycle handles */
        if (iter % 10 == 0) {
            rt_managed_gc_flush(arena);
        }
    }

    /* Final flush */
    rt_managed_gc_flush(arena);

    /* Verify arena is mostly empty now */
    size_t live = rt_managed_live_count(arena);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(live, 0, "free list recycling: all handles recycled");

    #undef RECYCLE_ITERS
    #undef RECYCLE_BATCH
}

/* ============================================================================
 * Epoch Invalidation Storm
 * Goal: Stress the fast-path epoch check by rapid compaction during allocation
 * ============================================================================ */

typedef struct {
    RtManagedArena *arena;
    atomic_bool *stop;
    atomic_int *alloc_count;
    atomic_int *error_count;
} EpochStormArgs;

static void *epoch_storm_allocator(void *arg)
{
    EpochStormArgs *args = (EpochStormArgs *)arg;
    RtHandle current = RT_HANDLE_NULL;
    int local_allocs = 0;

    while (!atomic_load(args->stop)) {
        current = rt_managed_alloc(args->arena, current, 128);
        if (current != RT_HANDLE_NULL) {
            char *ptr = (char *)rt_managed_pin(args->arena, current);
            if (ptr) {
                snprintf(ptr, 128, "epoch-test-%d", local_allocs);

                /* Immediately verify */
                if (strncmp(ptr, "epoch-test-", 11) != 0) {
                    atomic_fetch_add(args->error_count, 1);
                }
                rt_managed_unpin(args->arena, current);
            }
            local_allocs++;
        }
    }

    atomic_fetch_add(args->alloc_count, local_allocs);
    return NULL;
}

static void test_epoch_invalidation_storm(void)
{
    RtManagedArena *arena = rt_managed_arena_create();
    atomic_bool stop = false;
    atomic_int alloc_count = 0;
    atomic_int error_count = 0;

    /* Many allocator threads */
    #define EPOCH_THREADS 8

    pthread_t threads[EPOCH_THREADS];
    EpochStormArgs args[EPOCH_THREADS];

    for (int i = 0; i < EPOCH_THREADS; i++) {
        args[i] = (EpochStormArgs){
            .arena = arena,
            .stop = &stop,
            .alloc_count = &alloc_count,
            .error_count = &error_count
        };
        pthread_create(&threads[i], NULL, epoch_storm_allocator, &args[i]);
    }

    /* Main thread compacts aggressively */
    for (int c = 0; c < 50; c++) {
        usleep(5000);  /* 5ms between compactions */
        rt_managed_compact(arena);
    }

    atomic_store(&stop, true);
    for (int i = 0; i < EPOCH_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int allocs = atomic_load(&alloc_count);
    int errors = atomic_load(&error_count);

    TEST_STATS("%d allocs, %d errors", allocs, errors);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(errors, 0, "epoch storm: no stale pointer errors");
    ASSERT(allocs > 5000, "epoch storm: sufficient allocations");

    #undef EPOCH_THREADS
}

/* ============================================================================
 * Clone Across Arenas Stress
 * Goal: Multiple threads cloning between different arena pairs
 * ============================================================================ */

typedef struct {
    RtManagedArena *root;
    RtManagedArena **children;
    int num_children;
    atomic_bool *stop;
    atomic_int *clone_count;
    atomic_int *error_count;
    int thread_id;
} CloneStressArgs;

static void *clone_stress_worker(void *arg)
{
    CloneStressArgs *args = (CloneStressArgs *)arg;
    int tid = args->thread_id;
    int local_clones = 0;

    while (!atomic_load(args->stop)) {
        /* Pick two different children */
        int src_idx = (tid + local_clones) % args->num_children;
        int dst_idx = (tid + local_clones + 1) % args->num_children;

        RtManagedArena *src = args->children[src_idx];
        RtManagedArena *dst = args->children[dst_idx];

        /* Allocate in source */
        RtHandle h = rt_managed_alloc(src, RT_HANDLE_NULL, 48);
        if (h != RT_HANDLE_NULL) {
            char *ptr = (char *)rt_managed_pin(src, h);
            if (ptr) {
                snprintf(ptr, 48, "clone-t%d-n%d", tid, local_clones);
                rt_managed_unpin(src, h);
            }

            /* Clone to destination */
            RtHandle cloned = rt_managed_clone(dst, src, h);
            if (cloned != RT_HANDLE_NULL) {
                /* Verify cloned data */
                char *cptr = (char *)rt_managed_pin(dst, cloned);
                if (cptr) {
                    char expected[48];
                    snprintf(expected, sizeof(expected), "clone-t%d-n%d", tid, local_clones);
                    if (strcmp(cptr, expected) != 0) {
                        atomic_fetch_add(args->error_count, 1);
                    }
                    rt_managed_unpin(dst, cloned);
                }
                local_clones++;
            }

            /* Mark source dead */
            rt_managed_mark_dead(src, h);
        }
    }

    atomic_fetch_add(args->clone_count, local_clones);
    return NULL;
}

static void test_clone_across_arenas(void)
{
    RtManagedArena *root = rt_managed_arena_create();

    #define CLONE_CHILDREN 4
    #define CLONE_THREADS 6

    RtManagedArena *children[CLONE_CHILDREN];
    for (int i = 0; i < CLONE_CHILDREN; i++) {
        children[i] = rt_managed_arena_create_child(root);
    }

    atomic_bool stop = false;
    atomic_int clone_count = 0;
    atomic_int error_count = 0;

    pthread_t threads[CLONE_THREADS];
    CloneStressArgs args[CLONE_THREADS];

    for (int i = 0; i < CLONE_THREADS; i++) {
        args[i] = (CloneStressArgs){
            .root = root,
            .children = children,
            .num_children = CLONE_CHILDREN,
            .stop = &stop,
            .clone_count = &clone_count,
            .error_count = &error_count,
            .thread_id = i
        };
        pthread_create(&threads[i], NULL, clone_stress_worker, &args[i]);
    }

    usleep(300000);

    atomic_store(&stop, true);
    for (int i = 0; i < CLONE_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    int clones = atomic_load(&clone_count);
    int errors = atomic_load(&error_count);

    TEST_STATS("%d clones, %d errors", clones, errors);

    /* Cleanup */
    for (int i = 0; i < CLONE_CHILDREN; i++) {
        rt_managed_arena_destroy_child(children[i]);
    }
    rt_managed_arena_destroy(root);

    ASSERT_EQ(errors, 0, "clone stress: no data corruption");
    ASSERT(clones > 1000, "clone stress: sufficient clones");

    #undef CLONE_CHILDREN
    #undef CLONE_THREADS
}

/* ============================================================================
 * Block Retirement Drain
 * Goal: Force blocks to retire and verify they're freed when leases drain
 * ============================================================================ */

static void test_block_retirement_drain(void)
{
    RtManagedArena *arena = rt_managed_arena_create();

    #define RETIRE_HANDLES 200

    RtHandle handles[RETIRE_HANDLES];
    void *pinned_ptrs[RETIRE_HANDLES];

    /* Allocate many entries to create multiple blocks */
    for (int i = 0; i < RETIRE_HANDLES; i++) {
        handles[i] = rt_managed_alloc(arena, RT_HANDLE_NULL, 512);  /* Large to fill blocks */
        char *ptr = (char *)rt_managed_pin(arena, handles[i]);
        snprintf(ptr, 512, "block-data-%d", i);
        pinned_ptrs[i] = ptr;
        /* Keep pinned */
    }

    /* Trigger compaction - blocks should be retired but not freed (pinned) */
    rt_managed_compact(arena);

    /* Verify data still accessible while pinned */
    int valid_while_pinned = 0;
    for (int i = 0; i < RETIRE_HANDLES; i++) {
        char expected[64];
        snprintf(expected, sizeof(expected), "block-data-%d", i);
        if (strncmp((char *)pinned_ptrs[i], expected, strlen(expected)) == 0) {
            valid_while_pinned++;
        }
    }

    /* Now unpin all - blocks should become freeable */
    for (int i = 0; i < RETIRE_HANDLES; i++) {
        rt_managed_unpin(arena, handles[i]);
    }

    /* Trigger another compaction to free retired blocks */
    rt_managed_gc_flush(arena);
    rt_managed_compact(arena);
    rt_managed_gc_flush(arena);

    /* Mark all dead and let GC clean up */
    for (int i = 0; i < RETIRE_HANDLES; i++) {
        rt_managed_mark_dead(arena, handles[i]);
    }
    rt_managed_gc_flush(arena);

    rt_managed_arena_destroy(arena);

    ASSERT_EQ(valid_while_pinned, RETIRE_HANDLES, "block retirement: data valid while pinned");

    #undef RETIRE_HANDLES
}

/* ============================================================================
 * Runner
 * ============================================================================ */

void test_race_detection_run(void)
{
    TEST_SECTION("Thread Scaling (same arena)");
    TEST_RUN("2 threads x 2000 iterations", test_scaling_2_threads);
    TEST_RUN("4 threads x 2000 iterations", test_scaling_4_threads);
    TEST_RUN("8 threads x 1000 iterations", test_scaling_8_threads);
    TEST_RUN("16 threads x 500 iterations", test_scaling_16_threads);
    TEST_RUN("32 threads x 250 iterations", test_scaling_32_threads);

    TEST_SECTION("Mixed Operations Scaling");
    TEST_RUN("8 threads mixed ops x 1000", test_mixed_ops_scaling);

    TEST_SECTION("Rapid Arena Lifecycle");
    TEST_RUN("4 threads x 200 create/destroy cycles", test_rapid_lifecycle_4_threads);
    TEST_RUN("8 threads x 100 create/destroy cycles", test_rapid_lifecycle_8_threads);

    TEST_SECTION("Compaction Under Load");
    TEST_RUN("compaction during allocation storm", test_compaction_during_storm);

    TEST_SECTION("Promotion Contention");
    TEST_RUN("8 threads promoting to same parent", test_promotion_contention);

    TEST_SECTION("Handle Table Growth");
    TEST_RUN("table growth while readers active", test_handle_table_growth_race);

    TEST_SECTION("Long-Running Stability");
    TEST_RUN("8 threads x 500ms mixed operations", test_long_running_stability);

    TEST_SECTION("Pin Duration Variance");
    TEST_RUN("mixed long/short pins during compaction", test_pin_duration_variance);

    TEST_SECTION("Deep Hierarchy");
    TEST_RUN("20-level deep arena tree operations", test_deep_hierarchy_stress);

    TEST_SECTION("Concurrent Destroy + Promote");
    TEST_RUN("destroy child while promoting from it", test_concurrent_destroy_promote);

    TEST_SECTION("Reset Under Load");
    TEST_RUN("reset while threads allocating", test_reset_under_load);

    TEST_SECTION("Pinned Allocation");
    TEST_RUN("permanently pinned entries survive compaction", test_pinned_allocation_stress);

    TEST_SECTION("Free List Recycling");
    TEST_RUN("rapid alloc/dead/recycle cycles", test_free_list_recycling_stress);

    TEST_SECTION("Epoch Invalidation");
    TEST_RUN("rapid compaction during allocation", test_epoch_invalidation_storm);

    TEST_SECTION("Clone Across Arenas");
    TEST_RUN("6 threads cloning between 4 children", test_clone_across_arenas);

    TEST_SECTION("Block Retirement");
    TEST_RUN("blocks freed after pins drain", test_block_retirement_drain);
}
