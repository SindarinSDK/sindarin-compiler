/*
 * Arena V2 - Malloc Redirect Tests
 * =================================
 * Tests the malloc/free/calloc/realloc redirection to arena.
 *
 * Compile:
 *   gcc -o arena_v2_redirect_test \
 *       arena_v2.c arena_v2_redirect_test.c \
 *       ../malloc/runtime_malloc_hooks.c \
 *       -I.. -I../.. -lpthread
 *
 * On Windows with MinHook, also link the minhook sources.
 */

#include "../arena_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

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

/* Test: Basic malloc redirect */
static int test_malloc_redirect(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    /* Get initial stats */
    RtArenaV2Stats stats_before;
    rt_arena_stats_get(arena, &stats_before);

    /* Push redirect - malloc should now go to arena */
    rt_arena_v2_redirect_push(arena);

    /* Allocate via malloc (should go to arena) */
    void *ptr = malloc(100);
    if (ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Write to it to ensure it's valid memory */
    memset(ptr, 0xAB, 100);

    /* Get stats after - should show allocation */
    RtArenaV2Stats stats_after;
    rt_arena_stats_get(arena, &stats_after);

    /* Pop redirect */
    rt_arena_v2_redirect_pop();

    /* Verify allocation went to arena */
    int success = (stats_after.handles.total > stats_before.handles.total);

    /* Note: We don't call free(ptr) because it's arena-managed.
     * The arena destroy will clean it up. */

    rt_arena_v2_condemn(arena);
    return success;
}

/* Test: free() on redirected pointer marks handle dead */
static int test_free_redirect(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    /* Allocate and free */
    void *ptr = malloc(100);
    if (ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    RtArenaV2Stats stats_before_free;
    rt_arena_stats_get(arena, &stats_before_free);

    /* Free should mark handle as dead */
    free(ptr);

    /* Run GC to collect the dead handle */
    size_t collected = rt_arena_v2_gc(arena);

    rt_arena_v2_redirect_pop();

    /* Should have collected 1 handle */
    int success = (collected == 1);

    rt_arena_v2_condemn(arena);
    return success;
}

/* Test: calloc redirect (zeroed memory) */
static int test_calloc_redirect(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    /* Allocate zeroed memory */
    int *arr = calloc(10, sizeof(int));
    if (arr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Verify it's zeroed */
    int all_zero = 1;
    for (int i = 0; i < 10; i++) {
        if (arr[i] != 0) {
            all_zero = 0;
            break;
        }
    }

    /* Verify it went to arena */
    RtArenaV2Stats stats;
    rt_arena_stats_get(arena, &stats);
    int in_arena = (stats.handles.total >= 1);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return all_zero && in_arena;
}

/* Test: realloc redirect (grow) */
static int test_realloc_grow(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    /* Allocate initial */
    char *ptr = malloc(10);
    if (ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Write pattern */
    memcpy(ptr, "ABCDEFGHI", 10);

    /* Realloc to grow */
    char *new_ptr = realloc(ptr, 100);
    if (new_ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Verify data preserved */
    int data_ok = (memcmp(new_ptr, "ABCDEFGHI", 10) == 0);

    /* Can write to extended area */
    memset(new_ptr + 10, 'X', 90);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return data_ok;
}

/* Test: realloc(NULL, size) acts like malloc */
static int test_realloc_null(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    /* realloc(NULL, size) should act like malloc */
    void *ptr = realloc(NULL, 50);
    if (ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Verify it went to arena */
    RtArenaV2Stats stats;
    rt_arena_stats_get(arena, &stats);
    int in_arena = (stats.handles.total >= 1);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return in_arena;
}

/* Test: realloc(ptr, 0) acts like free */
static int test_realloc_zero(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    void *ptr = malloc(50);
    if (ptr == NULL) {
        rt_arena_v2_redirect_pop();
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* realloc(ptr, 0) should act like free */
    void *result = realloc(ptr, 0);

    /* Result should be NULL */
    int is_null = (result == NULL);

    /* GC should collect the freed handle */
    size_t collected = rt_arena_v2_gc(arena);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return is_null && (collected == 1);
}

/* Test: Nested redirect (push/push/pop/pop) */
static int test_nested_redirect(void)
{
    RtArenaV2 *arena1 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "arena1");
    RtArenaV2 *arena2 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "arena2");
    if (arena1 == NULL || arena2 == NULL) {
        if (arena1) rt_arena_v2_condemn(arena1);
        if (arena2) rt_arena_v2_condemn(arena2);
        return 0;
    }

    /* Push arena1 */
    rt_arena_v2_redirect_push(arena1);
    void *ptr1 = malloc(100);

    /* Push arena2 (nested) */
    rt_arena_v2_redirect_push(arena2);
    void *ptr2 = malloc(100);

    /* Pop arena2 */
    rt_arena_v2_redirect_pop();
    void *ptr3 = malloc(100);  /* Should go to arena1 */

    /* Pop arena1 */
    rt_arena_v2_redirect_pop();

    /* Check allocations went to correct arenas */
    RtArenaV2Stats stats1, stats2;
    rt_arena_stats_get(arena1, &stats1);
    rt_arena_stats_get(arena2, &stats2);

    /* arena1 should have 2 allocations (ptr1, ptr3) */
    /* arena2 should have 1 allocation (ptr2) */
    int success = (stats1.handles.total == 2 && stats2.handles.total == 1);

    rt_arena_v2_condemn(arena1);
    rt_arena_v2_condemn(arena2);

    return success;
}

/* Test: Non-redirected malloc still works */
static int test_passthrough(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    /* No redirect pushed - should use system malloc */
    void *ptr = malloc(100);
    if (ptr == NULL) {
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Arena should have no allocations */
    RtArenaV2Stats stats;
    rt_arena_stats_get(arena, &stats);
    int not_in_arena = (stats.handles.total == 0);

    /* Must free with system free since it didn't go to arena */
    free(ptr);

    rt_arena_v2_condemn(arena);
    return not_in_arena;
}

/* Test: Many allocations stress test */
static int test_many_allocations(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (arena == NULL) return 0;

    rt_arena_v2_redirect_push(arena);

    /* Allocate many small blocks */
    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = malloc(64);
        if (ptrs[i] == NULL) {
            rt_arena_v2_redirect_pop();
            rt_arena_v2_condemn(arena);
            return 0;
        }
        memset(ptrs[i], i, 64);
    }

    /* Free half of them */
    for (int i = 0; i < 50; i++) {
        free(ptrs[i]);
    }

    /* GC should collect 50 */
    size_t collected = rt_arena_v2_gc(arena);

    /* Verify remaining count */
    RtArenaV2Stats stats;
    rt_arena_stats_get(arena, &stats);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return (collected == 50 && stats.handles.total == 50);
}

/* ============================================================================
 * Multi-threaded Tests
 * ============================================================================ */

/* Shared state for thread tests */
typedef struct {
    RtArenaV2 *arena;
    int thread_id;
    int alloc_count;
    int success;
    void **ptrs;
} ThreadTestData;

/* Thread function: allocate to own arena, verify isolation */
static void *thread_allocate_to_arena(void *arg)
{
    ThreadTestData *data = (ThreadTestData *)arg;

    /* Push this thread's arena */
    rt_arena_v2_redirect_push(data->arena);

    /* Allocate several blocks */
    data->ptrs = malloc(data->alloc_count * sizeof(void *));
    for (int i = 0; i < data->alloc_count; i++) {
        data->ptrs[i] = malloc(64);
        if (data->ptrs[i] == NULL) {
            data->success = 0;
            rt_arena_v2_redirect_pop();
            return NULL;
        }
        /* Write thread ID to detect cross-thread corruption */
        memset(data->ptrs[i], data->thread_id, 64);
    }

    /* Verify all allocations went to our arena */
    RtArenaV2Stats stats;
    rt_arena_stats_get(data->arena, &stats);
    /* +1 for the ptrs array itself */
    data->success = (stats.handles.total >= (size_t)data->alloc_count);

    rt_arena_v2_redirect_pop();
    return NULL;
}

/* Test: Thread isolation - each thread's redirect only affects that thread */
static int test_thread_isolation(void)
{
    RtArenaV2 *arena1 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "thread1");
    RtArenaV2 *arena2 = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "thread2");
    if (!arena1 || !arena2) {
        if (arena1) rt_arena_v2_condemn(arena1);
        if (arena2) rt_arena_v2_condemn(arena2);
        return 0;
    }

    ThreadTestData data1 = { .arena = arena1, .thread_id = 1, .alloc_count = 10, .success = 0 };
    ThreadTestData data2 = { .arena = arena2, .thread_id = 2, .alloc_count = 10, .success = 0 };

    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread_allocate_to_arena, &data1);
    pthread_create(&t2, NULL, thread_allocate_to_arena, &data2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    /* Verify each arena has its allocations */
    RtArenaV2Stats stats1, stats2;
    rt_arena_stats_get(arena1, &stats1);
    rt_arena_stats_get(arena2, &stats2);

    int success = data1.success && data2.success;

    /* Verify data integrity - each thread's data should have its ID */
    for (int i = 0; i < 10 && success; i++) {
        unsigned char *p1 = (unsigned char *)data1.ptrs[i];
        unsigned char *p2 = (unsigned char *)data2.ptrs[i];
        for (int j = 0; j < 64; j++) {
            if (p1[j] != 1 || p2[j] != 2) {
                success = 0;
                break;
            }
        }
    }

    rt_arena_v2_condemn(arena1);
    rt_arena_v2_condemn(arena2);

    return success;
}

/* Thread function: allocate, then exit (tests cleanup) */
static void *thread_allocate_and_die(void *arg)
{
    ThreadTestData *data = (ThreadTestData *)arg;

    rt_arena_v2_redirect_push(data->arena);

    /* Allocate without freeing - thread exit should clean up */
    for (int i = 0; i < data->alloc_count; i++) {
        void *ptr = malloc(64);
        if (ptr) memset(ptr, 0xAB, 64);
    }

    data->success = 1;

    /* Don't pop - let thread exit handle cleanup */
    /* rt_arena_v2_redirect_pop(); */
    return NULL;
}

/* Test: Thread death cleanup - handles marked dead when thread exits */
static int test_thread_death_cleanup(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    if (!arena) return 0;

    ThreadTestData data = { .arena = arena, .thread_id = 1, .alloc_count = 5, .success = 0 };

    pthread_t t;
    pthread_create(&t, NULL, thread_allocate_and_die, &data);
    pthread_join(t, NULL);

    if (!data.success) {
        rt_arena_v2_condemn(arena);
        return 0;
    }

    /* Thread exited - cleanup should have marked handles dead */
    /* Run GC to collect the dead handles */
    size_t collected = rt_arena_v2_gc(arena);

    /* Should have collected the allocations from dead thread */
    int success = (collected == 5);

    rt_arena_v2_condemn(arena);
    return success;
}

/* Thread function for concurrent stress test */
static void *thread_stress(void *arg)
{
    ThreadTestData *data = (ThreadTestData *)arg;

    rt_arena_v2_redirect_push(data->arena);

    /* Rapid alloc/free cycles */
    for (int cycle = 0; cycle < 100; cycle++) {
        void *ptrs[10];
        for (int i = 0; i < 10; i++) {
            ptrs[i] = malloc(32 + (cycle % 64));
            if (ptrs[i]) {
                memset(ptrs[i], data->thread_id, 32);
            }
        }
        for (int i = 0; i < 10; i++) {
            free(ptrs[i]);
        }
    }

    data->success = 1;
    rt_arena_v2_redirect_pop();
    return NULL;
}

/* Test: Concurrent stress - many threads allocating/freeing rapidly */
static int test_concurrent_stress(void)
{
    #define NUM_THREADS 4
    RtArenaV2 *arenas[NUM_THREADS];
    ThreadTestData data[NUM_THREADS];
    pthread_t threads[NUM_THREADS];

    /* Create separate arena for each thread */
    for (int i = 0; i < NUM_THREADS; i++) {
        arenas[i] = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "stress");
        if (!arenas[i]) {
            for (int j = 0; j < i; j++) rt_arena_v2_condemn(arenas[j]);
            return 0;
        }
        data[i].arena = arenas[i];
        data[i].thread_id = i + 1;
        data[i].success = 0;
    }

    /* Launch all threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_stress, &data[i]);
    }

    /* Wait for all to complete */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Check all succeeded */
    int success = 1;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (!data[i].success) success = 0;
    }

    /* GC each arena - should collect all the freed handles */
    for (int i = 0; i < NUM_THREADS; i++) {
        rt_arena_v2_gc(arenas[i]);
        RtArenaV2Stats stats;
        rt_arena_stats_get(arenas[i], &stats);
        /* After GC, should have no live handles (all were freed) */
        if (stats.handles.total != 0) success = 0;
        rt_arena_v2_condemn(arenas[i]);
    }

    return success;
    #undef NUM_THREADS
}

/* Test: Main thread redirect doesn't affect spawned threads */
static void *thread_check_no_redirect(void *arg)
{
    int *result = (int *)arg;

    /* This thread should NOT have redirect active */
    RtArenaV2 *current = rt_arena_v2_redirect_current();
    *result = (current == NULL);

    /* Allocate with system malloc */
    void *ptr = malloc(100);
    if (ptr) {
        memset(ptr, 0, 100);
        free(ptr);
    }

    return NULL;
}

static int test_redirect_not_inherited(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    if (!arena) return 0;

    /* Push redirect in main thread */
    rt_arena_v2_redirect_push(arena);

    /* Spawn thread - should NOT inherit redirect */
    int thread_result = 0;
    pthread_t t;
    pthread_create(&t, NULL, thread_check_no_redirect, &thread_result);
    pthread_join(t, NULL);

    rt_arena_v2_redirect_pop();
    rt_arena_v2_condemn(arena);

    return thread_result;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("Arena V2 Redirect Tests\n");
    printf("=======================\n\n");

    printf("--- Single-threaded Tests ---\n");
    TEST(malloc_redirect);
    TEST(free_redirect);
    TEST(calloc_redirect);
    TEST(realloc_grow);
    TEST(realloc_null);
    TEST(realloc_zero);
    TEST(nested_redirect);
    TEST(passthrough);
    TEST(many_allocations);

    printf("\n--- Multi-threaded Tests ---\n");
    TEST(thread_isolation);
    TEST(thread_death_cleanup);
    TEST(concurrent_stress);
    TEST(redirect_not_inherited);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
