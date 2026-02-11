/*
 * Arena V2 - Basic Tests
 * =======================
 * Compile: gcc -o arena_v2_test arena_v2.c arena_v2_test.c -lpthread
 */

#include "../arena_v2.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

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
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_basic_alloc(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_destroy(arena); return 0; }

    /* Check handle fields */
    if (h->ptr == NULL) { rt_arena_v2_destroy(arena); return 0; }
    if (h->size != 100) { rt_arena_v2_destroy(arena); return 0; }
    if (h->arena != arena) { rt_arena_v2_destroy(arena); return 0; }

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_strdup(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_strdup(arena, "Hello, World!");
    if (h == NULL) { rt_arena_v2_destroy(arena); return 0; }

    char *str = (char *)(h ? h->ptr : NULL);
    if (strcmp(str, "Hello, World!") != 0) { rt_arena_v2_destroy(arena); return 0; }

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_pin_unpin(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_destroy(arena); return 0; }

    /* Initial pin count should be 0 */
    if (h->pin_count != 0) { rt_arena_v2_destroy(arena); return 0; }

    /* Pin increments count */
    rt_handle_v2_pin(h);
    void *ptr = h->ptr;
    if (ptr != h->ptr) { rt_arena_v2_destroy(arena); return 0; }
    if (h->pin_count != 1) { rt_arena_v2_destroy(arena); return 0; }

    /* Unpin decrements count */
    rt_handle_v2_unpin(h);
    if (h->pin_count != 0) { rt_arena_v2_destroy(arena); return 0; }

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_gc_collects_dead(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    /* Allocate some handles */
    (void)rt_arena_v2_alloc(arena, 100);  /* h1 - kept */
    RtHandleV2 *h2 = rt_arena_v2_alloc(arena, 100);
    (void)rt_arena_v2_alloc(arena, 100);  /* h3 - kept */

    if (arena->handle_count != 3) { rt_arena_v2_destroy(arena); return 0; }

    /* Mark h2 as dead */
    rt_arena_v2_free(h2);

    /* GC should collect h2 */
    size_t collected = rt_arena_v2_gc(arena);
    if (collected != 1) { rt_arena_v2_destroy(arena); return 0; }
    if (arena->handle_count != 2) { rt_arena_v2_destroy(arena); return 0; }

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_gc_skips_pinned(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100);
    if (h == NULL) { rt_arena_v2_destroy(arena); return 0; }

    /* Pin, mark dead, try GC */
    rt_handle_v2_pin(h);
    rt_arena_v2_free(h);

    size_t collected = rt_arena_v2_gc(arena);
    if (collected != 0) { rt_arena_v2_destroy(arena); return 0; }
    if (arena->handle_count != 1) { rt_arena_v2_destroy(arena); return 0; }

    /* Unpin, GC should now collect */
    rt_handle_v2_unpin(h);
    collected = rt_arena_v2_gc(arena);
    if (collected != 1) { rt_arena_v2_destroy(arena); return 0; }

    rt_arena_v2_destroy(arena);
    return 1;
}

static int test_child_arenas(void)
{
    RtArenaV2 *parent = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "parent");
    if (parent == NULL) return 0;

    RtArenaV2 *child1 = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child1");
    RtArenaV2 *child2 = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child2");

    if (child1 == NULL || child2 == NULL) { rt_arena_v2_destroy(parent); return 0; }

    /* Children should have parent set */
    if (child1->parent != parent) { rt_arena_v2_destroy(parent); return 0; }
    if (child2->parent != parent) { rt_arena_v2_destroy(parent); return 0; }

    /* Destroying parent should destroy children too */
    rt_arena_v2_destroy(parent);
    return 1;
}

static int test_promote(void)
{
    RtArenaV2 *parent = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "parent");
    RtArenaV2 *child = rt_arena_v2_create(parent, RT_ARENA_MODE_DEFAULT, "child");
    if (parent == NULL || child == NULL) return 0;

    /* Allocate in child */
    RtHandleV2 *h = rt_arena_v2_strdup(child, "test data");
    if (h == NULL) { rt_arena_v2_destroy(parent); return 0; }
    if (h->arena != child) { rt_arena_v2_destroy(parent); return 0; }

    /* Promote to parent */
    RtHandleV2 *promoted = rt_arena_v2_promote(parent, h);
    if (promoted == NULL) { rt_arena_v2_destroy(parent); return 0; }
    if (promoted->arena != parent) { rt_arena_v2_destroy(parent); return 0; }

    /* Original should be dead */
    if (!(h->flags & RT_HANDLE_FLAG_DEAD)) { rt_arena_v2_destroy(parent); return 0; }

    /* Data should be copied */
    if (strcmp((char *)promoted->ptr, "test data") != 0) { rt_arena_v2_destroy(parent); return 0; }

    rt_arena_v2_destroy(parent);
    return 1;
}

static int test_redirect_stack(void)
{
    RtArenaV2 *arena1 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "arena1");
    RtArenaV2 *arena2 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "arena2");

    /* Initially no redirect */
    if (rt_arena_v2_redirect_current() != NULL) {
        rt_arena_v2_destroy(arena1);
        rt_arena_v2_destroy(arena2);
        return 0;
    }

    /* Push arena1 */
    rt_arena_v2_redirect_push(arena1);
    if (rt_arena_v2_redirect_current() != arena1) {
        rt_arena_v2_destroy(arena1);
        rt_arena_v2_destroy(arena2);
        return 0;
    }

    /* Push arena2 */
    rt_arena_v2_redirect_push(arena2);
    if (rt_arena_v2_redirect_current() != arena2) {
        rt_arena_v2_destroy(arena1);
        rt_arena_v2_destroy(arena2);
        return 0;
    }

    /* Pop should return to arena1 */
    rt_arena_v2_redirect_pop();
    if (rt_arena_v2_redirect_current() != arena1) {
        rt_arena_v2_destroy(arena1);
        rt_arena_v2_destroy(arena2);
        return 0;
    }

    /* Pop should return to NULL */
    rt_arena_v2_redirect_pop();
    if (rt_arena_v2_redirect_current() != NULL) {
        rt_arena_v2_destroy(arena1);
        rt_arena_v2_destroy(arena2);
        return 0;
    }

    rt_arena_v2_destroy(arena1);
    rt_arena_v2_destroy(arena2);
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

    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_on_cleanup(arena, NULL, cleanup_increment, 0);
    rt_arena_v2_destroy(arena);

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

    TEST(arena_create_destroy);
    TEST(basic_alloc);
    TEST(strdup);
    TEST(pin_unpin);
    TEST(gc_collects_dead);
    TEST(gc_skips_pinned);
    TEST(child_arenas);
    TEST(promote);
    TEST(redirect_stack);
    TEST(cleanup_callbacks_real);
    (void)cleanup_increment;  /* Silence unused warning if TEST macro expands differently */

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
