// tests/unit/type_checker/type_checker_tests_edge_coercion.c
// Numeric Promotion Edge Cases

static void test_promotion_int_to_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    assert(can_promote_numeric(int_type, double_type) == true);

    arena_free(&arena);
}

static void test_promotion_double_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    assert(can_promote_numeric(double_type, int_type) == false);

    arena_free(&arena);
}

static void test_promotion_byte_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(can_promote_numeric(byte_type, int_type) == false);

    arena_free(&arena);
}

static void test_promotion_char_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(can_promote_numeric(char_type, int_type) == false);

    arena_free(&arena);
}

static void test_promotion_same_type_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    assert(can_promote_numeric(int_type, int_type) == false);
    assert(can_promote_numeric(double_type, double_type) == false);
    assert(can_promote_numeric(bool_type, bool_type) == false);

    arena_free(&arena);
}

static void test_promotion_string_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(can_promote_numeric(string_type, int_type) == false);

    arena_free(&arena);
}

static void test_promotion_bool_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(can_promote_numeric(bool_type, int_type) == false);

    arena_free(&arena);
}
