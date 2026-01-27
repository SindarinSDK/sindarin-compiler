// tests/unit/ast/ast_tests_types.c
// AST type creation and manipulation tests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../ast.h"
#include "../test_harness.h"

#define TYPE_ARENA_SIZE 4096

/* ============================================================================
 * Primitive Type Creation Tests
 * ============================================================================ */

static void test_create_type_int(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(type != NULL);
    assert(type->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_create_type_int32(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_INT32);
    assert(type != NULL);
    assert(type->kind == TYPE_INT32);

    arena_free(&arena);
}

static void test_create_type_uint(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_UINT);
    assert(type != NULL);
    assert(type->kind == TYPE_UINT);

    arena_free(&arena);
}

static void test_create_type_uint32(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_UINT32);
    assert(type != NULL);
    assert(type->kind == TYPE_UINT32);

    arena_free(&arena);
}

static void test_create_type_long(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(type != NULL);
    assert(type->kind == TYPE_LONG);

    arena_free(&arena);
}

static void test_create_type_double(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(type != NULL);
    assert(type->kind == TYPE_DOUBLE);

    arena_free(&arena);
}

static void test_create_type_float(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    assert(type != NULL);
    assert(type->kind == TYPE_FLOAT);

    arena_free(&arena);
}

static void test_create_type_char(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(type != NULL);
    assert(type->kind == TYPE_CHAR);

    arena_free(&arena);
}

static void test_create_type_string(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(type != NULL);
    assert(type->kind == TYPE_STRING);

    arena_free(&arena);
}

static void test_create_type_bool(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(type != NULL);
    assert(type->kind == TYPE_BOOL);

    arena_free(&arena);
}

static void test_create_type_byte(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(type != NULL);
    assert(type->kind == TYPE_BYTE);

    arena_free(&arena);
}

static void test_create_type_void(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(type != NULL);
    assert(type->kind == TYPE_VOID);

    arena_free(&arena);
}

static void test_create_type_nil(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_NIL);
    assert(type != NULL);
    assert(type->kind == TYPE_NIL);

    arena_free(&arena);
}

static void test_create_type_any(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *type = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(type != NULL);
    assert(type->kind == TYPE_ANY);

    arena_free(&arena);
}

/* ============================================================================
 * Array Type Tests
 * ============================================================================ */

static void test_create_array_type_int(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    assert(arr_type != NULL);
    assert(arr_type->kind == TYPE_ARRAY);
    assert(arr_type->as.array.element_type != NULL);
    assert(arr_type->as.array.element_type->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_create_array_type_string(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    assert(arr_type != NULL);
    assert(arr_type->kind == TYPE_ARRAY);
    assert(arr_type->as.array.element_type->kind == TYPE_STRING);

    arena_free(&arena);
}

static void test_create_nested_array_type(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *nested_arr = ast_create_array_type(&arena, arr_type);

    assert(nested_arr != NULL);
    assert(nested_arr->kind == TYPE_ARRAY);
    assert(nested_arr->as.array.element_type->kind == TYPE_ARRAY);
    assert(nested_arr->as.array.element_type->as.array.element_type->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_create_array_type_bool(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    assert(arr_type->kind == TYPE_ARRAY);
    assert(arr_type->as.array.element_type->kind == TYPE_BOOL);

    arena_free(&arena);
}

static void test_create_array_type_double(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    assert(arr_type->kind == TYPE_ARRAY);
    assert(arr_type->as.array.element_type->kind == TYPE_DOUBLE);

    arena_free(&arena);
}

/* ============================================================================
 * Function Type Tests
 * ============================================================================ */

static void test_create_function_type_no_params(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, ret_type, NULL, 0);

    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.return_type->kind == TYPE_VOID);
    assert(func_type->as.function.param_count == 0);

    arena_free(&arena);
}

static void test_create_function_type_one_param(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *param_types[1];
    param_types[0] = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, ret_type, param_types, 1);

    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.return_type->kind == TYPE_INT);
    assert(func_type->as.function.param_count == 1);
    assert(func_type->as.function.param_types[0]->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_create_function_type_multiple_params(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *param_types[3];
    param_types[0] = ast_create_primitive_type(&arena, TYPE_INT);
    param_types[1] = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    param_types[2] = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *func_type = ast_create_function_type(&arena, ret_type, param_types, 3);

    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.return_type->kind == TYPE_STRING);
    assert(func_type->as.function.param_count == 3);
    assert(func_type->as.function.param_types[0]->kind == TYPE_INT);
    assert(func_type->as.function.param_types[1]->kind == TYPE_DOUBLE);
    assert(func_type->as.function.param_types[2]->kind == TYPE_BOOL);

    arena_free(&arena);
}

static void test_create_function_type_returning_array(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, arr_type, NULL, 0);

    assert(func_type != NULL);
    assert(func_type->kind == TYPE_FUNCTION);
    assert(func_type->as.function.return_type->kind == TYPE_ARRAY);

    arena_free(&arena);
}

/* ============================================================================
 * Pointer Type Tests
 * ============================================================================ */

static void test_create_pointer_type_int(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *base = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr_type = ast_create_pointer_type(&arena, base);

    assert(ptr_type != NULL);
    assert(ptr_type->kind == TYPE_POINTER);
    assert(ptr_type->as.pointer.base_type->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_create_pointer_type_void(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *base = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_type = ast_create_pointer_type(&arena, base);

    assert(ptr_type != NULL);
    assert(ptr_type->kind == TYPE_POINTER);
    assert(ptr_type->as.pointer.base_type->kind == TYPE_VOID);

    arena_free(&arena);
}

static void test_create_double_pointer_type(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr1 = ast_create_pointer_type(&arena, int_type);
    Type *ptr2 = ast_create_pointer_type(&arena, ptr1);

    assert(ptr2 != NULL);
    assert(ptr2->kind == TYPE_POINTER);
    assert(ptr2->as.pointer.base_type->kind == TYPE_POINTER);
    assert(ptr2->as.pointer.base_type->as.pointer.base_type->kind == TYPE_INT);

    arena_free(&arena);
}

/* ============================================================================
 * Opaque Type Tests
 * ============================================================================ */

static void test_create_opaque_type(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *opaque = ast_create_opaque_type(&arena, "FILE");

    assert(opaque != NULL);
    assert(opaque->kind == TYPE_OPAQUE);
    assert(strcmp(opaque->as.opaque.name, "FILE") == 0);

    arena_free(&arena);
}

static void test_create_opaque_type_different_name(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *opaque = ast_create_opaque_type(&arena, "CustomHandle");

    assert(opaque != NULL);
    assert(opaque->kind == TYPE_OPAQUE);
    assert(strcmp(opaque->as.opaque.name, "CustomHandle") == 0);

    arena_free(&arena);
}

/* ============================================================================
 * Type Comparison Tests
 * ============================================================================ */

static void test_types_equal_same_int(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *t1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t2 = ast_create_primitive_type(&arena, TYPE_INT);

    assert(types_equal(t1, t2) == true);

    arena_free(&arena);
}

static void test_types_equal_different_primitives(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *t1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t2 = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    assert(types_equal(t1, t2) == false);

    arena_free(&arena);
}

static void test_types_equal_same_array(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int2 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int1);
    Type *arr2 = ast_create_array_type(&arena, int2);

    assert(types_equal(arr1, arr2) == true);

    arena_free(&arena);
}

static void test_types_equal_different_array_elements(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *arr2 = ast_create_array_type(&arena, str_type);

    assert(types_equal(arr1, arr2) == false);

    arena_free(&arena);
}

static void test_types_equal_null_first(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);

    assert(types_equal(NULL, t) == false);

    arena_free(&arena);
}

static void test_types_equal_null_second(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);

    assert(types_equal(t, NULL) == false);

    arena_free(&arena);
}

static void test_types_equal_both_null(void)
{
    assert(types_equal(NULL, NULL) == true);
}

static void test_types_equal_functions_same(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ret2 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func1 = ast_create_function_type(&arena, ret1, NULL, 0);
    Type *func2 = ast_create_function_type(&arena, ret2, NULL, 0);

    assert(types_equal(func1, func2) == true);

    arena_free(&arena);
}

static void test_types_equal_functions_different_returns(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ret2 = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func1 = ast_create_function_type(&arena, ret1, NULL, 0);
    Type *func2 = ast_create_function_type(&arena, ret2, NULL, 0);

    assert(types_equal(func1, func2) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Type Clone Tests
 * ============================================================================ */

static void test_clone_type_int(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *original = ast_create_primitive_type(&arena, TYPE_INT);
    Type *clone = ast_clone_type(&arena, original);

    assert(clone != NULL);
    assert(clone != original);
    assert(clone->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_clone_type_array(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr = ast_create_array_type(&arena, int_type);
    Type *clone = ast_clone_type(&arena, arr);

    assert(clone != NULL);
    assert(clone != arr);
    assert(clone->kind == TYPE_ARRAY);
    assert(clone->as.array.element_type != arr->as.array.element_type);
    assert(clone->as.array.element_type->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_clone_type_function(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *ret = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *param_types[1];
    param_types[0] = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func = ast_create_function_type(&arena, ret, param_types, 1);
    Type *clone = ast_clone_type(&arena, func);

    assert(clone != NULL);
    assert(clone != func);
    assert(clone->kind == TYPE_FUNCTION);
    assert(clone->as.function.return_type->kind == TYPE_VOID);
    assert(clone->as.function.param_count == 1);

    arena_free(&arena);
}

static void test_clone_type_null(void)
{
    Arena arena;
    arena_init(&arena, TYPE_ARENA_SIZE);

    Type *clone = ast_clone_type(&arena, NULL);
    assert(clone == NULL);

    arena_free(&arena);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_ast_types_main(void)
{
    TEST_SECTION("AST Types - Primitive Type Creation");
    TEST_RUN("create_type_int", test_create_type_int);
    TEST_RUN("create_type_int32", test_create_type_int32);
    TEST_RUN("create_type_uint", test_create_type_uint);
    TEST_RUN("create_type_uint32", test_create_type_uint32);
    TEST_RUN("create_type_long", test_create_type_long);
    TEST_RUN("create_type_double", test_create_type_double);
    TEST_RUN("create_type_float", test_create_type_float);
    TEST_RUN("create_type_char", test_create_type_char);
    TEST_RUN("create_type_string", test_create_type_string);
    TEST_RUN("create_type_bool", test_create_type_bool);
    TEST_RUN("create_type_byte", test_create_type_byte);
    TEST_RUN("create_type_void", test_create_type_void);
    TEST_RUN("create_type_nil", test_create_type_nil);
    TEST_RUN("create_type_any", test_create_type_any);

    TEST_SECTION("AST Types - Array Types");
    TEST_RUN("create_array_type_int", test_create_array_type_int);
    TEST_RUN("create_array_type_string", test_create_array_type_string);
    TEST_RUN("create_nested_array_type", test_create_nested_array_type);
    TEST_RUN("create_array_type_bool", test_create_array_type_bool);
    TEST_RUN("create_array_type_double", test_create_array_type_double);

    TEST_SECTION("AST Types - Function Types");
    TEST_RUN("create_function_type_no_params", test_create_function_type_no_params);
    TEST_RUN("create_function_type_one_param", test_create_function_type_one_param);
    TEST_RUN("create_function_type_multiple_params", test_create_function_type_multiple_params);
    TEST_RUN("create_function_type_returning_array", test_create_function_type_returning_array);

    TEST_SECTION("AST Types - Pointer Types");
    TEST_RUN("create_pointer_type_int", test_create_pointer_type_int);
    TEST_RUN("create_pointer_type_void", test_create_pointer_type_void);
    TEST_RUN("create_double_pointer_type", test_create_double_pointer_type);

    TEST_SECTION("AST Types - Opaque Types");
    TEST_RUN("create_opaque_type", test_create_opaque_type);
    TEST_RUN("create_opaque_type_different_name", test_create_opaque_type_different_name);

    TEST_SECTION("AST Types - Type Comparison");
    TEST_RUN("types_equal_same_int", test_types_equal_same_int);
    TEST_RUN("types_equal_different_primitives", test_types_equal_different_primitives);
    TEST_RUN("types_equal_same_array", test_types_equal_same_array);
    TEST_RUN("types_equal_different_array_elements", test_types_equal_different_array_elements);
    TEST_RUN("types_equal_null_first", test_types_equal_null_first);
    TEST_RUN("types_equal_null_second", test_types_equal_null_second);
    TEST_RUN("types_equal_both_null", test_types_equal_both_null);
    TEST_RUN("types_equal_functions_same", test_types_equal_functions_same);
    TEST_RUN("types_equal_functions_different_returns", test_types_equal_functions_different_returns);

    TEST_SECTION("AST Types - Type Cloning");
    TEST_RUN("clone_type_int", test_clone_type_int);
    TEST_RUN("clone_type_array", test_clone_type_array);
    TEST_RUN("clone_type_function", test_clone_type_function);
    TEST_RUN("clone_type_null", test_clone_type_null);
}
