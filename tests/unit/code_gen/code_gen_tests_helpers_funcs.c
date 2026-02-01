// tests/code_gen_tests_helpers_funcs.c
// Tests for code generation helper functions - boxing/unboxing and operators

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
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_helpers_funcs_main(void)
{
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
