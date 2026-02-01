// tests/code_gen_tests_helpers_types.c
// Tests for code generation helper functions - type checking and escaping

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
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_helpers_types_main(void)
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
}
