// tests/unit/runtime/runtime_any_tests.c
// Tests for runtime Any type boxing, unboxing, type checking

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../test_harness.h"
#include "runtime_any.h"
#include "arena/arena_v2.h"

/* ============================================================================
 * Boxing Tests
 * ============================================================================ */

static void test_rt_box_nil(void)
{
    RtAny any = rt_box_nil();
    assert(any.tag == RT_ANY_NIL);
}

static void test_rt_box_int_zero(void)
{
    RtAny any = rt_box_int(0);
    assert(any.tag == RT_ANY_INT);
    assert(any.value.i64 == 0);
}

static void test_rt_box_int_positive(void)
{
    RtAny any = rt_box_int(42);
    assert(any.tag == RT_ANY_INT);
    assert(any.value.i64 == 42);
}

static void test_rt_box_int_negative(void)
{
    RtAny any = rt_box_int(-100);
    assert(any.tag == RT_ANY_INT);
    assert(any.value.i64 == -100);
}

static void test_rt_box_int_max(void)
{
    RtAny any = rt_box_int(INT64_MAX);
    assert(any.tag == RT_ANY_INT);
    assert(any.value.i64 == INT64_MAX);
}

static void test_rt_box_int_min(void)
{
    RtAny any = rt_box_int(INT64_MIN);
    assert(any.tag == RT_ANY_INT);
    assert(any.value.i64 == INT64_MIN);
}

static void test_rt_box_long(void)
{
    RtAny any = rt_box_long(123456789012345LL);
    assert(any.tag == RT_ANY_LONG);
    assert(any.value.i64 == 123456789012345LL);
}

static void test_rt_box_double_zero(void)
{
    RtAny any = rt_box_double(0.0);
    assert(any.tag == RT_ANY_DOUBLE);
    assert(any.value.d == 0.0);
}

static void test_rt_box_double_positive(void)
{
    RtAny any = rt_box_double(3.14159);
    assert(any.tag == RT_ANY_DOUBLE);
    assert(any.value.d == 3.14159);
}

static void test_rt_box_double_negative(void)
{
    RtAny any = rt_box_double(-2.718);
    assert(any.tag == RT_ANY_DOUBLE);
    assert(any.value.d == -2.718);
}

static void test_rt_box_bool_true(void)
{
    RtAny any = rt_box_bool(true);
    assert(any.tag == RT_ANY_BOOL);
    assert(any.value.b == true);
}

static void test_rt_box_bool_false(void)
{
    RtAny any = rt_box_bool(false);
    assert(any.tag == RT_ANY_BOOL);
    assert(any.value.b == false);
}

static void test_rt_box_char_letter(void)
{
    RtAny any = rt_box_char('A');
    assert(any.tag == RT_ANY_CHAR);
    assert(any.value.c == 'A');
}

static void test_rt_box_char_digit(void)
{
    RtAny any = rt_box_char('9');
    assert(any.tag == RT_ANY_CHAR);
    assert(any.value.c == '9');
}

static void test_rt_box_char_special(void)
{
    RtAny any = rt_box_char('\n');
    assert(any.tag == RT_ANY_CHAR);
    assert(any.value.c == '\n');
}

static void test_rt_box_byte_zero(void)
{
    RtAny any = rt_box_byte(0);
    assert(any.tag == RT_ANY_BYTE);
    assert(any.value.byte == 0);
}

static void test_rt_box_byte_max(void)
{
    RtAny any = rt_box_byte(255);
    assert(any.tag == RT_ANY_BYTE);
    assert(any.value.byte == 255);
}

static void test_rt_box_string(void)
{
    RtAny any = rt_box_string("hello");
    assert(any.tag == RT_ANY_STRING);
    assert(strcmp(any.value.s, "hello") == 0);
}

static void test_rt_box_string_empty(void)
{
    RtAny any = rt_box_string("");
    assert(any.tag == RT_ANY_STRING);
    assert(strcmp(any.value.s, "") == 0);
}

/* ============================================================================
 * Unboxing Tests
 * ============================================================================ */

static void test_rt_unbox_int(void)
{
    RtAny any = rt_box_int(42);
    int64_t result = rt_unbox_int(any);
    assert(result == 42);
}

static void test_rt_unbox_long(void)
{
    RtAny any = rt_box_long(123456789LL);
    int64_t result = rt_unbox_long(any);
    assert(result == 123456789LL);
}

static void test_rt_unbox_double(void)
{
    RtAny any = rt_box_double(3.14);
    double result = rt_unbox_double(any);
    assert(result == 3.14);
}

static void test_rt_unbox_bool_true(void)
{
    RtAny any = rt_box_bool(true);
    bool result = rt_unbox_bool(any);
    assert(result == true);
}

static void test_rt_unbox_bool_false(void)
{
    RtAny any = rt_box_bool(false);
    bool result = rt_unbox_bool(any);
    assert(result == false);
}

static void test_rt_unbox_char(void)
{
    RtAny any = rt_box_char('Z');
    char result = rt_unbox_char(any);
    assert(result == 'Z');
}

static void test_rt_unbox_byte(void)
{
    RtAny any = rt_box_byte(128);
    uint8_t result = rt_unbox_byte(any);
    assert(result == 128);
}

static void test_rt_unbox_string(void)
{
    RtAny any = rt_box_string("test");
    const char *result = rt_unbox_string(any);
    assert(strcmp(result, "test") == 0);
}

/* ============================================================================
 * Type Checking Tests
 * ============================================================================ */

static void test_rt_any_is_nil(void)
{
    RtAny any = rt_box_nil();
    assert(rt_any_is_nil(any) == true);
    assert(rt_any_is_int(any) == false);
}

static void test_rt_any_is_int(void)
{
    RtAny any = rt_box_int(100);
    assert(rt_any_is_int(any) == true);
    assert(rt_any_is_nil(any) == false);
    assert(rt_any_is_double(any) == false);
}

static void test_rt_any_is_long(void)
{
    RtAny any = rt_box_long(999999999999LL);
    assert(rt_any_is_long(any) == true);
    assert(rt_any_is_int(any) == false);
}

static void test_rt_any_is_double(void)
{
    RtAny any = rt_box_double(1.5);
    assert(rt_any_is_double(any) == true);
    assert(rt_any_is_int(any) == false);
}

static void test_rt_any_is_bool(void)
{
    RtAny any = rt_box_bool(true);
    assert(rt_any_is_bool(any) == true);
    assert(rt_any_is_int(any) == false);
}

static void test_rt_any_is_char(void)
{
    RtAny any = rt_box_char('x');
    assert(rt_any_is_char(any) == true);
    assert(rt_any_is_string(any) == false);
}

static void test_rt_any_is_byte(void)
{
    RtAny any = rt_box_byte(200);
    assert(rt_any_is_byte(any) == true);
    assert(rt_any_is_int(any) == false);
}

static void test_rt_any_is_string(void)
{
    RtAny any = rt_box_string("hello");
    assert(rt_any_is_string(any) == true);
    assert(rt_any_is_char(any) == false);
}

/* ============================================================================
 * Get Tag Tests
 * ============================================================================ */

static void test_rt_any_get_tag_nil(void)
{
    RtAny any = rt_box_nil();
    assert(rt_any_get_tag(any) == RT_ANY_NIL);
}

static void test_rt_any_get_tag_int(void)
{
    RtAny any = rt_box_int(1);
    assert(rt_any_get_tag(any) == RT_ANY_INT);
}

static void test_rt_any_get_tag_double(void)
{
    RtAny any = rt_box_double(1.0);
    assert(rt_any_get_tag(any) == RT_ANY_DOUBLE);
}

static void test_rt_any_get_tag_bool(void)
{
    RtAny any = rt_box_bool(true);
    assert(rt_any_get_tag(any) == RT_ANY_BOOL);
}

static void test_rt_any_get_tag_string(void)
{
    RtAny any = rt_box_string("test");
    assert(rt_any_get_tag(any) == RT_ANY_STRING);
}

/* ============================================================================
 * Comparison Tests
 * ============================================================================ */

static void test_rt_any_equals_nil(void)
{
    RtAny a = rt_box_nil();
    RtAny b = rt_box_nil();
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_int_same(void)
{
    RtAny a = rt_box_int(42);
    RtAny b = rt_box_int(42);
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_int_different(void)
{
    RtAny a = rt_box_int(42);
    RtAny b = rt_box_int(43);
    assert(rt_any_equals(a, b) == false);
}

static void test_rt_any_equals_double_same(void)
{
    RtAny a = rt_box_double(3.14);
    RtAny b = rt_box_double(3.14);
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_double_different(void)
{
    RtAny a = rt_box_double(3.14);
    RtAny b = rt_box_double(2.71);
    assert(rt_any_equals(a, b) == false);
}

static void test_rt_any_equals_bool_same(void)
{
    RtAny a = rt_box_bool(true);
    RtAny b = rt_box_bool(true);
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_bool_different(void)
{
    RtAny a = rt_box_bool(true);
    RtAny b = rt_box_bool(false);
    assert(rt_any_equals(a, b) == false);
}

static void test_rt_any_equals_char_same(void)
{
    RtAny a = rt_box_char('A');
    RtAny b = rt_box_char('A');
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_char_different(void)
{
    RtAny a = rt_box_char('A');
    RtAny b = rt_box_char('B');
    assert(rt_any_equals(a, b) == false);
}

static void test_rt_any_equals_string_same(void)
{
    RtAny a = rt_box_string("hello");
    RtAny b = rt_box_string("hello");
    assert(rt_any_equals(a, b) == true);
}

static void test_rt_any_equals_string_different(void)
{
    RtAny a = rt_box_string("hello");
    RtAny b = rt_box_string("world");
    assert(rt_any_equals(a, b) == false);
}

static void test_rt_any_equals_different_types(void)
{
    RtAny a = rt_box_int(42);
    RtAny b = rt_box_double(42.0);
    assert(rt_any_equals(a, b) == false);
}

/* ============================================================================
 * Same Type Tests
 * ============================================================================ */

static void test_rt_any_same_type_both_int(void)
{
    RtAny a = rt_box_int(1);
    RtAny b = rt_box_int(2);
    assert(rt_any_same_type(a, b) == true);
}

static void test_rt_any_same_type_int_double(void)
{
    RtAny a = rt_box_int(1);
    RtAny b = rt_box_double(1.0);
    assert(rt_any_same_type(a, b) == false);
}

static void test_rt_any_same_type_both_string(void)
{
    RtAny a = rt_box_string("a");
    RtAny b = rt_box_string("b");
    assert(rt_any_same_type(a, b) == true);
}

static void test_rt_any_same_type_bool_nil(void)
{
    RtAny a = rt_box_bool(true);
    RtAny b = rt_box_nil();
    assert(rt_any_same_type(a, b) == false);
}

/* ============================================================================
 * Type Name Tests
 * ============================================================================ */

static void test_rt_any_type_name_nil(void)
{
    RtAny any = rt_box_nil();
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "nil") == 0);
}

static void test_rt_any_type_name_int(void)
{
    RtAny any = rt_box_int(1);
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "int") == 0);
}

static void test_rt_any_type_name_double(void)
{
    RtAny any = rt_box_double(1.0);
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "double") == 0);
}

static void test_rt_any_type_name_bool(void)
{
    RtAny any = rt_box_bool(true);
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "bool") == 0);
}

static void test_rt_any_type_name_char(void)
{
    RtAny any = rt_box_char('x');
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "char") == 0);
}

static void test_rt_any_type_name_string(void)
{
    RtAny any = rt_box_string("test");
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "str") == 0);
}

static void test_rt_any_type_name_byte(void)
{
    RtAny any = rt_box_byte(1);
    const char *name = rt_any_type_name(any);
    assert(strcmp(name, "byte") == 0);
}

/* ============================================================================
 * Tag Name Tests
 * ============================================================================ */

static void test_rt_any_tag_name_nil(void)
{
    const char *name = rt_any_tag_name(RT_ANY_NIL);
    assert(strcmp(name, "nil") == 0);
}

static void test_rt_any_tag_name_int(void)
{
    const char *name = rt_any_tag_name(RT_ANY_INT);
    assert(strcmp(name, "int") == 0);
}

static void test_rt_any_tag_name_double(void)
{
    const char *name = rt_any_tag_name(RT_ANY_DOUBLE);
    assert(strcmp(name, "double") == 0);
}

static void test_rt_any_tag_name_bool(void)
{
    const char *name = rt_any_tag_name(RT_ANY_BOOL);
    assert(strcmp(name, "bool") == 0);
}

static void test_rt_any_tag_name_string(void)
{
    const char *name = rt_any_tag_name(RT_ANY_STRING);
    assert(strcmp(name, "str") == 0);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_rt_any_main(void)
{
    TEST_SECTION("Runtime Any - Boxing");
    TEST_RUN("rt_box_nil", test_rt_box_nil);
    TEST_RUN("rt_box_int_zero", test_rt_box_int_zero);
    TEST_RUN("rt_box_int_positive", test_rt_box_int_positive);
    TEST_RUN("rt_box_int_negative", test_rt_box_int_negative);
    TEST_RUN("rt_box_int_max", test_rt_box_int_max);
    TEST_RUN("rt_box_int_min", test_rt_box_int_min);
    TEST_RUN("rt_box_long", test_rt_box_long);
    TEST_RUN("rt_box_double_zero", test_rt_box_double_zero);
    TEST_RUN("rt_box_double_positive", test_rt_box_double_positive);
    TEST_RUN("rt_box_double_negative", test_rt_box_double_negative);
    TEST_RUN("rt_box_bool_true", test_rt_box_bool_true);
    TEST_RUN("rt_box_bool_false", test_rt_box_bool_false);
    TEST_RUN("rt_box_char_letter", test_rt_box_char_letter);
    TEST_RUN("rt_box_char_digit", test_rt_box_char_digit);
    TEST_RUN("rt_box_char_special", test_rt_box_char_special);
    TEST_RUN("rt_box_byte_zero", test_rt_box_byte_zero);
    TEST_RUN("rt_box_byte_max", test_rt_box_byte_max);
    TEST_RUN("rt_box_string", test_rt_box_string);
    TEST_RUN("rt_box_string_empty", test_rt_box_string_empty);

    TEST_SECTION("Runtime Any - Unboxing");
    TEST_RUN("rt_unbox_int", test_rt_unbox_int);
    TEST_RUN("rt_unbox_long", test_rt_unbox_long);
    TEST_RUN("rt_unbox_double", test_rt_unbox_double);
    TEST_RUN("rt_unbox_bool_true", test_rt_unbox_bool_true);
    TEST_RUN("rt_unbox_bool_false", test_rt_unbox_bool_false);
    TEST_RUN("rt_unbox_char", test_rt_unbox_char);
    TEST_RUN("rt_unbox_byte", test_rt_unbox_byte);
    TEST_RUN("rt_unbox_string", test_rt_unbox_string);

    TEST_SECTION("Runtime Any - Type Checking");
    TEST_RUN("rt_any_is_nil", test_rt_any_is_nil);
    TEST_RUN("rt_any_is_int", test_rt_any_is_int);
    TEST_RUN("rt_any_is_long", test_rt_any_is_long);
    TEST_RUN("rt_any_is_double", test_rt_any_is_double);
    TEST_RUN("rt_any_is_bool", test_rt_any_is_bool);
    TEST_RUN("rt_any_is_char", test_rt_any_is_char);
    TEST_RUN("rt_any_is_byte", test_rt_any_is_byte);
    TEST_RUN("rt_any_is_string", test_rt_any_is_string);

    TEST_SECTION("Runtime Any - Get Tag");
    TEST_RUN("rt_any_get_tag_nil", test_rt_any_get_tag_nil);
    TEST_RUN("rt_any_get_tag_int", test_rt_any_get_tag_int);
    TEST_RUN("rt_any_get_tag_double", test_rt_any_get_tag_double);
    TEST_RUN("rt_any_get_tag_bool", test_rt_any_get_tag_bool);
    TEST_RUN("rt_any_get_tag_string", test_rt_any_get_tag_string);

    TEST_SECTION("Runtime Any - Equals");
    TEST_RUN("rt_any_equals_nil", test_rt_any_equals_nil);
    TEST_RUN("rt_any_equals_int_same", test_rt_any_equals_int_same);
    TEST_RUN("rt_any_equals_int_different", test_rt_any_equals_int_different);
    TEST_RUN("rt_any_equals_double_same", test_rt_any_equals_double_same);
    TEST_RUN("rt_any_equals_double_different", test_rt_any_equals_double_different);
    TEST_RUN("rt_any_equals_bool_same", test_rt_any_equals_bool_same);
    TEST_RUN("rt_any_equals_bool_different", test_rt_any_equals_bool_different);
    TEST_RUN("rt_any_equals_char_same", test_rt_any_equals_char_same);
    TEST_RUN("rt_any_equals_char_different", test_rt_any_equals_char_different);
    TEST_RUN("rt_any_equals_string_same", test_rt_any_equals_string_same);
    TEST_RUN("rt_any_equals_string_different", test_rt_any_equals_string_different);
    TEST_RUN("rt_any_equals_different_types", test_rt_any_equals_different_types);

    TEST_SECTION("Runtime Any - Same Type");
    TEST_RUN("rt_any_same_type_both_int", test_rt_any_same_type_both_int);
    TEST_RUN("rt_any_same_type_int_double", test_rt_any_same_type_int_double);
    TEST_RUN("rt_any_same_type_both_string", test_rt_any_same_type_both_string);
    TEST_RUN("rt_any_same_type_bool_nil", test_rt_any_same_type_bool_nil);

    TEST_SECTION("Runtime Any - Type Name");
    TEST_RUN("rt_any_type_name_nil", test_rt_any_type_name_nil);
    TEST_RUN("rt_any_type_name_int", test_rt_any_type_name_int);
    TEST_RUN("rt_any_type_name_double", test_rt_any_type_name_double);
    TEST_RUN("rt_any_type_name_bool", test_rt_any_type_name_bool);
    TEST_RUN("rt_any_type_name_char", test_rt_any_type_name_char);
    TEST_RUN("rt_any_type_name_string", test_rt_any_type_name_string);
    TEST_RUN("rt_any_type_name_byte", test_rt_any_type_name_byte);

    TEST_SECTION("Runtime Any - Tag Name");
    TEST_RUN("rt_any_tag_name_nil", test_rt_any_tag_name_nil);
    TEST_RUN("rt_any_tag_name_int", test_rt_any_tag_name_int);
    TEST_RUN("rt_any_tag_name_double", test_rt_any_tag_name_double);
    TEST_RUN("rt_any_tag_name_bool", test_rt_any_tag_name_bool);
    TEST_RUN("rt_any_tag_name_string", test_rt_any_tag_name_string);
}
