#include "test_framework.h"

/* ============================================================================
 * Stress
 * ============================================================================ */

static void test_stress_alloc_reassign(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle globals[5] = {0};
    for (int iter = 0; iter < 1000; iter++) {
        for (int g = 0; g < 5; g++) {
            globals[g] = rt_managed_alloc(ma, globals[g], 128);
            char *ptr = (char *)rt_managed_pin(ma, globals[g]);
            snprintf(ptr, 128, "g%d-iter%d", g, iter);
            rt_managed_unpin(ma, globals[g]);
        }
    }

    ASSERT_EQ(rt_managed_live_count(ma), 5, "only 5 live after stress");

    for (int g = 0; g < 5; g++) {
        char expected[128];
        snprintf(expected, sizeof(expected), "g%d-iter999", g);
        char *ptr = (char *)rt_managed_pin(ma, globals[g]);
        ASSERT(strcmp(ptr, expected) == 0, "final value correct after stress");
        rt_managed_unpin(ma, globals[g]);
    }

    rt_managed_gc_flush(ma);
    rt_managed_compact(ma);
    rt_managed_gc_flush(ma);

    size_t used = rt_managed_arena_used(ma);
    ASSERT(used < 5000 * 128, "memory should be reclaimed by GC");

    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Concurrent Pin + Compact
 * ============================================================================ */

typedef struct {
    RtManagedArena *ma;
    RtHandle *handles;
    int count;
    int iterations;
    atomic_bool *stop;
} PinnerArgs;

static void *pinner_thread(void *arg)
{
    PinnerArgs *args = (PinnerArgs *)arg;

    for (int i = 0; i < args->iterations && !atomic_load(args->stop); i++) {
        int idx = i % args->count;
        RtHandle h = args->handles[idx];
        if (h == RT_HANDLE_NULL) continue;

        char *ptr = (char *)rt_managed_pin(args->ma, h);
        if (ptr != NULL) {
            volatile char c = ptr[0];
            (void)c;
        }
        rt_managed_unpin(args->ma, h);
    }

    return NULL;
}

static void test_concurrent_pin_compact(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    atomic_bool stop = false;

    RtHandle handles[20];
    for (int i = 0; i < 20; i++) {
        handles[i] = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(ma, handles[i]);
        snprintf(ptr, 64, "entry-%d", i);
        rt_managed_unpin(ma, handles[i]);
    }

    PinnerArgs args = { .ma = ma, .handles = handles, .count = 20,
                        .iterations = 10000, .stop = &stop };
    pthread_t pinners[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&pinners[i], NULL, pinner_thread, &args);
    }

    for (int i = 0; i < 5; i++) {
        rt_managed_compact(ma);
        usleep(10000);
    }

    atomic_store(&stop, true);
    for (int i = 0; i < 4; i++) {
        pthread_join(pinners[i], NULL);
    }

    for (int i = 0; i < 20; i++) {
        char expected[64];
        snprintf(expected, sizeof(expected), "entry-%d", i);
        char *ptr = (char *)rt_managed_pin(ma, handles[i]);
        ASSERT(ptr != NULL, "entry accessible after concurrent test");
        ASSERT(strcmp(ptr, expected) == 0, "data intact after concurrent access");
        rt_managed_unpin(ma, handles[i]);
    }

    rt_managed_arena_destroy(ma);
}

void test_concurrency_run(void)
{
    TEST_SECTION("Stress");
    TEST_RUN("5 globals x 1000 reassignments", test_stress_alloc_reassign);

    TEST_SECTION("Concurrency");
    TEST_RUN("concurrent pin + compact", test_concurrent_pin_compact);
}
