// tests/code_gen_tests_helpers.c
// Tests for code generation helper/utility functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"

/* ============================================================================
 * is_handle_type Tests
 * ============================================================================ */

static void test_is_handle_type_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_handle_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_handle_type_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_handle_type(int_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_handle_type(bool_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_handle_type(double_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(is_handle_type(char_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(is_handle_type(byte_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(is_handle_type(void_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(is_handle_type(long_type) == false);

    arena_free(&arena);
}

static void test_is_handle_type_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(is_handle_type(arr_type) == true);

    arena_free(&arena);
}

static void test_is_handle_type_any(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *any_type = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(is_handle_type(any_type) == true);

    arena_free(&arena);
}

/* ============================================================================
 * escape_char_literal Tests
 * ============================================================================ */

static void test_escape_char_newline(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\n');
    assert(strcmp(result, "'\\n'") == 0);

    arena_free(&arena);
}

static void test_escape_char_tab(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\t');
    assert(strcmp(result, "'\\t'") == 0);

    arena_free(&arena);
}

static void test_escape_char_backslash(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\\');
    assert(strcmp(result, "'\\\\'") == 0);

    arena_free(&arena);
}

static void test_escape_char_single_quote(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\'');
    assert(strcmp(result, "'\\''") == 0);

    arena_free(&arena);
}

static void test_escape_char_normal(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, 'a');
    assert(strcmp(result, "'a'") == 0);

    arena_free(&arena);
}

static void test_escape_char_carriage_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\r');
    assert(strcmp(result, "'\\r'") == 0);

    arena_free(&arena);
}

static void test_escape_char_null(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_char_literal(&arena, '\0');
    assert(strcmp(result, "'\\0'") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * escape_c_string Tests
 * ============================================================================ */

static void test_escape_string_empty(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "");
    assert(strcmp(result, "") == 0);

    arena_free(&arena);
}

static void test_escape_string_normal(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "hello");
    assert(strcmp(result, "hello") == 0);

    arena_free(&arena);
}

static void test_escape_string_with_newline(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "hello\nworld");
    assert(strcmp(result, "hello\\nworld") == 0);

    arena_free(&arena);
}

static void test_escape_string_with_tab(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "hello\tworld");
    assert(strcmp(result, "hello\\tworld") == 0);

    arena_free(&arena);
}

static void test_escape_string_with_backslash(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "path\\to\\file");
    assert(strcmp(result, "path\\\\to\\\\file") == 0);

    arena_free(&arena);
}

static void test_escape_string_with_quote(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    char *result = escape_c_string(&arena, "say \"hello\"");
    assert(strcmp(result, "say \\\"hello\\\"") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * get_c_type Tests
 * ============================================================================ */

static void test_get_c_type_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    const char *result = get_c_type(&arena, int_type);
    assert(strcmp(result, "long long") == 0);

    arena_free(&arena);
}

static void test_get_c_type_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    const char *result = get_c_type(&arena, double_type);
    assert(strcmp(result, "double") == 0);

    arena_free(&arena);
}

static void test_get_c_type_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    const char *result = get_c_type(&arena, bool_type);
    assert(strcmp(result, "bool") == 0);

    arena_free(&arena);
}

static void test_get_c_type_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    const char *result = get_c_type(&arena, char_type);
    assert(strcmp(result, "char") == 0);

    arena_free(&arena);
}

static void test_get_c_type_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    const char *result = get_c_type(&arena, byte_type);
    assert(strcmp(result, "uint8_t") == 0);

    arena_free(&arena);
}

static void test_get_c_type_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    const char *result = get_c_type(&arena, void_type);
    assert(strcmp(result, "void") == 0);

    arena_free(&arena);
}

static void test_get_c_type_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    const char *result = get_c_type(&arena, long_type);
    assert(strcmp(result, "long long") == 0);

    arena_free(&arena);
}

static void test_get_c_type_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    const char *result = get_c_type(&arena, str_type);
    assert(strcmp(result, "RtString *") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * get_default_value Tests
 * ============================================================================ */

static void test_get_default_value_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    const char *result = get_default_value(int_type);
    assert(strcmp(result, "0") == 0);

    arena_free(&arena);
}

static void test_get_default_value_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    const char *result = get_default_value(double_type);
    assert(strcmp(result, "0.0") == 0);

    arena_free(&arena);
}

static void test_get_default_value_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    const char *result = get_default_value(bool_type);
    assert(strcmp(result, "false") == 0);

    arena_free(&arena);
}

static void test_get_default_value_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    const char *result = get_default_value(char_type);
    assert(strcmp(result, "'\\0'") == 0);

    arena_free(&arena);
}

static void test_get_default_value_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    const char *result = get_default_value(byte_type);
    assert(strcmp(result, "0") == 0);

    arena_free(&arena);
}

static void test_get_default_value_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    const char *result = get_default_value(str_type);
    assert(strcmp(result, "NULL") == 0);

    arena_free(&arena);
}

static void test_get_default_value_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    const char *result = get_default_value(long_type);
    assert(strcmp(result, "0") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * get_boxing_function Tests
 * ============================================================================ */

static void test_get_boxing_function_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    const char *result = get_boxing_function(int_type);
    assert(strcmp(result, "rt_any_box_int") == 0);

    arena_free(&arena);
}

static void test_get_boxing_function_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    const char *result = get_boxing_function(double_type);
    assert(strcmp(result, "rt_any_box_double") == 0);

    arena_free(&arena);
}

static void test_get_boxing_function_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    const char *result = get_boxing_function(bool_type);
    assert(strcmp(result, "rt_any_box_bool") == 0);

    arena_free(&arena);
}

static void test_get_boxing_function_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    const char *result = get_boxing_function(char_type);
    assert(strcmp(result, "rt_any_box_char") == 0);

    arena_free(&arena);
}

static void test_get_boxing_function_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    const char *result = get_boxing_function(str_type);
    assert(strcmp(result, "rt_any_box_string") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * get_unboxing_function Tests
 * ============================================================================ */

static void test_get_unboxing_function_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    const char *result = get_unboxing_function(int_type);
    assert(strcmp(result, "rt_any_unbox_int") == 0);

    arena_free(&arena);
}

static void test_get_unboxing_function_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    const char *result = get_unboxing_function(double_type);
    assert(strcmp(result, "rt_any_unbox_double") == 0);

    arena_free(&arena);
}

static void test_get_unboxing_function_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    const char *result = get_unboxing_function(bool_type);
    assert(strcmp(result, "rt_any_unbox_bool") == 0);

    arena_free(&arena);
}

static void test_get_unboxing_function_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    const char *result = get_unboxing_function(char_type);
    assert(strcmp(result, "rt_any_unbox_char") == 0);

    arena_free(&arena);
}

static void test_get_unboxing_function_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    const char *result = get_unboxing_function(str_type);
    assert(strcmp(result, "rt_any_unbox_string") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * get_rt_to_string_func Tests
 * ============================================================================ */

static void test_get_rt_to_string_func_int(void)
{
    const char *result = get_rt_to_string_func(TYPE_INT);
    assert(strcmp(result, "rt_int_to_string") == 0);
}

static void test_get_rt_to_string_func_double(void)
{
    const char *result = get_rt_to_string_func(TYPE_DOUBLE);
    assert(strcmp(result, "rt_double_to_string") == 0);
}

static void test_get_rt_to_string_func_bool(void)
{
    const char *result = get_rt_to_string_func(TYPE_BOOL);
    assert(strcmp(result, "rt_bool_to_string") == 0);
}

static void test_get_rt_to_string_func_char(void)
{
    const char *result = get_rt_to_string_func(TYPE_CHAR);
    assert(strcmp(result, "rt_char_to_string") == 0);
}

static void test_get_rt_to_string_func_byte(void)
{
    const char *result = get_rt_to_string_func(TYPE_BYTE);
    assert(strcmp(result, "rt_byte_to_string") == 0);
}

static void test_get_rt_to_string_func_long(void)
{
    const char *result = get_rt_to_string_func(TYPE_LONG);
    assert(strcmp(result, "rt_long_to_string") == 0);
}

/* ============================================================================
 * code_gen_binary_op_str Tests
 * ============================================================================ */

static void test_binary_op_str_plus(void)
{
    char *result = code_gen_binary_op_str(TOKEN_PLUS);
    assert(strcmp(result, "+") == 0);
}

static void test_binary_op_str_minus(void)
{
    char *result = code_gen_binary_op_str(TOKEN_MINUS);
    assert(strcmp(result, "-") == 0);
}

static void test_binary_op_str_star(void)
{
    char *result = code_gen_binary_op_str(TOKEN_STAR);
    assert(strcmp(result, "*") == 0);
}

static void test_binary_op_str_slash(void)
{
    char *result = code_gen_binary_op_str(TOKEN_SLASH);
    assert(strcmp(result, "/") == 0);
}

static void test_binary_op_str_modulo(void)
{
    char *result = code_gen_binary_op_str(TOKEN_MODULO);
    assert(strcmp(result, "%") == 0);
}

static void test_binary_op_str_equal_equal(void)
{
    char *result = code_gen_binary_op_str(TOKEN_EQUAL_EQUAL);
    assert(strcmp(result, "==") == 0);
}

static void test_binary_op_str_bang_equal(void)
{
    char *result = code_gen_binary_op_str(TOKEN_BANG_EQUAL);
    assert(strcmp(result, "!=") == 0);
}

static void test_binary_op_str_less(void)
{
    char *result = code_gen_binary_op_str(TOKEN_LESS);
    assert(strcmp(result, "<") == 0);
}

static void test_binary_op_str_less_equal(void)
{
    char *result = code_gen_binary_op_str(TOKEN_LESS_EQUAL);
    assert(strcmp(result, "<=") == 0);
}

static void test_binary_op_str_greater(void)
{
    char *result = code_gen_binary_op_str(TOKEN_GREATER);
    assert(strcmp(result, ">") == 0);
}

static void test_binary_op_str_greater_equal(void)
{
    char *result = code_gen_binary_op_str(TOKEN_GREATER_EQUAL);
    assert(strcmp(result, ">=") == 0);
}

static void test_binary_op_str_and(void)
{
    char *result = code_gen_binary_op_str(TOKEN_AND);
    assert(strcmp(result, "&&") == 0);
}

static void test_binary_op_str_or(void)
{
    char *result = code_gen_binary_op_str(TOKEN_OR);
    assert(strcmp(result, "||") == 0);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_code_gen_helpers_main(void)
{
    TEST_SECTION("Code Gen Helpers - is_handle_type");
    TEST_RUN("is_handle_type_string", test_is_handle_type_string);
    TEST_RUN("is_handle_type_int", test_is_handle_type_int);
    TEST_RUN("is_handle_type_bool", test_is_handle_type_bool);
    TEST_RUN("is_handle_type_double", test_is_handle_type_double);
    TEST_RUN("is_handle_type_char", test_is_handle_type_char);
    TEST_RUN("is_handle_type_byte", test_is_handle_type_byte);
    TEST_RUN("is_handle_type_void", test_is_handle_type_void);
    TEST_RUN("is_handle_type_long", test_is_handle_type_long);
    TEST_RUN("is_handle_type_array", test_is_handle_type_array);
    TEST_RUN("is_handle_type_any", test_is_handle_type_any);

    TEST_SECTION("Code Gen Helpers - escape_char_literal");
    TEST_RUN("escape_char_newline", test_escape_char_newline);
    TEST_RUN("escape_char_tab", test_escape_char_tab);
    TEST_RUN("escape_char_backslash", test_escape_char_backslash);
    TEST_RUN("escape_char_single_quote", test_escape_char_single_quote);
    TEST_RUN("escape_char_normal", test_escape_char_normal);
    TEST_RUN("escape_char_carriage_return", test_escape_char_carriage_return);
    TEST_RUN("escape_char_null", test_escape_char_null);

    TEST_SECTION("Code Gen Helpers - escape_c_string");
    TEST_RUN("escape_string_empty", test_escape_string_empty);
    TEST_RUN("escape_string_normal", test_escape_string_normal);
    TEST_RUN("escape_string_with_newline", test_escape_string_with_newline);
    TEST_RUN("escape_string_with_tab", test_escape_string_with_tab);
    TEST_RUN("escape_string_with_backslash", test_escape_string_with_backslash);
    TEST_RUN("escape_string_with_quote", test_escape_string_with_quote);

    TEST_SECTION("Code Gen Helpers - get_c_type");
    TEST_RUN("get_c_type_int", test_get_c_type_int);
    TEST_RUN("get_c_type_double", test_get_c_type_double);
    TEST_RUN("get_c_type_bool", test_get_c_type_bool);
    TEST_RUN("get_c_type_char", test_get_c_type_char);
    TEST_RUN("get_c_type_byte", test_get_c_type_byte);
    TEST_RUN("get_c_type_void", test_get_c_type_void);
    TEST_RUN("get_c_type_long", test_get_c_type_long);
    TEST_RUN("get_c_type_string", test_get_c_type_string);

    TEST_SECTION("Code Gen Helpers - get_default_value");
    TEST_RUN("get_default_value_int", test_get_default_value_int);
    TEST_RUN("get_default_value_double", test_get_default_value_double);
    TEST_RUN("get_default_value_bool", test_get_default_value_bool);
    TEST_RUN("get_default_value_char", test_get_default_value_char);
    TEST_RUN("get_default_value_byte", test_get_default_value_byte);
    TEST_RUN("get_default_value_string", test_get_default_value_string);
    TEST_RUN("get_default_value_long", test_get_default_value_long);

    TEST_SECTION("Code Gen Helpers - get_boxing_function");
    TEST_RUN("get_boxing_function_int", test_get_boxing_function_int);
    TEST_RUN("get_boxing_function_double", test_get_boxing_function_double);
    TEST_RUN("get_boxing_function_bool", test_get_boxing_function_bool);
    TEST_RUN("get_boxing_function_char", test_get_boxing_function_char);
    TEST_RUN("get_boxing_function_string", test_get_boxing_function_string);

    TEST_SECTION("Code Gen Helpers - get_unboxing_function");
    TEST_RUN("get_unboxing_function_int", test_get_unboxing_function_int);
    TEST_RUN("get_unboxing_function_double", test_get_unboxing_function_double);
    TEST_RUN("get_unboxing_function_bool", test_get_unboxing_function_bool);
    TEST_RUN("get_unboxing_function_char", test_get_unboxing_function_char);
    TEST_RUN("get_unboxing_function_string", test_get_unboxing_function_string);

    TEST_SECTION("Code Gen Helpers - get_rt_to_string_func");
    TEST_RUN("get_rt_to_string_func_int", test_get_rt_to_string_func_int);
    TEST_RUN("get_rt_to_string_func_double", test_get_rt_to_string_func_double);
    TEST_RUN("get_rt_to_string_func_bool", test_get_rt_to_string_func_bool);
    TEST_RUN("get_rt_to_string_func_char", test_get_rt_to_string_func_char);
    TEST_RUN("get_rt_to_string_func_byte", test_get_rt_to_string_func_byte);
    TEST_RUN("get_rt_to_string_func_long", test_get_rt_to_string_func_long);

    TEST_SECTION("Code Gen Helpers - code_gen_binary_op_str");
    TEST_RUN("binary_op_str_plus", test_binary_op_str_plus);
    TEST_RUN("binary_op_str_minus", test_binary_op_str_minus);
    TEST_RUN("binary_op_str_star", test_binary_op_str_star);
    TEST_RUN("binary_op_str_slash", test_binary_op_str_slash);
    TEST_RUN("binary_op_str_modulo", test_binary_op_str_modulo);
    TEST_RUN("binary_op_str_equal_equal", test_binary_op_str_equal_equal);
    TEST_RUN("binary_op_str_bang_equal", test_binary_op_str_bang_equal);
    TEST_RUN("binary_op_str_less", test_binary_op_str_less);
    TEST_RUN("binary_op_str_less_equal", test_binary_op_str_less_equal);
    TEST_RUN("binary_op_str_greater", test_binary_op_str_greater);
    TEST_RUN("binary_op_str_greater_equal", test_binary_op_str_greater_equal);
    TEST_RUN("binary_op_str_and", test_binary_op_str_and);
    TEST_RUN("binary_op_str_or", test_binary_op_str_or);
}
