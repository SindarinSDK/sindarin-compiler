// tests/type_checker_tests_struct_layout.c
// Struct layout calculation tests

/* ============================================================================
 * Struct Layout Calculation Tests
 * ============================================================================
 * These tests verify calculate_struct_layout computes correct field offsets,
 * struct size, and alignment matching C compiler behavior.
 * ============================================================================ */

static void test_struct_layout_all_8byte_fields()
{
    DEBUG_INFO("Starting test_struct_layout_all_8byte_fields");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { int64_t a; int64_t b; int64_t c; };
     * Expected: a at offset 0, b at offset 8, c at offset 16
     * Size: 24, Alignment: 8
     */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 3;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 3);

    struct_type->as.struct_type.fields[0].name = "a";
    struct_type->as.struct_type.fields[0].type = int_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "b";
    struct_type->as.struct_type.fields[1].type = int_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    struct_type->as.struct_type.fields[2].name = "c";
    struct_type->as.struct_type.fields[2].type = int_type;
    struct_type->as.struct_type.fields[2].offset = 0;
    struct_type->as.struct_type.fields[2].default_value = NULL;
    struct_type->as.struct_type.fields[2].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 8);
    assert(struct_type->as.struct_type.fields[2].offset == 16);
    assert(struct_type->as.struct_type.size == 24);
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_all_8byte_fields");
}

/* Test: layout with padding between 1-byte and 8-byte fields */
static void test_struct_layout_byte_int_padding()
{
    DEBUG_INFO("Starting test_struct_layout_byte_int_padding");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { int64_t a; char b; int64_t c; };
     * Expected: a at 0, b at 8, c at 16 (7 bytes padding after b)
     * Size: 24, Alignment: 8
     */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 3;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 3);

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

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 8);
    assert(struct_type->as.struct_type.fields[2].offset == 16);
    assert(struct_type->as.struct_type.size == 24);
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_byte_int_padding");
}

/* Test: layout with trailing padding for struct alignment */
static void test_struct_layout_trailing_padding()
{
    DEBUG_INFO("Starting test_struct_layout_trailing_padding");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { int64_t a; char b; };
     * Expected: a at 0, b at 8
     * Size: 16 (7 bytes trailing padding), Alignment: 8
     */
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

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 8);
    assert(struct_type->as.struct_type.size == 16);  /* Trailing padding to 8-byte alignment */
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_trailing_padding");
}

/* Test: layout with 4-byte fields (int32, float) */
static void test_struct_layout_4byte_fields()
{
    DEBUG_INFO("Starting test_struct_layout_4byte_fields");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { int32_t a; int32_t b; float c; };
     * Expected: a at 0, b at 4, c at 8
     * Size: 12, Alignment: 4
     */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 3;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 3);

    struct_type->as.struct_type.fields[0].name = "a";
    struct_type->as.struct_type.fields[0].type = int32_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "b";
    struct_type->as.struct_type.fields[1].type = int32_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    struct_type->as.struct_type.fields[2].name = "c";
    struct_type->as.struct_type.fields[2].type = float_type;
    struct_type->as.struct_type.fields[2].offset = 0;
    struct_type->as.struct_type.fields[2].default_value = NULL;
    struct_type->as.struct_type.fields[2].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 4);
    assert(struct_type->as.struct_type.fields[2].offset == 8);
    assert(struct_type->as.struct_type.size == 12);
    assert(struct_type->as.struct_type.alignment == 4);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_4byte_fields");
}

/* Test: layout with mixed alignment - 4-byte then 8-byte field */
static void test_struct_layout_mixed_alignment()
{
    DEBUG_INFO("Starting test_struct_layout_mixed_alignment");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { int32_t a; int64_t b; };
     * Expected: a at 0, b at 8 (4 bytes padding)
     * Size: 16, Alignment: 8
     */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 2;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    struct_type->as.struct_type.fields[0].name = "a";
    struct_type->as.struct_type.fields[0].type = int32_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "b";
    struct_type->as.struct_type.fields[1].type = int_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 8);  /* Padded to 8-byte alignment */
    assert(struct_type->as.struct_type.size == 16);
    assert(struct_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_mixed_alignment");
}

/* Test: layout with all 1-byte fields (no padding) */
static void test_struct_layout_all_1byte_fields()
{
    DEBUG_INFO("Starting test_struct_layout_all_1byte_fields");

    Arena arena;
    arena_init(&arena, 4096);

    /* C equivalent:
     * struct Test { char a; char b; char c; bool d; };
     * Expected: a at 0, b at 1, c at 2, d at 3
     * Size: 4, Alignment: 1
     */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Test";
    struct_type->as.struct_type.field_count = 4;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 4);

    struct_type->as.struct_type.fields[0].name = "a";
    struct_type->as.struct_type.fields[0].type = byte_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "b";
    struct_type->as.struct_type.fields[1].type = char_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    struct_type->as.struct_type.fields[2].name = "c";
    struct_type->as.struct_type.fields[2].type = byte_type;
    struct_type->as.struct_type.fields[2].offset = 0;
    struct_type->as.struct_type.fields[2].default_value = NULL;
    struct_type->as.struct_type.fields[2].c_alias = NULL;

    struct_type->as.struct_type.fields[3].name = "d";
    struct_type->as.struct_type.fields[3].type = bool_type;
    struct_type->as.struct_type.fields[3].offset = 0;
    struct_type->as.struct_type.fields[3].default_value = NULL;
    struct_type->as.struct_type.fields[3].c_alias = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 1);
    assert(struct_type->as.struct_type.fields[2].offset == 2);
    assert(struct_type->as.struct_type.fields[3].offset == 3);
    assert(struct_type->as.struct_type.size == 4);
    assert(struct_type->as.struct_type.alignment == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_all_1byte_fields");
}

/* Test: layout for empty struct */
static void test_struct_layout_empty()
{
    DEBUG_INFO("Starting test_struct_layout_empty");

    Arena arena;
    arena_init(&arena, 4096);

    /* Empty struct - size and alignment should be 0/1 */
    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "Empty";
    struct_type->as.struct_type.field_count = 0;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.fields = NULL;

    calculate_struct_layout(struct_type);

    assert(struct_type->as.struct_type.size == 0);
    assert(struct_type->as.struct_type.alignment == 1);  /* Minimum alignment is 1 */

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_empty");
}

/* Test: layout with nested struct */
static void test_struct_layout_nested()
{
    DEBUG_INFO("Starting test_struct_layout_nested");

    Arena arena;
    arena_init(&arena, 4096);

    /* First create inner struct Point { double x; double y; } */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    Type *point_type = arena_alloc(&arena, sizeof(Type));
    memset(point_type, 0, sizeof(Type));
    point_type->kind = TYPE_STRUCT;
    point_type->as.struct_type.name = "Point";
    point_type->as.struct_type.field_count = 2;
    point_type->as.struct_type.is_native = false;
    point_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    point_type->as.struct_type.fields[0].name = "x";
    point_type->as.struct_type.fields[0].type = double_type;
    point_type->as.struct_type.fields[0].offset = 0;
    point_type->as.struct_type.fields[0].default_value = NULL;
    point_type->as.struct_type.fields[0].c_alias = NULL;

    point_type->as.struct_type.fields[1].name = "y";
    point_type->as.struct_type.fields[1].type = double_type;
    point_type->as.struct_type.fields[1].offset = 0;
    point_type->as.struct_type.fields[1].default_value = NULL;
    point_type->as.struct_type.fields[1].c_alias = NULL;

    /* Calculate Point layout: size=16, alignment=8 */
    calculate_struct_layout(point_type);

    assert(point_type->as.struct_type.size == 16);
    assert(point_type->as.struct_type.alignment == 8);

    /* Now create outer struct Rect { Point origin; int32_t width; int32_t height; } */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);

    Type *rect_type = arena_alloc(&arena, sizeof(Type));
    memset(rect_type, 0, sizeof(Type));
    rect_type->kind = TYPE_STRUCT;
    rect_type->as.struct_type.name = "Rect";
    rect_type->as.struct_type.field_count = 3;
    rect_type->as.struct_type.is_native = false;
    rect_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 3);

    rect_type->as.struct_type.fields[0].name = "origin";
    rect_type->as.struct_type.fields[0].type = point_type;
    rect_type->as.struct_type.fields[0].offset = 0;
    rect_type->as.struct_type.fields[0].default_value = NULL;
    rect_type->as.struct_type.fields[0].c_alias = NULL;

    rect_type->as.struct_type.fields[1].name = "width";
    rect_type->as.struct_type.fields[1].type = int32_type;
    rect_type->as.struct_type.fields[1].offset = 0;
    rect_type->as.struct_type.fields[1].default_value = NULL;
    rect_type->as.struct_type.fields[1].c_alias = NULL;

    rect_type->as.struct_type.fields[2].name = "height";
    rect_type->as.struct_type.fields[2].type = int32_type;
    rect_type->as.struct_type.fields[2].offset = 0;
    rect_type->as.struct_type.fields[2].default_value = NULL;
    rect_type->as.struct_type.fields[2].c_alias = NULL;

    /* Calculate Rect layout:
     * origin at 0 (size 16, alignment 8)
     * width at 16 (size 4, alignment 4)
     * height at 20 (size 4, alignment 4)
     * Total: 24, Alignment: 8
     */
    calculate_struct_layout(rect_type);

    assert(rect_type->as.struct_type.fields[0].offset == 0);
    assert(rect_type->as.struct_type.fields[1].offset == 16);
    assert(rect_type->as.struct_type.fields[2].offset == 20);
    assert(rect_type->as.struct_type.size == 24);
    assert(rect_type->as.struct_type.alignment == 8);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_nested");
}

void test_type_checker_struct_layout_main(void)
{
    TEST_SECTION("Struct Type Checker - Layout");

    TEST_RUN("struct_layout_all_8byte_fields", test_struct_layout_all_8byte_fields);
    TEST_RUN("struct_layout_byte_int_padding", test_struct_layout_byte_int_padding);
    TEST_RUN("struct_layout_trailing_padding", test_struct_layout_trailing_padding);
    TEST_RUN("struct_layout_4byte_fields", test_struct_layout_4byte_fields);
    TEST_RUN("struct_layout_mixed_alignment", test_struct_layout_mixed_alignment);
    TEST_RUN("struct_layout_all_1byte_fields", test_struct_layout_all_1byte_fields);
    TEST_RUN("struct_layout_empty", test_struct_layout_empty);
    TEST_RUN("struct_layout_nested", test_struct_layout_nested);
}
