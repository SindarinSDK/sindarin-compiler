// tests/unit/type_checker/type_checker_tests_coverage.c
// Additional type checker utility function coverage tests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../ast.h"
#include "../type_checker.h"
#include "../symbol_table.h"
#include "../test_harness.h"

#define TC_ARENA_SIZE 4096

/* ============================================================================
 * is_variadic_compatible_type Tests
 * ============================================================================ */

static void test_is_variadic_compatible_int(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_variadic_compatible_type(int_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_double(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_variadic_compatible_type(double_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_string(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_variadic_compatible_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_bool(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_variadic_compatible_type(bool_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_char(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(is_variadic_compatible_type(char_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_byte(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(is_variadic_compatible_type(byte_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_long(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(is_variadic_compatible_type(long_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_any(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *any_type = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(is_variadic_compatible_type(any_type) == true);

    arena_free(&arena);
}

static void test_is_variadic_compatible_void(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(is_variadic_compatible_type(void_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_c_compatible_type Tests
 * ============================================================================ */

static void test_is_c_compatible_int(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_c_compatible_type(int_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_double(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_c_compatible_type(double_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_bool(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_c_compatible_type(bool_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_char(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(is_c_compatible_type(char_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_byte(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(is_c_compatible_type(byte_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_long(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(is_c_compatible_type(long_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_void(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(is_c_compatible_type(void_type) == true);

    arena_free(&arena);
}

static void test_is_c_compatible_string(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    // Strings are not directly C-compatible
    assert(is_c_compatible_type(str_type) == false);

    arena_free(&arena);
}

static void test_is_c_compatible_array(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    // Arrays are not C-compatible
    assert(is_c_compatible_type(arr_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * can_escape_private Tests
 * ============================================================================ */

static void test_can_escape_private_int(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    // Primitives can escape
    assert(can_escape_private(int_type) == true);

    arena_free(&arena);
}

static void test_can_escape_private_double(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(can_escape_private(double_type) == true);

    arena_free(&arena);
}

static void test_can_escape_private_bool(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(can_escape_private(bool_type) == true);

    arena_free(&arena);
}

static void test_can_escape_private_char(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(can_escape_private(char_type) == true);

    arena_free(&arena);
}

static void test_can_escape_private_void(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(can_escape_private(void_type) == true);

    arena_free(&arena);
}

static void test_can_escape_private_string(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    // Strings (heap-allocated) cannot escape private by default
    assert(can_escape_private(str_type) == false);

    arena_free(&arena);
}

static void test_can_escape_private_array(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    // Arrays cannot escape private
    assert(can_escape_private(arr_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Additional is_numeric_type edge cases
 * ============================================================================ */

static void test_cov_is_numeric_type_int32(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    assert(is_numeric_type(int32_type) == true);

    arena_free(&arena);
}

static void test_cov_is_numeric_type_uint(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    assert(is_numeric_type(uint_type) == true);

    arena_free(&arena);
}

static void test_cov_is_numeric_type_uint32(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    assert(is_numeric_type(uint32_type) == true);

    arena_free(&arena);
}

static void test_cov_is_numeric_type_float(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    assert(is_numeric_type(float_type) == true);

    arena_free(&arena);
}

/* ============================================================================
 * Additional is_primitive_type edge cases
 * ============================================================================ */

static void test_is_primitive_type_int32(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    assert(is_primitive_type(int32_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_type_uint(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    assert(is_primitive_type(uint_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_type_uint32(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    assert(is_primitive_type(uint32_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_type_float(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    assert(is_primitive_type(float_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_type_any(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *any_type = ast_create_primitive_type(&arena, TYPE_ANY);
    // Any is not considered a primitive
    assert(is_primitive_type(any_type) == false);

    arena_free(&arena);
}

static void test_is_primitive_type_array(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(is_primitive_type(arr_type) == false);

    arena_free(&arena);
}

static void test_is_primitive_type_function(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);
    assert(is_primitive_type(func_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_reference_type Tests
 * ============================================================================ */

static void test_is_reference_type_string(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_reference_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_reference_type_array(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(is_reference_type(arr_type) == true);

    arena_free(&arena);
}

static void test_is_reference_type_int(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_reference_type(int_type) == false);

    arena_free(&arena);
}

static void test_is_reference_type_double(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_reference_type(double_type) == false);

    arena_free(&arena);
}

static void test_is_reference_type_bool(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_reference_type(bool_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Additional is_printable_type edge cases
 * ============================================================================ */

static void test_is_printable_type_any(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *any_type = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(is_printable_type(any_type) == true);

    arena_free(&arena);
}

static void test_is_printable_type_void(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    // void is not printable
    assert(is_printable_type(void_type) == false);

    arena_free(&arena);
}

static void test_is_printable_type_long(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(is_printable_type(long_type) == true);

    arena_free(&arena);
}

static void test_is_printable_type_byte(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(is_printable_type(byte_type) == true);

    arena_free(&arena);
}

static void test_is_printable_type_function(void)
{
    Arena arena;
    arena_init(&arena, TC_ARENA_SIZE);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);
    // Functions are not directly printable
    assert(is_printable_type(func_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_comparison_operator edge cases
 * ============================================================================ */

static void test_is_comparison_operator_equal(void)
{
    assert(is_comparison_operator(TOKEN_EQUAL_EQUAL) == true);
}

static void test_is_comparison_operator_not_equal(void)
{
    assert(is_comparison_operator(TOKEN_BANG_EQUAL) == true);
}

static void test_is_comparison_operator_less(void)
{
    assert(is_comparison_operator(TOKEN_LESS) == true);
}

static void test_is_comparison_operator_less_equal(void)
{
    assert(is_comparison_operator(TOKEN_LESS_EQUAL) == true);
}

static void test_is_comparison_operator_greater(void)
{
    assert(is_comparison_operator(TOKEN_GREATER) == true);
}

static void test_is_comparison_operator_greater_equal(void)
{
    assert(is_comparison_operator(TOKEN_GREATER_EQUAL) == true);
}

static void test_is_comparison_operator_plus_false(void)
{
    assert(is_comparison_operator(TOKEN_PLUS) == false);
}

static void test_is_comparison_operator_minus_false(void)
{
    assert(is_comparison_operator(TOKEN_MINUS) == false);
}

static void test_is_comparison_operator_and_false(void)
{
    assert(is_comparison_operator(TOKEN_AND) == false);
}

static void test_is_comparison_operator_or_false(void)
{
    assert(is_comparison_operator(TOKEN_OR) == false);
}

/* ============================================================================
 * is_arithmetic_operator edge cases
 * ============================================================================ */

static void test_is_arithmetic_operator_plus(void)
{
    assert(is_arithmetic_operator(TOKEN_PLUS) == true);
}

static void test_is_arithmetic_operator_minus(void)
{
    assert(is_arithmetic_operator(TOKEN_MINUS) == true);
}

static void test_is_arithmetic_operator_star(void)
{
    assert(is_arithmetic_operator(TOKEN_STAR) == true);
}

static void test_is_arithmetic_operator_slash(void)
{
    assert(is_arithmetic_operator(TOKEN_SLASH) == true);
}

static void test_is_arithmetic_operator_modulo(void)
{
    assert(is_arithmetic_operator(TOKEN_MODULO) == true);
}

static void test_is_arithmetic_operator_equal_false(void)
{
    assert(is_arithmetic_operator(TOKEN_EQUAL_EQUAL) == false);
}

static void test_is_arithmetic_operator_less_false(void)
{
    assert(is_arithmetic_operator(TOKEN_LESS) == false);
}

static void test_is_arithmetic_operator_and_false(void)
{
    assert(is_arithmetic_operator(TOKEN_AND) == false);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_type_checker_coverage_main(void)
{
    TEST_SECTION("Type Checker - is_variadic_compatible_type");
    TEST_RUN("is_variadic_compatible_int", test_is_variadic_compatible_int);
    TEST_RUN("is_variadic_compatible_double", test_is_variadic_compatible_double);
    TEST_RUN("is_variadic_compatible_string", test_is_variadic_compatible_string);
    TEST_RUN("is_variadic_compatible_bool", test_is_variadic_compatible_bool);
    TEST_RUN("is_variadic_compatible_char", test_is_variadic_compatible_char);
    TEST_RUN("is_variadic_compatible_byte", test_is_variadic_compatible_byte);
    TEST_RUN("is_variadic_compatible_long", test_is_variadic_compatible_long);
    TEST_RUN("is_variadic_compatible_any", test_is_variadic_compatible_any);
    TEST_RUN("is_variadic_compatible_void", test_is_variadic_compatible_void);

    TEST_SECTION("Type Checker - is_c_compatible_type");
    TEST_RUN("is_c_compatible_int", test_is_c_compatible_int);
    TEST_RUN("is_c_compatible_double", test_is_c_compatible_double);
    TEST_RUN("is_c_compatible_bool", test_is_c_compatible_bool);
    TEST_RUN("is_c_compatible_char", test_is_c_compatible_char);
    TEST_RUN("is_c_compatible_byte", test_is_c_compatible_byte);
    TEST_RUN("is_c_compatible_long", test_is_c_compatible_long);
    TEST_RUN("is_c_compatible_void", test_is_c_compatible_void);
    TEST_RUN("is_c_compatible_string", test_is_c_compatible_string);
    TEST_RUN("is_c_compatible_array", test_is_c_compatible_array);

    TEST_SECTION("Type Checker - can_escape_private");
    TEST_RUN("can_escape_private_int", test_can_escape_private_int);
    TEST_RUN("can_escape_private_double", test_can_escape_private_double);
    TEST_RUN("can_escape_private_bool", test_can_escape_private_bool);
    TEST_RUN("can_escape_private_char", test_can_escape_private_char);
    TEST_RUN("can_escape_private_void", test_can_escape_private_void);
    TEST_RUN("can_escape_private_string", test_can_escape_private_string);
    TEST_RUN("can_escape_private_array", test_can_escape_private_array);

    TEST_SECTION("Type Checker - is_numeric_type edge cases");
    TEST_RUN("is_numeric_type_int32", test_cov_is_numeric_type_int32);
    TEST_RUN("is_numeric_type_uint", test_cov_is_numeric_type_uint);
    TEST_RUN("is_numeric_type_uint32", test_cov_is_numeric_type_uint32);
    TEST_RUN("is_numeric_type_float", test_cov_is_numeric_type_float);

    TEST_SECTION("Type Checker - is_primitive_type edge cases");
    TEST_RUN("is_primitive_type_int32", test_is_primitive_type_int32);
    TEST_RUN("is_primitive_type_uint", test_is_primitive_type_uint);
    TEST_RUN("is_primitive_type_uint32", test_is_primitive_type_uint32);
    TEST_RUN("is_primitive_type_float", test_is_primitive_type_float);
    TEST_RUN("is_primitive_type_any", test_is_primitive_type_any);
    TEST_RUN("is_primitive_type_array", test_is_primitive_type_array);
    TEST_RUN("is_primitive_type_function", test_is_primitive_type_function);

    TEST_SECTION("Type Checker - is_reference_type");
    TEST_RUN("is_reference_type_string", test_is_reference_type_string);
    TEST_RUN("is_reference_type_array", test_is_reference_type_array);
    TEST_RUN("is_reference_type_int", test_is_reference_type_int);
    TEST_RUN("is_reference_type_double", test_is_reference_type_double);
    TEST_RUN("is_reference_type_bool", test_is_reference_type_bool);

    TEST_SECTION("Type Checker - is_printable_type edge cases");
    TEST_RUN("is_printable_type_any", test_is_printable_type_any);
    TEST_RUN("is_printable_type_void", test_is_printable_type_void);
    TEST_RUN("is_printable_type_long", test_is_printable_type_long);
    TEST_RUN("is_printable_type_byte", test_is_printable_type_byte);
    TEST_RUN("is_printable_type_function", test_is_printable_type_function);

    TEST_SECTION("Type Checker - is_comparison_operator");
    TEST_RUN("is_comparison_operator_equal", test_is_comparison_operator_equal);
    TEST_RUN("is_comparison_operator_not_equal", test_is_comparison_operator_not_equal);
    TEST_RUN("is_comparison_operator_less", test_is_comparison_operator_less);
    TEST_RUN("is_comparison_operator_less_equal", test_is_comparison_operator_less_equal);
    TEST_RUN("is_comparison_operator_greater", test_is_comparison_operator_greater);
    TEST_RUN("is_comparison_operator_greater_equal", test_is_comparison_operator_greater_equal);
    TEST_RUN("is_comparison_operator_plus_false", test_is_comparison_operator_plus_false);
    TEST_RUN("is_comparison_operator_minus_false", test_is_comparison_operator_minus_false);
    TEST_RUN("is_comparison_operator_and_false", test_is_comparison_operator_and_false);
    TEST_RUN("is_comparison_operator_or_false", test_is_comparison_operator_or_false);

    TEST_SECTION("Type Checker - is_arithmetic_operator");
    TEST_RUN("is_arithmetic_operator_plus", test_is_arithmetic_operator_plus);
    TEST_RUN("is_arithmetic_operator_minus", test_is_arithmetic_operator_minus);
    TEST_RUN("is_arithmetic_operator_star", test_is_arithmetic_operator_star);
    TEST_RUN("is_arithmetic_operator_slash", test_is_arithmetic_operator_slash);
    TEST_RUN("is_arithmetic_operator_modulo", test_is_arithmetic_operator_modulo);
    TEST_RUN("is_arithmetic_operator_equal_false", test_is_arithmetic_operator_equal_false);
    TEST_RUN("is_arithmetic_operator_less_false", test_is_arithmetic_operator_less_false);
    TEST_RUN("is_arithmetic_operator_and_false", test_is_arithmetic_operator_and_false);
}
