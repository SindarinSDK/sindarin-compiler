#include "test_framework.h"

static void test_reassignment_marks_dead(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h1 = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    ASSERT_EQ(rt_managed_live_count(ma), 1, "one live");
    ASSERT_EQ(rt_managed_dead_count(ma), 0, "none dead");

    RtHandle h2 = rt_managed_alloc(ma, h1, 64);
    ASSERT(h2 != h1, "new handle differs from old");
    ASSERT_EQ(rt_managed_live_count(ma), 1, "still one live");
    ASSERT_EQ(rt_managed_dead_count(ma), 1, "one dead");

    rt_managed_arena_destroy(ma);
}

static void test_rapid_reassignment(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = RT_HANDLE_NULL;
    for (int i = 0; i < 100; i++) {
        h = rt_managed_alloc(ma, h, 64);

        char *ptr = (char *)rt_managed_pin(ma, h);
        snprintf(ptr, 64, "iteration-%d", i);
        rt_managed_unpin(ma, h);
    }

    ASSERT_EQ(rt_managed_live_count(ma), 1, "only last allocation live");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "iteration-99") == 0, "last value preserved");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_multiple_globals_reassignment(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle globals[3] = { RT_HANDLE_NULL, RT_HANDLE_NULL, RT_HANDLE_NULL };

    for (int i = 0; i < 50; i++) {
        for (int g = 0; g < 3; g++) {
            globals[g] = rt_managed_alloc(ma, globals[g], 32);
            char *ptr = (char *)rt_managed_pin(ma, globals[g]);
            snprintf(ptr, 32, "g%d-v%d", g, i);
            rt_managed_unpin(ma, globals[g]);
        }
    }

    ASSERT_EQ(rt_managed_live_count(ma), 3, "three live globals");

    for (int g = 0; g < 3; g++) {
        char expected[32];
        snprintf(expected, sizeof(expected), "g%d-v49", g);
        char *ptr = (char *)rt_managed_pin(ma, globals[g]);
        ASSERT(strcmp(ptr, expected) == 0, "global final value correct");
        rt_managed_unpin(ma, globals[g]);
    }

    rt_managed_arena_destroy(ma);
}

static void test_fragmentation_ratio(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    double frag = rt_managed_fragmentation(ma);
    ASSERT(frag == 0.0, "no fragmentation with no allocations");

    RtHandle h1 = rt_managed_alloc(ma, RT_HANDLE_NULL, 50);
    RtHandle h2 = rt_managed_alloc(ma, RT_HANDLE_NULL, 50);
    (void)h2;

    rt_managed_alloc(ma, h1, 1);

    frag = rt_managed_fragmentation(ma);
    ASSERT(frag > 0.3, "fragmentation should be significant");
    ASSERT(frag < 0.7, "fragmentation should be bounded");

    rt_managed_arena_destroy(ma);
}

void test_reassignment_run(void)
{
    printf("\n-- Reassignment --\n");
    TEST_RUN("reassignment marks old as dead", test_reassignment_marks_dead);
    TEST_RUN("rapid reassignment (100x)", test_rapid_reassignment);
    TEST_RUN("multiple globals (3x50 reassignments)", test_multiple_globals_reassignment);

    printf("\n-- Diagnostics --\n");
    TEST_RUN("fragmentation ratio", test_fragmentation_ratio);
}
