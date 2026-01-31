// tests/type_checker_tests_struct_packed.c
// Packed struct layout tests

/* ============================================================================
 * Packed Struct Layout Tests
 * ============================================================================
 * These tests verify that packed structs (is_packed=true) have no padding.
 * ============================================================================ */

static void test_struct_layout_packed_mixed()
{
    DEBUG_INFO("Starting test_struct_layout_packed_mixed");

    Arena arena;
    arena_init(&arena, 4096);

    /* Packed struct with int64 + byte + int64 should have no padding:
     * C equivalent with #pragma pack(1):
     * struct Test { int64_t a; char b; int64_t c; };
     * Expected: a at 0 (8 bytes), b at 8 (1 byte), c at 9 (8 bytes)
     * Size: 17 (no padding), Alignment: 1
     */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "PackedTest";
    struct_type->as.struct_type.field_count = 3;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.is_packed = true;  /* PACKED! */
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

    /* No padding - fields are consecutive */
    assert(struct_type->as.struct_type.fields[0].offset == 0);
    assert(struct_type->as.struct_type.fields[1].offset == 8);   /* immediately after int */
    assert(struct_type->as.struct_type.fields[2].offset == 9);   /* immediately after byte */
    assert(struct_type->as.struct_type.size == 17);              /* 8 + 1 + 8 = 17 */
    assert(struct_type->as.struct_type.alignment == 1);          /* packed = alignment 1 */

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_packed_mixed");
}

/* Test: packed struct for binary format (file header example) */
static void test_struct_layout_packed_binary_header()
{
    DEBUG_INFO("Starting test_struct_layout_packed_binary_header");

    Arena arena;
    arena_init(&arena, 4096);

    /* Binary file header with packed layout:
     * struct FileHeader { int32 magic; byte version; byte flags; int32 size; };
     * Expected: magic at 0 (4 bytes), version at 4 (1 byte), flags at 5 (1 byte), size at 6 (4 bytes)
     * Size: 10 (no padding), Alignment: 1
     */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);

    Type *struct_type = arena_alloc(&arena, sizeof(Type));
    memset(struct_type, 0, sizeof(Type));
    struct_type->kind = TYPE_STRUCT;
    struct_type->as.struct_type.name = "FileHeader";
    struct_type->as.struct_type.field_count = 4;
    struct_type->as.struct_type.is_native = false;
    struct_type->as.struct_type.is_packed = true;  /* PACKED! */
    struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 4);

    struct_type->as.struct_type.fields[0].name = "magic";
    struct_type->as.struct_type.fields[0].type = int32_type;
    struct_type->as.struct_type.fields[0].offset = 0;
    struct_type->as.struct_type.fields[0].default_value = NULL;
    struct_type->as.struct_type.fields[0].c_alias = NULL;

    struct_type->as.struct_type.fields[1].name = "version";
    struct_type->as.struct_type.fields[1].type = byte_type;
    struct_type->as.struct_type.fields[1].offset = 0;
    struct_type->as.struct_type.fields[1].default_value = NULL;
    struct_type->as.struct_type.fields[1].c_alias = NULL;

    struct_type->as.struct_type.fields[2].name = "flags";
    struct_type->as.struct_type.fields[2].type = byte_type;
    struct_type->as.struct_type.fields[2].offset = 0;
    struct_type->as.struct_type.fields[2].default_value = NULL;
    struct_type->as.struct_type.fields[2].c_alias = NULL;

    struct_type->as.struct_type.fields[3].name = "size";
    struct_type->as.struct_type.fields[3].type = int32_type;
    struct_type->as.struct_type.fields[3].offset = 0;
    struct_type->as.struct_type.fields[3].default_value = NULL;
    struct_type->as.struct_type.fields[3].c_alias = NULL;

    calculate_struct_layout(struct_type);

    /* Verify consecutive layout */
    assert(struct_type->as.struct_type.fields[0].offset == 0);   /* magic: 4 bytes */
    assert(struct_type->as.struct_type.fields[1].offset == 4);   /* version: 1 byte */
    assert(struct_type->as.struct_type.fields[2].offset == 5);   /* flags: 1 byte */
    assert(struct_type->as.struct_type.fields[3].offset == 6);   /* size: 4 bytes */
    assert(struct_type->as.struct_type.size == 10);              /* 4 + 1 + 1 + 4 = 10 */
    assert(struct_type->as.struct_type.alignment == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_packed_binary_header");
}

/* Test: compare packed vs non-packed layout */
static void test_struct_layout_packed_vs_unpacked()
{
    DEBUG_INFO("Starting test_struct_layout_packed_vs_unpacked");

    Arena arena;
    arena_init(&arena, 4096);

    /* Same struct, packed vs unpacked:
     * struct Test { int32 a; int64 b; };
     * Unpacked: a at 0, b at 8 (4 bytes padding), size=16, alignment=8
     * Packed: a at 0, b at 4 (no padding), size=12, alignment=1
     */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create unpacked struct */
    Type *unpacked_type = arena_alloc(&arena, sizeof(Type));
    memset(unpacked_type, 0, sizeof(Type));
    unpacked_type->kind = TYPE_STRUCT;
    unpacked_type->as.struct_type.name = "Unpacked";
    unpacked_type->as.struct_type.field_count = 2;
    unpacked_type->as.struct_type.is_native = false;
    unpacked_type->as.struct_type.is_packed = false;
    unpacked_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    unpacked_type->as.struct_type.fields[0].name = "a";
    unpacked_type->as.struct_type.fields[0].type = int32_type;
    unpacked_type->as.struct_type.fields[0].offset = 0;
    unpacked_type->as.struct_type.fields[0].default_value = NULL;
    unpacked_type->as.struct_type.fields[0].c_alias = NULL;

    unpacked_type->as.struct_type.fields[1].name = "b";
    unpacked_type->as.struct_type.fields[1].type = int_type;
    unpacked_type->as.struct_type.fields[1].offset = 0;
    unpacked_type->as.struct_type.fields[1].default_value = NULL;
    unpacked_type->as.struct_type.fields[1].c_alias = NULL;

    /* Create packed struct with same fields */
    Type *packed_type = arena_alloc(&arena, sizeof(Type));
    memset(packed_type, 0, sizeof(Type));
    packed_type->kind = TYPE_STRUCT;
    packed_type->as.struct_type.name = "Packed";
    packed_type->as.struct_type.field_count = 2;
    packed_type->as.struct_type.is_native = false;
    packed_type->as.struct_type.is_packed = true;  /* PACKED! */
    packed_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    packed_type->as.struct_type.fields[0].name = "a";
    packed_type->as.struct_type.fields[0].type = int32_type;
    packed_type->as.struct_type.fields[0].offset = 0;
    packed_type->as.struct_type.fields[0].default_value = NULL;
    packed_type->as.struct_type.fields[0].c_alias = NULL;

    packed_type->as.struct_type.fields[1].name = "b";
    packed_type->as.struct_type.fields[1].type = int_type;
    packed_type->as.struct_type.fields[1].offset = 0;
    packed_type->as.struct_type.fields[1].default_value = NULL;
    packed_type->as.struct_type.fields[1].c_alias = NULL;

    /* Calculate layouts */
    calculate_struct_layout(unpacked_type);
    calculate_struct_layout(packed_type);

    /* Verify unpacked has padding */
    assert(unpacked_type->as.struct_type.fields[0].offset == 0);
    assert(unpacked_type->as.struct_type.fields[1].offset == 8);   /* 4 bytes padding */
    assert(unpacked_type->as.struct_type.size == 16);
    assert(unpacked_type->as.struct_type.alignment == 8);

    /* Verify packed has no padding */
    assert(packed_type->as.struct_type.fields[0].offset == 0);
    assert(packed_type->as.struct_type.fields[1].offset == 4);     /* no padding */
    assert(packed_type->as.struct_type.size == 12);                /* 4 + 8 = 12 */
    assert(packed_type->as.struct_type.alignment == 1);

    /* Packed is 4 bytes smaller */
    assert(unpacked_type->as.struct_type.size - packed_type->as.struct_type.size == 4);

    arena_free(&arena);
    DEBUG_INFO("Finished test_struct_layout_packed_vs_unpacked");
}

void test_type_checker_struct_packed_main(void)
{
    TEST_SECTION("Struct Type Checker - Packed Layout");

    TEST_RUN("struct_layout_packed_mixed", test_struct_layout_packed_mixed);
    TEST_RUN("struct_layout_packed_binary_header", test_struct_layout_packed_binary_header);
    TEST_RUN("struct_layout_packed_vs_unpacked", test_struct_layout_packed_vs_unpacked);
}
