// tests/type_checker_tests_struct_misc.c
// Miscellaneous struct tests (large, single field, clone, string, predicate, deeply nested)

/* ============================================================================
 * Large and Single Field Struct Tests
 * ============================================================================ */

static void test_struct_layout_large()
{
    DEBUG_INFO("Starting test_struct_layout_large");

    Arena arena;
    arena_init(&arena, 8192);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create struct with 20 int fields */
    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "LargeStruct";
    struct_type->as.struct_type.field_count = 20;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 20);

    for (int i = 0; i < 20; i++) {
        char name[16];
        snprintf(name, sizeof(name), "field_%d", i);
        struct_type->as.struct_type.fields[i].name = arena_strdup(&arena, name);
        struct_type->as.struct_type.fields[i].type = int_type;
        struct_type->as.struct_type.fields[i].offset = 0;
        struct_type->as.struct_type.fields[i].default_value = NULL;
    }

    calculate_struct_layout(struct_type);

    /* 20 int64 fields, each 8 bytes = 160 bytes */
    assert(struct_type->as.struct_type.size == 160);
    assert(struct_type->as.struct_type.alignment == 8);

    /* Verify field offsets */
    for (int i = 0; i < 20; i++) {
        assert(struct_type->as.struct_type.fields[i].offset == (size_t)(i * 8));
    }

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_large");
}

/* Test: large struct with mixed types */
static void test_struct_layout_large_mixed()
{
    DEBUG_INFO("Starting test_struct_layout_large_mixed");

    Arena arena;
    arena_init(&arena, 8192);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create struct with alternating field types to test complex padding */
    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "MixedLarge";
    struct_type->as.struct_type.field_count = 8;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 8);

    /* int64, byte, int64, byte, int32, byte, double, byte */
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

    struct_type->as.struct_type.fields[2].name = "c";
    struct_type->as.struct_type.fields[2].type = int_type;
    struct_type->as.struct_type.fields[2].offset = 0;
    struct_type->as.struct_type.fields[2].default_value = NULL;
    struct_type->as.struct_type.fields[2].c_alias = NULL;

    struct_type->as.struct_type.fields[3].name = "d";
    struct_type->as.struct_type.fields[3].type = byte_type;
    struct_type->as.struct_type.fields[3].offset = 0;
    struct_type->as.struct_type.fields[3].default_value = NULL;
    struct_type->as.struct_type.fields[3].c_alias = NULL;

    struct_type->as.struct_type.fields[4].name = "e";
    struct_type->as.struct_type.fields[4].type = int32_type;
    struct_type->as.struct_type.fields[4].offset = 0;
    struct_type->as.struct_type.fields[4].default_value = NULL;
    struct_type->as.struct_type.fields[4].c_alias = NULL;

    struct_type->as.struct_type.fields[5].name = "f";
    struct_type->as.struct_type.fields[5].type = byte_type;
    struct_type->as.struct_type.fields[5].offset = 0;
    struct_type->as.struct_type.fields[5].default_value = NULL;
    struct_type->as.struct_type.fields[5].c_alias = NULL;

    struct_type->as.struct_type.fields[6].name = "g";
    struct_type->as.struct_type.fields[6].type = double_type;
    struct_type->as.struct_type.fields[6].offset = 0;
    struct_type->as.struct_type.fields[6].default_value = NULL;
    struct_type->as.struct_type.fields[6].c_alias = NULL;

    struct_type->as.struct_type.fields[7].name = "h";
    struct_type->as.struct_type.fields[7].type = byte_type;
    struct_type->as.struct_type.fields[7].offset = 0;
    struct_type->as.struct_type.fields[7].default_value = NULL;
    struct_type->as.struct_type.fields[7].c_alias = NULL;

    calculate_struct_layout(struct_type);

    /* Layout calculation:
     * a: offset 0, size 8
     * b: offset 8, size 1 (needs 7 bytes padding before next int64)
     * c: offset 16, size 8
     * d: offset 24, size 1 (needs 3 bytes padding before int32)
     * e: offset 28, size 4
     * f: offset 32, size 1 (needs 7 bytes padding before double)
     * g: offset 40, size 8
     * h: offset 48, size 1 (needs 7 bytes trailing padding)
     * Total: 56 bytes, alignment 8
     */
    assert(struct_type->as.struct_type.fields[0].offset == 0);   /* a */
    assert(struct_type->as.struct_type.fields[1].offset == 8);   /* b */
    assert(struct_type->as.struct_type.fields[2].offset == 16);  /* c */
    assert(struct_type->as.struct_type.fields[3].offset == 24);  /* d */
    assert(struct_type->as.struct_type.fields[4].offset == 28);  /* e */
    assert(struct_type->as.struct_type.fields[5].offset == 32);  /* f */
    assert(struct_type->as.struct_type.fields[6].offset == 40);  /* g */
    assert(struct_type->as.struct_type.fields[7].offset == 48);  /* h */
    assert(struct_type->as.struct_type.size == 56);
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_large_mixed");
}

/* ============================================================================
 * Single Field Struct Tests
 * ============================================================================
 * These tests verify struct handling with exactly one field.
 * ============================================================================ */

/* Test: single field struct with int */
static void test_struct_layout_single_int()
{
    DEBUG_INFO("Starting test_struct_layout_single_int");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "SingleInt";
    struct_type->as.struct_type.field_count = 1;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 1);

    struct_type->as.struct_type.fields[0].name = "val";
    struct_type->as.struct_type.fields[0].type = int_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.size == 8);
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_single_int");
}

/* Test: single field struct with byte */
static void test_struct_layout_single_byte()
{
    DEBUG_INFO("Starting test_struct_layout_single_byte");

    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "SingleByte";
    struct_type->as.struct_type.field_count = 1;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 1);

    struct_type->as.struct_type.fields[0].name = "val";
    struct_type->as.struct_type.fields[0].type = byte_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.size == 1);
    assert(struct_type->as.struct_type.alignment == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_single_byte");
}

/* Test: single field struct with nested struct */
static void test_struct_layout_single_nested()
{
    DEBUG_INFO("Starting test_struct_layout_single_nested");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create inner struct with 2 int fields */
    Type *inner_type = arena_alloc(&arena, sizeof(Type));
    memset(inner_type, 0, sizeof(Type));
    inner_type->kind = TYPE_STRUCT;
    inner_type->as.struct_type.name = "Inner";
    inner_type->as.struct_type.field_count = 2;
    inner_type->as.struct_type.is_native = false;
    inner_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    inner_type->as.struct_type.fields[0].name = "x";
    inner_type->as.struct_type.fields[0].type = int_type;
    inner_type->as.struct_type.fields[0].offset = 0;
    inner_type->as.struct_type.fields[0].default_value = NULL;
    inner_type->as.struct_type.fields[0].c_alias = NULL;

    inner_type->as.struct_type.fields[1].name = "y";
    inner_type->as.struct_type.fields[1].type = int_type;
    inner_type->as.struct_type.fields[1].offset = 0;
    inner_type->as.struct_type.fields[1].default_value = NULL;
    inner_type->as.struct_type.fields[1].c_alias = NULL;

    calculate_struct_layout(inner_type);
    assert(inner_type->as.struct_type.size == 16);

    /* Create outer struct with single inner field */
    Type *outer_type = arena_alloc(&arena, sizeof(Type));
    memset(outer_type, 0, sizeof(Type));
    outer_type->kind = TYPE_STRUCT;
    outer_type->as.struct_type.name = "Outer";
    outer_type->as.struct_type.field_count = 1;
    outer_type->as.struct_type.is_native = false;
    outer_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 1);

    outer_type->as.struct_type.fields[0].name = "inner";
    outer_type->as.struct_type.fields[0].type = inner_type;
    outer_type->as.struct_type.fields[0].offset = 0;
    outer_type->as.struct_type.fields[0].default_value = NULL;
    outer_type->as.struct_type.fields[0].c_alias = NULL;

    calculate_struct_layout(outer_type);

    /* Outer should have same size as inner */
    assert(outer_type->as.struct_type.fields[0].offset == 0);
    assert(outer_type->as.struct_type.size == 16);
    assert(outer_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_single_nested");
}

/* ============================================================================
 * Struct Type Clone Tests
 * ============================================================================
 * These tests verify ast_clone_type works correctly for struct types.
 * ============================================================================ */

/* Test: clone struct type preserves all metadata */
static void test_struct_clone_type()
{
    DEBUG_INFO("Starting test_struct_clone_type");

    Arena arena;
    arena_init(&arena, 8192);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "x", int_type, NULL);
    fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Type *original = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    calculate_struct_layout(original);

    /* Clone the type */
    Type *cloned = ast_clone_type(&arena, original);

    /* Verify clone is not NULL and is a struct */
    assert(cloned != NULL);
    assert(cloned->kind == TYPE_STRUCT);

    /* Verify they are different pointers */
    assert(cloned != original);

    /* Verify metadata is preserved */
    assert(strcmp(cloned->as.struct_type.name, "Point") == 0);
    assert(cloned->as.struct_type.field_count == 2);
    assert(cloned->as.struct_type.is_native == false);
    assert(cloned->as.struct_type.size == original->as.struct_type.size);
    assert(cloned->as.struct_type.alignment == original->as.struct_type.alignment);

    /* Verify fields are cloned (different pointers) */
    assert(cloned->as.struct_type.fields != original->as.struct_type.fields);
    assert(strcmp(cloned->as.struct_type.fields[0].name, "x") == 0);
    assert(strcmp(cloned->as.struct_type.fields[1].name, "y") == 0);
    assert(cloned->as.struct_type.fields[0].type->kind == TYPE_INT);
    assert(cloned->as.struct_type.fields[1].type->kind == TYPE_DOUBLE);

    /* Verify field offsets are preserved */
    assert(cloned->as.struct_type.fields[0].offset == original->as.struct_type.fields[0].offset);
    assert(cloned->as.struct_type.fields[1].offset == original->as.struct_type.fields[1].offset);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_clone_type");
}

/* Test: clone native struct preserves is_native flag */
static void test_struct_clone_native()
{
    DEBUG_INFO("Starting test_struct_clone_native");

    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte = ast_create_pointer_type(&arena, byte_type);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "data", ptr_byte, NULL);

    Type *original = ast_create_struct_type(&arena, "Buffer", fields, 1, NULL, 0, true, false, false, NULL);
    Type *cloned = ast_clone_type(&arena, original);

    assert(cloned->as.struct_type.is_native == true);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_clone_native");
}

/* Test: clone NULL type returns NULL */
static void test_struct_clone_null()
{
    DEBUG_INFO("Starting test_struct_clone_null");

    Arena arena;
    arena_init(&arena, 4096);

    Type *cloned = ast_clone_type(&arena, NULL);
    assert(cloned == NULL);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_clone_null");
}

/* ============================================================================
 * Struct Type String Conversion Tests
 * ============================================================================
 * These tests verify ast_type_to_string works for struct types.
 * ============================================================================ */

/* Test: type to string for named struct */
static void test_struct_type_to_string()
{
    DEBUG_INFO("Starting test_struct_type_to_string");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Type *struct_type = ast_create_struct_type(&arena, "MyStruct", fields, 1, NULL, 0, false, false, false, NULL);

    const char *str = ast_type_to_string(&arena, struct_type);
    assert(str != NULL);
    assert(strcmp(str, "MyStruct") == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_to_string");
}

/* Test: type to string for anonymous struct (NULL name) */
static void test_struct_type_to_string_anonymous()
{
    DEBUG_INFO("Starting test_struct_type_to_string_anonymous");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Type *struct_type = ast_create_struct_type(&arena, NULL, fields, 1, NULL, 0, false, false, false, NULL);

    const char *str = ast_type_to_string(&arena, struct_type);
    assert(str != NULL);
    assert(strcmp(str, "struct") == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_type_to_string_anonymous");
}

/* ============================================================================
 * Struct Type Predicate Tests
 * ============================================================================
 * These tests verify ast_type_is_struct works correctly.
 * ============================================================================ */

/* Test: ast_type_is_struct correctly identifies struct types */
static void test_ast_type_is_struct()
{
    DEBUG_INFO("Starting test_ast_type_is_struct");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    StructField fields[1];
    fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Type *struct_type = ast_create_struct_type(&arena, "Test", fields, 1, NULL, 0, false, false, false, NULL);

    /* struct_type should return true */
    assert(ast_type_is_struct(struct_type) == 1);

    /* Non-struct types should return false */
    assert(ast_type_is_struct(int_type) == 0);
    assert(ast_type_is_struct(ast_create_primitive_type(&arena, TYPE_STRING)) == 0);
    assert(ast_type_is_struct(ast_create_pointer_type(&arena, int_type)) == 0);
    assert(ast_type_is_struct(ast_create_array_type(&arena, int_type)) == 0);

    /* NULL should return false */
    assert(ast_type_is_struct(NULL) == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_ast_type_is_struct");
}

/* ============================================================================
 * Deeply Nested Struct Tests
 * ============================================================================
 * These tests verify layout calculation for deeply nested struct types.
 * ============================================================================ */

/* Test: 3-level nested struct layout */
static void test_struct_layout_deeply_nested()
{
    DEBUG_INFO("Starting test_struct_layout_deeply_nested");

    Arena arena;
    arena_init(&arena, 8192);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    /* Level 1: Inner { val: int } -> size 8, alignment 8 */
    Type *inner_type = arena_alloc(&arena, sizeof(Type));
    memset(inner_type, 0, sizeof(Type));
    inner_type->kind = TYPE_STRUCT;
    inner_type->as.struct_type.name = "Inner";
    inner_type->as.struct_type.field_count = 1;
    inner_type->as.struct_type.is_native = false;
    inner_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 1);
    inner_type->as.struct_type.fields[0].name = "val";
    inner_type->as.struct_type.fields[0].type = int_type;
    inner_type->as.struct_type.fields[0].offset = 0;
    inner_type->as.struct_type.fields[0].default_value = NULL;
    inner_type->as.struct_type.fields[0].c_alias = NULL;
    calculate_struct_layout(inner_type);
    assert(inner_type->as.struct_type.size == 8);

    /* Level 2: Middle { inner: Inner, flag: byte } -> size 16, alignment 8 */
    Type *middle_type = arena_alloc(&arena, sizeof(Type));
    memset(middle_type, 0, sizeof(Type));
    middle_type->kind = TYPE_STRUCT;
    middle_type->as.struct_type.name = "Middle";
    middle_type->as.struct_type.field_count = 2;
    middle_type->as.struct_type.is_native = false;
    middle_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    middle_type->as.struct_type.fields[0].name = "inner";
    middle_type->as.struct_type.fields[0].type = inner_type;
    middle_type->as.struct_type.fields[0].offset = 0;
    middle_type->as.struct_type.fields[0].default_value = NULL;
    middle_type->as.struct_type.fields[0].c_alias = NULL;
    middle_type->as.struct_type.fields[1].name = "flag";
    middle_type->as.struct_type.fields[1].type = byte_type;
    middle_type->as.struct_type.fields[1].offset = 0;
    middle_type->as.struct_type.fields[1].default_value = NULL;
    middle_type->as.struct_type.fields[1].c_alias = NULL;
    calculate_struct_layout(middle_type);
    assert(middle_type->as.struct_type.size == 16);

    /* Level 3: Outer { middle: Middle, count: int } -> size 24, alignment 8 */
    Type *outer_type = arena_alloc(&arena, sizeof(Type));
    memset(outer_type, 0, sizeof(Type));
    outer_type->kind = TYPE_STRUCT;
    outer_type->as.struct_type.name = "Outer";
    outer_type->as.struct_type.field_count = 2;
    outer_type->as.struct_type.is_native = false;
    outer_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    outer_type->as.struct_type.fields[0].name = "middle";
    outer_type->as.struct_type.fields[0].type = middle_type;
    outer_type->as.struct_type.fields[0].offset = 0;
    outer_type->as.struct_type.fields[0].default_value = NULL;
    outer_type->as.struct_type.fields[0].c_alias = NULL;
    outer_type->as.struct_type.fields[1].name = "count";
    outer_type->as.struct_type.fields[1].type = int_type;
    outer_type->as.struct_type.fields[1].offset = 0;
    outer_type->as.struct_type.fields[1].default_value = NULL;
    outer_type->as.struct_type.fields[1].c_alias = NULL;
    calculate_struct_layout(outer_type);

    assert(outer_type->as.struct_type.fields[0].offset == 0);
    assert(outer_type->as.struct_type.fields[1].offset == 16);
    assert(outer_type->as.struct_type.size == 24);
    assert(outer_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_deeply_nested");
}

/* Test: calculate_struct_layout with NULL type */
static void test_struct_layout_null()
{
    DEBUG_INFO("Starting test_struct_layout_null");

    /* Should not crash when given NULL */
    calculate_struct_layout(NULL);

    Arena arena;
    arena_init(&arena, 4096);

    /* Should not crash when given non-struct type */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    calculate_struct_layout(int_type);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_null");
}

void test_type_checker_struct_misc_main(void)
{
    TEST_SECTION("Struct Type Checker - Miscellaneous");

    /* Large and single field struct tests */
    TEST_RUN("struct_layout_large", test_struct_layout_large);
    TEST_RUN("struct_layout_large_mixed", test_struct_layout_large_mixed);
    TEST_RUN("struct_layout_single_int", test_struct_layout_single_int);
    TEST_RUN("struct_layout_single_byte", test_struct_layout_single_byte);
    TEST_RUN("struct_layout_single_nested", test_struct_layout_single_nested);

    /* Struct type clone tests */
    TEST_RUN("struct_clone_type", test_struct_clone_type);
    TEST_RUN("struct_clone_native", test_struct_clone_native);
    TEST_RUN("struct_clone_null", test_struct_clone_null);

    /* Struct type string conversion tests */
    TEST_RUN("struct_type_to_string", test_struct_type_to_string);
    TEST_RUN("struct_type_to_string_anonymous", test_struct_type_to_string_anonymous);

    /* Struct type predicate tests */
    TEST_RUN("ast_type_is_struct", test_ast_type_is_struct);

    /* Deeply nested struct tests */
    TEST_RUN("struct_layout_deeply_nested", test_struct_layout_deeply_nested);
    TEST_RUN("struct_layout_null", test_struct_layout_null);
}
