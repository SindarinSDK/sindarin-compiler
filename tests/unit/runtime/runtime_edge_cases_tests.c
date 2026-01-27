// tests/unit/runtime/runtime_edge_cases_tests.c
// Edge case tests for runtime operations

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * String Edge Cases
 * ============================================================================ */

static void test_rt_str_long_string_operations(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Create a long string
    char long_str[1001];
    memset(long_str, 'a', 1000);
    long_str[1000] = '\0';

    char *result = rt_str_concat(arena, long_str, "suffix");
    assert(strlen(result) == 1006);
    assert(rt_str_endsWith(result, "suffix") == 1);

    rt_arena_destroy(arena);
}

static void test_rt_str_unicode_like_sequences(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Test with escape-like sequences (not actual unicode, just chars)
    char *result = rt_str_concat(arena, "hello\\n", "world\\t");
    assert(strcmp(result, "hello\\nworld\\t") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_special_chars(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_concat(arena, "line1\nline2", "\ttab");
    assert(rt_str_contains(result, "\n") == 1);
    assert(rt_str_contains(result, "\t") == 1);

    rt_arena_destroy(arena);
}

static void test_rt_str_repeated_replace(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Replace in string with overlapping patterns
    char *result = rt_str_replace(arena, "ababab", "ab", "X");
    assert(strcmp(result, "XXX") == 0);

    result = rt_str_replace(arena, "aaa", "aa", "b");
    // First 'aa' replaced with 'b', leaving 'ba'
    assert(strcmp(result, "ba") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_indexOf_edge_positions(void)
{
    // Test indexOf at exact boundaries
    assert(rt_str_indexOf("abc", "a") == 0);
    assert(rt_str_indexOf("abc", "c") == 2);
    assert(rt_str_indexOf("abc", "abc") == 0);
    assert(rt_str_indexOf("abc", "abcd") == -1);  // Search longer than string
}

static void test_rt_str_charAt_large_indices(void)
{
    assert(rt_str_charAt("hello", 1000) == 0);
    assert(rt_str_charAt("hello", -1000) == 0);
    assert(rt_str_charAt("hello", 2147483647) == 0);  // INT_MAX
}

static void test_rt_str_substring_boundary(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Test exact boundaries
    char *result = rt_str_substring(arena, "hello", 0, 5);
    assert(strcmp(result, "hello") == 0);

    // Indices at string boundary
    result = rt_str_substring(arena, "x", 0, 1);
    assert(strcmp(result, "x") == 0);

    // Out of bounds indices clamped
    result = rt_str_substring(arena, "hello", 0, 100);
    assert(strcmp(result, "hello") == 0);

    result = rt_str_substring(arena, "hello", -100, 5);
    assert(strcmp(result, "hello") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_split_empty_results(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Split with delimiter at every position
    char **parts = rt_str_split(arena, "|||", "|");
    assert(rt_array_length(parts) == 4);
    assert(strcmp(parts[0], "") == 0);
    assert(strcmp(parts[1], "") == 0);

    // Split empty string
    parts = rt_str_split(arena, "", ",");
    assert(rt_array_length(parts) == 1);
    assert(strcmp(parts[0], "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_trim_various_whitespace(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_trim(arena, " \t\r\n hello \t\r\n ");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_trim(arena, "nowhitespace");
    assert(strcmp(result, "nowhitespace") == 0);

    result = rt_str_trim(arena, "\n\n\n");
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_startsWith_endsWith_full_match(void)
{
    // Full string match
    assert(rt_str_startsWith("hello", "hello") == 1);
    assert(rt_str_endsWith("hello", "hello") == 1);

    // Single char string
    assert(rt_str_startsWith("a", "a") == 1);
    assert(rt_str_endsWith("a", "a") == 1);
}

/* ============================================================================
 * Array Edge Cases
 * ============================================================================ */

static void test_rt_array_very_large_allocation(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Create a moderately large array
    long long *arr = rt_array_alloc_long(arena, 1000, 42);
    assert(rt_array_length(arr) == 1000);
    assert(arr[0] == 42);
    assert(arr[999] == 42);

    rt_arena_destroy(arena);
}

static void test_rt_array_push_until_realloc(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 0, 0);

    // Push enough to trigger multiple reallocations
    for (int i = 0; i < 128; i++) {
        arr = rt_array_push_long(arena, arr, i);
    }

    assert(rt_array_length(arr) == 128);
    for (int i = 0; i < 128; i++) {
        assert(arr[i] == i);
    }

    rt_arena_destroy(arena);
}

static void test_rt_array_slice_step_variations(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 10, 0);
    for (int i = 0; i < 10; i++) arr[i] = i;

    // Step 3
    long long *slice = rt_array_slice_long(arena, arr, 0, 10, 3);
    assert(rt_array_length(slice) == 4);  // 0, 3, 6, 9
    assert(slice[0] == 0);
    assert(slice[1] == 3);
    assert(slice[2] == 6);
    assert(slice[3] == 9);

    // Step larger than array
    slice = rt_array_slice_long(arena, arr, 0, 10, 15);
    assert(rt_array_length(slice) == 1);  // Just element 0
    assert(slice[0] == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_concat_empty_arrays(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *empty1 = rt_array_alloc_long(arena, 0, 0);
    long long *empty2 = rt_array_alloc_long(arena, 0, 0);

    long long *result = rt_array_concat_long(arena, empty1, empty2);
    assert(rt_array_length(result) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_remove_first_last(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i + 1;

    // Remove first
    long long *result = rt_array_rem_long(arena, arr, 0);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 2);

    // Remove last from original
    result = rt_array_rem_long(arena, arr, 4);
    assert(rt_array_length(result) == 4);
    assert(result[3] == 4);

    rt_arena_destroy(arena);
}

static void test_rt_array_insert_at_boundaries(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 3, 0);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;

    // Insert at beginning
    long long *result = rt_array_ins_long(arena, arr, 0, 0);
    assert(rt_array_length(result) == 4);
    assert(result[0] == 0);
    assert(result[1] == 1);

    // Insert at end
    result = rt_array_ins_long(arena, arr, 100, 3);
    assert(rt_array_length(result) == 4);
    assert(result[3] == 100);

    rt_arena_destroy(arena);
}

static void test_rt_array_join_with_long_separator(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 3, 0);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;

    char *result = rt_array_join_long(arena, arr, " --- ");
    assert(strcmp(result, "1 --- 2 --- 3") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_array_indexOf_not_found(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_alloc_long(arena, 5, 0);
    for (int i = 0; i < 5; i++) arr[i] = i * 2;

    // Search for odd number that doesn't exist
    assert(rt_array_indexOf_long(arr, 1) == -1);
    assert(rt_array_indexOf_long(arr, 3) == -1);
    assert(rt_array_indexOf_long(arr, 100) == -1);

    rt_arena_destroy(arena);
}

static void test_rt_array_range_negative_bounds(void)
{
    RtArena *arena = rt_arena_create(NULL);

    long long *arr = rt_array_range(arena, -5, 5);
    assert(rt_array_length(arr) == 10);
    assert(arr[0] == -5);
    assert(arr[5] == 0);
    assert(arr[9] == 4);

    rt_arena_destroy(arena);
}

static void test_rt_array_double_operations(void)
{
    RtArena *arena = rt_arena_create(NULL);

    double *arr = rt_array_alloc_double(arena, 3, 0.0);
    arr[0] = 1.5; arr[1] = 2.5; arr[2] = 3.5;

    // Clone
    double *clone = rt_array_clone_double(arena, arr);
    assert(rt_array_length(clone) == 3);
    assert(clone[0] == 1.5);

    // Reverse
    double *rev = rt_array_rev_double(arena, arr);
    assert(rev[0] == 3.5);
    assert(rev[2] == 1.5);

    rt_arena_destroy(arena);
}

static void test_rt_array_bool_operations(void)
{
    RtArena *arena = rt_arena_create(NULL);

    int *arr = rt_array_alloc_bool(arena, 0, 0);
    arr = rt_array_push_bool(arena, arr, 1);  // true
    arr = rt_array_push_bool(arena, arr, 0);  // false
    arr = rt_array_push_bool(arena, arr, 1);  // true

    assert(rt_array_length(arr) == 3);
    assert(arr[0] == 1);
    assert(arr[1] == 0);
    assert(arr[2] == 1);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Arena Edge Cases
 * ============================================================================ */

static void test_rt_arena_multiple_creates(void)
{
    // Create and destroy multiple arenas in sequence
    for (int i = 0; i < 10; i++) {
        RtArena *arena = rt_arena_create(NULL);
        long long *arr = rt_array_alloc_long(arena, 10, i);
        assert(rt_array_length(arr) == 10);
        assert(arr[0] == i);
        rt_arena_destroy(arena);
    }
}

static void test_rt_arena_mixed_allocations(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Mix of different allocation types
    char *str1 = rt_str_concat(arena, "hello", " world");
    long long *arr = rt_array_alloc_long(arena, 5, 0);
    char *str2 = rt_str_toUpper(arena, str1);
    double *darr = rt_array_alloc_double(arena, 3, 0.0);

    assert(strcmp(str1, "hello world") == 0);
    assert(strcmp(str2, "HELLO WORLD") == 0);
    assert(rt_array_length(arr) == 5);
    assert(rt_array_length(darr) == 3);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Format Edge Cases
 * ============================================================================ */

static void test_rt_format_long_edge_values(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Zero
    char *result = rt_format_long(arena, 0, "d");
    assert(strcmp(result, "0") == 0);

    // Negative
    result = rt_format_long(arena, -12345, "d");
    assert(strcmp(result, "-12345") == 0);

    // Binary of 0
    result = rt_format_long(arena, 0, "b");
    assert(strcmp(result, "0") == 0);

    // Hex of 0
    result = rt_format_long(arena, 0, "x");
    assert(strcmp(result, "0") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_format_double_edge_values(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Zero with precision
    char *result = rt_format_double(arena, 0.0, ".2f");
    assert(strcmp(result, "0.00") == 0);

    // Negative with precision
    result = rt_format_double(arena, -3.14159, ".3f");
    assert(strcmp(result, "-3.142") == 0);

    // Very small value
    result = rt_format_double(arena, 0.001, ".4f");
    assert(strcmp(result, "0.0010") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_format_string_edge_cases(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Empty string with width
    char *result = rt_format_string(arena, "", "5");
    assert(strcmp(result, "     ") == 0);

    // String longer than max length
    result = rt_format_string(arena, "hello world", ".3");
    assert(strcmp(result, "hel") == 0);

    // String shorter than width
    result = rt_format_string(arena, "x", "5");
    assert(strcmp(result, "    x") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Type Conversion Edge Cases
 * ============================================================================ */

static void test_rt_to_string_edge_values(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Large numbers
    char *result = rt_to_string_long(arena, 9999999999LL);
    assert(strcmp(result, "9999999999") == 0);

    result = rt_to_string_long(arena, -9999999999LL);
    assert(strcmp(result, "-9999999999") == 0);

    // Byte boundary values
    result = rt_to_string_byte(arena, 0);
    assert(strcmp(result, "0") == 0);

    result = rt_to_string_byte(arena, 255);
    assert(strcmp(result, "255") == 0);

    result = rt_to_string_byte(arena, 128);
    assert(strcmp(result, "128") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_special_doubles(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Very small
    char *result = rt_to_string_double(arena, 0.00001);
    assert(strlen(result) > 0);
    assert(strstr(result, "1") != NULL);

    // Very large
    result = rt_to_string_double(arena, 1e10);
    assert(strlen(result) > 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Mutable String Edge Cases
 * ============================================================================ */

static void test_rt_string_capacity(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *str = rt_string_with_capacity(arena, 100);
    assert(RT_STR_META(str)->capacity >= 100);
    assert(RT_STR_META(str)->length == 0);
    assert(strcmp(str, "") == 0);

    // Use the capacity
    str = rt_string_append(str, "hello");
    str = rt_string_append(str, " ");
    str = rt_string_append(str, "world");
    assert(strcmp(str, "hello world") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_string_append_chain(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *str = rt_string_with_capacity(arena, 10);

    // Chain of appends
    for (int i = 0; i < 10; i++) {
        str = rt_string_append(str, "a");
    }

    assert(strcmp(str, "aaaaaaaaaa") == 0);
    assert(RT_STR_META(str)->length == 10);

    rt_arena_destroy(arena);
}

static void test_rt_string_from_empty(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *str = rt_string_from(arena, "");
    assert(strcmp(str, "") == 0);
    assert(RT_STR_META(str)->length == 0);

    // Can append to empty string
    str = rt_string_append(str, "test");
    assert(strcmp(str, "test") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comparison Edge Cases
 * ============================================================================ */

static void test_rt_str_compare_edge_cases(void)
{
    // Case sensitivity
    assert(rt_str_contains("ABC", "abc") == 0);
    assert(rt_str_contains("abc", "ABC") == 0);

    // Overlapping substrings
    assert(rt_str_indexOf("aaaa", "aa") == 0);

    // Contains with single char
    assert(rt_str_contains("hello", "h") == 1);
    assert(rt_str_contains("hello", "o") == 1);
    assert(rt_str_contains("hello", "x") == 0);
}

static void test_rt_array_eq_edge_cases(void)
{
    RtArena *arena = rt_arena_create(NULL);

    // Single element arrays
    long long *arr1 = rt_array_alloc_long(arena, 1, 42);
    long long *arr2 = rt_array_alloc_long(arena, 1, 42);
    long long *arr3 = rt_array_alloc_long(arena, 1, 43);

    assert(rt_array_eq_long(arr1, arr2) == 1);
    assert(rt_array_eq_long(arr1, arr3) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_edge_cases_main(void)
{
    TEST_SECTION("Runtime Edge Cases");

    // String edge cases
    TEST_RUN("rt_str_long_string_operations", test_rt_str_long_string_operations);
    TEST_RUN("rt_str_unicode_like_sequences", test_rt_str_unicode_like_sequences);
    TEST_RUN("rt_str_special_chars", test_rt_str_special_chars);
    TEST_RUN("rt_str_repeated_replace", test_rt_str_repeated_replace);
    TEST_RUN("rt_str_indexOf_edge_positions", test_rt_str_indexOf_edge_positions);
    TEST_RUN("rt_str_charAt_large_indices", test_rt_str_charAt_large_indices);
    TEST_RUN("rt_str_substring_boundary", test_rt_str_substring_boundary);
    TEST_RUN("rt_str_split_empty_results", test_rt_str_split_empty_results);
    TEST_RUN("rt_str_trim_various_whitespace", test_rt_str_trim_various_whitespace);
    TEST_RUN("rt_str_startsWith_endsWith_full_match", test_rt_str_startsWith_endsWith_full_match);

    // Array edge cases
    TEST_RUN("rt_array_very_large_allocation", test_rt_array_very_large_allocation);
    TEST_RUN("rt_array_push_until_realloc", test_rt_array_push_until_realloc);
    TEST_RUN("rt_array_slice_step_variations", test_rt_array_slice_step_variations);
    TEST_RUN("rt_array_concat_empty_arrays", test_rt_array_concat_empty_arrays);
    TEST_RUN("rt_array_remove_first_last", test_rt_array_remove_first_last);
    TEST_RUN("rt_array_insert_at_boundaries", test_rt_array_insert_at_boundaries);
    TEST_RUN("rt_array_join_with_long_separator", test_rt_array_join_with_long_separator);
    TEST_RUN("rt_array_indexOf_not_found", test_rt_array_indexOf_not_found);
    TEST_RUN("rt_array_range_negative_bounds", test_rt_array_range_negative_bounds);
    TEST_RUN("rt_array_double_operations", test_rt_array_double_operations);
    TEST_RUN("rt_array_bool_operations", test_rt_array_bool_operations);

    // Arena edge cases
    TEST_RUN("rt_arena_multiple_creates", test_rt_arena_multiple_creates);
    TEST_RUN("rt_arena_mixed_allocations", test_rt_arena_mixed_allocations);

    // Format edge cases
    TEST_RUN("rt_format_long_edge_values", test_rt_format_long_edge_values);
    TEST_RUN("rt_format_double_edge_values", test_rt_format_double_edge_values);
    TEST_RUN("rt_format_string_edge_cases", test_rt_format_string_edge_cases);

    // Type conversion edge cases
    TEST_RUN("rt_to_string_edge_values", test_rt_to_string_edge_values);
    TEST_RUN("rt_to_string_special_doubles", test_rt_to_string_special_doubles);

    // Mutable string edge cases
    TEST_RUN("rt_string_capacity", test_rt_string_capacity);
    TEST_RUN("rt_string_append_chain", test_rt_string_append_chain);
    TEST_RUN("rt_string_from_empty", test_rt_string_from_empty);

    // Comparison edge cases
    TEST_RUN("rt_str_compare_edge_cases", test_rt_str_compare_edge_cases);
    TEST_RUN("rt_array_eq_edge_cases", test_rt_array_eq_edge_cases);
}
