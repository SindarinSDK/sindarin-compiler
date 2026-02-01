#include "test_race_detection.h"

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

void test_race_scaling_run(void)
{
    TEST_SECTION("Thread Scaling (same arena)");
    TEST_RUN("2 threads x 2000 iterations", test_scaling_2_threads);
    TEST_RUN("4 threads x 2000 iterations", test_scaling_4_threads);
    TEST_RUN("8 threads x 1000 iterations", test_scaling_8_threads);
    TEST_RUN("16 threads x 500 iterations", test_scaling_16_threads);
    TEST_RUN("32 threads x 250 iterations", test_scaling_32_threads);
}
