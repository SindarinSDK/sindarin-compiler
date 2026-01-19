// tests/unit/runtime/runtime_array_tests.c
// Tests for runtime array operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Array Clear Tests
 * ============================================================================ */

static void test_rt_array_clear(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create and populate array */
    long long *arr = rt_array_alloc_long(arena, 5, 42);
    assert(rt_array_length(arr) == 5);

    /* Clear the array */
    rt_array_clear(arr);
    assert(rt_array_length(arr) == 0);

    /* Should be able to push after clear */
    arr = rt_array_push_long(arena, arr, 100);
    assert(rt_array_length(arr) == 1);
    assert(arr[0] == 100);

    /* Clear NULL should not crash */
    rt_array_clear(NULL);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Push Tests
 * ============================================================================ */

static void test_rt_array_push_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Start with empty array */
    long long *arr = rt_array_alloc_long(arena, 0, 0);
    assert(rt_array_length(arr) == 0);

    /* Push elements */
    arr = rt_array_push_long(arena, arr, 10);
    assert(rt_array_length(arr) == 1);
    assert(arr[0] == 10);

    arr = rt_array_push_long(arena, arr, 20);
    assert(rt_array_length(arr) == 2);
    assert(arr[1] == 20);

    arr = rt_array_push_long(arena, arr, 30);
    assert(rt_array_length(arr) == 3);
    assert(arr[2] == 30);

    /* Push many to test capacity growth */
    for (int i = 0; i < 100; i++) {
        arr = rt_array_push_long(arena, arr, i * 10);
    }
    assert(rt_array_length(arr) == 103);

    rt_arena_destroy(arena);
}

static void test_rt_array_push_double(void)
{
    RtArena *arena = rt_arena_create(NULL);

    double *arr = rt_array_alloc_double(arena, 0, 0.0);

    arr = rt_array_push_double(arena, arr, 1.5);
    arr = rt_array_push_double(arena, arr, 2.5);
    arr = rt_array_push_double(arena, arr, 3.5);

    assert(rt_array_length(arr) == 3);
    assert(arr[0] == 1.5);
    assert(arr[1] == 2.5);
    assert(arr[2] == 3.5);

    rt_arena_destroy(arena);
}

static void test_rt_array_push_char(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *arr = rt_array_alloc_char(arena, 0, 0);

    arr = rt_array_push_char(arena, arr, 'a');
    arr = rt_array_push_char(arena, arr, 'b');
    arr = rt_array_push_char(arena, arr, 'c');

    assert(rt_array_length(arr) == 3);
    assert(arr[0] == 'a');
    assert(arr[1] == 'b');
    assert(arr[2] == 'c');

    rt_arena_destroy(arena);
}

static void test_rt_array_push_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);

    arr = rt_array_push_string(arena, arr, "hello");
    arr = rt_array_push_string(arena, arr, "world");
    arr = rt_array_push_string(arena, arr, "test");

    assert(rt_array_length(arr) == 3);
    assert(strcmp(arr[0], "hello") == 0);
    assert(strcmp(arr[1], "world") == 0);
    assert(strcmp(arr[2], "test") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_push_byte(void)
{
    RtArena *arena = rt_arena_create(NULL);

    unsigned char *arr = rt_array_alloc_byte(arena, 0, 0);

    arr = rt_array_push_byte(arena, arr, 0xFF);
    arr = rt_array_push_byte(arena, arr, 0x00);
    arr = rt_array_push_byte(arena, arr, 0xAB);

    assert(rt_array_length(arr) == 3);
    assert(arr[0] == 0xFF);
    assert(arr[1] == 0x00);
    assert(arr[2] == 0xAB);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Pop Tests
 * ============================================================================ */

static void test_rt_array_pop_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 0, 0);
    arr = rt_array_push_long(arena, arr, 10);
    arr = rt_array_push_long(arena, arr, 20);
    arr = rt_array_push_long(arena, arr, 30);

    assert(rt_array_length(arr) == 3);

    long val = rt_array_pop_long(arr);
    assert(val == 30);
    assert(rt_array_length(arr) == 2);

    val = rt_array_pop_long(arr);
    assert(val == 20);
    assert(rt_array_length(arr) == 1);

    val = rt_array_pop_long(arr);
    assert(val == 10);
    assert(rt_array_length(arr) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_pop_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "first");
    arr = rt_array_push_string(arena, arr, "second");
    arr = rt_array_push_string(arena, arr, "third");

    char *val = rt_array_pop_string(arr);
    assert(strcmp(val, "third") == 0);
    assert(rt_array_length(arr) == 2);

    val = rt_array_pop_string(arr);
    assert(strcmp(val, "second") == 0);
    assert(rt_array_length(arr) == 1);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Concat Tests
 * ============================================================================ */

static void test_rt_array_concat_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr1 = rt_array_alloc_long(arena, 3, 0);
    arr1[0] = 1; arr1[1] = 2; arr1[2] = 3;

    long long *arr2 = rt_array_alloc_long(arena, 2, 0);
    arr2[0] = 4; arr2[1] = 5;

    long long *result = rt_array_concat_long(arena, arr1, arr2);
    assert(rt_array_length(result) == 5);
    assert(result[0] == 1);
    assert(result[1] == 2);
    assert(result[2] == 3);
    assert(result[3] == 4);
    assert(result[4] == 5);

    /* Original arrays unchanged */
    assert(rt_array_length(arr1) == 3);
    assert(rt_array_length(arr2) == 2);

    /* Concat with empty array */
    long long *empty = rt_array_alloc_long(arena, 0, 0);
    result = rt_array_concat_long(arena, arr1, empty);
    assert(rt_array_length(result) == 3);

    result = rt_array_concat_long(arena, empty, arr2);
    assert(rt_array_length(result) == 2);

    rt_arena_destroy(arena);
}

static void test_rt_array_concat_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr1 = rt_array_alloc_string(arena, 0, NULL);
    arr1 = rt_array_push_string(arena, arr1, "a");
    arr1 = rt_array_push_string(arena, arr1, "b");

    char **arr2 = rt_array_alloc_string(arena, 0, NULL);
    arr2 = rt_array_push_string(arena, arr2, "c");
    arr2 = rt_array_push_string(arena, arr2, "d");

    char **result = rt_array_concat_string(arena, arr1, arr2);
    assert(rt_array_length(result) == 4);
    assert(strcmp(result[0], "a") == 0);
    assert(strcmp(result[1], "b") == 0);
    assert(strcmp(result[2], "c") == 0);
    assert(strcmp(result[3], "d") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Slice Tests
 * ============================================================================ */

static void test_rt_array_slice_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i * 10;  /* 0, 10, 20, 30, 40 */

    /* Basic slice [1:4] */
    long long *slice = rt_array_slice_long(arena, arr, 1, 4, 1);
    assert(rt_array_length(slice) == 3);
    assert(slice[0] == 10);
    assert(slice[1] == 20);
    assert(slice[2] == 30);

    /* Slice with step [0:5:2] */
    slice = rt_array_slice_long(arena, arr, 0, 5, 2);
    assert(rt_array_length(slice) == 3);
    assert(slice[0] == 0);
    assert(slice[1] == 20);
    assert(slice[2] == 40);

    /* Negative indices */
    slice = rt_array_slice_long(arena, arr, -3, -1, 1);
    assert(rt_array_length(slice) == 2);
    assert(slice[0] == 20);
    assert(slice[1] == 30);

    /* Full slice */
    slice = rt_array_slice_long(arena, arr, 0, 5, 1);
    assert(rt_array_length(slice) == 5);

    rt_arena_destroy(arena);
}

static void test_rt_array_slice_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "a");
    arr = rt_array_push_string(arena, arr, "b");
    arr = rt_array_push_string(arena, arr, "c");
    arr = rt_array_push_string(arena, arr, "d");
    arr = rt_array_push_string(arena, arr, "e");

    char **slice = rt_array_slice_string(arena, arr, 1, 4, 1);
    assert(rt_array_length(slice) == 3);
    assert(strcmp(slice[0], "b") == 0);
    assert(strcmp(slice[1], "c") == 0);
    assert(strcmp(slice[2], "d") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Reverse Tests
 * ============================================================================ */

static void test_rt_array_rev_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i + 1;  /* 1, 2, 3, 4, 5 */

    long long *rev = rt_array_rev_long(arena, arr);
    assert(rt_array_length(rev) == 5);
    assert(rev[0] == 5);
    assert(rev[1] == 4);
    assert(rev[2] == 3);
    assert(rev[3] == 2);
    assert(rev[4] == 1);

    /* Original unchanged */
    assert(arr[0] == 1);
    assert(arr[4] == 5);

    /* Empty array */
    long long *empty = rt_array_alloc_long(arena, 0, 0);
    rev = rt_array_rev_long(arena, empty);
    assert(rt_array_length(rev) == 0);

    /* Single element */
    long long *single = rt_array_alloc_long(arena, 1, 42);
    rev = rt_array_rev_long(arena, single);
    assert(rt_array_length(rev) == 1);
    assert(rev[0] == 42);

    rt_arena_destroy(arena);
}

static void test_rt_array_rev_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "first");
    arr = rt_array_push_string(arena, arr, "second");
    arr = rt_array_push_string(arena, arr, "third");

    char **rev = rt_array_rev_string(arena, arr);
    assert(rt_array_length(rev) == 3);
    assert(strcmp(rev[0], "third") == 0);
    assert(strcmp(rev[1], "second") == 0);
    assert(strcmp(rev[2], "first") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Remove Tests
 * ============================================================================ */

static void test_rt_array_rem_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i + 1;  /* 1, 2, 3, 4, 5 */

    /* Remove middle element */
    long long *result = rt_array_rem_long(arena, arr, 2);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 1);
    assert(result[1] == 2);
    assert(result[2] == 4);
    assert(result[3] == 5);

    /* Remove first element */
    result = rt_array_rem_long(arena, arr, 0);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 2);

    /* Remove last element */
    result = rt_array_rem_long(arena, arr, 4);
    assert(rt_array_length(result) == 4);
    assert(result[3] == 4);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Insert Tests
 * ============================================================================ */

static void test_rt_array_ins_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 3, 0);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;

    /* Insert in middle */
    long long *result = rt_array_ins_long(arena, arr, 99, 1);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 1);
    assert(result[1] == 99);
    assert(result[2] == 2);
    assert(result[3] == 3);

    /* Insert at beginning */
    result = rt_array_ins_long(arena, arr, 0, 0);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 0);
    assert(result[1] == 1);

    /* Insert at end */
    result = rt_array_ins_long(arena, arr, 100, 3);
    assert(rt_array_length(result) == 4);
    assert(result[3] == 100);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array IndexOf Tests
 * ============================================================================ */

static void test_rt_array_indexOf_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 20; arr[4] = 40;

    /* Find existing element (returns first occurrence) */
    assert(rt_array_indexOf_long(arr, 20) == 1);
    assert(rt_array_indexOf_long(arr, 10) == 0);
    assert(rt_array_indexOf_long(arr, 40) == 4);

    /* Element not found */
    assert(rt_array_indexOf_long(arr, 99) == -1);

    /* Empty array */
    long long *empty = rt_array_alloc_long(arena, 0, 0);
    assert(rt_array_indexOf_long(empty, 10) == -1);

    /* NULL array */
    assert(rt_array_indexOf_long(NULL, 10) == -1);

    rt_arena_destroy(arena);
}

static void test_rt_array_indexOf_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "apple");
    arr = rt_array_push_string(arena, arr, "banana");
    arr = rt_array_push_string(arena, arr, "cherry");

    assert(rt_array_indexOf_string(arr, "banana") == 1);
    assert(rt_array_indexOf_string(arr, "apple") == 0);
    assert(rt_array_indexOf_string(arr, "cherry") == 2);
    assert(rt_array_indexOf_string(arr, "grape") == -1);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Contains Tests
 * ============================================================================ */

static void test_rt_array_contains_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50;

    assert(rt_array_contains_long(arr, 30) == 1);
    assert(rt_array_contains_long(arr, 10) == 1);
    assert(rt_array_contains_long(arr, 50) == 1);
    assert(rt_array_contains_long(arr, 99) == 0);
    assert(rt_array_contains_long(NULL, 10) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_contains_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "red");
    arr = rt_array_push_string(arena, arr, "green");
    arr = rt_array_push_string(arena, arr, "blue");

    assert(rt_array_contains_string(arr, "green") == 1);
    assert(rt_array_contains_string(arr, "yellow") == 0);
    assert(rt_array_contains_string(NULL, "red") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Clone Tests
 * ============================================================================ */

static void test_rt_array_clone_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i * 10;

    long long *clone = rt_array_clone_long(arena, arr);
    assert(rt_array_length(clone) == 5);
    assert(clone != arr);  /* Different memory */

    for (int i = 0; i < 5; i++) {
        assert(clone[i] == arr[i]);
    }

    /* Modify original, clone unchanged */
    arr[0] = 999;
    assert(clone[0] == 0);

    /* Clone NULL returns NULL */
    long long *null_clone = rt_array_clone_long(arena, NULL);
    assert(null_clone == NULL);

    rt_arena_destroy(arena);
}

static void test_rt_array_clone_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "one");
    arr = rt_array_push_string(arena, arr, "two");
    arr = rt_array_push_string(arena, arr, "three");

    char **clone = rt_array_clone_string(arena, arr);
    assert(rt_array_length(clone) == 3);
    assert(strcmp(clone[0], "one") == 0);
    assert(strcmp(clone[1], "two") == 0);
    assert(strcmp(clone[2], "three") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Join Tests
 * ============================================================================ */

static void test_rt_array_join_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 3, 0);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;

    char *result = rt_array_join_long(arena, arr, ", ");
    assert(strcmp(result, "1, 2, 3") == 0);

    result = rt_array_join_long(arena, arr, "-");
    assert(strcmp(result, "1-2-3") == 0);

    result = rt_array_join_long(arena, arr, "");
    assert(strcmp(result, "123") == 0);

    /* Single element */
    long long *single = rt_array_alloc_long(arena, 1, 42);
    result = rt_array_join_long(arena, single, ", ");
    assert(strcmp(result, "42") == 0);

    /* Empty array */
    long long *empty = rt_array_alloc_long(arena, 0, 0);
    result = rt_array_join_long(arena, empty, ", ");
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_join_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr = rt_array_alloc_string(arena, 0, NULL);
    arr = rt_array_push_string(arena, arr, "hello");
    arr = rt_array_push_string(arena, arr, "world");
    arr = rt_array_push_string(arena, arr, "test");

    char *result = rt_array_join_string(arena, arr, " ");
    assert(strcmp(result, "hello world test") == 0);

    result = rt_array_join_string(arena, arr, ", ");
    assert(strcmp(result, "hello, world, test") == 0);

    result = rt_array_join_string(arena, arr, "");
    assert(strcmp(result, "helloworldtest") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Equality Tests
 * ============================================================================ */

static void test_rt_array_eq_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr1 = rt_array_alloc_long(arena, 3, 0);
    arr1[0] = 1; arr1[1] = 2; arr1[2] = 3;

    long long *arr2 = rt_array_alloc_long(arena, 3, 0);
    arr2[0] = 1; arr2[1] = 2; arr2[2] = 3;

    long long *arr3 = rt_array_alloc_long(arena, 3, 0);
    arr3[0] = 1; arr3[1] = 2; arr3[2] = 4;  /* Different last element */

    long long *arr4 = rt_array_alloc_long(arena, 2, 0);
    arr4[0] = 1; arr4[1] = 2;  /* Different length */

    /* Equal arrays */
    assert(rt_array_eq_long(arr1, arr2) == 1);

    /* Different values */
    assert(rt_array_eq_long(arr1, arr3) == 0);

    /* Different lengths */
    assert(rt_array_eq_long(arr1, arr4) == 0);

    /* NULL comparisons */
    assert(rt_array_eq_long(NULL, NULL) == 1);
    assert(rt_array_eq_long(arr1, NULL) == 0);
    assert(rt_array_eq_long(NULL, arr1) == 0);

    /* Empty arrays */
    long long *empty1 = rt_array_alloc_long(arena, 0, 0);
    long long *empty2 = rt_array_alloc_long(arena, 0, 0);
    assert(rt_array_eq_long(empty1, empty2) == 1);

    rt_arena_destroy(arena);
}

static void test_rt_array_eq_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char **arr1 = rt_array_alloc_string(arena, 0, NULL);
    arr1 = rt_array_push_string(arena, arr1, "a");
    arr1 = rt_array_push_string(arena, arr1, "b");

    char **arr2 = rt_array_alloc_string(arena, 0, NULL);
    arr2 = rt_array_push_string(arena, arr2, "a");
    arr2 = rt_array_push_string(arena, arr2, "b");

    char **arr3 = rt_array_alloc_string(arena, 0, NULL);
    arr3 = rt_array_push_string(arena, arr3, "a");
    arr3 = rt_array_push_string(arena, arr3, "c");

    assert(rt_array_eq_string(arr1, arr2) == 1);
    assert(rt_array_eq_string(arr1, arr3) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Range Tests
 * ============================================================================ */

static void test_rt_array_range(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic range 0 to 5 */
    long long *arr = rt_array_range(arena, 0, 5);
    assert(rt_array_length(arr) == 5);
    assert(arr[0] == 0);
    assert(arr[1] == 1);
    assert(arr[2] == 2);
    assert(arr[3] == 3);
    assert(arr[4] == 4);

    /* Range 5 to 10 */
    arr = rt_array_range(arena, 5, 10);
    assert(rt_array_length(arr) == 5);
    assert(arr[0] == 5);
    assert(arr[4] == 9);

    /* Negative range */
    arr = rt_array_range(arena, -3, 2);
    assert(rt_array_length(arr) == 5);
    assert(arr[0] == -3);
    assert(arr[4] == 1);

    /* Empty range (start == end) */
    arr = rt_array_range(arena, 5, 5);
    assert(rt_array_length(arr) == 0);

    /* Invalid range (start > end) */
    arr = rt_array_range(arena, 10, 5);
    assert(rt_array_length(arr) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Create Tests
 * ============================================================================ */

static void test_rt_array_create_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long data[] = {10, 20, 30, 40, 50};
    long long *arr = rt_array_create_long(arena, 5, data);

    assert(rt_array_length(arr) == 5);
    assert(arr[0] == 10);
    assert(arr[1] == 20);
    assert(arr[2] == 30);
    assert(arr[3] == 40);
    assert(arr[4] == 50);

    /* Modify original data, array unchanged */
    data[0] = 999;
    assert(arr[0] == 10);

    /* Empty array */
    arr = rt_array_create_long(arena, 0, NULL);
    assert(rt_array_length(arr) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_create_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"first", "second", "third"};
    char **arr = rt_array_create_string(arena, 3, data);

    assert(rt_array_length(arr) == 3);
    assert(strcmp(arr[0], "first") == 0);
    assert(strcmp(arr[1], "second") == 0);
    assert(strcmp(arr[2], "third") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Array Push Copy Tests (non-mutating)
 * ============================================================================ */

static void test_rt_array_push_copy_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 3, 0);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;

    long long *new_arr = rt_array_push_copy_long(arena, arr, 4);

    /* Original unchanged */
    assert(rt_array_length(arr) == 3);
    assert(arr[2] == 3);

    /* New array has element appended */
    assert(rt_array_length(new_arr) == 4);
    assert(new_arr[0] == 1);
    assert(new_arr[1] == 2);
    assert(new_arr[2] == 3);
    assert(new_arr[3] == 4);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_array_main(void)
{
    TEST_SECTION("Runtime Array");

    /* Clear */
    TEST_RUN("rt_array_clear", test_rt_array_clear);

    /* Push */
    TEST_RUN("rt_array_push_long", test_rt_array_push_long);
    TEST_RUN("rt_array_push_double", test_rt_array_push_double);
    TEST_RUN("rt_array_push_char", test_rt_array_push_char);
    TEST_RUN("rt_array_push_string", test_rt_array_push_string);
    TEST_RUN("rt_array_push_byte", test_rt_array_push_byte);

    /* Pop */
    TEST_RUN("rt_array_pop_long", test_rt_array_pop_long);
    TEST_RUN("rt_array_pop_string", test_rt_array_pop_string);

    /* Concat */
    TEST_RUN("rt_array_concat_long", test_rt_array_concat_long);
    TEST_RUN("rt_array_concat_string", test_rt_array_concat_string);

    /* Slice */
    TEST_RUN("rt_array_slice_long", test_rt_array_slice_long);
    TEST_RUN("rt_array_slice_string", test_rt_array_slice_string);

    /* Reverse */
    TEST_RUN("rt_array_rev_long", test_rt_array_rev_long);
    TEST_RUN("rt_array_rev_string", test_rt_array_rev_string);

    /* Remove */
    TEST_RUN("rt_array_rem_long", test_rt_array_rem_long);

    /* Insert */
    TEST_RUN("rt_array_ins_long", test_rt_array_ins_long);

    /* IndexOf */
    TEST_RUN("rt_array_indexOf_long", test_rt_array_indexOf_long);
    TEST_RUN("rt_array_indexOf_string", test_rt_array_indexOf_string);

    /* Contains */
    TEST_RUN("rt_array_contains_long", test_rt_array_contains_long);
    TEST_RUN("rt_array_contains_string", test_rt_array_contains_string);

    /* Clone */
    TEST_RUN("rt_array_clone_long", test_rt_array_clone_long);
    TEST_RUN("rt_array_clone_string", test_rt_array_clone_string);

    /* Join */
    TEST_RUN("rt_array_join_long", test_rt_array_join_long);
    TEST_RUN("rt_array_join_string", test_rt_array_join_string);

    /* Equality */
    TEST_RUN("rt_array_eq_long", test_rt_array_eq_long);
    TEST_RUN("rt_array_eq_string", test_rt_array_eq_string);

    /* Range */
    TEST_RUN("rt_array_range", test_rt_array_range);

    /* Create */
    TEST_RUN("rt_array_create_long", test_rt_array_create_long);
    TEST_RUN("rt_array_create_string", test_rt_array_create_string);

    /* Push Copy */
    TEST_RUN("rt_array_push_copy_long", test_rt_array_push_copy_long);
}
