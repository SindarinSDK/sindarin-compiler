// runtime_array_tests_query.c
// Tests for array query operations: indexOf, contains, clone, join, equality, range, create

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

