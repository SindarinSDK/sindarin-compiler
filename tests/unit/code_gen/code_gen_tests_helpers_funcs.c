// tests/code_gen_tests_helpers_funcs.c
// Tests for code generation helper functions - operators

/* ============================================================================
 * get_rt_to_string_func_v2 Tests
 * ============================================================================ */

static void test_get_rt_to_string_func_v2_int(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_INT);
    assert(strcmp(result, "rt_to_string_long_v2") == 0);
}

static void test_get_rt_to_string_func_v2_double(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_DOUBLE);
    assert(strcmp(result, "rt_to_string_double_v2") == 0);
}

static void test_get_rt_to_string_func_v2_bool(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_BOOL);
    assert(strcmp(result, "rt_to_string_bool_v2") == 0);
}

static void test_get_rt_to_string_func_v2_char(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_CHAR);
    assert(strcmp(result, "rt_to_string_char_v2") == 0);
}

static void test_get_rt_to_string_func_v2_byte(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_BYTE);
    assert(strcmp(result, "rt_to_string_byte_v2") == 0);
}

static void test_get_rt_to_string_func_v2_long(void)
{
    const char *result = get_rt_to_string_func_v2(TYPE_LONG);
    assert(strcmp(result, "rt_to_string_long_v2") == 0);
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
    TEST_SECTION("Code Gen Helpers - get_rt_to_string_func_v2");
    TEST_RUN("get_rt_to_string_func_v2_int", test_get_rt_to_string_func_v2_int);
    TEST_RUN("get_rt_to_string_func_v2_double", test_get_rt_to_string_func_v2_double);
    TEST_RUN("get_rt_to_string_func_v2_bool", test_get_rt_to_string_func_v2_bool);
    TEST_RUN("get_rt_to_string_func_v2_char", test_get_rt_to_string_func_v2_char);
    TEST_RUN("get_rt_to_string_func_v2_byte", test_get_rt_to_string_func_v2_byte);
    TEST_RUN("get_rt_to_string_func_v2_long", test_get_rt_to_string_func_v2_long);

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
