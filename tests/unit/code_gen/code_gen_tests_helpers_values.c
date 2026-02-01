// tests/code_gen_tests_helpers_values.c
// Tests for code generation helper functions - type mapping and default values

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
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_helpers_values_main(void)
{
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
}
