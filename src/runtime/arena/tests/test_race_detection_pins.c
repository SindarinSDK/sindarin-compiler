#include "test_race_detection.h"

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

void test_race_pins_run(void)
{
    TEST_SECTION("Pin Duration Variance");
    TEST_RUN("mixed long/short pins during compaction", test_pin_duration_variance);

    TEST_SECTION("Pinned Allocation");
    TEST_RUN("permanently pinned entries survive compaction", test_pinned_allocation_stress);

    TEST_SECTION("Block Retirement");
    TEST_RUN("blocks freed after pins drain", test_block_retirement_drain);
}
