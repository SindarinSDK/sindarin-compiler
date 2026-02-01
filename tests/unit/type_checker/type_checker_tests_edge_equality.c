// tests/unit/type_checker/type_checker_tests_edge_equality.c
// Type Equality Edge Cases

static void test_type_equality_same_primitives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int2 = ast_create_primitive_type(&arena, TYPE_INT);

    assert(ast_type_equals(int1, int2) == 1);

    arena_free(&arena);
}

static void test_type_equality_different_primitives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    assert(ast_type_equals(int_type, bool_type) == 0);
    assert(ast_type_equals(int_type, char_type) == 0);
    assert(ast_type_equals(int_type, double_type) == 0);
    assert(ast_type_equals(int_type, string_type) == 0);
    assert(ast_type_equals(bool_type, char_type) == 0);

    arena_free(&arena);
}

static void test_type_equality_arrays_same_element(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *arr2 = ast_create_array_type(&arena, int_type);

    assert(ast_type_equals(arr1, arr2) == 1);

    arena_free(&arena);
}

static void test_type_equality_arrays_different_element(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *arr_int = ast_create_array_type(&arena, int_type);
    Type *arr_bool = ast_create_array_type(&arena, bool_type);

    assert(ast_type_equals(arr_int, arr_bool) == 0);

    arena_free(&arena);
}

static void test_type_equality_nested_arrays(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *nested_arr1 = ast_create_array_type(&arena, arr1);

    Type *arr2 = ast_create_array_type(&arena, int_type);
    Type *nested_arr2 = ast_create_array_type(&arena, arr2);

    assert(ast_type_equals(nested_arr1, nested_arr2) == 1);

    arena_free(&arena);
}

static void test_type_equality_function_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *params1[] = {int_type, int_type};
    Type *params2[] = {int_type, int_type};

    Type *fn1 = ast_create_function_type(&arena, int_type, params1, 2);
    Type *fn2 = ast_create_function_type(&arena, int_type, params2, 2);

    assert(ast_type_equals(fn1, fn2) == 1);

    // Different return type
    Type *fn3 = ast_create_function_type(&arena, void_type, params1, 2);
    assert(ast_type_equals(fn1, fn3) == 0);

    arena_free(&arena);
}

static void test_type_equality_function_different_params(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *params1[] = {int_type};
    Type *params2[] = {bool_type};
    Type *params3[] = {int_type, int_type};

    Type *fn1 = ast_create_function_type(&arena, int_type, params1, 1);
    Type *fn2 = ast_create_function_type(&arena, int_type, params2, 1);
    Type *fn3 = ast_create_function_type(&arena, int_type, params3, 2);

    // Different param type
    assert(ast_type_equals(fn1, fn2) == 0);
    // Different param count
    assert(ast_type_equals(fn1, fn3) == 0);

    arena_free(&arena);
}

static void test_type_equality_with_null(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(ast_type_equals(NULL, NULL) == 0);
    assert(ast_type_equals(int_type, NULL) == 0);
    assert(ast_type_equals(NULL, int_type) == 0);

    arena_free(&arena);
}
