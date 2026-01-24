#include "test_framework.h"

/* ============================================================================
 * Test: String Helpers
 * ============================================================================ */

static void test_strdup_basic(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_strdup(ma, RT_HANDLE_NULL, "hello world");
    ASSERT(h != RT_HANDLE_NULL, "strdup should return valid handle");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "hello world") == 0, "strdup data correct");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_strdup_reassignment(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_strdup(ma, RT_HANDLE_NULL, "first");
    ASSERT_EQ(rt_managed_live_count(ma), 1, "one live after first strdup");

    /* Reassign — old handle marked dead */
    h = rt_managed_strdup(ma, h, "second");
    ASSERT_EQ(rt_managed_live_count(ma), 1, "still one live after reassign");
    ASSERT_EQ(rt_managed_dead_count(ma), 1, "one dead after reassign");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "second") == 0, "reassigned strdup correct");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_strdup_empty(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_strdup(ma, RT_HANDLE_NULL, "");
    ASSERT(h != RT_HANDLE_NULL, "empty string strdup succeeds");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(ptr[0] == '\0', "empty string is null-terminated");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_strdup_null(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_strdup(ma, RT_HANDLE_NULL, NULL);
    ASSERT_EQ(h, RT_HANDLE_NULL, "null string returns null handle");

    h = rt_managed_strdup(NULL, RT_HANDLE_NULL, "test");
    ASSERT_EQ(h, RT_HANDLE_NULL, "null arena returns null handle");

    rt_managed_arena_destroy(ma);
}

static void test_strndup_basic(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    RtHandle h = rt_managed_strndup(ma, RT_HANDLE_NULL, "hello world", 5);
    ASSERT(h != RT_HANDLE_NULL, "strndup should succeed");

    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "hello") == 0, "strndup truncates correctly");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

static void test_strndup_longer_than_string(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    /* n > strlen — should copy whole string */
    RtHandle h = rt_managed_strndup(ma, RT_HANDLE_NULL, "short", 100);
    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "short") == 0, "strndup with large n copies whole string");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Test: Promote String
 * ============================================================================ */

static void test_promote_string(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);

    RtHandle ch = rt_managed_strdup(child, RT_HANDLE_NULL, "escape-me");
    RtHandle rh = rt_managed_promote_string(root, child, ch);

    ASSERT(rh != RT_HANDLE_NULL, "promote_string returns valid handle");

    rt_managed_arena_destroy_child(child);

    char *ptr = (char *)rt_managed_pin(root, rh);
    ASSERT(strcmp(ptr, "escape-me") == 0, "promoted string survives child destroy");
    rt_managed_unpin(root, rh);

    rt_managed_arena_destroy(root);
}

/* ============================================================================
 * Test: Cleanup Callbacks
 * ============================================================================ */

static int cleanup_call_count = 0;
static int cleanup_order[10] = {0};
static int cleanup_order_idx = 0;

static void cleanup_counter(void *data)
{
    (void)data;
    cleanup_call_count++;
}

static void cleanup_order_recorder(void *data)
{
    int val = *(int *)data;
    if (cleanup_order_idx < 10) {
        cleanup_order[cleanup_order_idx++] = val;
    }
}

static void test_cleanup_on_destroy(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    cleanup_call_count = 0;

    int data1 = 1, data2 = 2, data3 = 3;
    rt_managed_on_cleanup(ma, &data1, cleanup_counter, 50);
    rt_managed_on_cleanup(ma, &data2, cleanup_counter, 50);
    rt_managed_on_cleanup(ma, &data3, cleanup_counter, 50);

    rt_managed_arena_destroy(ma);

    ASSERT_EQ(cleanup_call_count, 3, "all 3 callbacks invoked on destroy");
}

static void test_cleanup_priority_order(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    cleanup_order_idx = 0;
    memset(cleanup_order, 0, sizeof(cleanup_order));

    int val_high = 10, val_med = 20, val_low = 30;

    /* Register in non-priority order */
    rt_managed_on_cleanup(ma, &val_med, cleanup_order_recorder, 50);
    rt_managed_on_cleanup(ma, &val_low, cleanup_order_recorder, 100);
    rt_managed_on_cleanup(ma, &val_high, cleanup_order_recorder, 0);

    rt_managed_arena_destroy(ma);

    ASSERT_EQ(cleanup_order[0], 10, "highest priority (0) invoked first");
    ASSERT_EQ(cleanup_order[1], 20, "medium priority (50) invoked second");
    ASSERT_EQ(cleanup_order[2], 30, "lowest priority (100) invoked last");
}

static void test_cleanup_on_child_destroy(void)
{
    RtManagedArena *root = rt_managed_arena_create();
    RtManagedArena *child = rt_managed_arena_create_child(root);
    cleanup_call_count = 0;

    int data = 42;
    rt_managed_on_cleanup(child, &data, cleanup_counter, 50);

    rt_managed_arena_destroy_child(child);

    ASSERT_EQ(cleanup_call_count, 1, "cleanup invoked on child destroy");

    rt_managed_arena_destroy(root);
}

static void test_cleanup_remove(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    cleanup_call_count = 0;

    int data1 = 1, data2 = 2;
    rt_managed_on_cleanup(ma, &data1, cleanup_counter, 50);
    rt_managed_on_cleanup(ma, &data2, cleanup_counter, 50);

    /* Remove data1's callback */
    rt_managed_remove_cleanup(ma, &data1);

    rt_managed_arena_destroy(ma);

    ASSERT_EQ(cleanup_call_count, 1, "only non-removed callback invoked");
}

static void test_cleanup_null_data_fires(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    cleanup_call_count = 0;

    /* Register callback with NULL data — should still fire */
    rt_managed_on_cleanup(ma, NULL, cleanup_counter, 50);

    rt_managed_arena_destroy(ma);

    ASSERT_EQ(cleanup_call_count, 1, "callback with NULL data fires");
}

static void test_cleanup_null_cases(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    /* Null function */
    RtManagedCleanupNode *node = rt_managed_on_cleanup(ma, (void *)1, NULL, 0);
    ASSERT(node == NULL, "null fn returns NULL");

    /* Null arena */
    node = rt_managed_on_cleanup(NULL, (void *)1, cleanup_counter, 0);
    ASSERT(node == NULL, "null arena returns NULL");

    /* Remove from null arena */
    rt_managed_remove_cleanup(NULL, (void *)1); /* Should not crash */

    /* Remove non-existent data */
    rt_managed_remove_cleanup(ma, (void *)0xDEAD); /* Should not crash */

    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Test: Reset
 * ============================================================================ */

static void test_reset_marks_all_dead(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    /* Create several live entries */
    for (int i = 0; i < 10; i++) {
        RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 64);
        char *ptr = (char *)rt_managed_pin(ma, h);
        snprintf(ptr, 64, "entry-%d", i);
        rt_managed_unpin(ma, h);
    }
    ASSERT_EQ(rt_managed_live_count(ma), 10, "10 live before reset");

    rt_managed_arena_reset(ma);

    ASSERT_EQ(rt_managed_live_count(ma), 0, "0 live after reset");
    ASSERT_EQ(rt_managed_dead_count(ma), 10, "10 dead after reset");

    rt_managed_arena_destroy(ma);
}

static void test_reset_invokes_cleanup(void)
{
    RtManagedArena *ma = rt_managed_arena_create();
    cleanup_call_count = 0;

    int data = 99;
    rt_managed_on_cleanup(ma, &data, cleanup_counter, 50);

    rt_managed_arena_reset(ma);

    ASSERT_EQ(cleanup_call_count, 1, "cleanup invoked on reset");

    /* Can still allocate after reset */
    RtHandle h = rt_managed_alloc(ma, RT_HANDLE_NULL, 32);
    ASSERT(h != RT_HANDLE_NULL, "can allocate after reset");

    rt_managed_arena_destroy(ma);
}

static void test_reset_allows_reuse(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    /* Fill with data */
    RtHandle h = RT_HANDLE_NULL;
    for (int i = 0; i < 50; i++) {
        h = rt_managed_alloc(ma, h, 128);
    }

    rt_managed_arena_reset(ma);

    /* Allocate again — should work */
    h = rt_managed_strdup(ma, RT_HANDLE_NULL, "after-reset");
    char *ptr = (char *)rt_managed_pin(ma, h);
    ASSERT(strcmp(ptr, "after-reset") == 0, "new allocation after reset works");
    rt_managed_unpin(ma, h);

    rt_managed_arena_destroy(ma);
}

/* ============================================================================
 * Test: total_allocated
 * ============================================================================ */

static void test_total_allocated(void)
{
    RtManagedArena *ma = rt_managed_arena_create();

    size_t initial = rt_managed_total_allocated(ma);
    ASSERT(initial > 0, "initial allocation includes first block");

    /* Allocate more than one block */
    for (int i = 0; i < 100; i++) {
        rt_managed_alloc(ma, RT_HANDLE_NULL, 1024);
    }

    size_t after = rt_managed_total_allocated(ma);
    ASSERT(after > initial, "total_allocated grows with allocations");

    rt_managed_arena_destroy(ma);
}

static void test_total_allocated_null(void)
{
    ASSERT_EQ(rt_managed_total_allocated(NULL), 0, "null arena returns 0");
}

/* ============================================================================
 * Runner
 * ============================================================================ */

void test_api_run(void)
{
    printf("\n-- String Helpers --\n");
    TEST_RUN("strdup basic", test_strdup_basic);
    TEST_RUN("strdup with reassignment", test_strdup_reassignment);
    TEST_RUN("strdup empty string", test_strdup_empty);
    TEST_RUN("strdup null cases", test_strdup_null);
    TEST_RUN("strndup basic (truncate)", test_strndup_basic);
    TEST_RUN("strndup n > strlen", test_strndup_longer_than_string);
    TEST_RUN("promote_string convenience", test_promote_string);

    printf("\n-- Cleanup Callbacks --\n");
    TEST_RUN("cleanup invoked on destroy", test_cleanup_on_destroy);
    TEST_RUN("cleanup priority order", test_cleanup_priority_order);
    TEST_RUN("cleanup on child destroy", test_cleanup_on_child_destroy);
    TEST_RUN("cleanup remove", test_cleanup_remove);
    TEST_RUN("cleanup null data fires", test_cleanup_null_data_fires);
    TEST_RUN("cleanup null cases", test_cleanup_null_cases);

    printf("\n-- Reset --\n");
    TEST_RUN("reset marks all dead", test_reset_marks_all_dead);
    TEST_RUN("reset invokes cleanup", test_reset_invokes_cleanup);
    TEST_RUN("reset allows reuse", test_reset_allows_reuse);

    printf("\n-- total_allocated --\n");
    TEST_RUN("total_allocated grows", test_total_allocated);
    TEST_RUN("total_allocated null arena", test_total_allocated_null);
}
