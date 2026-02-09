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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

    // Test with escape-like sequences (not actual unicode, just chars)
    char *result = rt_str_concat(arena, "hello\\n", "world\\t");
    assert(strcmp(result, "hello\\nworld\\t") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_special_chars(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

    char *result = rt_str_concat(arena, "line1\nline2", "\ttab");
    assert(rt_str_contains(result, "\n") == 1);
    assert(rt_str_contains(result, "\t") == 1);

    rt_arena_destroy(arena);
}

static void test_rt_str_repeated_replace(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
 * Format Edge Cases
 * ============================================================================ */

static void test_rt_format_long_edge_values(void)
{
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
    RtArenaV2 *arena = rt_arena_create(NULL);

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
}
