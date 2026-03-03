// tests/unit/runtime/runtime_string_tests.c
// Tests for runtime string operations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Type to String Conversion Tests (V2)
 * ============================================================================ */

static void test_rt_to_string_string_v2(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

    RtHandleV2 *hello = rt_arena_v2_strdup(arena, "hello");
    RtHandleV2 *result = rt_to_string_string_v2(arena, hello);
    rt_handle_begin_transaction(result);
    assert(strcmp((const char *)result->ptr, "hello") == 0);
    rt_handle_end_transaction(result);

    RtHandleV2 *empty = rt_arena_v2_strdup(arena, "");
    result = rt_to_string_string_v2(arena, empty);
    rt_handle_begin_transaction(result);
    assert(strcmp((const char *)result->ptr, "") == 0);
    rt_handle_end_transaction(result);

    result = rt_to_string_string_v2(arena, NULL);
    rt_handle_begin_transaction(result);
    assert(strcmp((const char *)result->ptr, "") == 0);
    rt_handle_end_transaction(result);

    rt_arena_v2_condemn(arena);
}

static void test_rt_to_string_pointer_v2(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

    RtHandleV2 *result = rt_to_string_pointer_v2(arena, NULL);
    rt_handle_begin_transaction(result);
    assert(strcmp((const char *)result->ptr, "nil") == 0);
    rt_handle_end_transaction(result);

    /* Create a handle to test non-NULL pointer conversion */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, sizeof(int));
    result = rt_to_string_pointer_v2(arena, handle);
    rt_handle_begin_transaction(result);
    assert(result->ptr != NULL);
    assert(strlen((const char *)result->ptr) > 0);
    assert(strcmp((const char *)result->ptr, "nil") != 0);
    rt_handle_end_transaction(result);

    rt_arena_v2_condemn(arena);
}

static void test_rt_to_string_void_v2(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

    RtHandleV2 *result = rt_to_string_void_v2(arena);
    rt_handle_begin_transaction(result);
    assert(strcmp((const char *)result->ptr, "void") == 0);
    rt_handle_end_transaction(result);

    rt_arena_v2_condemn(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_string_main(void)
{
    TEST_SECTION("Runtime String");

    TEST_RUN("rt_to_string_string_v2", test_rt_to_string_string_v2);
    TEST_RUN("rt_to_string_pointer_v2", test_rt_to_string_pointer_v2);
    TEST_RUN("rt_to_string_void_v2", test_rt_to_string_void_v2);
}
