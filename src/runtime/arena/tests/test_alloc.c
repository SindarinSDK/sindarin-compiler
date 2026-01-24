#include "test_framework.h"

static void test_create_destroy(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    ASSERT(ma != NULL, "arena should not be NULL");
    ASSERT_EQ(rt_managed_live_count(ma), 0, "no live allocations initially");
    ASSERT_EQ(rt_managed_dead_count(ma), 0, "no dead allocations initially");
    rt_managed_arena_destroy(ma);
}

static void test_single_alloc(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    ASSERT(h != RT_HANDLE_NULL, "handle should not be null");
    ASSERT_EQ(rt_managed_live_count(ma), 1, "one live allocation");

    rt_managed_arena_destroy(ma);
}

static void test_multiple_allocs(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h1 = rt_managed_alloc(ma, RT_HANDLE_NULL, 32);
    RtHandle h2 = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    RtHandle h3 = rt_managed_alloc(ma, RT_HANDLE_NULL, 128);

    ASSERT(h1 != h2, "handles should be unique");
    ASSERT(h2 != h3, "handles should be unique");
    ASSERT_EQ(rt_managed_live_count(ma), 3, "three live allocations");

    rt_managed_arena_destroy(ma);
}

static void test_null_handle_alloc(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 0);
    ASSERT_EQ(h, RT_HANDLE_NULL, "zero size should return null handle");

    h = rt_managed_alloc(NULL, RT_HANDLE_NULL, 64);
    ASSERT_EQ(h, RT_HANDLE_NULL, "null arena should return null handle");

    rt_managed_arena_destroy(ma);
}

static void test_large_allocation(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    size_t large_size = RT_MANAGED_BLOCK_SIZE + 1024;
    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, large_size);
    ASSERT(h != RT_HANDLE_NULL, "large allocation should succeed");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ptr[large_size - 1] = 'Z';
    ASSERT(ptr[large_size - 1] == 'Z', "write to end of large alloc");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

void test_alloc_run(void)
{
    printf("-- Basic Allocation --\n");
    TEST_RUN("create and destroy", test_create_destroy);
    TEST_RUN("single allocation", test_single_alloc);
    TEST_RUN("multiple allocations", test_multiple_allocs);
    TEST_RUN("null/zero edge cases", test_null_handle_alloc);
    TEST_RUN("allocation larger than block size", test_large_allocation);
}
