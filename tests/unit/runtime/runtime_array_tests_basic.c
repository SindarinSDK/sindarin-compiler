// runtime_array_tests_basic.c
// Tests for basic array operations: clear, push, pop

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

