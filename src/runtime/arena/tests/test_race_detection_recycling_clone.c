#include "test_race_detection.h"

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

void test_race_recycling_clone_run(void)
{
    TEST_SECTION("Free List Recycling");
    TEST_RUN("rapid alloc/dead/recycle cycles", test_free_list_recycling_stress);

    TEST_SECTION("Epoch Invalidation");
    TEST_RUN("rapid compaction during allocation", test_epoch_invalidation_storm);

    TEST_SECTION("Clone Across Arenas");
    TEST_RUN("6 threads cloning between 4 children", test_clone_across_arenas);
}
