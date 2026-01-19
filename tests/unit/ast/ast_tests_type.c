// tests/ast_tests_type.c
// Type-related AST tests

static void test_ast_create_primitive_type()
{
    Arena arena;
    setup_arena(&arena);

    // Test all primitive kinds
    Type *t_int = ast_create_primitive_type(&arena, TYPE_INT);
    assert(t_int != NULL);
    assert(t_int->kind == TYPE_INT);

    Type *t_long = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(t_long != NULL);
    assert(t_long->kind == TYPE_LONG);

    Type *t_double = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(t_double != NULL);
    assert(t_double->kind == TYPE_DOUBLE);

    Type *t_char = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(t_char != NULL);
    assert(t_char->kind == TYPE_CHAR);

    Type *t_string = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(t_string != NULL);
    assert(t_string->kind == TYPE_STRING);

    Type *t_bool = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(t_bool != NULL);
    assert(t_bool->kind == TYPE_BOOL);

    Type *t_byte = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(t_byte != NULL);
    assert(t_byte->kind == TYPE_BYTE);

    Type *t_void = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(t_void != NULL);
    assert(t_void->kind == TYPE_VOID);

    Type *t_nil = ast_create_primitive_type(&arena, TYPE_NIL);
    assert(t_nil != NULL);
    assert(t_nil->kind == TYPE_NIL);

    Type *t_any = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(t_any != NULL);
    assert(t_any->kind == TYPE_ANY);

    cleanup_arena(&arena);
}

static void test_ast_create_array_type()
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type == elem);

    // Nested array
    Type *nested_arr = ast_create_array_type(&arena, arr);
    assert(nested_arr != NULL);
    assert(nested_arr->kind == TYPE_ARRAY);
    assert(nested_arr->as.array.element_type == arr);
    assert(nested_arr->as.array.element_type->as.array.element_type == elem);

    // Edge case: NULL element
    Type *arr_null = ast_create_array_type(&arena, NULL);
    assert(arr_null != NULL);
    assert(arr_null->kind == TYPE_ARRAY);
    assert(arr_null->as.array.element_type == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_function_type()
{
    Arena arena;
    setup_arena(&arena);
    Type *ret = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *params[2];
    params[0] = ast_create_primitive_type(&arena, TYPE_INT);
    params[1] = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *fn = ast_create_function_type(&arena, ret, params, 2);
    assert(fn != NULL);
    assert(fn->kind == TYPE_FUNCTION);
    assert(ast_type_equals(fn->as.function.return_type, ret));
    assert(fn->as.function.param_count == 2);
    assert(ast_type_equals(fn->as.function.param_types[0], params[0]));
    assert(ast_type_equals(fn->as.function.param_types[1], params[1]));
    // Complex params: array type
    Type *arr_param = ast_create_array_type(&arena, params[0]);
    Type *complex_params[1] = {arr_param};
    Type *complex_fn = ast_create_function_type(&arena, ret, complex_params, 1);
    assert(complex_fn != NULL);
    assert(complex_fn->as.function.param_count == 1);
    assert(ast_type_equals(complex_fn->as.function.param_types[0], arr_param));
    // Empty params
    Type *fn_empty = ast_create_function_type(&arena, ret, NULL, 0);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.param_types == NULL);
    // NULL return
    Type *fn_null_ret = ast_create_function_type(&arena, NULL, params, 2);
    assert(fn_null_ret != NULL);
    assert(fn_null_ret->as.function.return_type == NULL);
    // NULL params with count > 0 (should handle gracefully by returning NULL)
    Type *fn_null_params = ast_create_function_type(&arena, ret, NULL, 2);
    assert(fn_null_params == NULL);
    cleanup_arena(&arena);
}

static void test_ast_clone_type()
{
    Arena arena;
    setup_arena(&arena);

    // Primitive
    Type *orig_prim = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *clone_prim = ast_clone_type(&arena, orig_prim);
    assert(clone_prim != NULL);
    assert(clone_prim != orig_prim);
    assert(clone_prim->kind == TYPE_BOOL);

    // Array
    Type *elem = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *orig_arr = ast_create_array_type(&arena, elem);
    Type *clone_arr = ast_clone_type(&arena, orig_arr);
    assert(clone_arr != NULL);
    assert(clone_arr != orig_arr);
    assert(clone_arr->kind == TYPE_ARRAY);
    assert(clone_arr->as.array.element_type != elem);
    assert(clone_arr->as.array.element_type->kind == TYPE_CHAR);

    // Nested array
    Type *nested_orig = ast_create_array_type(&arena, orig_arr);
    Type *nested_clone = ast_clone_type(&arena, nested_orig);
    assert(nested_clone != NULL);
    assert(nested_clone->as.array.element_type->kind == TYPE_ARRAY);
    assert(nested_clone->as.array.element_type->as.array.element_type->kind == TYPE_CHAR);

    // Function
    Type *ret = ast_create_primitive_type(&arena, TYPE_INT);
    Type *params[1] = {ast_create_primitive_type(&arena, TYPE_DOUBLE)};
    Type *orig_fn = ast_create_function_type(&arena, ret, params, 1);
    Type *clone_fn = ast_clone_type(&arena, orig_fn);
    assert(clone_fn != NULL);
    assert(clone_fn != orig_fn);
    assert(clone_fn->kind == TYPE_FUNCTION);
    assert(clone_fn->as.function.return_type->kind == TYPE_INT);
    assert(clone_fn->as.function.param_count == 1);
    assert(clone_fn->as.function.param_types[0]->kind == TYPE_DOUBLE);

    // Function with complex param
    Type *complex_params[1] = {orig_arr};
    Type *complex_orig_fn = ast_create_function_type(&arena, ret, complex_params, 1);
    Type *complex_clone_fn = ast_clone_type(&arena, complex_orig_fn);
    assert(complex_clone_fn != NULL);
    assert(complex_clone_fn->as.function.param_types[0]->kind == TYPE_ARRAY);

    // NULL
    assert(ast_clone_type(&arena, NULL) == NULL);

    cleanup_arena(&arena);
}

static void test_ast_type_equals()
{
    Arena arena;
    setup_arena(&arena);

    Type *t1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t2 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t3 = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(ast_type_equals(t1, t2) == 1);
    assert(ast_type_equals(t1, t3) == 0);

    // All primitives
    Type *t_long = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(ast_type_equals(t1, t_long) == 0);

    // Arrays
    Type *arr1 = ast_create_array_type(&arena, t1);
    Type *arr2 = ast_create_array_type(&arena, t2);
    Type *arr3 = ast_create_array_type(&arena, t3);
    assert(ast_type_equals(arr1, arr2) == 1);
    assert(ast_type_equals(arr1, arr3) == 0);

    // Nested arrays
    Type *nested1 = ast_create_array_type(&arena, arr1);
    Type *nested2 = ast_create_array_type(&arena, arr2);
    Type *nested3 = ast_create_array_type(&arena, arr1); // Same as nested1
    assert(ast_type_equals(nested1, nested2) == 1);
    assert(ast_type_equals(nested1, arr1) == 0); // Different depth
    assert(ast_type_equals(nested1, nested3) == 1);

    // Functions
    Type *params1[2] = {t1, t3};
    Type *fn1 = ast_create_function_type(&arena, t1, params1, 2);
    Type *params2[2] = {t2, t3};
    Type *fn2 = ast_create_function_type(&arena, t2, params2, 2);
    Type *params3[1] = {t1};
    Type *fn3 = ast_create_function_type(&arena, t1, params3, 1);
    assert(ast_type_equals(fn1, fn2) == 1);
    assert(ast_type_equals(fn1, fn3) == 0);

    // Function with different return
    Type *fn_diff_ret = ast_create_function_type(&arena, t3, params1, 2);
    assert(ast_type_equals(fn1, fn_diff_ret) == 0);

    // Function with different param count
    Type *fn_diff_count = ast_create_function_type(&arena, t1, params1, 1);
    assert(ast_type_equals(fn1, fn_diff_count) == 0);

    // Function with different param types
    Type *params_diff[2] = {t1, t1};
    Type *fn_diff_params = ast_create_function_type(&arena, t1, params_diff, 2);
    assert(ast_type_equals(fn1, fn_diff_params) == 0);

    // Empty functions
    Type *empty1 = ast_create_function_type(&arena, t1, NULL, 0);
    Type *empty2 = ast_create_function_type(&arena, t1, NULL, 0);
    assert(ast_type_equals(empty1, empty2) == 1);

    // NULL cases
    assert(ast_type_equals(NULL, NULL) == 1);
    assert(ast_type_equals(t1, NULL) == 0);
    assert(ast_type_equals(NULL, t1) == 0);

    cleanup_arena(&arena);
}

static void test_ast_type_to_string()
{
    Arena arena;
    setup_arena(&arena);

    // Primitives
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_INT)), "int") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_LONG)), "long") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_DOUBLE)), "double") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_CHAR)), "char") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_STRING)), "string") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_BOOL)), "bool") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_BYTE)), "byte") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_VOID)), "void") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_NIL)), "nil") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_ANY)), "any") == 0);

    // Array
    Type *arr = ast_create_array_type(&arena, ast_create_primitive_type(&arena, TYPE_CHAR));
    const char *actual = ast_type_to_string(&arena, arr);
    assert(strcmp(actual, "array of char") == 0);
    assert(strcmp(ast_type_to_string(&arena, arr), "array of char") == 0);

    // Nested array
    Type *nested_arr = ast_create_array_type(&arena, arr);
    assert(strcmp(ast_type_to_string(&arena, nested_arr), "array of array of char") == 0);

    // Function
    Type *params[1] = {ast_create_primitive_type(&arena, TYPE_BOOL)};
    Type *fn = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_STRING), params, 1);
    assert(strcmp(ast_type_to_string(&arena, fn), "function(bool) -> string") == 0);

    // Function with multiple params
    Type *params_multi[2] = {ast_create_primitive_type(&arena, TYPE_INT), ast_create_primitive_type(&arena, TYPE_DOUBLE)};
    Type *fn_multi = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_VOID), params_multi, 2);
    assert(strcmp(ast_type_to_string(&arena, fn_multi), "function(int, double) -> void") == 0);

    // Function with array param
    Type *params_arr[1] = {arr};
    Type *fn_arr = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_INT), params_arr, 1);
    assert(strcmp(ast_type_to_string(&arena, fn_arr), "function(array of char) -> int") == 0);

    // Empty function
    Type *fn_empty = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_VOID), NULL, 0);
    assert(strcmp(ast_type_to_string(&arena, fn_empty), "function() -> void") == 0);

    // Unknown kind
    Type *unknown = arena_alloc(&arena, sizeof(Type));
    unknown->kind = -1; // Invalid
    assert(strcmp(ast_type_to_string(&arena, unknown), "unknown") == 0);

    // NULL
    assert(ast_type_to_string(&arena, NULL) == NULL);

    cleanup_arena(&arena);
}

void test_ast_type_main()
{
    TEST_SECTION("AST Type Tests");
    TEST_RUN("ast_create_primitive_type", test_ast_create_primitive_type);
    TEST_RUN("ast_create_array_type", test_ast_create_array_type);
    TEST_RUN("ast_create_function_type", test_ast_create_function_type);
    TEST_RUN("ast_clone_type", test_ast_clone_type);
    TEST_RUN("ast_type_equals", test_ast_type_equals);
    TEST_RUN("ast_type_to_string", test_ast_type_to_string);
}
