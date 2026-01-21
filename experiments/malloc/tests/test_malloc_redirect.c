/*
 * Test program for Arena-Redirected Malloc System
 *
 * Tests the malloc redirect functionality that routes malloc/free/realloc/calloc
 * to an arena allocator.
 *
 * Build and run:
 *   make test-redirect
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Enable the redirect code if not already defined */
#ifndef SN_MALLOC_REDIRECT
#define SN_MALLOC_REDIRECT
#endif

#include "runtime_arena.h"
#include "runtime_malloc_redirect.h"

/* Test helper macros */
#define TEST(name) do { printf("Testing: %s... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test: Basic redirect enable/disable
 * ============================================================================ */
static void test_basic_redirect(void)
{
    TEST("basic redirect enable/disable");

    /* Initially not active */
    CHECK(!rt_malloc_redirect_is_active(), "should not be active initially");
    CHECK(rt_malloc_redirect_depth() == 0, "depth should be 0");

    /* Create arena and push */
    RtArena *arena = rt_arena_create(NULL);
    CHECK(arena != NULL, "arena creation failed");

    bool pushed = rt_malloc_redirect_push(arena, NULL);
    CHECK(pushed, "push failed");
    CHECK(rt_malloc_redirect_is_active(), "should be active after push");
    CHECK(rt_malloc_redirect_depth() == 1, "depth should be 1");
    CHECK(rt_malloc_redirect_arena() == arena, "arena should match");

    /* Pop */
    bool popped = rt_malloc_redirect_pop();
    CHECK(popped, "pop failed");
    CHECK(!rt_malloc_redirect_is_active(), "should not be active after pop");
    CHECK(rt_malloc_redirect_depth() == 0, "depth should be 0 after pop");

    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Malloc redirection
 * ============================================================================ */
static void test_malloc_redirect(void)
{
    TEST("malloc redirection");

    RtArena *arena = rt_arena_create(NULL);
    CHECK(arena != NULL, "arena creation failed");

    rt_malloc_redirect_push(arena, NULL);

    /* Allocate using malloc - should go to arena */
    char *str = (char *)malloc(100);
    CHECK(str != NULL, "malloc returned NULL");

    /* Verify it's in the arena */
    CHECK(rt_malloc_redirect_is_arena_ptr(str), "ptr should be in arena");
    CHECK(rt_malloc_redirect_ptr_size(str) == 100, "size should be 100");

    /* Write to it to verify it's valid memory */
    strcpy(str, "Hello, arena-redirected world!");
    CHECK(strcmp(str, "Hello, arena-redirected world!") == 0, "string mismatch");

    /* Check stats */
    RtRedirectStats stats;
    CHECK(rt_malloc_redirect_get_stats(&stats), "get_stats failed");
    CHECK(stats.alloc_count >= 1, "alloc_count should be >= 1");
    CHECK(stats.total_requested >= 100, "total_requested should be >= 100");

    /* Free - should be a no-op but tracked */
    free(str);
    CHECK(rt_malloc_redirect_get_stats(&stats), "get_stats failed after free");
    CHECK(stats.free_count >= 1, "free_count should be >= 1");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Calloc redirection
 * ============================================================================ */
static void test_calloc_redirect(void)
{
    TEST("calloc redirection");

    RtArena *arena = rt_arena_create(NULL);
    rt_malloc_redirect_push(arena, NULL);

    /* Allocate using calloc - should go to arena and be zeroed */
    int *arr = (int *)calloc(10, sizeof(int));
    CHECK(arr != NULL, "calloc returned NULL");
    CHECK(rt_malloc_redirect_is_arena_ptr(arr), "ptr should be in arena");

    /* Verify it's zeroed */
    for (int i = 0; i < 10; i++) {
        CHECK(arr[i] == 0, "calloc memory not zeroed");
    }

    /* Use it */
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
    }
    CHECK(arr[5] == 25, "array value mismatch");

    free(arr);
    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Realloc redirection
 * ============================================================================ */
static void test_realloc_redirect(void)
{
    TEST("realloc redirection");

    RtArena *arena = rt_arena_create(NULL);
    rt_malloc_redirect_push(arena, NULL);

    /* Start with malloc */
    char *str = (char *)malloc(20);
    CHECK(str != NULL, "malloc returned NULL");
    strcpy(str, "Hello");

    /* Grow with realloc */
    char *str2 = (char *)realloc(str, 100);
    CHECK(str2 != NULL, "realloc returned NULL");
    CHECK(rt_malloc_redirect_is_arena_ptr(str2), "realloc result should be in arena");

    /* Original data should be preserved */
    CHECK(strcmp(str2, "Hello") == 0, "data not preserved after realloc");

    /* Extend the string */
    strcat(str2, ", World!");
    CHECK(strcmp(str2, "Hello, World!") == 0, "string mismatch after strcat");

    /* Check stats */
    RtRedirectStats stats;
    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.realloc_count >= 1, "realloc_count should be >= 1");

    /* Test realloc with NULL (equivalent to malloc) */
    char *str3 = (char *)realloc(NULL, 50);
    CHECK(str3 != NULL, "realloc(NULL, 50) returned NULL");
    CHECK(rt_malloc_redirect_is_arena_ptr(str3), "realloc(NULL) result should be in arena");

    free(str2);
    free(str3);
    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Nested redirect scopes
 * ============================================================================ */
static void test_nested_redirect(void)
{
    TEST("nested redirect scopes");

    /* Create two arenas */
    RtArena *arena1 = rt_arena_create(NULL);
    RtArena *arena2 = rt_arena_create(NULL);

    /* Push first scope */
    rt_malloc_redirect_push(arena1, NULL);
    CHECK(rt_malloc_redirect_depth() == 1, "depth should be 1");
    CHECK(rt_malloc_redirect_arena() == arena1, "arena should be arena1");

    char *str1 = (char *)malloc(50);
    CHECK(rt_malloc_redirect_is_arena_ptr(str1), "str1 should be in arena");

    /* Push second scope (nested) */
    rt_malloc_redirect_push(arena2, NULL);
    CHECK(rt_malloc_redirect_depth() == 2, "depth should be 2");
    CHECK(rt_malloc_redirect_arena() == arena2, "arena should be arena2");

    char *str2 = (char *)malloc(50);
    CHECK(rt_malloc_redirect_is_arena_ptr(str2), "str2 should be in arena");

    /* Pop inner scope */
    rt_malloc_redirect_pop();
    CHECK(rt_malloc_redirect_depth() == 1, "depth should be 1 after inner pop");
    CHECK(rt_malloc_redirect_arena() == arena1, "arena should be arena1 after pop");

    /* str2 is now in destroyed scope's hash set - but memory still valid in arena2 */

    /* Allocate in outer scope */
    char *str3 = (char *)malloc(50);
    CHECK(rt_malloc_redirect_is_arena_ptr(str3), "str3 should be in arena");

    /* Pop outer scope */
    rt_malloc_redirect_pop();
    CHECK(rt_malloc_redirect_depth() == 0, "depth should be 0 after outer pop");
    CHECK(!rt_malloc_redirect_is_active(), "should not be active");

    rt_arena_destroy(arena2);
    rt_arena_destroy(arena1);
    PASS();
}

/* ============================================================================
 * Test: Custom configuration - track allocations
 * ============================================================================ */
static void test_tracking_config(void)
{
    TEST("allocation tracking");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.track_allocations = true;
    config.free_policy = RT_REDIRECT_FREE_TRACK;

    rt_malloc_redirect_push(arena, &config);

    /* Make some allocations */
    void *p1 = malloc(100);
    void *p2 = malloc(200);
    void *p3 = malloc(300);

    /* Free one */
    free(p2);

    /* Check for leaks */
    void *leaks[10];
    size_t sizes[10];
    size_t leak_count = rt_malloc_redirect_track_leaks(leaks, sizes, 10);

    /* Should have 2 "leaks" (p1 and p3 not freed) */
    CHECK(leak_count == 2, "should have 2 leaks");

    /* Clean up */
    free(p1);
    free(p3);

    /* Now should have 0 leaks */
    leak_count = rt_malloc_redirect_track_leaks(leaks, sizes, 10);
    CHECK(leak_count == 0, "should have 0 leaks after freeing all");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Free policy - warn
 * ============================================================================ */
static void test_free_policy_warn(void)
{
    TEST("free policy warn");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.free_policy = RT_REDIRECT_FREE_WARN;

    rt_malloc_redirect_push(arena, &config);

    void *ptr = malloc(100);
    CHECK(ptr != NULL, "malloc failed");

    /* This should print a warning to stderr */
    fprintf(stderr, "  (Expecting warning below)\n");
    free(ptr);
    fprintf(stderr, "  (Warning above is expected)\n");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Zero on free
 * ============================================================================ */
static void test_zero_on_free(void)
{
    TEST("zero on free");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.zero_on_free = true;

    rt_malloc_redirect_push(arena, &config);

    char *str = (char *)malloc(100);
    CHECK(str != NULL, "malloc failed");

    /* Write a pattern */
    memset(str, 0xAA, 100);
    CHECK(str[50] == (char)0xAA, "pattern not written");

    /* Free should zero the memory */
    free(str);

    /* Memory should be zeroed (it's still valid because arena owns it).
     * We access after free intentionally - the arena still owns the memory.
     * Use volatile to prevent compiler from optimizing away or warning. */
    volatile char *vstr = str;
    CHECK(vstr[50] == 0, "memory not zeroed after free");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Mixed arena and system allocations
 * ============================================================================ */
static void test_mixed_allocations(void)
{
    TEST("mixed arena and system allocations");

    /* Allocate from system before redirect */
    char *sys_ptr = (char *)malloc(100);
    CHECK(sys_ptr != NULL, "system malloc failed");
    strcpy(sys_ptr, "system allocation");

    /* Start redirect */
    RtArena *arena = rt_arena_create(NULL);
    rt_malloc_redirect_push(arena, NULL);

    /* This should go to arena */
    char *arena_ptr = (char *)malloc(100);
    CHECK(arena_ptr != NULL, "redirected malloc failed");
    CHECK(rt_malloc_redirect_is_arena_ptr(arena_ptr), "should be arena ptr");
    strcpy(arena_ptr, "arena allocation");

    /* System pointer should not be detected as arena pointer */
    CHECK(!rt_malloc_redirect_is_arena_ptr(sys_ptr), "sys_ptr should not be arena ptr");

    /* Free system pointer should work (goes to real free) */
    free(sys_ptr);

    /* Free arena pointer (tracked as free but not actually freed) */
    free(arena_ptr);

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Statistics
 * ============================================================================ */
static void test_statistics(void)
{
    TEST("statistics tracking");

    RtArena *arena = rt_arena_create(NULL);
    rt_malloc_redirect_push(arena, NULL);

    RtRedirectStats stats;

    /* Initial stats */
    rt_malloc_redirect_get_stats(&stats);
    size_t initial_allocs = stats.alloc_count;

    /* Make allocations */
    void *p1 = malloc(100);
    void *p2 = malloc(200);
    void *p3 = calloc(10, 30);  /* 300 bytes */

    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.alloc_count == initial_allocs + 3, "alloc_count mismatch");
    CHECK(stats.total_requested >= 600, "total_requested should be >= 600");
    CHECK(stats.current_live == 3, "current_live should be 3");

    /* Free one */
    free(p2);
    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.free_count >= 1, "free_count should be >= 1");
    CHECK(stats.current_live == 2, "current_live should be 2");

    /* Realloc */
    void *p4 = realloc(p1, 500);
    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.realloc_count >= 1, "realloc_count should be >= 1");

    /* Check peak */
    CHECK(stats.peak_live >= 3, "peak_live should be >= 3");

    free(p3);
    free(p4);
    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Overflow policy - fallback to system malloc
 * ============================================================================ */
static void test_overflow_fallback(void)
{
    TEST("overflow policy fallback");

    RtArena *arena = rt_arena_create(NULL);

    /* Set max_arena_size larger than initial block (~65KB) but small enough
     * that a large allocation will exceed it */
    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.max_arena_size = 70000;  /* ~68KB - just above initial block */
    config.overflow_policy = RT_REDIRECT_OVERFLOW_FALLBACK;

    rt_malloc_redirect_push(arena, &config);

    /* First small allocation should fit in existing block */
    void *p1 = malloc(100);
    CHECK(p1 != NULL, "first malloc failed");
    CHECK(rt_malloc_redirect_is_arena_ptr(p1), "p1 should be arena ptr");

    /* This large allocation should exceed limit and fall back to system */
    void *p2 = malloc(10000);
    CHECK(p2 != NULL, "fallback malloc failed");
    CHECK(!rt_malloc_redirect_is_arena_ptr(p2), "p2 should NOT be arena ptr (fallback)");

    /* Check fallback was counted */
    RtRedirectStats stats;
    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.fallback_count >= 1, "fallback_count should be >= 1");

    /* Free the fallback pointer (goes to real free) */
    free(p2);

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Overflow policy - fail (return NULL)
 * ============================================================================ */
static void test_overflow_fail(void)
{
    TEST("overflow policy fail");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.max_arena_size = 70000;  /* ~68KB - just above initial block */
    config.overflow_policy = RT_REDIRECT_OVERFLOW_FAIL;

    rt_malloc_redirect_push(arena, &config);

    /* First small allocation should fit */
    void *p1 = malloc(100);
    CHECK(p1 != NULL, "first malloc failed");

    /* This large allocation should exceed limit and return NULL */
    void *p2 = malloc(10000);
    CHECK(p2 == NULL, "overflow should return NULL");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Overflow policy - grow (ignore limit)
 * ============================================================================ */
static void test_overflow_grow(void)
{
    TEST("overflow policy grow");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.max_arena_size = 70000;  /* Small limit, but GROW ignores it */
    config.overflow_policy = RT_REDIRECT_OVERFLOW_GROW;

    rt_malloc_redirect_push(arena, &config);

    /* First allocation should fit */
    void *p1 = malloc(100);
    CHECK(p1 != NULL, "first malloc failed");
    CHECK(rt_malloc_redirect_is_arena_ptr(p1), "p1 should be arena ptr");

    /* This would exceed limit but GROW policy ignores it */
    void *p2 = malloc(10000);
    CHECK(p2 != NULL, "grow malloc should succeed");
    CHECK(rt_malloc_redirect_is_arena_ptr(p2), "p2 should still be arena ptr");

    /* No fallbacks should have occurred */
    RtRedirectStats stats;
    rt_malloc_redirect_get_stats(&stats);
    CHECK(stats.fallback_count == 0, "fallback_count should be 0");

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Hash set operations (internal API)
 * ============================================================================ */
static void test_hash_set(void)
{
    TEST("hash set operations");

    RtAllocHashSet *set = rt_alloc_hash_set_create(16);
    CHECK(set != NULL, "hash set creation failed");

    /* Insert some pointers */
    void *p1 = (void *)0x1000;
    void *p2 = (void *)0x2000;
    void *p3 = (void *)0x3000;

    CHECK(rt_alloc_hash_set_insert(set, p1, 100), "insert p1 failed");
    CHECK(rt_alloc_hash_set_insert(set, p2, 200), "insert p2 failed");
    CHECK(rt_alloc_hash_set_insert(set, p3, 300), "insert p3 failed");

    /* Check contains */
    CHECK(rt_alloc_hash_set_contains(set, p1), "p1 should be in set");
    CHECK(rt_alloc_hash_set_contains(set, p2), "p2 should be in set");
    CHECK(rt_alloc_hash_set_contains(set, p3), "p3 should be in set");
    CHECK(!rt_alloc_hash_set_contains(set, (void *)0x4000), "0x4000 should not be in set");

    /* Check sizes */
    CHECK(rt_alloc_hash_set_get_size(set, p1) == 100, "p1 size mismatch");
    CHECK(rt_alloc_hash_set_get_size(set, p2) == 200, "p2 size mismatch");
    CHECK(rt_alloc_hash_set_get_size(set, p3) == 300, "p3 size mismatch");

    /* Remove one */
    CHECK(rt_alloc_hash_set_remove(set, p2), "remove p2 failed");
    CHECK(!rt_alloc_hash_set_contains(set, p2), "p2 should not be in set after remove");
    CHECK(rt_alloc_hash_set_contains(set, p1), "p1 should still be in set");
    CHECK(rt_alloc_hash_set_contains(set, p3), "p3 should still be in set");

    /* Test rehashing - insert many elements */
    for (int i = 0; i < 100; i++) {
        void *p = (void *)(uintptr_t)(0x10000 + i * 0x100);
        rt_alloc_hash_set_insert(set, p, i);
    }

    /* Verify all are still accessible */
    for (int i = 0; i < 100; i++) {
        void *p = (void *)(uintptr_t)(0x10000 + i * 0x100);
        CHECK(rt_alloc_hash_set_contains(set, p), "element lost after rehash");
        CHECK(rt_alloc_hash_set_get_size(set, p) == (size_t)i, "size lost after rehash");
    }

    rt_alloc_hash_set_destroy(set);
    PASS();
}

/* ============================================================================
 * Test: Thread-safe mode (basic)
 * ============================================================================ */
static void test_thread_safe_mode(void)
{
    TEST("thread-safe mode (basic)");

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.thread_safe = true;

    CHECK(rt_malloc_redirect_push(arena, &config), "push with thread_safe failed");

    /* Basic operations should still work */
    void *p1 = malloc(100);
    CHECK(p1 != NULL, "malloc failed in thread-safe mode");
    CHECK(rt_malloc_redirect_is_arena_ptr(p1), "should be arena ptr");

    void *p2 = calloc(10, 20);
    CHECK(p2 != NULL, "calloc failed in thread-safe mode");

    void *p3 = realloc(p1, 200);
    CHECK(p3 != NULL, "realloc failed in thread-safe mode");

    free(p2);
    free(p3);

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Test: Callbacks
 * ============================================================================ */
static int callback_alloc_count = 0;
static int callback_free_count = 0;

static void test_on_alloc(void *ptr, size_t size, void *user_data)
{
    (void)ptr;
    (void)size;
    int *counter = (int *)user_data;
    (*counter)++;
}

static void test_on_free(void *ptr, size_t size, void *user_data)
{
    (void)ptr;
    (void)size;
    int *counter = (int *)user_data;
    (*counter)++;
}

static void test_callbacks(void)
{
    TEST("allocation callbacks");

    callback_alloc_count = 0;
    callback_free_count = 0;

    RtArena *arena = rt_arena_create(NULL);

    RtRedirectConfig config = RT_REDIRECT_CONFIG_DEFAULT;
    config.on_alloc = test_on_alloc;
    config.on_free = test_on_free;
    config.callback_user_data = &callback_alloc_count;

    /* Note: using same counter for both for simplicity */
    rt_malloc_redirect_push(arena, &config);

    void *p1 = malloc(100);
    void *p2 = malloc(200);
    CHECK(callback_alloc_count == 2, "on_alloc not called correctly");

    /* For on_free callback, we need separate user_data, but for test we'll check alloc */
    free(p1);
    free(p2);

    rt_malloc_redirect_pop();
    rt_arena_destroy(arena);
    PASS();
}

/* ============================================================================
 * Main
 * ============================================================================ */
int main(void)
{
    printf("========================================\n");
    printf("Arena Malloc Redirect Tests\n");
    printf("========================================\n\n");

    /* Check if hooks are installed */
    if (!rt_malloc_redirect_hooks_installed()) {
        printf("WARNING: Malloc hooks not installed!\n");
        printf("Tests may not work correctly.\n\n");
    } else {
        printf("Malloc hooks: INSTALLED\n\n");
    }

    /* Run tests */
    test_basic_redirect();
    test_malloc_redirect();
    test_calloc_redirect();
    test_realloc_redirect();
    test_nested_redirect();
    test_tracking_config();
    test_free_policy_warn();
    test_zero_on_free();
    test_mixed_allocations();
    test_statistics();
    test_overflow_fallback();
    test_overflow_fail();
    test_overflow_grow();
    test_hash_set();
    test_thread_safe_mode();
    test_callbacks();

    /* Summary */
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
