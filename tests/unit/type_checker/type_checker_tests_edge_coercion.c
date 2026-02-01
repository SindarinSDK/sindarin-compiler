// tests/unit/type_checker/type_checker_tests_edge_coercion.c
// Type Coercion Edge Cases

static void test_coercion_int_to_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // int is coercible to double
    assert(is_type_coercible_to(int_type, double_type) == 1);

    arena_free(&arena);
}

static void test_coercion_double_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // double is NOT coercible to int
    assert(is_type_coercible_to(double_type, int_type) == 0);

    arena_free(&arena);
}

static void test_coercion_byte_to_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // byte is coercible to int
    assert(is_type_coercible_to(byte_type, int_type) == 1);

    arena_free(&arena);
}

static void test_coercion_char_to_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // char is coercible to int
    assert(is_type_coercible_to(char_type, int_type) == 1);

    arena_free(&arena);
}

static void test_coercion_same_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    // Same types are coercible
    assert(is_type_coercible_to(int_type, int_type) == 1);
    assert(is_type_coercible_to(double_type, double_type) == 1);
    assert(is_type_coercible_to(bool_type, bool_type) == 1);

    arena_free(&arena);
}

static void test_coercion_string_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // string is NOT coercible to int
    assert(is_type_coercible_to(string_type, int_type) == 0);

    arena_free(&arena);
}

static void test_coercion_bool_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // bool is NOT coercible to int (in strict typing)
    assert(is_type_coercible_to(bool_type, int_type) == 0);

    arena_free(&arena);
}
