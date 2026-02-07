// tests/runtime_arena_tests.c
// Tests for the runtime arena memory management system (V2 Arena)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../debug.h"
#include "../test_harness.h"

static void test_rt_arena_create(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    assert(arena != NULL);
    rt_arena_v2_destroy(arena);
}

static void test_rt_arena_create_with_parent(void)
{
    RtArenaV2 *parent = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "parent");
    RtArenaV2 *child = rt_arena_v2_create(parent, RT_ARENA_MODE_PRIVATE, "child");
    assert(parent != NULL);
    assert(child != NULL);
    rt_arena_v2_destroy(child);
    rt_arena_v2_destroy(parent);
}

static void test_rt_arena_alloc_small(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    RtHandleV2 *h1 = rt_arena_v2_alloc(arena, 16);
    RtHandleV2 *h2 = rt_arena_v2_alloc(arena, 32);
    RtHandleV2 *h3 = rt_arena_v2_alloc(arena, 8);
    void *p1 = rt_handle_v2_pin(h1);
    void *p2 = rt_handle_v2_pin(h2);
    void *p3 = rt_handle_v2_pin(h3);
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    /* Each allocation is independent */
    assert(p1 != p2);
    assert(p2 != p3);
    memset(p1, 0xAA, 16);
    memset(p2, 0xBB, 32);
    memset(p3, 0xCC, 8);
    rt_arena_v2_destroy(arena);
}

static void test_rt_arena_alloc_large(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    /* Allocate a large block */
    RtHandleV2 *h = rt_arena_v2_alloc(arena, 100000);
    void *p = rt_handle_v2_pin(h);
    assert(p != NULL);
    rt_arena_v2_destroy(arena);
}

static void test_rt_arena_alloc_null_arena(void)
{
    RtHandleV2 *h = rt_arena_v2_alloc(NULL, 16);
    assert(h == NULL);
}

static void test_rt_arena_calloc(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    RtHandleV2 *h = rt_arena_v2_calloc(arena, 10, sizeof(int));
    int *arr = (int *)rt_handle_v2_pin(h);
    assert(arr != NULL);
    for (int i = 0; i < 10; i++) {
        assert(arr[i] == 0);
    }
    rt_arena_v2_destroy(arena);
}

static void test_rt_arena_strdup(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    RtHandleV2 *h1 = rt_arena_v2_strdup(arena, NULL);
    assert(h1 == NULL);
    RtHandleV2 *h2 = rt_arena_v2_strdup(arena, "");
    char *s2 = (char *)rt_handle_v2_pin(h2);
    assert(s2 != NULL);
    assert(strcmp(s2, "") == 0);
    RtHandleV2 *h3 = rt_arena_v2_strdup(arena, "hello world");
    char *s3 = (char *)rt_handle_v2_pin(h3);
    assert(s3 != NULL);
    assert(strcmp(s3, "hello world") == 0);
    const char *long_str = "This is a longer string that should still work correctly with the arena allocator.";
    RtHandleV2 *h4 = rt_arena_v2_strdup(arena, long_str);
    char *s4 = (char *)rt_handle_v2_pin(h4);
    assert(s4 != NULL);
    assert(strcmp(s4, long_str) == 0);
    rt_arena_v2_destroy(arena);
}

static void test_rt_arena_destroy_null(void)
{
    rt_arena_v2_destroy(NULL);
}

static void test_rt_arena_many_allocations(void)
{
    RtArenaV2 *arena = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "test");
    for (int i = 0; i < 1000; i++) {
        RtHandleV2 *h = rt_arena_v2_alloc(arena, 64);
        void *p = rt_handle_v2_pin(h);
        assert(p != NULL);
        memset(p, i & 0xFF, 64);
    }
    rt_arena_v2_destroy(arena);
}

static void test_rt_string_with_capacity(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 10);
    assert(str != NULL);
    RtStringMeta *meta = RT_STR_META(str);
    assert(meta->capacity == 10);
    assert(meta->length == 0);
    assert(meta->arena == arena);
    assert(strcmp(str, "") == 0);
    assert(str[0] == '\0');
    char *str2 = rt_string_with_capacity(arena, 0);
    assert(str2 != NULL);
    RtStringMeta *meta2 = RT_STR_META(str2);
    assert(meta2->capacity == 0);
    assert(meta2->length == 0);
    assert(str2[0] == '\0');
    char *str3 = rt_string_with_capacity(arena, 1000);
    assert(str3 != NULL);
    RtStringMeta *meta3 = RT_STR_META(str3);
    assert(meta3->capacity == 1000);
    assert(meta3->length == 0);
    rt_arena_destroy(arena);
}

static void test_rt_string_append_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 20);
    assert(str != NULL);
    str = rt_string_append(str, "hello");
    assert(str != NULL);
    assert(strcmp(str, "hello") == 0);
    RtStringMeta *meta = RT_STR_META(str);
    assert(meta->length == 5);
    assert(meta->capacity == 20);
    rt_arena_destroy(arena);
}

static void test_rt_string_append_multiple(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 10);
    assert(str != NULL);
    str = rt_string_append(str, "hello");
    assert(strcmp(str, "hello") == 0);
    assert(RT_STR_META(str)->length == 5);
    str = rt_string_append(str, " ");
    assert(strcmp(str, "hello ") == 0);
    assert(RT_STR_META(str)->length == 6);
    str = rt_string_append(str, "world!");
    assert(strcmp(str, "hello world!") == 0);
    assert(RT_STR_META(str)->length == 12);
    assert(RT_STR_META(str)->capacity > 10);
    rt_arena_destroy(arena);
}

static void test_rt_string_append_no_realloc(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 100);
    char *original_ptr = str;
    str = rt_string_append(str, "one");
    assert(str == original_ptr);
    assert(RT_STR_META(str)->capacity == 100);
    str = rt_string_append(str, " two");
    assert(str == original_ptr);
    assert(RT_STR_META(str)->capacity == 100);
    str = rt_string_append(str, " three");
    assert(str == original_ptr);
    assert(RT_STR_META(str)->capacity == 100);
    assert(strcmp(str, "one two three") == 0);
    assert(RT_STR_META(str)->length == 13);
    rt_arena_destroy(arena);
}

static void test_rt_string_append_null_src(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 20);
    str = rt_string_append(str, "test");
    assert(strcmp(str, "test") == 0);
    char *result = rt_string_append(str, NULL);
    assert(result == str);
    assert(strcmp(str, "test") == 0);
    assert(RT_STR_META(str)->length == 4);
    rt_arena_destroy(arena);
}

static void test_rt_string_append_empty_src(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 20);
    str = rt_string_append(str, "initial");
    assert(strcmp(str, "initial") == 0);
    str = rt_string_append(str, "");
    assert(strcmp(str, "initial") == 0);
    assert(RT_STR_META(str)->length == 7);
    rt_arena_destroy(arena);
}

static void test_rt_string_length_tracking(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *str = rt_string_with_capacity(arena, 50);
    assert(RT_STR_META(str)->length == 0);
    str = rt_string_append(str, "a");
    assert(RT_STR_META(str)->length == 1);
    str = rt_string_append(str, "bb");
    assert(RT_STR_META(str)->length == 3);
    str = rt_string_append(str, "ccc");
    assert(RT_STR_META(str)->length == 6);
    str = rt_string_append(str, "dddd");
    assert(RT_STR_META(str)->length == 10);
    assert(strcmp(str, "abbcccdddd") == 0);
    assert(strlen(str) == RT_STR_META(str)->length);
    rt_arena_destroy(arena);
}

void test_rt_arena_main(void)
{
    TEST_SECTION("Runtime Arena V2");

    TEST_RUN("rt_arena_create", test_rt_arena_create);
    TEST_RUN("rt_arena_create_with_parent", test_rt_arena_create_with_parent);
    TEST_RUN("rt_arena_alloc_small", test_rt_arena_alloc_small);
    TEST_RUN("rt_arena_alloc_large", test_rt_arena_alloc_large);
    TEST_RUN("rt_arena_alloc_null_arena", test_rt_arena_alloc_null_arena);
    TEST_RUN("rt_arena_calloc", test_rt_arena_calloc);
    TEST_RUN("rt_arena_strdup", test_rt_arena_strdup);
    TEST_RUN("rt_arena_destroy_null", test_rt_arena_destroy_null);
    TEST_RUN("rt_arena_many_allocations", test_rt_arena_many_allocations);
    TEST_RUN("rt_string_with_capacity", test_rt_string_with_capacity);
    TEST_RUN("rt_string_append_empty", test_rt_string_append_empty);
    TEST_RUN("rt_string_append_multiple", test_rt_string_append_multiple);
    TEST_RUN("rt_string_append_no_realloc", test_rt_string_append_no_realloc);
    TEST_RUN("rt_string_append_null_src", test_rt_string_append_null_src);
    TEST_RUN("rt_string_append_empty_src", test_rt_string_append_empty_src);
    TEST_RUN("rt_string_length_tracking", test_rt_string_length_tracking);
}
