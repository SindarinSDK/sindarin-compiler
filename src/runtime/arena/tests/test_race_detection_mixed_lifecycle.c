#include "test_race_detection.h"

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

void test_race_mixed_lifecycle_run(void)
{
    TEST_SECTION("Mixed Operations Scaling");
    TEST_RUN("8 threads mixed ops x 1000", test_mixed_ops_scaling);

    TEST_SECTION("Rapid Arena Lifecycle");
    TEST_RUN("4 threads x 200 create/destroy cycles", test_rapid_lifecycle_4_threads);
    TEST_RUN("8 threads x 100 create/destroy cycles", test_rapid_lifecycle_8_threads);
}
