// tests/unit/type_checker/type_checker_tests_utils.c
// Type checker utility function tests

/* ============================================================================
 * is_numeric_type Tests
 * ============================================================================ */

static void test_is_numeric_type_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_numeric_type(int_type) == true);

    arena_free(&arena);
}

static void test_is_numeric_type_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(is_numeric_type(long_type) == true);

    arena_free(&arena);
}

static void test_is_numeric_type_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_numeric_type(double_type) == true);

    arena_free(&arena);
}

static void test_is_numeric_type_float(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    assert(is_numeric_type(float_type) == true);

    arena_free(&arena);
}

static void test_is_numeric_type_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(is_numeric_type(byte_type) == true);

    arena_free(&arena);
}

static void test_is_numeric_type_string_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_numeric_type(str_type) == false);

    arena_free(&arena);
}

static void test_is_numeric_type_bool_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_numeric_type(bool_type) == false);

    arena_free(&arena);
}

static void test_is_numeric_type_void_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(is_numeric_type(void_type) == false);

    arena_free(&arena);
}

static void test_is_numeric_type_char_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(is_numeric_type(char_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_comparison_operator Tests
 * ============================================================================ */

static void test_is_comparison_less(void)
{
    assert(is_comparison_operator(TOKEN_LESS) == true);
}

static void test_is_comparison_greater(void)
{
    assert(is_comparison_operator(TOKEN_GREATER) == true);
}

static void test_is_comparison_less_equal(void)
{
    assert(is_comparison_operator(TOKEN_LESS_EQUAL) == true);
}

static void test_is_comparison_greater_equal(void)
{
    assert(is_comparison_operator(TOKEN_GREATER_EQUAL) == true);
}

static void test_is_comparison_equal(void)
{
    assert(is_comparison_operator(TOKEN_EQUAL_EQUAL) == true);
}

static void test_is_comparison_not_equal(void)
{
    assert(is_comparison_operator(TOKEN_BANG_EQUAL) == true);
}

static void test_is_comparison_plus_false(void)
{
    assert(is_comparison_operator(TOKEN_PLUS) == false);
}

static void test_is_comparison_minus_false(void)
{
    assert(is_comparison_operator(TOKEN_MINUS) == false);
}

static void test_is_comparison_and_false(void)
{
    assert(is_comparison_operator(TOKEN_AND) == false);
}

/* ============================================================================
 * is_arithmetic_operator Tests
 * ============================================================================ */

static void test_is_arithmetic_plus(void)
{
    assert(is_arithmetic_operator(TOKEN_PLUS) == true);
}

static void test_is_arithmetic_minus(void)
{
    assert(is_arithmetic_operator(TOKEN_MINUS) == true);
}

static void test_is_arithmetic_star(void)
{
    assert(is_arithmetic_operator(TOKEN_STAR) == true);
}

static void test_is_arithmetic_slash(void)
{
    assert(is_arithmetic_operator(TOKEN_SLASH) == true);
}

static void test_is_arithmetic_modulo(void)
{
    assert(is_arithmetic_operator(TOKEN_MODULO) == true);
}

static void test_is_arithmetic_less_false(void)
{
    assert(is_arithmetic_operator(TOKEN_LESS) == false);
}

static void test_is_arithmetic_and_false(void)
{
    assert(is_arithmetic_operator(TOKEN_AND) == false);
}

/* ============================================================================
 * can_promote_numeric Tests
 * ============================================================================ */

static void test_promote_int_to_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(can_promote_numeric(int_type, double_type) == true);

    arena_free(&arena);
}

static void test_promote_int_to_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(can_promote_numeric(int_type, long_type) == true);

    arena_free(&arena);
}

static void test_promote_byte_to_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(can_promote_numeric(byte_type, int_type) == true);

    arena_free(&arena);
}

static void test_promote_float_to_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(can_promote_numeric(float_type, double_type) == true);

    arena_free(&arena);
}

static void test_promote_same_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(can_promote_numeric(int_type, int_type) == true);

    arena_free(&arena);
}

/* ============================================================================
 * get_promoted_type Tests
 * ============================================================================ */

static void test_get_promoted_int_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *result = get_promoted_type(&arena, int_type, double_type);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);

    arena_free(&arena);
}

static void test_get_promoted_double_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *result = get_promoted_type(&arena, double_type, int_type);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);

    arena_free(&arena);
}

static void test_get_promoted_int_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *result = get_promoted_type(&arena, int_type, int_type);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);

    arena_free(&arena);
}

static void test_get_promoted_long_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    Type *result = get_promoted_type(&arena, long_type, int_type);
    assert(result != NULL);
    assert(result->kind == TYPE_LONG);

    arena_free(&arena);
}

/* ============================================================================
 * is_primitive_type Tests
 * ============================================================================ */

static void test_is_primitive_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_primitive_type(int_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_primitive_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_primitive_type(bool_type) == true);

    arena_free(&arena);
}

static void test_is_primitive_array_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(is_primitive_type(arr_type) == false);

    arena_free(&arena);
}

static void test_is_primitive_pointer_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr_type = ast_create_pointer_type(&arena, int_type);
    assert(is_primitive_type(ptr_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_reference_type Tests
 * ============================================================================ */

static void test_is_reference_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(is_reference_type(arr_type) == true);

    arena_free(&arena);
}

static void test_is_reference_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_reference_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_reference_int_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_reference_type(int_type) == false);

    arena_free(&arena);
}

static void test_is_reference_bool_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_reference_type(bool_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * is_printable_type Tests
 * ============================================================================ */

static void test_is_printable_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(is_printable_type(int_type) == true);

    arena_free(&arena);
}

static void test_is_printable_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(is_printable_type(str_type) == true);

    arena_free(&arena);
}

static void test_is_printable_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(is_printable_type(double_type) == true);

    arena_free(&arena);
}

static void test_is_printable_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(is_printable_type(bool_type) == true);

    arena_free(&arena);
}

static void test_is_printable_void_false(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(is_printable_type(void_type) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_type_checker_utils_main(void)
{
    TEST_SECTION("Type Checker - is_numeric_type");
    TEST_RUN("is_numeric_type_int", test_is_numeric_type_int);
    TEST_RUN("is_numeric_type_long", test_is_numeric_type_long);
    TEST_RUN("is_numeric_type_double", test_is_numeric_type_double);
    TEST_RUN("is_numeric_type_float", test_is_numeric_type_float);
    TEST_RUN("is_numeric_type_byte", test_is_numeric_type_byte);
    TEST_RUN("is_numeric_type_string_false", test_is_numeric_type_string_false);
    TEST_RUN("is_numeric_type_bool_false", test_is_numeric_type_bool_false);
    TEST_RUN("is_numeric_type_void_false", test_is_numeric_type_void_false);
    TEST_RUN("is_numeric_type_char_false", test_is_numeric_type_char_false);

    TEST_SECTION("Type Checker - is_comparison_operator");
    TEST_RUN("is_comparison_less", test_is_comparison_less);
    TEST_RUN("is_comparison_greater", test_is_comparison_greater);
    TEST_RUN("is_comparison_less_equal", test_is_comparison_less_equal);
    TEST_RUN("is_comparison_greater_equal", test_is_comparison_greater_equal);
    TEST_RUN("is_comparison_equal", test_is_comparison_equal);
    TEST_RUN("is_comparison_not_equal", test_is_comparison_not_equal);
    TEST_RUN("is_comparison_plus_false", test_is_comparison_plus_false);
    TEST_RUN("is_comparison_minus_false", test_is_comparison_minus_false);
    TEST_RUN("is_comparison_and_false", test_is_comparison_and_false);

    TEST_SECTION("Type Checker - is_arithmetic_operator");
    TEST_RUN("is_arithmetic_plus", test_is_arithmetic_plus);
    TEST_RUN("is_arithmetic_minus", test_is_arithmetic_minus);
    TEST_RUN("is_arithmetic_star", test_is_arithmetic_star);
    TEST_RUN("is_arithmetic_slash", test_is_arithmetic_slash);
    TEST_RUN("is_arithmetic_modulo", test_is_arithmetic_modulo);
    TEST_RUN("is_arithmetic_less_false", test_is_arithmetic_less_false);
    TEST_RUN("is_arithmetic_and_false", test_is_arithmetic_and_false);

    TEST_SECTION("Type Checker - can_promote_numeric");
    TEST_RUN("promote_int_to_double", test_promote_int_to_double);
    TEST_RUN("promote_int_to_long", test_promote_int_to_long);
    TEST_RUN("promote_byte_to_int", test_promote_byte_to_int);
    TEST_RUN("promote_float_to_double", test_promote_float_to_double);
    TEST_RUN("promote_same_type", test_promote_same_type);

    TEST_SECTION("Type Checker - get_promoted_type");
    TEST_RUN("get_promoted_int_double", test_get_promoted_int_double);
    TEST_RUN("get_promoted_double_int", test_get_promoted_double_int);
    TEST_RUN("get_promoted_int_int", test_get_promoted_int_int);
    TEST_RUN("get_promoted_long_int", test_get_promoted_long_int);

    TEST_SECTION("Type Checker - is_primitive_type");
    TEST_RUN("is_primitive_int", test_is_primitive_int);
    TEST_RUN("is_primitive_string", test_is_primitive_string);
    TEST_RUN("is_primitive_bool", test_is_primitive_bool);
    TEST_RUN("is_primitive_array_false", test_is_primitive_array_false);
    TEST_RUN("is_primitive_pointer_false", test_is_primitive_pointer_false);

    TEST_SECTION("Type Checker - is_reference_type");
    TEST_RUN("is_reference_array", test_is_reference_array);
    TEST_RUN("is_reference_string", test_is_reference_string);
    TEST_RUN("is_reference_int_false", test_is_reference_int_false);
    TEST_RUN("is_reference_bool_false", test_is_reference_bool_false);

    TEST_SECTION("Type Checker - is_printable_type");
    TEST_RUN("is_printable_int", test_is_printable_int);
    TEST_RUN("is_printable_string", test_is_printable_string);
    TEST_RUN("is_printable_double", test_is_printable_double);
    TEST_RUN("is_printable_bool", test_is_printable_bool);
    TEST_RUN("is_printable_void_false", test_is_printable_void_false);
}
