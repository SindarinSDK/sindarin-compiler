// tests/unit/type_checker/type_checker_tests_edge_size.c
// Type Size and Alignment

static void test_type_size_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(get_type_size(int_type) == 8);

    arena_free(&arena);
}

static void test_type_size_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(get_type_size(bool_type) == 1);

    arena_free(&arena);
}

static void test_type_size_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(get_type_size(char_type) == 1);

    arena_free(&arena);
}

static void test_type_size_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(get_type_size(double_type) == 8);

    arena_free(&arena);
}

static void test_type_size_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(get_type_size(string_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(get_type_size(arr_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_pointer(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr_type = ast_create_pointer_type(&arena, int_type);
    assert(get_type_size(ptr_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(get_type_size(void_type) == 0);

    arena_free(&arena);
}

static void test_type_size_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(get_type_size(byte_type) == 1);

    arena_free(&arena);
}

static void test_type_size_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(get_type_size(long_type) == 8);

    arena_free(&arena);
}
