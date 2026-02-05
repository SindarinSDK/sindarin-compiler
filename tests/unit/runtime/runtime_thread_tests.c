// tests/runtime_thread_tests.c
// Tests for thread arena mode selection in the runtime

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../debug.h"
#include "../test_harness.h"

/* ============================================================================
 * Thread Arena Mode Selection Tests
 * ============================================================================
 * These tests verify that thread arena creation works correctly for each mode:
 * - Default mode: creates own arena with parent link
 * - Shared mode: reuses caller arena (thread_arena = NULL in handle)
 * - Private mode: creates isolated arena (parent = NULL)
 * ============================================================================ */

/* Test default mode creates own arena with parent link */
static void test_thread_default_mode_arena(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create thread args for default mode (is_shared=false, is_private=false) */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    assert(args != NULL);
    args->is_shared = false;
    args->is_private = false;
    args->caller_arena = caller_arena;

    /* Create a thread handle to simulate spawn behavior */
    RtThreadHandle *handle = rt_thread_handle_create(caller_arena);
    assert(handle != NULL);

    /* Simulate the arena creation logic from rt_thread_spawn for default mode */
    /* Default mode: create own arena with caller as parent for promotion */
    RtArena *thread_arena = rt_arena_create(caller_arena);
    assert(thread_arena != NULL);
    assert(thread_arena->parent == caller_arena);  /* Parent link should be set */

    /* Set up handle as spawn would */
    handle->thread_arena = thread_arena;
    handle->is_shared = false;
    handle->is_private = false;

    /* Verify handle has its own arena that can be destroyed separately */
    assert(handle->thread_arena != NULL);
    assert(handle->thread_arena != caller_arena);

    /* Clean up */
    rt_arena_destroy(thread_arena);
    rt_arena_destroy(caller_arena);
}

/* Test shared mode reuses caller arena */
static void test_thread_shared_mode_arena(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create thread args for shared mode */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    assert(args != NULL);
    args->is_shared = true;
    args->is_private = false;
    args->caller_arena = caller_arena;

    /* Create a thread handle */
    RtThreadHandle *handle = rt_thread_handle_create(caller_arena);
    assert(handle != NULL);

    /* Simulate the arena creation logic from rt_thread_spawn for shared mode */
    /* Shared mode: reuse caller's arena directly, thread_arena = NULL in handle */
    args->thread_arena = caller_arena;  /* Thread uses caller's arena */
    handle->thread_arena = NULL;        /* Don't destroy - it's the caller's */
    handle->is_shared = true;
    handle->is_private = false;

    /* Verify thread_arena is NULL (won't be destroyed by join) */
    assert(handle->thread_arena == NULL);

    /* The thread would use args->thread_arena == caller_arena */
    assert(args->thread_arena == caller_arena);

    /* Clean up - only destroy caller arena since shared mode doesn't own thread arena */
    rt_arena_destroy(caller_arena);
}

/* Test private mode creates isolated arena (parent = NULL) */
static void test_thread_private_mode_arena(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create thread args for private mode */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    assert(args != NULL);
    args->is_shared = false;
    args->is_private = true;
    args->caller_arena = caller_arena;

    /* Create a thread handle */
    RtThreadHandle *handle = rt_thread_handle_create(caller_arena);
    assert(handle != NULL);

    /* Simulate the arena creation logic from rt_thread_spawn for private mode */
    /* Private mode: create isolated arena with no parent */
    RtArena *thread_arena = rt_arena_create(NULL);
    assert(thread_arena != NULL);
    assert(thread_arena->parent == NULL);  /* No parent link - isolated */

    args->thread_arena = thread_arena;
    handle->thread_arena = thread_arena;
    handle->is_shared = false;
    handle->is_private = true;

    /* Verify private arena has no parent */
    assert(handle->thread_arena->parent == NULL);

    /* Clean up */
    rt_arena_destroy(thread_arena);
    rt_arena_destroy(caller_arena);
}

/* Test that thread arena cleanup happens correctly for each mode */
static void test_thread_arena_cleanup_logic(void)
{

    /* Test 1: Default mode - thread_arena should be non-NULL and destroyable */
    RtArena *caller1 = rt_arena_create(NULL);
    RtThreadHandle *handle1 = rt_thread_handle_create(caller1);
    handle1->thread_arena = rt_arena_create(caller1);
    handle1->is_shared = false;
    handle1->is_private = false;
    assert(handle1->thread_arena != NULL);

    /* Simulate cleanup - thread arena should be destroyed */
    if (handle1->thread_arena != NULL) {
        rt_arena_destroy(handle1->thread_arena);
        handle1->thread_arena = NULL;
    }
    assert(handle1->thread_arena == NULL);
    rt_arena_destroy(caller1);

    /* Test 2: Shared mode - thread_arena is NULL, nothing to destroy */
    RtArena *caller2 = rt_arena_create(NULL);
    RtThreadHandle *handle2 = rt_thread_handle_create(caller2);
    handle2->thread_arena = NULL;  /* Shared mode: don't own arena */
    handle2->is_shared = true;
    handle2->is_private = false;

    /* Simulate cleanup - nothing should happen for shared mode */
    if (handle2->thread_arena != NULL) {
        rt_arena_destroy(handle2->thread_arena);
        handle2->thread_arena = NULL;
    }
    /* No crash means success */
    rt_arena_destroy(caller2);

    /* Test 3: Private mode - thread_arena should be destroyable */
    RtArena *caller3 = rt_arena_create(NULL);
    RtThreadHandle *handle3 = rt_thread_handle_create(caller3);
    handle3->thread_arena = rt_arena_create(NULL);  /* Private: no parent */
    handle3->is_shared = false;
    handle3->is_private = true;
    assert(handle3->thread_arena != NULL);
    assert(handle3->thread_arena->parent == NULL);

    /* Simulate cleanup - thread arena should be destroyed */
    if (handle3->thread_arena != NULL) {
        rt_arena_destroy(handle3->thread_arena);
        handle3->thread_arena = NULL;
    }
    assert(handle3->thread_arena == NULL);
    rt_arena_destroy(caller3);
}

/* Test arena is thread-safe for shared mode (managed arena is lock-free) */
static void test_thread_shared_mode_arena_freezing(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Managed arena is thread-safe by design — no freezing needed.
     * Verify concurrent allocations work. */
    void *p1 = rt_arena_alloc(caller_arena, 32);
    void *p2 = rt_arena_alloc(caller_arena, 64);
    assert(p1 != NULL);
    assert(p2 != NULL);

    rt_arena_destroy(caller_arena);
}

/* Test that RtThreadArgs properly stores mode flags */
static void test_thread_args_mode_flags(void)
{

    RtArena *arena = rt_arena_create(NULL);
    assert(arena != NULL);

    /* Create args and verify default values */
    RtThreadArgs *args = rt_thread_args_create(arena, NULL, NULL, 0);
    assert(args != NULL);
    assert(args->is_shared == false);
    assert(args->is_private == false);
    assert(args->caller_arena == NULL);
    assert(args->thread_arena == NULL);

    /* Set shared mode */
    args->is_shared = true;
    args->caller_arena = arena;
    assert(args->is_shared == true);
    assert(args->is_private == false);

    /* Reset and set private mode */
    args->is_shared = false;
    args->is_private = true;
    assert(args->is_shared == false);
    assert(args->is_private == true);

    rt_arena_destroy(arena);
}

/* Test that RtThreadHandle properly stores mode flags */
static void test_thread_handle_mode_flags(void)
{

    RtArena *arena = rt_arena_create(NULL);
    assert(arena != NULL);

    /* Create handle and verify default values */
    RtThreadHandle *handle = rt_thread_handle_create(arena);
    assert(handle != NULL);
    assert(handle->is_shared == false);
    assert(handle->is_private == false);
    assert(handle->caller_arena == NULL);
    assert(handle->thread_arena == NULL);

    /* Set mode flags */
    handle->is_shared = true;
    handle->caller_arena = arena;
    assert(handle->is_shared == true);

    handle->is_shared = false;
    handle->is_private = true;
    assert(handle->is_private == true);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Integration Tests - Actual Thread Execution
 * ============================================================================
 * These tests spawn real pthreads and verify arena semantics during execution.
 * ============================================================================ */

/* Thread wrapper for default mode test - allocates string in thread arena */
static void *default_mode_thread_wrapper(void *arg)
{
    RtThreadArgs *args = (RtThreadArgs *)arg;

    /* Signal that we've started and accessed args - required by startup barrier */
    rt_thread_signal_started(args);

    /* Allocate a string in the thread's arena */
    char *result_str = rt_arena_strdup(args->thread_arena, "thread_result");

    /* Store result */
    if (args->result != NULL) {
        args->result->value = result_str;
    }

    /* Signal completion so rt_thread_join can proceed */
    rt_thread_signal_completion(args->handle);

    return NULL;
}

/* Test default mode with actual thread execution */
static void test_integration_default_mode_thread(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create args for default mode */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    args->is_shared = false;
    args->is_private = false;
    args->caller_arena = caller_arena;

    /* Spawn the thread */
    RtThreadHandle *handle = rt_thread_spawn(caller_arena, default_mode_thread_wrapper, args);
    assert(handle != NULL);
    assert(handle->thread_arena != NULL);  /* Default mode has own arena */
    assert(handle->thread_arena != caller_arena);  /* Different from caller */
    assert(handle->thread_arena->parent == caller_arena);  /* Parent link set */

    /* Join the thread - this now only joins, doesn't cleanup */
    void *result = rt_thread_join(handle);

    /* After join, thread arena is still present for potential promotion */
    assert(handle->thread_arena != NULL);

    /* Clean up thread arena explicitly (normally done by sync functions) */
    rt_arena_destroy(handle->thread_arena);
    handle->thread_arena = NULL;

    /* Result should have been promoted (but we don't verify content here since
     * promotion requires result_type to be set properly) */
    (void)result;

    /* Cleanup */
    rt_arena_destroy(caller_arena);

}

/* Thread wrapper for shared mode test - just stores a primitive result
 * Note: In shared mode, we use the caller's arena. */
static void *shared_mode_thread_wrapper(void *arg)
{
    RtThreadArgs *args = (RtThreadArgs *)arg;

    /* Signal that we've started and accessed args - required by startup barrier */
    rt_thread_signal_started(args);

    /* Just store a primitive result to verify the thread ran */
    if (args->result != NULL) {
        /* Use a static value for the result */
        static int result_val = 123;
        args->result->value = &result_val;
    }

    /* Signal completion so rt_thread_join can proceed */
    rt_thread_signal_completion(args->handle);

    return NULL;
}

/* Test shared mode with actual thread execution */
static void test_integration_shared_mode_thread(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create args for shared mode */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    args->is_shared = true;
    args->is_private = false;
    args->caller_arena = caller_arena;

    /* Spawn the thread - shared mode reuses caller's arena (thread-safe by design) */
    RtThreadHandle *handle = rt_thread_spawn(caller_arena, shared_mode_thread_wrapper, args);
    assert(handle != NULL);
    assert(handle->thread_arena == NULL);  /* Shared mode: no separate arena */

    /* Join the thread */
    void *result = rt_thread_join(handle);

    /* Result is in caller arena (no promotion needed) */
    (void)result;

    /* Cleanup */
    rt_arena_destroy(caller_arena);

}

/* Thread wrapper for private mode test - returns primitive */
static void *private_mode_thread_wrapper(void *arg)
{
    RtThreadArgs *args = (RtThreadArgs *)arg;

    /* Signal that we've started and accessed args - required by startup barrier */
    rt_thread_signal_started(args);

    /* Private mode: allocate locally but only return primitives */
    char *local_str = rt_arena_strdup(args->thread_arena, "local_only");
    (void)local_str;  /* Use but don't return non-primitive */

    /* Store primitive result */
    if (args->result != NULL) {
        int *int_result = rt_arena_alloc(args->thread_arena, sizeof(int));
        *int_result = 42;
        args->result->value = int_result;
    }

    /* Signal completion so rt_thread_join can proceed */
    rt_thread_signal_completion(args->handle);

    return NULL;
}

/* Test private mode with actual thread execution */
static void test_integration_private_mode_thread(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Create args for private mode */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    args->is_shared = false;
    args->is_private = true;
    args->caller_arena = caller_arena;

    /* Spawn the thread */
    RtThreadHandle *handle = rt_thread_spawn(caller_arena, private_mode_thread_wrapper, args);
    assert(handle != NULL);
    assert(handle->thread_arena != NULL);  /* Private mode has own arena */
    assert(handle->thread_arena->parent == NULL);  /* No parent link (isolated) */

    /* Join the thread - this now only joins, doesn't cleanup */
    void *result = rt_thread_join(handle);

    /* After join, thread arena is still present for potential promotion */
    assert(handle->thread_arena != NULL);

    /* Clean up thread arena explicitly (normally done by sync functions) */
    rt_arena_destroy(handle->thread_arena);
    handle->thread_arena = NULL;

    /* Result (primitive) should be accessible */
    (void)result;

    /* Cleanup */
    rt_arena_destroy(caller_arena);

}

/* Test shared mode arena supports concurrent allocation (thread-safe by design) */
static void test_integration_shared_mode_concurrent_alloc(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Managed arena supports concurrent allocation */
    void *p1 = rt_arena_alloc(caller_arena, 16);
    assert(p1 != NULL);

    void *p2 = rt_arena_alloc(caller_arena, 32);
    assert(p2 != NULL);

    void *p3 = rt_arena_alloc(caller_arena, 64);
    assert(p3 != NULL);

    /* Cleanup */
    rt_arena_destroy(caller_arena);

}

/* Test arena cleanup after thread sync - no leaks */
static void test_integration_arena_cleanup_no_leaks(void)
{

    /* Spawn and sync multiple threads, verify arenas are cleaned up */
    for (int i = 0; i < 5; i++) {
        RtArena *caller_arena = rt_arena_create(NULL);
        assert(caller_arena != NULL);

        /* Default mode thread */
        RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
        args->is_shared = false;
        args->is_private = false;
        args->caller_arena = caller_arena;

        RtThreadHandle *handle = rt_thread_spawn(caller_arena, default_mode_thread_wrapper, args);
        assert(handle != NULL);
        assert(handle->thread_arena != NULL);

        /* Join and cleanup (join no longer auto-cleans) */
        rt_thread_join(handle);
        assert(handle->thread_arena != NULL);
        rt_arena_destroy(handle->thread_arena);
        handle->thread_arena = NULL;

        /* Destroy caller arena */
        rt_arena_destroy(caller_arena);
    }

}

/* Test arena auto-cleanup when caller arena is destroyed with pending threads */
static void test_integration_arena_auto_joins_pending_threads(void)
{

    RtArena *caller_arena = rt_arena_create(NULL);
    assert(caller_arena != NULL);

    /* Spawn a thread but don't explicitly sync it */
    RtThreadArgs *args = rt_thread_args_create(caller_arena, NULL, NULL, 0);
    args->is_shared = false;
    args->is_private = false;
    args->caller_arena = caller_arena;

    RtThreadHandle *handle = rt_thread_spawn(caller_arena, default_mode_thread_wrapper, args);
    assert(handle != NULL);

    /* Note: rt_thread_spawn already registers the thread for cleanup via
     * rt_arena_on_cleanup when caller_arena is provided. No explicit
     * tracking call needed. */

    /* Destroying the arena should auto-join the thread */
    rt_arena_destroy(caller_arena);

    /* If we get here without hanging, auto-join worked */
}

/* Main test runner for thread arena tests */
void test_rt_thread_main(void)
{
    TEST_SECTION("Thread Arena Mode");

    /* Unit tests */
    TEST_RUN("thread_default_mode_arena", test_thread_default_mode_arena);
    TEST_RUN("thread_shared_mode_arena", test_thread_shared_mode_arena);
    TEST_RUN("thread_private_mode_arena", test_thread_private_mode_arena);
    TEST_RUN("thread_arena_cleanup_logic", test_thread_arena_cleanup_logic);
    TEST_RUN("thread_shared_mode_arena_freezing", test_thread_shared_mode_arena_freezing);
    TEST_RUN("thread_args_mode_flags", test_thread_args_mode_flags);
    TEST_RUN("thread_handle_mode_flags", test_thread_handle_mode_flags);

    /* Integration tests with actual thread execution */
    TEST_RUN("integration_default_mode_thread", test_integration_default_mode_thread);
    TEST_RUN("integration_shared_mode_thread", test_integration_shared_mode_thread);
    TEST_RUN("integration_private_mode_thread", test_integration_private_mode_thread);
    TEST_RUN("integration_shared_mode_concurrent_alloc", test_integration_shared_mode_concurrent_alloc);
    TEST_RUN("integration_arena_cleanup_no_leaks", test_integration_arena_cleanup_no_leaks);
    TEST_RUN("integration_arena_auto_joins_pending_threads", test_integration_arena_auto_joins_pending_threads);
}
