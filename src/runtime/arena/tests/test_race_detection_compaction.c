#include "test_race_detection.h"

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

void test_race_compaction_run(void)
{
    TEST_SECTION("Compaction Under Load");
    TEST_RUN("compaction during allocation storm", test_compaction_during_storm);

    TEST_SECTION("Promotion Contention");
    TEST_RUN("8 threads promoting to same parent", test_promotion_contention);
}
