// tests/type_checker_tests_struct_utility.c
// Struct type utility function tests (equality, lookup, size, alignment)

/* ============================================================================
 * Struct Type Equality Tests
 * ============================================================================ */

static void test_struct_type_equals_matching_names()
{
    DEBUG_INFO("Starting test_struct_type_equals_matching_names");

    Arena arena;
    arena_init(&arena, 4096);

    /* Create two struct types with the same name */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Type *struct_a = ast_create_struct_type(&arena, "Point", fields, 1, NULL, 0, false, false, false, NULL);
    Type *struct_b = ast_create_struct_type(&arena, "Point", fields, 1, NULL, 0, false, false, false, NULL);

    /* Should be equal - same name */
    assert(ast_type_equals(struct_a, struct_b) == 1);
    assert(ast_type_equals(struct_b, struct_a) == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_equals_matching_names");
}

/* Test: struct type inequality with different names */
static void test_struct_type_equals_different_names()
{
    DEBUG_INFO("Starting test_struct_type_equals_different_names");

    Arena arena;
    arena_init(&arena, 4096);

    /* Create two struct types with different names */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Type *struct_a = ast_create_struct_type(&arena, "Point", fields, 1, NULL, 0, false, false, false, NULL);
    Type *struct_b = ast_create_struct_type(&arena, "Vector", fields, 1, NULL, 0, false, false, false, NULL);

    /* Should NOT be equal - different names */
    assert(ast_type_equals(struct_a, struct_b) == 0);
    assert(ast_type_equals(struct_b, struct_a) == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_equals_different_names");
}

/* Test: struct type equality with NULL names (anonymous structs) */
static void test_struct_type_equals_null_names()
{
    DEBUG_INFO("Starting test_struct_type_equals_null_names");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    /* Create two structs with NULL names */
    Type *struct_a = ast_create_struct_type(&arena, NULL, fields, 1, NULL, 0, false, false, false, NULL);
    Type *struct_b = ast_create_struct_type(&arena, NULL, fields, 1, NULL, 0, false, false, false, NULL);

    /* Both NULL names should be equal */
    assert(ast_type_equals(struct_a, struct_b) == 1);

    /* One NULL, one named should NOT be equal */
    Type *struct_c = ast_create_struct_type(&arena, "Named", fields, 1, NULL, 0, false, false, false, NULL);
    assert(ast_type_equals(struct_a, struct_c) == 0);
    assert(ast_type_equals(struct_c, struct_a) == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_equals_null_names");
}

/* Test: struct type equality with NULL type pointers */
static void test_struct_type_equals_null_types()
{
    DEBUG_INFO("Starting test_struct_type_equals_null_types");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);
    Type *struct_a = ast_create_struct_type(&arena, "Test", fields, 1, NULL, 0, false, false, false, NULL);

    /* NULL vs non-NULL should return 0 */
    assert(ast_type_equals(NULL, struct_a) == 0);
    assert(ast_type_equals(struct_a, NULL) == 0);

    /* NULL vs NULL should return 1 */
    assert(ast_type_equals(NULL, NULL) == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_equals_null_types");
}

/* Test: struct type self-equality */
static void test_struct_type_equals_self()
{
    DEBUG_INFO("Starting test_struct_type_equals_self");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);
    Type *struct_a = ast_create_struct_type(&arena, "Test", fields, 1, NULL, 0, false, false, false, NULL);

    /* Type should equal itself */
    assert(ast_type_equals(struct_a, struct_a) == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_equals_self");
}

/* ============================================================================
 * Struct Field Lookup Tests
 * ============================================================================
 * These tests verify ast_struct_get_field and ast_struct_get_field_index.
 * ============================================================================ */

/* Test: get_field finds existing field by name */
static void test_struct_get_field_exists()
{
    DEBUG_INFO("Starting test_struct_get_field_exists");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[3];
    fields[0] = create_test_field(&arena, "a", int_type, NULL);
    fields[1] = create_test_field(&arena, "b", double_type, NULL);
    fields[2] = create_test_field(&arena, "c", int_type, NULL);

    Type *struct_type = ast_create_struct_type(&arena, "Test", fields, 3, NULL, 0, false, false, false, NULL);

    /* Find each field */
    StructField *field_a = ast_struct_get_field(struct_type, "a");
    assert(field_a != NULL);
    assert(strcmp(field_a->name, "a") == 0);
    assert(field_a->type->kind == TYPE_INT);

    StructField *field_b = ast_struct_get_field(struct_type, "b");
    assert(field_b != NULL);
    assert(strcmp(field_b->name, "b") == 0);
    assert(field_b->type->kind == TYPE_DOUBLE);

    StructField *field_c = ast_struct_get_field(struct_type, "c");
    assert(field_c != NULL);
    assert(strcmp(field_c->name, "c") == 0);
    assert(field_c->type->kind == TYPE_INT);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_field_exists");
}

/* Test: get_field returns NULL for non-existent field */
static void test_struct_get_field_not_found()
{
    DEBUG_INFO("Starting test_struct_get_field_not_found");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);
    Type *struct_type = ast_create_struct_type(&arena, "Test", fields, 1, NULL, 0, false, false, false, NULL);

    /* Non-existent field should return NULL */
    StructField *field = ast_struct_get_field(struct_type, "nonexistent");
    assert(field == NULL);

    /* Case-sensitive lookup - different case should return NULL */
    field = ast_struct_get_field(struct_type, "VAL");
    assert(field == NULL);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_field_not_found");
}

/* Test: get_field with NULL inputs */
static void test_struct_get_field_null_inputs()
{
    DEBUG_INFO("Starting test_struct_get_field_null_inputs");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);
    Type *struct_type = ast_create_struct_type(&arena, "Test", fields, 1, NULL, 0, false, false, false, NULL);

    /* NULL struct type */
    StructField *field = ast_struct_get_field(NULL, "val");
    assert(field == NULL);

    /* NULL field name */
    field = ast_struct_get_field(struct_type, NULL);
    assert(field == NULL);

    /* Non-struct type */
    field = ast_struct_get_field(int_type, "val");
    assert(field == NULL);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_field_null_inputs");
}

/* Test: get_field_index returns correct index */
static void test_struct_get_field_index()
{
    DEBUG_INFO("Starting test_struct_get_field_index");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    StructField fields[3];
    fields[0] = create_test_field(&arena, "first", int_type, NULL);
    fields[1] = create_test_field(&arena, "second", double_type, NULL);
    fields[2] = create_test_field(&arena, "third", bool_type, NULL);

    Type *struct_type = ast_create_struct_type(&arena, "Test", fields, 3, NULL, 0, false, false, false, NULL);

    /* Get indices for each field */
    assert(ast_struct_get_field_index(struct_type, "first") == 0);
    assert(ast_struct_get_field_index(struct_type, "second") == 1);
    assert(ast_struct_get_field_index(struct_type, "third") == 2);

    /* Non-existent field should return -1 */
    assert(ast_struct_get_field_index(struct_type, "nonexistent") == -1);

    /* NULL inputs should return -1 */
    assert(ast_struct_get_field_index(NULL, "first") == -1);
    assert(ast_struct_get_field_index(struct_type, NULL) == -1);
    assert(ast_struct_get_field_index(int_type, "first") == -1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_field_index");
}

/* ============================================================================
 * get_type_size Tests for Structs
 * ============================================================================
 * These tests verify get_type_size returns correct sizes for struct types.
 * ============================================================================ */

/* Test: get_type_size returns computed struct size */
static void test_struct_get_type_size()
{
    DEBUG_INFO("Starting test_struct_get_type_size");

    Arena arena;
    arena_init(&arena, 4096);

    /* Create struct { a: int, b: byte } - should be 16 bytes with padding */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 2;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    struct_type->as.struct_type.fields[0].name = "a";
    struct_type->as.struct_type.fields[0].type = int_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "b";
    struct_type->as.struct_type.fields[1].type = byte_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    /* Calculate layout first */
    calculate_struct_layout(struct_type);

    /* Now get_type_size should return the computed size */
    int size = get_type_size(struct_type);
    assert(size == 16);  /* int(8) + byte(1) + padding(7) = 16 */

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_type_size");
}

/* Test: get_type_size returns 0 for empty struct */
static void test_struct_get_type_size_empty()
{
    DEBUG_INFO("Starting test_struct_get_type_size_empty");

    Arena arena;
    arena_init(&arena, 4096);

    /* Create empty struct */
    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Empty";
    struct_type->as.struct_type.field_count = 0;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = NULL;

    calculate_struct_layout(struct_type);

    int size = get_type_size(struct_type);
    assert(size == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_get_type_size_empty");
}

/* Test: get_type_size for various primitive types */
static void test_get_type_size_primitives()
{
    DEBUG_INFO("Starting test_get_type_size_primitives");

    Arena arena;
    arena_init(&arena, 4096);

    /* 1-byte types */
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_BYTE)) == 1);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_BOOL)) == 1);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_CHAR)) == 1);

    /* 4-byte types */
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_INT32)) == 4);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_UINT32)) == 4);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_FLOAT)) == 4);

    /* 8-byte types */
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_INT)) == 8);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_UINT)) == 8);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_LONG)) == 8);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_DOUBLE)) == 8);

    /* Pointer and reference types (8 bytes on 64-bit) */
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_STRING)) == 8);
    assert(get_type_size(ast_create_pointer_type(&arena, ast_create_primitive_type(&arena, TYPE_INT))) == 8);
    assert(get_type_size(ast_create_array_type(&arena, ast_create_primitive_type(&arena, TYPE_INT))) == 8);

    /* Special types */
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_VOID)) == 0);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_NIL)) == 0);
    assert(get_type_size(ast_create_primitive_type(&arena, TYPE_ANY)) == 16);

    /* NULL type */
    assert(get_type_size(NULL) == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_get_type_size_primitives");
}

/* ============================================================================
 * get_type_alignment Tests
 * ============================================================================
 * These tests verify get_type_alignment returns correct alignment for types.
 * ============================================================================ */

/* Test: get_type_alignment for various primitive types */
static void test_get_type_alignment_primitives()
{
    DEBUG_INFO("Starting test_get_type_alignment_primitives");

    Arena arena;
    arena_init(&arena, 4096);

    /* 1-byte alignment */
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_BYTE)) == 1);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_BOOL)) == 1);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_CHAR)) == 1);

    /* 4-byte alignment */
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_INT32)) == 4);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_UINT32)) == 4);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_FLOAT)) == 4);

    /* 8-byte alignment */
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_INT)) == 8);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_UINT)) == 8);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_LONG)) == 8);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_DOUBLE)) == 8);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_STRING)) == 8);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_ANY)) == 8);

    /* Pointer and array types (8-byte alignment) */
    assert(get_type_alignment(ast_create_pointer_type(&arena, ast_create_primitive_type(&arena, TYPE_INT))) == 8);
    assert(get_type_alignment(ast_create_array_type(&arena, ast_create_primitive_type(&arena, TYPE_INT))) == 8);

    /* Special types */
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_VOID)) == 1);
    assert(get_type_alignment(ast_create_primitive_type(&arena, TYPE_NIL)) == 1);

    /* NULL type returns 1 */
    assert(get_type_alignment(NULL) == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_get_type_alignment_primitives");
}

/* Test: get_type_alignment returns computed struct alignment */
static void test_get_type_alignment_struct()
{
    DEBUG_INFO("Starting test_get_type_alignment_struct");

    Arena arena;
    arena_init(&arena, 4096);

    /* Create struct with int64 field - alignment should be 8 */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 1;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 1);

    struct_type->as.struct_type.fields[0].name = "val";
    struct_type->as.struct_type.fields[0].type = int_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(get_type_alignment(struct_type) == 8);

    /* Create struct with only 1-byte fields - alignment should be 1 */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *byte_struct = arena_alloc(&arena, sizeof(Type));
    memset(byte_struct, 0, sizeof(Type));
    byte_struct->kind = TYPE_STRUCT;
    byte_struct->as.struct_type.name = "ByteStruct";
    byte_struct->as.struct_type.field_count = 2;
    byte_struct->as.struct_type.is_native = false;
    byte_struct->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    byte_struct->as.struct_type.fields[0].name = "a";
    byte_struct->as.struct_type.fields[0].type = byte_type;
    byte_struct->as.struct_type.fields[0].offset = 0;
    byte_struct->as.struct_type.fields[0].default_value = NULL;
    byte_struct->as.struct_type.fields[0].c_alias = NULL;

    byte_struct->as.struct_type.fields[1].name = "b";
    byte_struct->as.struct_type.fields[1].type = byte_type;
    byte_struct->as.struct_type.fields[1].offset = 0;
    byte_struct->as.struct_type.fields[1].default_value = NULL;
    byte_struct->as.struct_type.fields[1].c_alias = NULL;

    calculate_struct_layout(byte_struct);

    assert(get_type_alignment(byte_struct) == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_get_type_alignment_struct");
}


void test_type_checker_struct_utility_main(void)
{
    TEST_SECTION("Struct Type Checker - Utility Functions");

    /* Struct type equality tests */
    TEST_RUN("struct_type_equals_matching_names", test_struct_type_equals_matching_names);
    TEST_RUN("struct_type_equals_different_names", test_struct_type_equals_different_names);
    TEST_RUN("struct_type_equals_null_names", test_struct_type_equals_null_names);
    TEST_RUN("struct_type_equals_null_types", test_struct_type_equals_null_types);
    TEST_RUN("struct_type_equals_self", test_struct_type_equals_self);

    /* Struct field lookup tests */
    TEST_RUN("struct_get_field_exists", test_struct_get_field_exists);
    TEST_RUN("struct_get_field_not_found", test_struct_get_field_not_found);
    TEST_RUN("struct_get_field_null_inputs", test_struct_get_field_null_inputs);
    TEST_RUN("struct_get_field_index", test_struct_get_field_index);

    /* get_type_size tests */
    TEST_RUN("struct_get_type_size", test_struct_get_type_size);
    TEST_RUN("struct_get_type_size_empty", test_struct_get_type_size_empty);
    TEST_RUN("get_type_size_primitives", test_get_type_size_primitives);

    /* get_type_alignment tests */
    TEST_RUN("get_type_alignment_primitives", test_get_type_alignment_primitives);
    TEST_RUN("get_type_alignment_struct", test_get_type_alignment_struct);
}
