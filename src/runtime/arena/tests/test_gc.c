#include "test_framework.h"

/* ============================================================================
 * Cleaner Thread
 * ============================================================================ */

static void test_cleaner_zeros_dead(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = RT_HANDLE_NULL;
    for (int i = 0; i < 10; i++) {
        h = rt_managed_alloc(ma, h, 64);
    }

    ASSERT_EQ(rt_managed_dead_count(ma), 9, "nine dead after reassignment");

    rt_managed_gc_flush(ma);

    size_t dead = rt_managed_dead_count(ma);
    ASSERT(dead < 9, "cleaner should have reduced dead count");

    rt_managed_arena_destroy(ma);
}

static void test_cleaner_preserves_live(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle handles[5];
    for (int i = 0; i < 5; i++) {
        handles[i] = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(ma, handles[i]);
        snprintf(ptr, 64, "live-data-%d", i);
        rt_managed_unpin(ma, handles[i]);
    }

    rt_managed_gc_flush(ma);

    ASSERT_EQ(rt_managed_live_count(ma), 5, "all five still live");
    for (int i = 0; i < 5; i++) {
        char expected[64];
        snprintf(expected, sizeof(expected), "live-data-%d", i);
        char *ptr = (char *)rt_managed_pin(ma, handles[i]);
        ASSERT(strcmp(ptr, expected) == 0, "live data preserved by cleaner");
        rt_managed_unpin(ma, handles[i]);
    }

    rt_managed_arena_destroy(ma);
}

static void test_cleaner_respects_leases(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h1 = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    char *pinned = (char *)rt_managed_pin(ma, h1);
    strcpy(pinned, "pinned-data");

    RtHandle h2 = rt_managed_alloc(ma, h1, 64);
    (void)h2;

    rt_managed_gc_flush(ma);

    ASSERT(strcmp(pinned, "pinned-data") == 0, "pinned dead entry not cleaned");

    rt_managed_unpin(ma, h1);

    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Compaction
 * ============================================================================ */

static void test_compact_reduces_fragmentation(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle keep[10];
    for (int i = 0; i < 100; i++) {
        RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 256);
        if (i % 10 == 0) {
            keep[i / 10] = h;
            char *ptr = (char *)rt_managed_pin(ma, h);
            snprintf(ptr, 256, "keep-%d", i / 10);
            rt_managed_unpin(ma, h);
        } else {
            rt_managed_alloc(ma, h, 1);
        }
    }

    rt_managed_gc_flush(ma);

    rt_managed_compact(ma);

    for (int i = 0; i < 10; i++) {
        char expected[256];
        snprintf(expected, sizeof(expected), "keep-%d", i);
        char *ptr = (char *)rt_managed_pin(ma, keep[i]);
        ASSERT(ptr != NULL, "compacted entry should be accessible");
        ASSERT(strcmp(ptr, expected) == 0, "compacted data preserved");
        rt_managed_unpin(ma, keep[i]);
    }

    rt_managed_arena_destroy(ma);
}

static void test_compact_skips_pinned(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h1 = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    RtHandle h2 = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);

    char *ptr1 = (char *)rt_managed_pin(ma, h1);
    strcpy(ptr1, "pinned-entry");

    char *ptr2 = (char *)rt_managed_pin(ma, h2);
    strcpy(ptr2, "unpinned-entry");
    rt_managed_unpin(ma, h2);

    rt_managed_compact(ma);

    ASSERT(strcmp(ptr1, "pinned-entry") == 0, "pinned entry not moved");

    char *new_ptr2 = (char *)rt_managed_pin(ma, h2);
    ASSERT(strcmp(new_ptr2, "unpinned-entry") == 0, "moved entry data preserved");
    rt_managed_unpin(ma, h2);

    rt_managed_unpin(ma, h1);
    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Handle Recycling
 * ============================================================================ */

static void test_handle_recycling(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = RT_HANDLE_NULL;
    for (int i = 0; i < 500; i++) {
        h = rt_managed_alloc(ma, h, 32);
    }

    rt_managed_gc_flush(ma);

    ASSERT_EQ(rt_managed_live_count(ma), 1, "only one live handle");

    rt_managed_arena_destroy(ma);
}

void test_gc_run(void)
{
    printf("\n-- Cleaner Thread --\n");
    TEST_RUN("cleaner zeros dead entries", test_cleaner_zeros_dead);
    TEST_RUN("cleaner preserves live entries", test_cleaner_preserves_live);
    TEST_RUN("cleaner respects leases", test_cleaner_respects_leases);

    printf("\n-- Compaction --\n");
    TEST_RUN("compact reduces fragmentation", test_compact_reduces_fragmentation);
    TEST_RUN("compact skips pinned entries", test_compact_skips_pinned);

    printf("\n-- Handle Recycling --\n");
    TEST_RUN("handle recycling after cleanup", test_handle_recycling);
}
