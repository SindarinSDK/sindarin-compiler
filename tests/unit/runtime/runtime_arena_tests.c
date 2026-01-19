// tests/runtime_arena_tests.c
// Tests for the runtime arena memory management system (RtArena)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../debug.h"
#include "../test_harness.h"

static void test_rt_arena_create(void)
{
    RtArena *arena = rt_arena_create(NULL);
    assert(arena != NULL);
    assert(arena->parent == NULL);
    assert(arena->first != NULL);
    assert(arena->current == arena->first);
    assert(arena->default_block_size == RT_ARENA_DEFAULT_BLOCK_SIZE);
    assert(arena->total_allocated > 0);
    rt_arena_destroy(arena);
}

static void test_rt_arena_create_sized(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 1024);
    assert(arena != NULL);
    assert(arena->default_block_size == 1024);
    assert(arena->first->size == 1024);
    rt_arena_destroy(arena);

    arena = rt_arena_create_sized(NULL, 0);
    assert(arena != NULL);
    assert(arena->default_block_size == RT_ARENA_DEFAULT_BLOCK_SIZE);
    rt_arena_destroy(arena);
}

static void test_rt_arena_create_with_parent(void)
{
    RtArena *parent = rt_arena_create(NULL);
    RtArena *child = rt_arena_create(parent);
    assert(child->parent == parent);
    assert(parent->parent == NULL);
    rt_arena_destroy(child);
    rt_arena_destroy(parent);
}

static void test_rt_arena_alloc_small(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 256);
    void *p1 = rt_arena_alloc(arena, 16);
    void *p2 = rt_arena_alloc(arena, 32);
    void *p3 = rt_arena_alloc(arena, 8);
    assert(p1 != NULL);
    assert(p2 != NULL);
    assert(p3 != NULL);
    assert(p2 > p1);
    assert(p3 > p2);
    memset(p1, 0xAA, 16);
    memset(p2, 0xBB, 32);
    memset(p3, 0xCC, 8);
    rt_arena_destroy(arena);
}

static void test_rt_arena_alloc_large(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 64);
    void *p1 = rt_arena_alloc(arena, 100);
    assert(p1 != NULL);
    assert(arena->first != NULL);
    assert(arena->current != arena->first);
    rt_arena_destroy(arena);
}

static void test_rt_arena_alloc_zero(void)
{
    RtArena *arena = rt_arena_create(NULL);
    void *p = rt_arena_alloc(arena, 0);
    assert(p == NULL);
    rt_arena_destroy(arena);
}

static void test_rt_arena_alloc_null_arena(void)
{
    void *p = rt_arena_alloc(NULL, 16);
    assert(p == NULL);
}

static void test_rt_arena_calloc(void)
{
    RtArena *arena = rt_arena_create(NULL);
    int *arr = rt_arena_calloc(arena, 10, sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 10; i++) {
        assert(arr[i] == 0);
    }
    rt_arena_destroy(arena);
}

static void test_rt_arena_alloc_aligned(void)
{
    RtArena *arena = rt_arena_create(NULL);
    void *p1 = rt_arena_alloc_aligned(arena, 32, 16);
    assert(p1 != NULL);
    assert(((size_t)p1 % 16) == 0);
    void *p2 = rt_arena_alloc_aligned(arena, 64, 32);
    assert(p2 != NULL);
    assert(((size_t)p2 % 32) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_arena_strdup(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *s1 = rt_arena_strdup(arena, NULL);
    assert(s1 == NULL);
    char *s2 = rt_arena_strdup(arena, "");
    assert(s2 != NULL);
    assert(strcmp(s2, "") == 0);
    char *s3 = rt_arena_strdup(arena, "hello world");
    assert(s3 != NULL);
    assert(strcmp(s3, "hello world") == 0);
    const char *long_str = "This is a longer string that should still work correctly with the arena allocator.";
    char *s4 = rt_arena_strdup(arena, long_str);
    assert(s4 != NULL);
    assert(strcmp(s4, long_str) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_arena_strndup(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *s1 = rt_arena_strndup(arena, NULL, 5);
    assert(s1 == NULL);
    char *s2 = rt_arena_strndup(arena, "hello", 10);
    assert(s2 != NULL);
    assert(strcmp(s2, "hello") == 0);
    char *s3 = rt_arena_strndup(arena, "hello world", 5);
    assert(s3 != NULL);
    assert(strcmp(s3, "hello") == 0);
    char *s4 = rt_arena_strndup(arena, "hello", 0);
    assert(s4 != NULL);
    assert(strcmp(s4, "") == 0);
    rt_arena_destroy(arena);
}

static void test_rt_arena_reset(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 64);
    for (int i = 0; i < 10; i++) {
        rt_arena_alloc(arena, 100);
    }
    assert(arena->first->next != NULL);
    rt_arena_reset(arena);
    assert(arena->first != NULL);
    assert(arena->first->next == NULL);
    assert(arena->current == arena->first);
    assert(arena->first->used == 0);
    void *p = rt_arena_alloc(arena, 32);
    assert(p != NULL);
    rt_arena_destroy(arena);
}

static void test_rt_arena_promote(void)
{
    RtArena *src_arena = rt_arena_create(NULL);
    RtArena *dest_arena = rt_arena_create(NULL);
    int *src_data = rt_arena_alloc(src_arena, sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        src_data[i] = i * 10;
    }
    int *dest_data = rt_arena_promote(dest_arena, src_data, sizeof(int) * 5);
    assert(dest_data != NULL);
    assert(dest_data != src_data);
    for (int i = 0; i < 5; i++) {
        assert(dest_data[i] == i * 10);
    }
    src_data[0] = 999;
    assert(dest_data[0] == 0);
    rt_arena_destroy(src_arena);
    rt_arena_destroy(dest_arena);
}

static void test_rt_arena_promote_null(void)
{
    RtArena *arena = rt_arena_create(NULL);
    void *p1 = rt_arena_promote(NULL, "test", 4);
    assert(p1 == NULL);
    void *p2 = rt_arena_promote(arena, NULL, 4);
    assert(p2 == NULL);
    void *p3 = rt_arena_promote(arena, "test", 0);
    assert(p3 == NULL);
    rt_arena_destroy(arena);
}

static void test_rt_arena_promote_string(void)
{
    RtArena *src_arena = rt_arena_create(NULL);
    RtArena *dest_arena = rt_arena_create(NULL);
    char *src_str = rt_arena_strdup(src_arena, "hello from source");
    char *dest_str = rt_arena_promote_string(dest_arena, src_str);
    assert(dest_str != NULL);
    assert(dest_str != src_str);
    assert(strcmp(dest_str, "hello from source") == 0);
    rt_arena_destroy(src_arena);
    rt_arena_destroy(dest_arena);
}

static void test_rt_arena_total_allocated(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 1024);
    size_t initial = rt_arena_total_allocated(arena);
    assert(initial > 0);
    rt_arena_alloc(arena, 2000);
    size_t after = rt_arena_total_allocated(arena);
    assert(after > initial);
    assert(rt_arena_total_allocated(NULL) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_arena_destroy_null(void)
{
    rt_arena_destroy(NULL);
}

static void test_rt_arena_block_growth(void)
{
    RtArena *arena = rt_arena_create_sized(NULL, 32);
    assert(arena->first->size == 32);
    void *p1 = rt_arena_alloc(arena, 16);
    assert(p1 != NULL);
    assert(arena->current == arena->first);
    void *p2 = rt_arena_alloc(arena, 24);
    assert(p2 != NULL);
    assert(arena->current != arena->first);
    assert(arena->current->size >= 24);
    rt_arena_destroy(arena);
}

static void test_rt_arena_many_allocations(void)
{
    RtArena *arena = rt_arena_create(NULL);
    for (int i = 0; i < 1000; i++) {
        void *p = rt_arena_alloc(arena, 64);
        assert(p != NULL);
        memset(p, i & 0xFF, 64);
    }
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_long(void)
{
    RtArena *arena = rt_arena_create(NULL);
    long long *arr = rt_array_alloc_long(arena, 5, 42);
    assert(arr != NULL);
    assert(rt_array_length(arr) == 5);
    for (size_t i = 0; i < 5; i++) {
        assert(arr[i] == 42);
    }
    long long *arr2 = rt_array_alloc_long(arena, 10, 0);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 10);
    for (size_t i = 0; i < 10; i++) {
        assert(arr2[i] == 0);
    }
    long long *arr3 = rt_array_alloc_long(arena, 0, 99);
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_double(void)
{
    RtArena *arena = rt_arena_create(NULL);
    double *arr = rt_array_alloc_double(arena, 3, 3.14);
    assert(arr != NULL);
    assert(rt_array_length(arr) == 3);
    for (size_t i = 0; i < 3; i++) {
        assert(arr[i] == 3.14);
    }
    double *arr2 = rt_array_alloc_double(arena, 5, 0.0);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 5);
    for (size_t i = 0; i < 5; i++) {
        assert(arr2[i] == 0.0);
    }
    double *arr3 = rt_array_alloc_double(arena, 0, 1.5);
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_char(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char *arr = rt_array_alloc_char(arena, 10, 'x');
    assert(arr != NULL);
    assert(rt_array_length(arr) == 10);
    for (size_t i = 0; i < 10; i++) {
        assert(arr[i] == 'x');
    }
    char *arr2 = rt_array_alloc_char(arena, 5, 0);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 5);
    for (size_t i = 0; i < 5; i++) {
        assert(arr2[i] == 0);
    }
    char *arr3 = rt_array_alloc_char(arena, 0, 'a');
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_bool(void)
{
    RtArena *arena = rt_arena_create(NULL);
    int *arr = rt_array_alloc_bool(arena, 100, 1);
    assert(arr != NULL);
    assert(rt_array_length(arr) == 100);
    for (size_t i = 0; i < 100; i++) {
        assert(arr[i] == 1);
    }
    int *arr2 = rt_array_alloc_bool(arena, 50, 0);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 50);
    for (size_t i = 0; i < 50; i++) {
        assert(arr2[i] == 0);
    }
    int *arr3 = rt_array_alloc_bool(arena, 0, 1);
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_byte(void)
{
    RtArena *arena = rt_arena_create(NULL);
    unsigned char *arr = rt_array_alloc_byte(arena, 8, 255);
    assert(arr != NULL);
    assert(rt_array_length(arr) == 8);
    for (size_t i = 0; i < 8; i++) {
        assert(arr[i] == 255);
    }
    unsigned char *arr2 = rt_array_alloc_byte(arena, 16, 0);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 16);
    for (size_t i = 0; i < 16; i++) {
        assert(arr2[i] == 0);
    }
    unsigned char *arr3 = rt_array_alloc_byte(arena, 0, 128);
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
}

static void test_rt_array_alloc_string(void)
{
    RtArena *arena = rt_arena_create(NULL);
    char **arr = rt_array_alloc_string(arena, 5, "hello");
    assert(arr != NULL);
    assert(rt_array_length(arr) == 5);
    for (size_t i = 0; i < 5; i++) {
        assert(arr[i] != NULL);
        assert(strcmp(arr[i], "hello") == 0);
    }
    assert(arr[0] != arr[1]);
    char **arr2 = rt_array_alloc_string(arena, 3, NULL);
    assert(arr2 != NULL);
    assert(rt_array_length(arr2) == 3);
    for (size_t i = 0; i < 3; i++) {
        assert(arr2[i] == NULL);
    }
    char **arr3 = rt_array_alloc_string(arena, 0, "test");
    assert(arr3 != NULL);
    assert(rt_array_length(arr3) == 0);
    rt_arena_destroy(arena);
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
    TEST_SECTION("Runtime Arena");

    TEST_RUN("rt_arena_create", test_rt_arena_create);
    TEST_RUN("rt_arena_create_sized", test_rt_arena_create_sized);
    TEST_RUN("rt_arena_create_with_parent", test_rt_arena_create_with_parent);
    TEST_RUN("rt_arena_alloc_small", test_rt_arena_alloc_small);
    TEST_RUN("rt_arena_alloc_large", test_rt_arena_alloc_large);
    TEST_RUN("rt_arena_alloc_zero", test_rt_arena_alloc_zero);
    TEST_RUN("rt_arena_alloc_null_arena", test_rt_arena_alloc_null_arena);
    TEST_RUN("rt_arena_calloc", test_rt_arena_calloc);
    TEST_RUN("rt_arena_alloc_aligned", test_rt_arena_alloc_aligned);
    TEST_RUN("rt_arena_strdup", test_rt_arena_strdup);
    TEST_RUN("rt_arena_strndup", test_rt_arena_strndup);
    TEST_RUN("rt_arena_reset", test_rt_arena_reset);
    TEST_RUN("rt_arena_promote", test_rt_arena_promote);
    TEST_RUN("rt_arena_promote_null", test_rt_arena_promote_null);
    TEST_RUN("rt_arena_promote_string", test_rt_arena_promote_string);
    TEST_RUN("rt_arena_total_allocated", test_rt_arena_total_allocated);
    TEST_RUN("rt_arena_destroy_null", test_rt_arena_destroy_null);
    TEST_RUN("rt_arena_block_growth", test_rt_arena_block_growth);
    TEST_RUN("rt_arena_many_allocations", test_rt_arena_many_allocations);
    TEST_RUN("rt_array_alloc_long", test_rt_array_alloc_long);
    TEST_RUN("rt_array_alloc_double", test_rt_array_alloc_double);
    TEST_RUN("rt_array_alloc_char", test_rt_array_alloc_char);
    TEST_RUN("rt_array_alloc_bool", test_rt_array_alloc_bool);
    TEST_RUN("rt_array_alloc_byte", test_rt_array_alloc_byte);
    TEST_RUN("rt_array_alloc_string", test_rt_array_alloc_string);
    TEST_RUN("rt_string_with_capacity", test_rt_string_with_capacity);
    TEST_RUN("rt_string_append_empty", test_rt_string_append_empty);
    TEST_RUN("rt_string_append_multiple", test_rt_string_append_multiple);
    TEST_RUN("rt_string_append_no_realloc", test_rt_string_append_no_realloc);
    TEST_RUN("rt_string_append_null_src", test_rt_string_append_null_src);
    TEST_RUN("rt_string_append_empty_src", test_rt_string_append_empty_src);
    TEST_RUN("rt_string_length_tracking", test_rt_string_length_tracking);
}
