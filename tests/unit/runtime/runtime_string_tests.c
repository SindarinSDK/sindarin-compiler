// tests/unit/runtime/runtime_string_tests.c
// Tests for runtime string operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * String Concatenation Tests
 * ============================================================================ */

static void test_rt_str_concat_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_concat(arena, "hello", " world");
    assert(strcmp(result, "hello world") == 0);

    result = rt_str_concat(arena, "", "test");
    assert(strcmp(result, "test") == 0);

    result = rt_str_concat(arena, "test", "");
    assert(strcmp(result, "test") == 0);

    result = rt_str_concat(arena, "", "");
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_concat_null(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_concat(arena, NULL, "world");
    assert(strcmp(result, "world") == 0);

    result = rt_str_concat(arena, "hello", NULL);
    assert(strcmp(result, "hello") == 0);

    result = rt_str_concat(arena, NULL, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String Length Tests
 * ============================================================================ */

static void test_rt_str_length(void)
{
    assert(rt_str_length("hello") == 5);
    assert(rt_str_length("") == 0);
    assert(rt_str_length("a") == 1);
    assert(rt_str_length("hello world") == 11);
    assert(rt_str_length(NULL) == 0);
}

/* ============================================================================
 * String Index Of Tests
 * ============================================================================ */

static void test_rt_str_indexOf(void)
{
    assert(rt_str_indexOf("hello world", "world") == 6);
    assert(rt_str_indexOf("hello world", "hello") == 0);
    assert(rt_str_indexOf("hello world", "o") == 4);  /* First occurrence */
    assert(rt_str_indexOf("hello world", "x") == -1);
    assert(rt_str_indexOf("hello world", "") == 0);
    assert(rt_str_indexOf("", "test") == -1);
    assert(rt_str_indexOf(NULL, "test") == -1);
    assert(rt_str_indexOf("test", NULL) == -1);
}

/* ============================================================================
 * String Contains Tests
 * ============================================================================ */

static void test_rt_str_contains(void)
{
    assert(rt_str_contains("hello world", "world") == 1);
    assert(rt_str_contains("hello world", "hello") == 1);
    assert(rt_str_contains("hello world", "xyz") == 0);
    assert(rt_str_contains("hello world", "") == 1);
    assert(rt_str_contains("", "test") == 0);
    assert(rt_str_contains(NULL, "test") == 0);
    assert(rt_str_contains("test", NULL) == 0);
}

/* ============================================================================
 * String CharAt Tests
 * ============================================================================ */

static void test_rt_str_charAt(void)
{
    assert(rt_str_charAt("hello", 0) == 'h');
    assert(rt_str_charAt("hello", 1) == 'e');
    assert(rt_str_charAt("hello", 4) == 'o');

    /* Negative indexing */
    assert(rt_str_charAt("hello", -1) == 'o');
    assert(rt_str_charAt("hello", -2) == 'l');
    assert(rt_str_charAt("hello", -5) == 'h');

    /* Out of bounds */
    assert(rt_str_charAt("hello", 5) == 0);
    assert(rt_str_charAt("hello", 100) == 0);
    assert(rt_str_charAt("hello", -6) == 0);

    /* Edge cases */
    assert(rt_str_charAt("", 0) == 0);
    assert(rt_str_charAt(NULL, 0) == 0);
}

/* ============================================================================
 * String Substring Tests
 * ============================================================================ */

static void test_rt_str_substring(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_substring(arena, "hello world", 0, 5);
    assert(strcmp(result, "hello") == 0);

    result = rt_str_substring(arena, "hello world", 6, 11);
    assert(strcmp(result, "world") == 0);

    result = rt_str_substring(arena, "hello world", 0, 11);
    assert(strcmp(result, "hello world") == 0);

    /* Negative indices */
    result = rt_str_substring(arena, "hello world", -5, 11);
    assert(strcmp(result, "world") == 0);

    result = rt_str_substring(arena, "hello world", 0, -1);
    assert(strcmp(result, "hello worl") == 0);

    result = rt_str_substring(arena, "hello world", -5, -1);
    assert(strcmp(result, "worl") == 0);

    /* Edge cases */
    result = rt_str_substring(arena, "hello", 5, 5);
    assert(strcmp(result, "") == 0);

    result = rt_str_substring(arena, "hello", 3, 2);  /* start > end */
    assert(strcmp(result, "") == 0);

    result = rt_str_substring(arena, "", 0, 0);
    assert(strcmp(result, "") == 0);

    result = rt_str_substring(arena, NULL, 0, 5);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String Case Conversion Tests
 * ============================================================================ */

static void test_rt_str_toUpper(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_toUpper(arena, "hello");
    assert(strcmp(result, "HELLO") == 0);

    result = rt_str_toUpper(arena, "Hello World");
    assert(strcmp(result, "HELLO WORLD") == 0);

    result = rt_str_toUpper(arena, "ALREADY UPPER");
    assert(strcmp(result, "ALREADY UPPER") == 0);

    result = rt_str_toUpper(arena, "123abc");
    assert(strcmp(result, "123ABC") == 0);

    result = rt_str_toUpper(arena, "");
    assert(strcmp(result, "") == 0);

    result = rt_str_toUpper(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_str_toLower(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_toLower(arena, "HELLO");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_toLower(arena, "Hello World");
    assert(strcmp(result, "hello world") == 0);

    result = rt_str_toLower(arena, "already lower");
    assert(strcmp(result, "already lower") == 0);

    result = rt_str_toLower(arena, "123ABC");
    assert(strcmp(result, "123abc") == 0);

    result = rt_str_toLower(arena, "");
    assert(strcmp(result, "") == 0);

    result = rt_str_toLower(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String StartsWith/EndsWith Tests
 * ============================================================================ */

static void test_rt_str_startsWith(void)
{
    assert(rt_str_startsWith("hello world", "hello") == 1);
    assert(rt_str_startsWith("hello world", "") == 1);
    assert(rt_str_startsWith("hello world", "world") == 0);
    assert(rt_str_startsWith("hello", "hello world") == 0);  /* Prefix longer than string */
    assert(rt_str_startsWith("", "") == 1);
    assert(rt_str_startsWith("", "a") == 0);
    assert(rt_str_startsWith(NULL, "test") == 0);
    assert(rt_str_startsWith("test", NULL) == 0);
}

static void test_rt_str_endsWith(void)
{
    assert(rt_str_endsWith("hello world", "world") == 1);
    assert(rt_str_endsWith("hello world", "") == 1);
    assert(rt_str_endsWith("hello world", "hello") == 0);
    assert(rt_str_endsWith("world", "hello world") == 0);  /* Suffix longer than string */
    assert(rt_str_endsWith("", "") == 1);
    assert(rt_str_endsWith("", "a") == 0);
    assert(rt_str_endsWith(NULL, "test") == 0);
    assert(rt_str_endsWith("test", NULL) == 0);
}

/* ============================================================================
 * String Trim Tests
 * ============================================================================ */

static void test_rt_str_trim(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_str_trim(arena, "  hello  ");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_trim(arena, "hello");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_trim(arena, "   ");
    assert(strcmp(result, "") == 0);

    result = rt_str_trim(arena, "");
    assert(strcmp(result, "") == 0);

    result = rt_str_trim(arena, "\t\nhello\r\n");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_trim(arena, "  hello world  ");
    assert(strcmp(result, "hello world") == 0);

    result = rt_str_trim(arena, NULL);
    assert(strcmp(result, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String Replace Tests
 * ============================================================================ */

static void test_rt_str_replace(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic replacement */
    char *result = rt_str_replace(arena, "hello world", "world", "universe");
    assert(strcmp(result, "hello universe") == 0);

    /* Multiple occurrences */
    result = rt_str_replace(arena, "aaa", "a", "b");
    assert(strcmp(result, "bbb") == 0);

    /* No occurrences */
    result = rt_str_replace(arena, "hello", "x", "y");
    assert(strcmp(result, "hello") == 0);

    /* Replace with empty string */
    result = rt_str_replace(arena, "hello world", "world", "");
    assert(strcmp(result, "hello ") == 0);

    /* Replace with longer string */
    result = rt_str_replace(arena, "hi", "hi", "hello");
    assert(strcmp(result, "hello") == 0);

    /* Empty search string */
    result = rt_str_replace(arena, "hello", "", "x");
    assert(strcmp(result, "hello") == 0);

    /* Empty input */
    result = rt_str_replace(arena, "", "a", "b");
    assert(strcmp(result, "") == 0);

    /* NULL handling */
    result = rt_str_replace(arena, NULL, "a", "b");
    assert(strcmp(result, "") == 0);

    result = rt_str_replace(arena, "hello", NULL, "b");
    assert(strcmp(result, "hello") == 0);

    result = rt_str_replace(arena, "hello", "l", NULL);
    assert(strcmp(result, "hello") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String Split Tests
 * ============================================================================ */

static void test_rt_str_split(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic split */
    char **parts = rt_str_split(arena, "a,b,c", ",");
    assert(rt_array_length(parts) == 3);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "b") == 0);
    assert(strcmp(parts[2], "c") == 0);

    /* Split by multi-char delimiter */
    parts = rt_str_split(arena, "a::b::c", "::");
    assert(rt_array_length(parts) == 3);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "b") == 0);
    assert(strcmp(parts[2], "c") == 0);

    /* Empty parts */
    parts = rt_str_split(arena, "a,,b", ",");
    assert(rt_array_length(parts) == 3);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "") == 0);
    assert(strcmp(parts[2], "b") == 0);

    /* No delimiter found */
    parts = rt_str_split(arena, "hello", ",");
    assert(rt_array_length(parts) == 1);
    assert(strcmp(parts[0], "hello") == 0);

    /* Split into individual characters (empty delimiter) */
    parts = rt_str_split(arena, "abc", "");
    assert(rt_array_length(parts) == 3);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "b") == 0);
    assert(strcmp(parts[2], "c") == 0);

    /* Leading/trailing delimiter */
    parts = rt_str_split(arena, ",a,b,", ",");
    assert(rt_array_length(parts) == 4);
    assert(strcmp(parts[0], "") == 0);
    assert(strcmp(parts[1], "a") == 0);
    assert(strcmp(parts[2], "b") == 0);
    assert(strcmp(parts[3], "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Type to String Conversion Tests
 * ============================================================================ */

static void test_rt_to_string_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_long(arena, 42);
    assert(strcmp(result, "42") == 0);

    result = rt_to_string_long(arena, -42);
    assert(strcmp(result, "-42") == 0);

    result = rt_to_string_long(arena, 0);
    assert(strcmp(result, "0") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_double(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_double(arena, 3.14159);
    assert(strncmp(result, "3.14159", 7) == 0);

    result = rt_to_string_double(arena, -2.5);
    assert(strncmp(result, "-2.50000", 8) == 0);

    result = rt_to_string_double(arena, 0.0);
    assert(strncmp(result, "0.00000", 7) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_char(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_char(arena, 'a');
    assert(strcmp(result, "a") == 0);

    result = rt_to_string_char(arena, '0');
    assert(strcmp(result, "0") == 0);

    result = rt_to_string_char(arena, ' ');
    assert(strcmp(result, " ") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_bool(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_bool(arena, 1);
    assert(strcmp(result, "true") == 0);

    result = rt_to_string_bool(arena, 0);
    assert(strcmp(result, "false") == 0);

    result = rt_to_string_bool(arena, 42);  /* Non-zero is true */
    assert(strcmp(result, "true") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_byte(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_byte(arena, 0);
    assert(strcmp(result, "0") == 0);

    result = rt_to_string_byte(arena, 255);
    assert(strcmp(result, "255") == 0);

    result = rt_to_string_byte(arena, 171);
    assert(strcmp(result, "171") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_string(arena, "hello");
    assert(strcmp(result, "hello") == 0);

    result = rt_to_string_string(arena, "");
    assert(strcmp(result, "") == 0);

    result = rt_to_string_string(arena, NULL);
    assert(strcmp(result, "(null)") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_to_string_pointer(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *result = rt_to_string_pointer(arena, NULL);
    assert(strcmp(result, "nil") == 0);

    int x = 42;
    result = rt_to_string_pointer(arena, &x);
    /* Non-NULL pointer should return non-empty string that isn't "nil" */
    /* Format varies by platform (may or may not have "0x" prefix) */
    assert(result != NULL);
    assert(strlen(result) > 0);
    assert(strcmp(result, "nil") != 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Format Long Tests
 * ============================================================================ */

static void test_rt_format_long(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Default format */
    char *result = rt_format_long(arena, 42, NULL);
    assert(strcmp(result, "42") == 0);

    result = rt_format_long(arena, 42, "");
    assert(strcmp(result, "42") == 0);

    /* Decimal with width */
    result = rt_format_long(arena, 42, "5d");
    assert(strcmp(result, "   42") == 0);

    result = rt_format_long(arena, 42, "05d");
    assert(strcmp(result, "00042") == 0);

    /* Hexadecimal */
    result = rt_format_long(arena, 255, "x");
    assert(strcmp(result, "ff") == 0);

    result = rt_format_long(arena, 255, "X");
    assert(strcmp(result, "FF") == 0);

    result = rt_format_long(arena, 255, "04x");
    assert(strcmp(result, "00ff") == 0);

    /* Octal */
    result = rt_format_long(arena, 8, "o");
    assert(strcmp(result, "10") == 0);

    /* Binary */
    result = rt_format_long(arena, 5, "b");
    assert(strcmp(result, "101") == 0);

    result = rt_format_long(arena, 5, "08b");
    assert(strcmp(result, "00000101") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Format Double Tests
 * ============================================================================ */

static void test_rt_format_double(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Default format */
    char *result = rt_format_double(arena, 3.14159, NULL);
    assert(strstr(result, "3.14") != NULL);

    /* Fixed point with precision */
    result = rt_format_double(arena, 3.14159, ".2f");
    assert(strcmp(result, "3.14") == 0);

    result = rt_format_double(arena, 3.14159, ".4f");
    assert(strcmp(result, "3.1416") == 0);

    /* Scientific notation */
    result = rt_format_double(arena, 12345.0, "e");
    assert(strstr(result, "e") != NULL || strstr(result, "E") != NULL);

    /* Percentage */
    result = rt_format_double(arena, 0.75, ".0%");
    assert(strcmp(result, "75%") == 0);

    result = rt_format_double(arena, 0.755, ".1%");
    assert(strcmp(result, "75.5%") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Format String Tests
 * ============================================================================ */

static void test_rt_format_string(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Default format */
    char *result = rt_format_string(arena, "hello", NULL);
    assert(strcmp(result, "hello") == 0);

    /* Width padding (right-align by default) */
    result = rt_format_string(arena, "hi", "5");
    assert(strcmp(result, "   hi") == 0);

    /* Left align */
    result = rt_format_string(arena, "hi", "-5");
    assert(strcmp(result, "hi   ") == 0);

    /* Max length truncation */
    result = rt_format_string(arena, "hello world", ".5");
    assert(strcmp(result, "hello") == 0);

    /* Width and max length */
    result = rt_format_string(arena, "hello world", "10.5");
    assert(strcmp(result, "     hello") == 0);

    /* NULL input */
    result = rt_format_string(arena, NULL, NULL);
    assert(strcmp(result, "nil") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Mutable String Tests
 * ============================================================================ */

static void test_rt_string_from(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *str = rt_string_from(arena, "hello");
    assert(strcmp(str, "hello") == 0);
    assert(RT_STR_META(str)->length == 5);
    assert(RT_STR_META(str)->capacity >= 5);
    assert(RT_STR_META(str)->arena == arena);

    /* Empty string */
    str = rt_string_from(arena, "");
    assert(strcmp(str, "") == 0);
    assert(RT_STR_META(str)->length == 0);

    /* NULL becomes empty mutable string */
    str = rt_string_from(arena, NULL);
    assert(strcmp(str, "") == 0);
    assert(RT_STR_META(str)->length == 0);

    rt_arena_destroy(arena);
}

static void test_rt_string_ensure_mutable(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Already mutable string should return same pointer */
    char *mutable_str = rt_string_with_capacity(arena, 20);
    mutable_str = rt_string_append(mutable_str, "test");
    char *result = rt_string_ensure_mutable(arena, mutable_str);
    assert(result == mutable_str);

    /* NULL becomes empty mutable string */
    result = rt_string_ensure_mutable(arena, NULL);
    assert(result != NULL);
    assert(strcmp(result, "") == 0);
    assert(RT_STR_META(result)->arena == arena);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_string_main(void)
{
    TEST_SECTION("Runtime String");

    /* Concatenation */
    TEST_RUN("rt_str_concat_basic", test_rt_str_concat_basic);
    TEST_RUN("rt_str_concat_null", test_rt_str_concat_null);

    /* Query functions */
    TEST_RUN("rt_str_length", test_rt_str_length);
    TEST_RUN("rt_str_indexOf", test_rt_str_indexOf);
    TEST_RUN("rt_str_contains", test_rt_str_contains);
    TEST_RUN("rt_str_charAt", test_rt_str_charAt);
    TEST_RUN("rt_str_substring", test_rt_str_substring);

    /* Case conversion */
    TEST_RUN("rt_str_toUpper", test_rt_str_toUpper);
    TEST_RUN("rt_str_toLower", test_rt_str_toLower);

    /* Prefix/suffix */
    TEST_RUN("rt_str_startsWith", test_rt_str_startsWith);
    TEST_RUN("rt_str_endsWith", test_rt_str_endsWith);

    /* Trim and replace */
    TEST_RUN("rt_str_trim", test_rt_str_trim);
    TEST_RUN("rt_str_replace", test_rt_str_replace);

    /* Split */
    TEST_RUN("rt_str_split", test_rt_str_split);

    /* Type to string conversions */
    TEST_RUN("rt_to_string_long", test_rt_to_string_long);
    TEST_RUN("rt_to_string_double", test_rt_to_string_double);
    TEST_RUN("rt_to_string_char", test_rt_to_string_char);
    TEST_RUN("rt_to_string_bool", test_rt_to_string_bool);
    TEST_RUN("rt_to_string_byte", test_rt_to_string_byte);
    TEST_RUN("rt_to_string_string", test_rt_to_string_string);
    TEST_RUN("rt_to_string_pointer", test_rt_to_string_pointer);

    /* Format functions */
    TEST_RUN("rt_format_long", test_rt_format_long);
    TEST_RUN("rt_format_double", test_rt_format_double);
    TEST_RUN("rt_format_string", test_rt_format_string);

    /* Mutable strings */
    TEST_RUN("rt_string_from", test_rt_string_from);
    TEST_RUN("rt_string_ensure_mutable", test_rt_string_ensure_mutable);
}
