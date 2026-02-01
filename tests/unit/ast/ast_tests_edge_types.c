// ast_tests_edge_types.c
// Primitive and composite type creation tests

/* ============================================================================
 * Primitive Type Creation Tests
 * ============================================================================ */

static void test_ast_primitive_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    assert(t != NULL);
    assert(t->kind == TYPE_INT);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_long(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(t != NULL);
    assert(t->kind == TYPE_LONG);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_double(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(t != NULL);
    assert(t->kind == TYPE_DOUBLE);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_bool(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(t != NULL);
    assert(t->kind == TYPE_BOOL);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_char(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(t != NULL);
    assert(t->kind == TYPE_CHAR);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_byte(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(t != NULL);
    assert(t->kind == TYPE_BYTE);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_string(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(t != NULL);
    assert(t->kind == TYPE_STRING);

    cleanup_arena(&arena);
}

static void test_ast_primitive_type_void(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(t != NULL);
    assert(t->kind == TYPE_VOID);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Composite Type Creation Tests
 * ============================================================================ */

static void test_ast_array_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type == elem);

    cleanup_arena(&arena);
}

static void test_ast_array_type_string(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type->kind == TYPE_STRING);

    cleanup_arena(&arena);
}

static void test_ast_pointer_type_int(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *pointee = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr = ast_create_pointer_type(&arena, pointee);
    assert(ptr != NULL);
    assert(ptr->kind == TYPE_POINTER);
    assert(ptr->as.pointer.pointee == pointee);

    cleanup_arena(&arena);
}

static void test_ast_pointer_type_char(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *pointee = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *ptr = ast_create_pointer_type(&arena, pointee);
    assert(ptr != NULL);
    assert(ptr->kind == TYPE_POINTER);
    assert(ptr->as.pointer.pointee->kind == TYPE_CHAR);

    cleanup_arena(&arena);
}

static void test_ast_opaque_type(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *opaque = ast_create_opaque_type(&arena, "OpaqueHandle");
    assert(opaque != NULL);
    assert(opaque->kind == TYPE_OPAQUE);

    cleanup_arena(&arena);
}

static void test_ast_nested_array_type(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, elem);
    Type *arr2 = ast_create_array_type(&arena, arr1);
    assert(arr2 != NULL);
    assert(arr2->kind == TYPE_ARRAY);
    assert(arr2->as.array.element_type->kind == TYPE_ARRAY);

    cleanup_arena(&arena);
}

static void test_ast_pointer_to_pointer(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr1 = ast_create_pointer_type(&arena, int_type);
    Type *ptr2 = ast_create_pointer_type(&arena, ptr1);
    assert(ptr2 != NULL);
    assert(ptr2->kind == TYPE_POINTER);
    assert(ptr2->as.pointer.pointee->kind == TYPE_POINTER);

    cleanup_arena(&arena);
}
