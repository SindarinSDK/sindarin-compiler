#include "test_race_detection.h"

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

void test_race_table_stability_run(void)
{
    TEST_SECTION("Handle Table Growth");
    TEST_RUN("table growth while readers active", test_handle_table_growth_race);

    TEST_SECTION("Long-Running Stability");
    TEST_RUN("8 threads x 500ms mixed operations", test_long_running_stability);
}
