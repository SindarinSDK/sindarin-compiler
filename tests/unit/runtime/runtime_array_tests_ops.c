// runtime_array_tests_ops.c
// Tests for array operations: concat, slice, reverse, remove, insert

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

