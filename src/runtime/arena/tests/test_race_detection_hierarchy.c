#include "test_race_detection.h"

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

void test_race_hierarchy_run(void)
{
    TEST_SECTION("Deep Hierarchy");
    TEST_RUN("20-level deep arena tree operations", test_deep_hierarchy_stress);
}
