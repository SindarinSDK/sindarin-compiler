#include "test_framework.h"

static void test_pin_unpin(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
    void *ptr = rt_managed_pin(ma, h);
    ASSERT(ptr != NULL, "pinned pointer should not be NULL");

    memset(ptr, 0xAB, 64);

    rt_managed_unpin(ma, h);
    rt_managed_arena_destroy(ma);
}

static void test_pin_read_write(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 128);

    char *ptr = (char *)rt_managed_pin(ma, h);
    strcpy(ptr, "Hello, Managed Arena!");
    rt_managed_unpin(ma, h);

    ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "Hello, Managed Arena!") == 0, "data should persist across pin/unpin");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_multiple_pins(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 32);

    void *p1 = rt_managed_pin(ma, h);
    void *p2 = rt_managed_pin(ma, h);
    ASSERT(p1 == p2, "multiple pins return same pointer");

    rt_managed_unpin(ma, h);
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_pin_null_handle(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    void *ptr = rt_managed_pin(ma, RT_HANDLE_NULL);
    ASSERT(ptr == NULL, "null handle pin should return NULL");

    rt_managed_unpin(ma, RT_HANDLE_NULL); /* Should not crash */

    rt_managed_arena_destroy(ma);
}

void test_pin_run(void)
{
    printf("\n-- Pin / Unpin --\n");
    TEST_RUN("basic pin and unpin", test_pin_unpin);
    TEST_RUN("pin read/write data", test_pin_read_write);
    TEST_RUN("multiple pins (nested)", test_multiple_pins);
    TEST_RUN("pin null handle", test_pin_null_handle);
}
