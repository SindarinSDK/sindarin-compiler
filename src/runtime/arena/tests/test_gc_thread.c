/*
 * Arena V2 - GC Thread Tests
 * ==========================
 *
 * Tests for the background GC thread functionality.
 */

#include "../arena_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  %s... ", #name); \
    fflush(stdout); \
    test_##name(); \
    printf("PASS\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    Assertion failed: %s\n    at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* Helper to sleep for milliseconds */
static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * Test: Basic GC thread start/stop
 * ============================================================================ */
TEST(gc_thread_start_stop)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* Initially not running */
    ASSERT(!rt_arena_v2_gc_thread_running());

    /* Start */
    rt_arena_v2_gc_thread_start(root, 50);
    ASSERT(rt_arena_v2_gc_thread_running());

    /* Give it time to run at least once */
    sleep_ms(100);

    /* Stop */
    rt_arena_v2_gc_thread_stop();
    ASSERT(!rt_arena_v2_gc_thread_running());

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Test: GC thread collects dead handles
 * ============================================================================ */
TEST(gc_thread_collects_dead)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* Create some handles and mark them dead */
    for (int i = 0; i < 100; i++) {
        RtHandleV2 *h = rt_arena_v2_alloc(root, 64);
        ASSERT(h != NULL);
        rt_arena_v2_free(h);  /* Mark as dead */
    }

    RtArenaV2Stats stats;
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.dead_handle_count == 100);
    ASSERT(stats.gc_runs == 0);

    /* Start GC thread with short interval */
    rt_arena_v2_gc_thread_start(root, 20);

    /* Wait for GC to run */
    sleep_ms(100);

    rt_arena_v2_gc_thread_stop();

    /* Check that handles were collected */
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.handle_count == 0);
    ASSERT(stats.gc_runs > 0);

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Test: GC thread doesn't collect pinned handles
 * ============================================================================ */
TEST(gc_thread_respects_pinned)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    /* Create a pinned handle marked as dead */
    RtHandleV2 *pinned = rt_arena_v2_alloc(root, 64);
    ASSERT(pinned != NULL);
    rt_handle_v2_pin(pinned);  /* Pin it */
    rt_arena_v2_free(pinned);  /* Mark as dead (but pinned!) */

    /* Create unpinned dead handles */
    for (int i = 0; i < 10; i++) {
        RtHandleV2 *h = rt_arena_v2_alloc(root, 64);
        rt_arena_v2_free(h);
    }

    RtArenaV2Stats stats;
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.dead_handle_count == 11);

    /* Start GC thread */
    rt_arena_v2_gc_thread_start(root, 20);
    sleep_ms(100);
    rt_arena_v2_gc_thread_stop();

    /* Only the pinned handle should remain (dead but pinned) */
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.dead_handle_count == 1);

    /* Unpin and verify it gets collected next time */
    rt_handle_v2_unpin(pinned);

    rt_arena_v2_gc_thread_start(root, 20);
    sleep_ms(100);
    rt_arena_v2_gc_thread_stop();

    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.handle_count == 0);

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Test: GC thread works recursively on arena tree
 * ============================================================================ */
TEST(gc_thread_recursive)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    RtArenaV2 *child1 = rt_arena_v2_create(root, RT_ARENA_MODE_DEFAULT, "child1");
    RtArenaV2 *child2 = rt_arena_v2_create(root, RT_ARENA_MODE_DEFAULT, "child2");
    RtArenaV2 *grandchild = rt_arena_v2_create(child1, RT_ARENA_MODE_DEFAULT, "grandchild");

    ASSERT(root != NULL);
    ASSERT(child1 != NULL);
    ASSERT(child2 != NULL);
    ASSERT(grandchild != NULL);

    /* Create dead handles in all arenas */
    for (int i = 0; i < 10; i++) {
        RtHandleV2 *h1 = rt_arena_v2_alloc(root, 32);
        RtHandleV2 *h2 = rt_arena_v2_alloc(child1, 32);
        RtHandleV2 *h3 = rt_arena_v2_alloc(child2, 32);
        RtHandleV2 *h4 = rt_arena_v2_alloc(grandchild, 32);
        rt_arena_v2_free(h1);
        rt_arena_v2_free(h2);
        rt_arena_v2_free(h3);
        rt_arena_v2_free(h4);
    }

    /* Verify dead handles exist in all arenas */
    RtArenaV2Stats stats;
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.dead_handle_count == 10);
    rt_arena_v2_get_stats(child1, &stats);
    ASSERT(stats.dead_handle_count == 10);
    rt_arena_v2_get_stats(child2, &stats);
    ASSERT(stats.dead_handle_count == 10);
    rt_arena_v2_get_stats(grandchild, &stats);
    ASSERT(stats.dead_handle_count == 10);

    /* Start GC thread on root */
    rt_arena_v2_gc_thread_start(root, 20);
    sleep_ms(150);
    rt_arena_v2_gc_thread_stop();

    /* All arenas should be empty */
    rt_arena_v2_get_stats(root, &stats);
    ASSERT(stats.handle_count == 0);
    rt_arena_v2_get_stats(child1, &stats);
    ASSERT(stats.handle_count == 0);
    rt_arena_v2_get_stats(child2, &stats);
    ASSERT(stats.handle_count == 0);
    rt_arena_v2_get_stats(grandchild, &stats);
    ASSERT(stats.handle_count == 0);

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Test: Root pointer is correctly set
 * ============================================================================ */
TEST(root_pointer_correct)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    RtArenaV2 *child = rt_arena_v2_create(root, RT_ARENA_MODE_DEFAULT, "child");
    RtArenaV2 *grandchild = rt_arena_v2_create(child, RT_ARENA_MODE_DEFAULT, "grandchild");

    ASSERT(root != NULL);
    ASSERT(child != NULL);
    ASSERT(grandchild != NULL);

    /* Root's root should be itself */
    ASSERT(root->root == root);

    /* Child's root should be the actual root */
    ASSERT(child->root == root);

    /* Grandchild's root should also be the actual root */
    ASSERT(grandchild->root == root);

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Test: Multiple root arenas (independent trees)
 * ============================================================================ */
TEST(multiple_roots)
{
    RtArenaV2 *root1 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root1");
    RtArenaV2 *child1 = rt_arena_v2_create(root1, RT_ARENA_MODE_DEFAULT, "child1");

    RtArenaV2 *root2 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root2");
    RtArenaV2 *child2 = rt_arena_v2_create(root2, RT_ARENA_MODE_DEFAULT, "child2");

    ASSERT(root1 != NULL);
    ASSERT(child1 != NULL);
    ASSERT(root2 != NULL);
    ASSERT(child2 != NULL);

    /* Each tree has its own root */
    ASSERT(root1->root == root1);
    ASSERT(child1->root == root1);
    ASSERT(root2->root == root2);
    ASSERT(child2->root == root2);

    /* Create dead handles in both trees */
    for (int i = 0; i < 10; i++) {
        RtHandleV2 *h1 = rt_arena_v2_alloc(root1, 32);
        RtHandleV2 *h2 = rt_arena_v2_alloc(root2, 32);
        rt_arena_v2_free(h1);
        rt_arena_v2_free(h2);
    }

    /* Start GC thread only on root1 */
    rt_arena_v2_gc_thread_start(root1, 20);
    sleep_ms(100);
    rt_arena_v2_gc_thread_stop();

    /* Only root1's tree should be collected */
    RtArenaV2Stats stats;
    rt_arena_v2_get_stats(root1, &stats);
    ASSERT(stats.handle_count == 0);
    ASSERT(stats.gc_runs > 0);

    rt_arena_v2_get_stats(root2, &stats);
    ASSERT(stats.dead_handle_count == 10);
    ASSERT(stats.gc_runs == 0);

    rt_arena_v2_destroy(root1);
    rt_arena_v2_destroy(root2);
}

/* ============================================================================
 * Test: GC thread null root is rejected
 * ============================================================================ */
TEST(gc_thread_null_root)
{
    /* Should not crash or start with NULL root */
    rt_arena_v2_gc_thread_start(NULL, 50);
    ASSERT(!rt_arena_v2_gc_thread_running());
}

/* ============================================================================
 * Test: Double start is idempotent
 * ============================================================================ */
TEST(gc_thread_double_start)
{
    RtArenaV2 *root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "root");
    ASSERT(root != NULL);

    rt_arena_v2_gc_thread_start(root, 50);
    ASSERT(rt_arena_v2_gc_thread_running());

    /* Second start should be ignored */
    rt_arena_v2_gc_thread_start(root, 100);
    ASSERT(rt_arena_v2_gc_thread_running());

    rt_arena_v2_gc_thread_stop();
    ASSERT(!rt_arena_v2_gc_thread_running());

    rt_arena_v2_destroy(root);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("Arena V2 GC Thread Tests\n");
    printf("========================\n\n");

    RUN_TEST(gc_thread_start_stop);
    RUN_TEST(gc_thread_collects_dead);
    RUN_TEST(gc_thread_respects_pinned);
    RUN_TEST(gc_thread_recursive);
    RUN_TEST(root_pointer_correct);
    RUN_TEST(multiple_roots);
    RUN_TEST(gc_thread_null_root);
    RUN_TEST(gc_thread_double_start);

    printf("\n%d/%d tests passed\n", tests_passed, tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
