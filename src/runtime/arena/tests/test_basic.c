/*
 * Arena V2 - Basic Tests
 * =======================
 * Compile: gcc -o arena_v2_test arena_v2.c arena_v2_test.c -lpthread
 */

#include "../arena_v2.h"
#include "../arena_stats.h"
#include "../arena_gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;
static RtArenaV2 *test_root = NULL;

#define TEST(name) do { \
    printf("  %s... ", #name); \
    tests_run++; \
    if (test_##name()) { \
        printf("PASS\n"); \
        tests_passed++; \
    } else { \
        printf("FAIL\n"); \
    } \
} while(0)

/* ============================================================================
 * Tests
 * ============================================================================ */

static int test_arena_create_destroy(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_basic_alloc(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    /* Check handle fields */
    if (h->ptr == NULL) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }
    if (h->size != 100) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }
    if (h->arena != arena) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_strdup(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_strdup(arena, "Hello, World!");
    if (h == NULL) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    char *str = (char *)(h ? h->ptr : NULL);
    if (strcmp(str, "Hello, World!") != 0) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_transaction(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    /* Begin transaction and access ptr */
    rt_handle_begin_transaction(h);
    void *ptr = h->ptr;
    if (ptr != h->ptr) { rt_handle_end_transaction(h); rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    /* Nested transaction (same thread, same block) */
    rt_handle_begin_transaction(h);
    if (ptr != h->ptr) { rt_handle_end_transaction(h); rt_handle_end_transaction(h); rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }
    rt_handle_end_transaction(h);

    /* End outer transaction */
    rt_handle_end_transaction(h);

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_gc_collects_dead(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    /* Allocate some handles */
    (void)rt_arena_v2_alloc(arena, 100);  /* h1 - kept */
    RtHandleV2 *h2 = rt_arena_v2_alloc(arena, 100);
    (void)rt_arena_v2_alloc(arena, 100);  /* h3 - kept */

    RtArenaV2Stats stats;
    rt_arena_stats_recompute(arena);  /* Stats are cached, need to recompute */
    rt_arena_stats_get(arena, &stats);
    if (stats.handles.local != 3) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    /* Mark h2 as dead */
    rt_arena_v2_free(h2);

    /* GC should collect h2 */
    size_t collected = rt_arena_v2_gc(arena);
    if (collected != 1) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }
    rt_arena_stats_get(arena, &stats);
    if (stats.handles.local != 2) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_transaction_renewal(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_condemn(arena); rt_arena_v2_gc(test_root); return 0; }

    /* Begin transaction */
    rt_handle_begin_transaction(h);

    /* Renew transaction (resets timeout) */
    rt_handle_renew_transaction(h);

    /* End transaction */
    rt_handle_end_transaction(h);

    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_child_arenas(void)
{
    RtArenaV2 *parent = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "parent");
    if (parent == NULL) return 0;

    RtArenaV2 *child1 = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child1");
    RtArenaV2 *child2 = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child2");

    if (child1 == NULL || child2 == NULL) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    /* Children should have parent set */
    if (child1->parent != parent) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }
    if (child2->parent != parent) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    /* Destroying parent should destroy children too */
    rt_arena_v2_condemn(parent);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_promote(void)
{
    RtArenaV2 *parent = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "parent");
    RtArenaV2 *child = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child");
    if (parent == NULL || child == NULL) return 0;

    /* Allocate in child */
    RtHandleV2 *h = rt_arena_v2_strdup(child, "test data");
    if (h == NULL) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }
    if (h->arena != child) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    /* Promote to parent */
    RtHandleV2 *promoted = rt_arena_v2_promote(parent, h);
    if (promoted == NULL) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }
    if (promoted->arena != parent) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    /* Original should be dead */
    if (!(h->flags & RT_HANDLE_FLAG_DEAD)) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    /* Data should be copied */
    if (strcmp((char *)promoted->ptr, "test data") != 0) { rt_arena_v2_condemn(parent); rt_arena_v2_gc(test_root); return 0; }

    rt_arena_v2_condemn(parent);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int test_redirect_stack(void)
{
    RtArenaV2 *arena1 = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "arena1");
    RtArenaV2 *arena2 = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "arena2");

    /* Initially no redirect */
    if (rt_arena_v2_redirect_current() != NULL) {
        rt_arena_v2_condemn(arena1);
        rt_arena_v2_condemn(arena2);
        rt_arena_v2_gc(test_root);
        return 0;
    }

    /* Push arena1 */
    rt_arena_v2_redirect_push(arena1);
    if (rt_arena_v2_redirect_current() != arena1) {
        rt_arena_v2_condemn(arena1);
        rt_arena_v2_condemn(arena2);
        rt_arena_v2_gc(test_root);
        return 0;
    }

    /* Push arena2 */
    rt_arena_v2_redirect_push(arena2);
    if (rt_arena_v2_redirect_current() != arena2) {
        rt_arena_v2_condemn(arena1);
        rt_arena_v2_condemn(arena2);
        rt_arena_v2_gc(test_root);
        return 0;
    }

    /* Pop should return to arena1 */
    rt_arena_v2_redirect_pop();
    if (rt_arena_v2_redirect_current() != arena1) {
        rt_arena_v2_condemn(arena1);
        rt_arena_v2_condemn(arena2);
        rt_arena_v2_gc(test_root);
        return 0;
    }

    /* Pop should return to NULL */
    rt_arena_v2_redirect_pop();
    if (rt_arena_v2_redirect_current() != NULL) {
        rt_arena_v2_condemn(arena1);
        rt_arena_v2_condemn(arena2);
        rt_arena_v2_gc(test_root);
        return 0;
    }

    rt_arena_v2_condemn(arena1);
    rt_arena_v2_condemn(arena2);
    rt_arena_v2_gc(test_root);
    return 1;
}

static int g_cleanup_called = 0;

static void cleanup_increment(RtHandleV2 *data)
{
    (void)data;
    g_cleanup_called++;
}

static int test_cleanup_callbacks_real(void)
{
    g_cleanup_called = 0;

    RtArenaV2 *arena = rt_arena_v2_create(test_root, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_on_cleanup(arena, NULL, cleanup_increment, 0);
    rt_arena_v2_condemn(arena);
    rt_arena_v2_gc(test_root);

    if (g_cleanup_called != 1) return 0;
    return 1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("Arena V2 Tests\n");
    printf("==============\n\n");

    /* Create root arena for all tests */
    test_root = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test_root");
    if (test_root == NULL) {
        printf("Failed to create test root arena\n");
        return 1;
    }

    TEST(arena_create_destroy);
    TEST(basic_alloc);
    TEST(strdup);
    TEST(transaction);
    TEST(gc_collects_dead);
    TEST(transaction_renewal);
    TEST(child_arenas);
    TEST(promote);
    TEST(redirect_stack);
    TEST(cleanup_callbacks_real);
    (void)cleanup_increment;  /* Silence unused warning if TEST macro expands differently */

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);

    /* Final GC to clean up any remaining condemned children */
    rt_arena_v2_gc(test_root);
    /* test_root itself needs to be freed - it has no parent to GC it.
     * Since we never allocated directly to test_root, just destroy mutex and free. */
    pthread_mutex_destroy(&test_root->mutex);
    free(test_root);

    return tests_passed == tests_run ? 0 : 1;
}
