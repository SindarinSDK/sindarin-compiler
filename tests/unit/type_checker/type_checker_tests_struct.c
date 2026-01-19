// tests/type_checker_tests_struct.c
// Struct declaration type checker tests

/* Helper to create a struct field */
static StructField create_test_field(Arena *arena, const char *name, Type *type, Expr *default_value)
{
    StructField field;
    field.name = arena_strdup(arena, name);
    field.type = type;
    field.offset = 0;
    field.default_value = default_value;
    field.c_alias = NULL;  /* Must initialize to avoid garbage pointer */
    return field;
}

/* Test: struct with primitive fields passes type checking */
static void test_struct_primitive_fields()
{
    DEBUG_INFO("Starting test_struct_primitive_fields");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);
    fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    /* Create struct type and register it in symbol table */
    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all primitive fields are valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_primitive_fields");
}

/* Test: struct with all supported primitive field types */
static void test_struct_all_primitive_types()
{
    DEBUG_INFO("Starting test_struct_all_primitive_types");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct with various primitive types */
    StructField fields[9];
    fields[0] = create_test_field(&arena, "a", ast_create_primitive_type(&arena, TYPE_INT), NULL);
    fields[1] = create_test_field(&arena, "b", ast_create_primitive_type(&arena, TYPE_LONG), NULL);
    fields[2] = create_test_field(&arena, "c", ast_create_primitive_type(&arena, TYPE_DOUBLE), NULL);
    fields[3] = create_test_field(&arena, "d", ast_create_primitive_type(&arena, TYPE_FLOAT), NULL);
    fields[4] = create_test_field(&arena, "e", ast_create_primitive_type(&arena, TYPE_BOOL), NULL);
    fields[5] = create_test_field(&arena, "f", ast_create_primitive_type(&arena, TYPE_BYTE), NULL);
    fields[6] = create_test_field(&arena, "g", ast_create_primitive_type(&arena, TYPE_CHAR), NULL);
    fields[7] = create_test_field(&arena, "h", ast_create_primitive_type(&arena, TYPE_STRING), NULL);
    fields[8] = create_test_field(&arena, "i", ast_create_primitive_type(&arena, TYPE_INT32), NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "AllTypes", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "AllTypes", fields, 9, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 9, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all primitive types are valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_all_primitive_types");
}

/* Test: struct with nested struct type */
static void test_struct_nested_struct_type()
{
    DEBUG_INFO("Starting test_struct_nested_struct_type");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* First define Point struct */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    StructField point_fields[2];
    point_fields[0] = create_test_field(&arena, "x", double_type, NULL);
    point_fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Now define Rectangle struct with origin: Point */
    StructField rect_fields[3];
    rect_fields[0] = create_test_field(&arena, "origin", point_type, NULL);
    rect_fields[1] = create_test_field(&arena, "width", double_type, NULL);
    rect_fields[2] = create_test_field(&arena, "height", double_type, NULL);

    Token rect_tok;
    setup_token(&rect_tok, TOKEN_IDENTIFIER, "Rectangle", 2, "test.sn", &arena);

    Type *rect_type = ast_create_struct_type(&arena, "Rectangle", rect_fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, rect_tok, rect_type);

    Stmt *rect_decl = ast_create_struct_decl_stmt(&arena, rect_tok, rect_fields, 3, NULL, 0, false, false, false, NULL, &rect_tok);
    ast_module_add_statement(&arena, &module, rect_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - nested struct type is valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_nested_struct_type");
}

/* Test: struct with array field type */
static void test_struct_array_field()
{
    DEBUG_INFO("Starting test_struct_array_field");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct with array field: data: int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "data", int_array_type, NULL);
    fields[1] = create_test_field(&arena, "count", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Container", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Container", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - array fields are valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_array_field");
}

/* Test: struct with default values - valid types */
static void test_struct_default_value_valid()
{
    DEBUG_INFO("Starting test_struct_default_value_valid");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create literal expression for default value: 42 */
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *default_expr = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "value", int_type, default_expr);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - int literal default for int field */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_default_value_valid");
}

/* Test: struct with default value type mismatch - should fail */
static void test_struct_default_value_type_mismatch()
{
    DEBUG_INFO("Starting test_struct_default_value_type_mismatch");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    /* Create string literal as default for int field - type mismatch */
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue val = {.string_value = "hello"};
    Expr *default_expr = ast_create_literal_expr(&arena, val, string_type, false, &lit_tok);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "value", int_type, default_expr);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "BadConfig", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "BadConfig", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - string default for int field */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_default_value_type_mismatch");
}

/* Test: native struct with pointer fields - should pass */
static void test_native_struct_pointer_field()
{
    DEBUG_INFO("Starting test_native_struct_pointer_field");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct with pointer field */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "data", ptr_byte_type, NULL);
    fields[1] = create_test_field(&arena, "length", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);  /* native struct */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - pointer fields allowed in native struct */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_native_struct_pointer_field");
}

/* Test: non-native struct with pointer field - should fail */
static void test_non_native_struct_pointer_field_error()
{
    DEBUG_INFO("Starting test_non_native_struct_pointer_field_error");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create non-native struct with pointer field - should fail */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "data", ptr_byte_type, NULL);
    fields[1] = create_test_field(&arena, "length", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "BadBuffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "BadBuffer", fields, 2, NULL, 0, false, false, false, NULL);  /* NOT native */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - pointer fields not allowed in non-native struct */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_non_native_struct_pointer_field_error");
}

/* Test: empty struct - should pass */
static void test_struct_empty()
{
    DEBUG_INFO("Starting test_struct_empty");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create empty struct */
    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Empty", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Empty", NULL, 0, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, NULL, 0, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - empty structs are valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_empty");
}

/* Test: struct with opaque field type - should pass */
static void test_struct_opaque_field()
{
    DEBUG_INFO("Starting test_struct_opaque_field");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Register an opaque type first */
    Token file_tok;
    setup_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create native struct with opaque field (opaque types typically used in native contexts) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "handle", opaque_type, NULL);
    fields[1] = create_test_field(&arena, "fd", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "FileInfo", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "FileInfo", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - opaque field is valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_opaque_field");
}

/* Test: struct field with NULL type - should fail */
static void test_struct_null_field_type_error()
{
    DEBUG_INFO("Starting test_struct_null_field_type_error");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct with NULL field type */
    StructField fields[1];
    fields[0] = create_test_field(&arena, "bad_field", NULL, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "BadStruct", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "BadStruct", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - NULL field type is invalid */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_null_field_type_error");
}

/* ============================================================================
 * Circular Dependency Detection Tests
 * ============================================================================ */

/* Test: direct circular dependency (struct A contains field of type A) - should fail */
static void test_struct_direct_circular_dependency()
{
    DEBUG_INFO("Starting test_struct_direct_circular_dependency");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct Node with field of type Node (direct cycle) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Node", 1, "test.sn", &arena);

    /* Manually create the self-referencing struct type without using ast_create_struct_type
     * which would try to clone field types (causing infinite recursion) */
    Type *node_type = arena_alloc(&arena, sizeof(Type));
    memset(node_type, 0, sizeof(Type));
    node_type->kind = TYPE_STRUCT;
    node_type->as.struct_type.name = arena_strdup(&arena, "Node");
    node_type->as.struct_type.field_count = 2;
    node_type->as.struct_type.is_native = false;
    node_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    /* Field 0: value: int */
    node_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    node_type->as.struct_type.fields[0].type = int_type;
    node_type->as.struct_type.fields[0].offset = 0;
    node_type->as.struct_type.fields[0].default_value = NULL;
    node_type->as.struct_type.fields[0].c_alias = NULL;

    /* Field 1: next: Node (self-reference) */
    node_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    node_type->as.struct_type.fields[1].type = node_type;  /* Direct self-reference! */
    node_type->as.struct_type.fields[1].offset = 0;
    node_type->as.struct_type.fields[1].default_value = NULL;
    node_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type here as it calls ast_clone_type
     * which will infinitely recurse on self-referential types.
     * The type checker only needs the struct_decl fields, not the symbol table entry. */

    /* Create the struct declaration using the same fields pointer */
    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = node_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = false;

    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - direct circular dependency */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_direct_circular_dependency");
}

/* Test: indirect circular dependency (A -> B -> A) - should fail */
static void test_struct_indirect_circular_dependency()
{
    DEBUG_INFO("Starting test_struct_indirect_circular_dependency");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Manually create both struct types with all fields pre-configured.
     * We cannot use symbol_table_add_type because it calls ast_clone_type
     * which would infinitely recurse on circular structures. */

    /* Create struct A */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "StructA", 1, "test.sn", &arena);

    Type *struct_a = arena_alloc(&arena, sizeof(Type));
    memset(struct_a, 0, sizeof(Type));
    struct_a->kind = TYPE_STRUCT;
    struct_a->as.struct_type.name = arena_strdup(&arena, "StructA");
    struct_a->as.struct_type.field_count = 2;
    struct_a->as.struct_type.is_native = false;
    struct_a->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_a->as.struct_type.fields[0].name = arena_strdup(&arena, "value_a");
    struct_a->as.struct_type.fields[0].type = int_type;
    struct_a->as.struct_type.fields[0].offset = 0;
    struct_a->as.struct_type.fields[0].default_value = NULL;
    struct_a->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct B */
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "StructB", 2, "test.sn", &arena);

    Type *struct_b = arena_alloc(&arena, sizeof(Type));
    memset(struct_b, 0, sizeof(Type));
    struct_b->kind = TYPE_STRUCT;
    struct_b->as.struct_type.name = arena_strdup(&arena, "StructB");
    struct_b->as.struct_type.field_count = 2;
    struct_b->as.struct_type.is_native = false;
    struct_b->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_b->as.struct_type.fields[0].name = arena_strdup(&arena, "value_b");
    struct_b->as.struct_type.fields[0].type = int_type;
    struct_b->as.struct_type.fields[0].offset = 0;
    struct_b->as.struct_type.fields[0].default_value = NULL;
    struct_b->as.struct_type.fields[0].c_alias = NULL;
    struct_b->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_a");
    struct_b->as.struct_type.fields[1].type = struct_a;  /* B -> A */
    struct_b->as.struct_type.fields[1].offset = 0;
    struct_b->as.struct_type.fields[1].default_value = NULL;
    struct_b->as.struct_type.fields[1].c_alias = NULL;

    /* Complete the cycle: A -> B */
    struct_a->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_b");
    struct_a->as.struct_type.fields[1].type = struct_b;  /* A -> B */
    struct_a->as.struct_type.fields[1].offset = 0;
    struct_a->as.struct_type.fields[1].default_value = NULL;
    struct_a->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on circular references. */

    /* Create struct declarations */
    Stmt *a_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(a_decl, 0, sizeof(Stmt));
    a_decl->type = STMT_STRUCT_DECL;
    a_decl->as.struct_decl.name = a_tok;
    a_decl->as.struct_decl.fields = struct_a->as.struct_type.fields;
    a_decl->as.struct_decl.field_count = 2;
    a_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, a_decl);

    Stmt *b_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(b_decl, 0, sizeof(Stmt));
    b_decl->type = STMT_STRUCT_DECL;
    b_decl->as.struct_decl.name = b_tok;
    b_decl->as.struct_decl.fields = struct_b->as.struct_type.fields;
    b_decl->as.struct_decl.field_count = 2;
    b_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, b_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - indirect circular dependency A -> B -> A */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_indirect_circular_dependency");
}

/* Test: multi-level circular chain (A -> B -> C -> A) - should fail */
static void test_struct_multi_level_circular_chain()
{
    DEBUG_INFO("Starting test_struct_multi_level_circular_chain");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Manually create all three struct types with fields pre-configured.
     * We cannot use symbol_table_add_type because it calls ast_clone_type
     * which would infinitely recurse on circular structures. */

    /* Create struct A */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "LevelA", 1, "test.sn", &arena);
    Type *struct_a = arena_alloc(&arena, sizeof(Type));
    memset(struct_a, 0, sizeof(Type));
    struct_a->kind = TYPE_STRUCT;
    struct_a->as.struct_type.name = arena_strdup(&arena, "LevelA");
    struct_a->as.struct_type.field_count = 2;
    struct_a->as.struct_type.is_native = false;
    struct_a->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_a->as.struct_type.fields[0].name = arena_strdup(&arena, "value_a");
    struct_a->as.struct_type.fields[0].type = int_type;
    struct_a->as.struct_type.fields[0].offset = 0;
    struct_a->as.struct_type.fields[0].default_value = NULL;
    struct_a->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct B */
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "LevelB", 2, "test.sn", &arena);
    Type *struct_b = arena_alloc(&arena, sizeof(Type));
    memset(struct_b, 0, sizeof(Type));
    struct_b->kind = TYPE_STRUCT;
    struct_b->as.struct_type.name = arena_strdup(&arena, "LevelB");
    struct_b->as.struct_type.field_count = 2;
    struct_b->as.struct_type.is_native = false;
    struct_b->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_b->as.struct_type.fields[0].name = arena_strdup(&arena, "value_b");
    struct_b->as.struct_type.fields[0].type = int_type;
    struct_b->as.struct_type.fields[0].offset = 0;
    struct_b->as.struct_type.fields[0].default_value = NULL;
    struct_b->as.struct_type.fields[0].c_alias = NULL;

    /* Create struct C */
    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "LevelC", 3, "test.sn", &arena);
    Type *struct_c = arena_alloc(&arena, sizeof(Type));
    memset(struct_c, 0, sizeof(Type));
    struct_c->kind = TYPE_STRUCT;
    struct_c->as.struct_type.name = arena_strdup(&arena, "LevelC");
    struct_c->as.struct_type.field_count = 2;
    struct_c->as.struct_type.is_native = false;
    struct_c->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);
    struct_c->as.struct_type.fields[0].name = arena_strdup(&arena, "value_c");
    struct_c->as.struct_type.fields[0].type = int_type;
    struct_c->as.struct_type.fields[0].offset = 0;
    struct_c->as.struct_type.fields[0].default_value = NULL;
    struct_c->as.struct_type.fields[0].c_alias = NULL;
    struct_c->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_a");
    struct_c->as.struct_type.fields[1].type = struct_a;  /* C -> A */
    struct_c->as.struct_type.fields[1].offset = 0;
    struct_c->as.struct_type.fields[1].default_value = NULL;
    struct_c->as.struct_type.fields[1].c_alias = NULL;

    /* B references C: B -> C */
    struct_b->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_c");
    struct_b->as.struct_type.fields[1].type = struct_c;
    struct_b->as.struct_type.fields[1].offset = 0;
    struct_b->as.struct_type.fields[1].default_value = NULL;
    struct_b->as.struct_type.fields[1].c_alias = NULL;

    /* A references B: A -> B, completing the cycle A -> B -> C -> A */
    struct_a->as.struct_type.fields[1].name = arena_strdup(&arena, "ref_b");
    struct_a->as.struct_type.fields[1].type = struct_b;
    struct_a->as.struct_type.fields[1].offset = 0;
    struct_a->as.struct_type.fields[1].default_value = NULL;
    struct_a->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on circular references. */

    /* Create struct declarations */
    Stmt *a_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(a_decl, 0, sizeof(Stmt));
    a_decl->type = STMT_STRUCT_DECL;
    a_decl->as.struct_decl.name = a_tok;
    a_decl->as.struct_decl.fields = struct_a->as.struct_type.fields;
    a_decl->as.struct_decl.field_count = 2;
    a_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, a_decl);

    Stmt *b_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(b_decl, 0, sizeof(Stmt));
    b_decl->type = STMT_STRUCT_DECL;
    b_decl->as.struct_decl.name = b_tok;
    b_decl->as.struct_decl.fields = struct_b->as.struct_type.fields;
    b_decl->as.struct_decl.field_count = 2;
    b_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, b_decl);

    Stmt *c_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(c_decl, 0, sizeof(Stmt));
    c_decl->type = STMT_STRUCT_DECL;
    c_decl->as.struct_decl.name = c_tok;
    c_decl->as.struct_decl.fields = struct_c->as.struct_type.fields;
    c_decl->as.struct_decl.field_count = 2;
    c_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, c_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - multi-level circular chain */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_multi_level_circular_chain");
}

/* Test: pointer to self is allowed (breaks cycle) - should pass */
static void test_struct_pointer_breaks_cycle()
{
    DEBUG_INFO("Starting test_struct_pointer_breaks_cycle");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct Node with *Node pointer field (valid - pointer breaks cycle) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "LinkedNode", 1, "test.sn", &arena);

    /* Manually create struct to avoid clone issues */
    Type *node_type = arena_alloc(&arena, sizeof(Type));
    memset(node_type, 0, sizeof(Type));
    node_type->kind = TYPE_STRUCT;
    node_type->as.struct_type.name = arena_strdup(&arena, "LinkedNode");
    node_type->as.struct_type.field_count = 2;
    node_type->as.struct_type.is_native = true;  /* native struct allows pointers */
    node_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    node_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    node_type->as.struct_type.fields[0].type = int_type;
    node_type->as.struct_type.fields[0].offset = 0;
    node_type->as.struct_type.fields[0].default_value = NULL;
    node_type->as.struct_type.fields[0].c_alias = NULL;

    /* Create pointer to the struct - pointer breaks the cycle */
    Type *ptr_node_type = ast_create_pointer_type(&arena, node_type);
    node_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    node_type->as.struct_type.fields[1].type = ptr_node_type;  /* *LinkedNode - pointer breaks cycle */
    node_type->as.struct_type.fields[1].offset = 0;
    node_type->as.struct_type.fields[1].default_value = NULL;
    node_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses even on pointer-based self-references. */

    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = node_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = true;
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - pointer breaks the cycle */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_pointer_breaks_cycle");
}

/* Test: array of self (struct with field of type Foo[]) - should fail */
static void test_struct_array_of_self_circular()
{
    DEBUG_INFO("Starting test_struct_array_of_self_circular");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct with array of self - should be a circular dependency */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "TreeNode", 1, "test.sn", &arena);

    /* Manually create struct to avoid clone issues */
    Type *tree_type = arena_alloc(&arena, sizeof(Type));
    memset(tree_type, 0, sizeof(Type));
    tree_type->kind = TYPE_STRUCT;
    tree_type->as.struct_type.name = arena_strdup(&arena, "TreeNode");
    tree_type->as.struct_type.field_count = 2;
    tree_type->as.struct_type.is_native = false;
    tree_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    tree_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    tree_type->as.struct_type.fields[0].type = int_type;
    tree_type->as.struct_type.fields[0].offset = 0;
    tree_type->as.struct_type.fields[0].default_value = NULL;
    tree_type->as.struct_type.fields[0].c_alias = NULL;

    /* Create array of struct - this is still a circular dependency */
    Type *tree_array_type = ast_create_array_type(&arena, tree_type);
    tree_type->as.struct_type.fields[1].name = arena_strdup(&arena, "children");
    tree_type->as.struct_type.fields[1].type = tree_array_type;
    tree_type->as.struct_type.fields[1].offset = 0;
    tree_type->as.struct_type.fields[1].default_value = NULL;
    tree_type->as.struct_type.fields[1].c_alias = NULL;

    /* NOTE: Do NOT use symbol_table_add_type - it calls ast_clone_type
     * which infinitely recurses on self-referencing structures (even through arrays). */

    Stmt *struct_decl = arena_alloc(&arena, sizeof(Stmt));
    memset(struct_decl, 0, sizeof(Stmt));
    struct_decl->type = STMT_STRUCT_DECL;
    struct_decl->as.struct_decl.name = struct_name_tok;
    struct_decl->as.struct_decl.fields = tree_type->as.struct_type.fields;
    struct_decl->as.struct_decl.field_count = 2;
    struct_decl->as.struct_decl.is_native = false;
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - array of self is still circular */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_array_of_self_circular");
}

/* Test: detect_struct_circular_dependency function directly */
static void test_circular_dependency_detection_direct()
{
    DEBUG_INFO("Starting test_circular_dependency_detection_direct");

    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Test 1: No circular dependency - can use ast_create_struct_type since no self-ref */
    StructField simple_fields[2];
    simple_fields[0].name = "x";
    simple_fields[0].type = int_type;
    simple_fields[0].offset = 0;
    simple_fields[0].default_value = NULL;
    simple_fields[0].c_alias = NULL;
    simple_fields[1].name = "y";
    simple_fields[1].type = int_type;
    simple_fields[1].offset = 0;
    simple_fields[1].default_value = NULL;
    simple_fields[1].c_alias = NULL;

    Type *simple_type = ast_create_struct_type(&arena, "Simple", simple_fields, 2, NULL, 0, false, false, false, NULL);

    char chain[512];
    bool has_cycle = detect_struct_circular_dependency(simple_type, NULL, chain, sizeof(chain));
    assert(!has_cycle);  /* No cycle in simple struct with primitives */

    /* Test 2: Direct circular dependency - manually create to avoid clone infinite recursion */
    Type *self_ref_type = arena_alloc(&arena, sizeof(Type));
    memset(self_ref_type, 0, sizeof(Type));
    self_ref_type->kind = TYPE_STRUCT;
    self_ref_type->as.struct_type.name = arena_strdup(&arena, "SelfRef");
    self_ref_type->as.struct_type.field_count = 2;
    self_ref_type->as.struct_type.is_native = false;
    self_ref_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    self_ref_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    self_ref_type->as.struct_type.fields[0].type = int_type;
    self_ref_type->as.struct_type.fields[0].offset = 0;
    self_ref_type->as.struct_type.fields[0].default_value = NULL;
    self_ref_type->as.struct_type.fields[0].c_alias = NULL;

    self_ref_type->as.struct_type.fields[1].name = arena_strdup(&arena, "self");
    self_ref_type->as.struct_type.fields[1].type = self_ref_type;  /* Self-reference */
    self_ref_type->as.struct_type.fields[1].offset = 0;
    self_ref_type->as.struct_type.fields[1].default_value = NULL;
    self_ref_type->as.struct_type.fields[1].c_alias = NULL;

    has_cycle = detect_struct_circular_dependency(self_ref_type, NULL, chain, sizeof(chain));
    assert(has_cycle);  /* Should detect direct cycle */
    assert(strlen(chain) > 0);  /* Chain should be populated */

    /* Test 3: Pointer should break cycle - manually create to avoid clone issues */
    Type *ptr_struct_type = arena_alloc(&arena, sizeof(Type));
    memset(ptr_struct_type, 0, sizeof(Type));
    ptr_struct_type->kind = TYPE_STRUCT;
    ptr_struct_type->as.struct_type.name = arena_strdup(&arena, "PtrNode");
    ptr_struct_type->as.struct_type.field_count = 2;
    ptr_struct_type->as.struct_type.is_native = true;
    ptr_struct_type->as.struct_type.fields = arena_alloc(&arena, sizeof(StructField) * 2);

    ptr_struct_type->as.struct_type.fields[0].name = arena_strdup(&arena, "value");
    ptr_struct_type->as.struct_type.fields[0].type = int_type;
    ptr_struct_type->as.struct_type.fields[0].offset = 0;
    ptr_struct_type->as.struct_type.fields[0].default_value = NULL;
    ptr_struct_type->as.struct_type.fields[0].c_alias = NULL;

    Type *ptr_to_self = ast_create_pointer_type(&arena, ptr_struct_type);
    ptr_struct_type->as.struct_type.fields[1].name = arena_strdup(&arena, "next");
    ptr_struct_type->as.struct_type.fields[1].type = ptr_to_self;  /* Pointer to self - NOT a cycle */
    ptr_struct_type->as.struct_type.fields[1].offset = 0;
    ptr_struct_type->as.struct_type.fields[1].default_value = NULL;
    ptr_struct_type->as.struct_type.fields[1].c_alias = NULL;

    has_cycle = detect_struct_circular_dependency(ptr_struct_type, NULL, chain, sizeof(chain));
    assert(!has_cycle);  /* Pointer breaks the cycle */

    arena_free(&arena);

    DEBUG_INFO("Finished test_circular_dependency_detection_direct");
}

/* Test: native struct used in native fn context - should pass */
static void test_native_struct_in_native_fn_context()
{
    DEBUG_INFO("Starting test_native_struct_in_native_fn_context");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct with pointer field - give all fields defaults for this test */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default values for fields - nil for pointer, 0 for length */
    Token nil_tok;
    setup_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    LiteralValue nil_val = {0};
    Expr *nil_expr = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    Token len_def_tok;
    setup_token(&len_def_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue len_val;
    len_val.int_value = 0;
    Expr *len_default = ast_create_literal_expr(&arena, len_val, int_type, false, &len_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "data");
    fields[0].type = ptr_byte;
    fields[0].offset = 0;
    fields[0].default_value = nil_expr;  /* has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "length");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = len_default;  /* has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);  /* native struct */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create native function that uses the native struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var buf: Buffer = Buffer {} */
    Token buf_tok;
    setup_token(&buf_tok, TOKEN_IDENTIFIER, "buf", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, buf_tok, struct_type, struct_lit, &buf_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = true;  /* Mark as native function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - native struct in native fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_native_struct_in_native_fn_context");
}

/* Test: native struct used in regular fn context - should fail */
static void test_native_struct_in_regular_fn_error()
{
    DEBUG_INFO("Starting test_native_struct_in_regular_fn_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct with pointer field - give all fields defaults for this test */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default values for fields */
    Token nil_tok;
    setup_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    LiteralValue nil_val = {0};
    Expr *nil_expr = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    Token len_def_tok;
    setup_token(&len_def_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue len_val;
    len_val.int_value = 0;
    Expr *len_default = ast_create_literal_expr(&arena, len_val, int_type, false, &len_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "data");
    fields[0].type = ptr_byte;
    fields[0].offset = 0;
    fields[0].default_value = nil_expr;  /* has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "length");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = len_default;  /* has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);  /* native struct */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create REGULAR function that tries to use the native struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var buf: Buffer = Buffer {} */
    Token buf_tok;
    setup_token(&buf_tok, TOKEN_IDENTIFIER, "buf", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, buf_tok, struct_type, struct_lit, &buf_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;  /* NOT native function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - native struct in regular fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_native_struct_in_regular_fn_error");
}

/* Test: regular struct can be used anywhere - should pass */
static void test_regular_struct_in_regular_fn()
{
    DEBUG_INFO("Starting test_regular_struct_in_regular_fn");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create regular struct (not native) with primitive fields */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default values (0.0) for fields */
    Token x_lit_tok;
    setup_literal_token(&x_lit_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue x_val = {.double_value = 0.0};
    Expr *x_default = ast_create_literal_expr(&arena, x_val, double_type, false, &x_lit_tok);

    Token y_lit_tok;
    setup_literal_token(&y_lit_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_val, double_type, false, &y_lit_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = x_default;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);  /* NOT native */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create regular function that uses the regular struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var p: Point = Point {} */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;  /* Regular function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - regular struct in regular fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_regular_struct_in_regular_fn");
}

/* ============================================================================
 * Struct Layout Calculation Tests
 * ============================================================================
 * These tests verify calculate_struct_layout computes correct field offsets,
 * struct size, and alignment matching C compiler behavior.
 * ============================================================================ */

/* Test: layout for struct with all 8-byte fields (no padding needed) */
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

/* ============================================================================
 * Packed Struct Layout Tests
 * ============================================================================
 * These tests verify that packed structs (is_packed=true) have no padding.
 * ============================================================================ */

/* Test: packed struct with mixed types has no padding */
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

/* ============================================================================
 * Symbol Table Registration Tests
 * ============================================================================
 * These tests verify struct types are properly registered in the symbol table
 * with complete metadata and can be looked up by name.
 * ============================================================================ */

/* Test: struct is registered with SYMBOL_TYPE kind */
static void test_struct_symbol_table_registration()
{
    DEBUG_INFO("Starting test_struct_symbol_table_registration");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);
    fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    /* Create struct type and register it in symbol table */
    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Look up the struct type by name */
    Symbol *symbol = symbol_table_lookup_type(&table, struct_name_tok);

    /* Verify symbol was found */
    assert(symbol != NULL);

    /* Verify symbol kind is SYMBOL_TYPE */
    assert(symbol->kind == SYMBOL_TYPE);

    /* Verify symbol has correct name */
    assert(symbol->name.length == 5);
    assert(strncmp(symbol->name.start, "Point", 5) == 0);

    /* Verify the type is a struct type */
    assert(symbol->type != NULL);
    assert(symbol->type->kind == TYPE_STRUCT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_registration");
}

/* Test: struct metadata is correctly stored (name, fields, field_count, is_native) */
static void test_struct_symbol_table_metadata()
{
    DEBUG_INFO("Starting test_struct_symbol_table_metadata");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Config struct with multiple field types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    StructField fields[3];
    fields[0] = create_test_field(&arena, "timeout", int_type, NULL);
    fields[1] = create_test_field(&arena, "verbose", bool_type, NULL);
    fields[2] = create_test_field(&arena, "name", string_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    /* Create struct type and register it */
    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Look up and verify metadata */
    Symbol *symbol = symbol_table_lookup_type(&table, struct_name_tok);
    assert(symbol != NULL);

    Type *looked_up_type = symbol->type;
    assert(looked_up_type != NULL);
    assert(looked_up_type->kind == TYPE_STRUCT);

    /* Verify struct name */
    assert(strcmp(looked_up_type->as.struct_type.name, "Config") == 0);

    /* Verify field count */
    assert(looked_up_type->as.struct_type.field_count == 3);

    /* Verify is_native flag (should be false for regular struct) */
    assert(looked_up_type->as.struct_type.is_native == false);

    /* Verify fields array */
    assert(looked_up_type->as.struct_type.fields != NULL);

    /* Verify first field */
    assert(strcmp(looked_up_type->as.struct_type.fields[0].name, "timeout") == 0);
    assert(looked_up_type->as.struct_type.fields[0].type != NULL);
    assert(looked_up_type->as.struct_type.fields[0].type->kind == TYPE_INT);

    /* Verify second field */
    assert(strcmp(looked_up_type->as.struct_type.fields[1].name, "verbose") == 0);
    assert(looked_up_type->as.struct_type.fields[1].type != NULL);
    assert(looked_up_type->as.struct_type.fields[1].type->kind == TYPE_BOOL);

    /* Verify third field */
    assert(strcmp(looked_up_type->as.struct_type.fields[2].name, "name") == 0);
    assert(looked_up_type->as.struct_type.fields[2].type != NULL);
    assert(looked_up_type->as.struct_type.fields[2].type->kind == TYPE_STRING);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_metadata");
}

/* Test: native struct metadata includes is_native=true */
static void test_struct_symbol_table_native_metadata()
{
    DEBUG_INFO("Starting test_struct_symbol_table_native_metadata");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create native struct Buffer with pointer field */
    Type *byte_ptr_type = ast_create_pointer_type(&arena, ast_create_primitive_type(&arena, TYPE_BYTE));
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "data", byte_ptr_type, NULL);
    fields[1] = create_test_field(&arena, "size", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    /* Create native struct type */
    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Look up and verify is_native flag */
    Symbol *symbol = symbol_table_lookup_type(&table, struct_name_tok);
    assert(symbol != NULL);
    assert(symbol->type->as.struct_type.is_native == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_native_metadata");
}

/* Test: struct lookup returns correct size and alignment after layout calculation */
static void test_struct_symbol_table_size_alignment()
{
    DEBUG_INFO("Starting test_struct_symbol_table_size_alignment");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create struct: { a: int32, b: byte, c: int }
     * Expected layout with padding:
     * a: offset 0, size 4
     * b: offset 4, size 1, padding 3
     * c: offset 8, size 8
     * Total: 16 bytes, alignment 8 */
    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField fields[3];
    fields[0] = create_test_field(&arena, "a", int32_type, NULL);
    fields[1] = create_test_field(&arena, "b", byte_type, NULL);
    fields[2] = create_test_field(&arena, "c", int_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Padded", 1, "test.sn", &arena);

    /* Create struct type and register it */
    Type *struct_type = ast_create_struct_type(&arena, "Padded", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration and type check to calculate layout */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 3, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    /* Look up and verify size/alignment */
    Symbol *symbol = symbol_table_lookup_type(&table, struct_name_tok);
    assert(symbol != NULL);

    /* After type checking, size and alignment should be set */
    assert(symbol->type->as.struct_type.size == 16);
    assert(symbol->type->as.struct_type.alignment == 8);

    /* Verify field offsets */
    assert(symbol->type->as.struct_type.fields[0].offset == 0);  /* a */
    assert(symbol->type->as.struct_type.fields[1].offset == 4);  /* b */
    assert(symbol->type->as.struct_type.fields[2].offset == 8);  /* c */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_size_alignment");
}

/* Test: struct type can be looked up and used in later declarations */
static void test_struct_symbol_table_lookup_for_later_use()
{
    DEBUG_INFO("Starting test_struct_symbol_table_lookup_for_later_use");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField point_fields[2];
    point_fields[0] = create_test_field(&arena, "x", double_type, NULL);
    point_fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Rectangle struct that references Point */
    /* First look up Point type */
    Symbol *point_symbol = symbol_table_lookup_type(&table, point_tok);
    assert(point_symbol != NULL);
    assert(point_symbol->type->kind == TYPE_STRUCT);

    /* Use the looked-up Point type for Rectangle fields */
    StructField rect_fields[2];
    rect_fields[0] = create_test_field(&arena, "top_left", point_symbol->type, NULL);
    rect_fields[1] = create_test_field(&arena, "bottom_right", point_symbol->type, NULL);

    Token rect_tok;
    setup_token(&rect_tok, TOKEN_IDENTIFIER, "Rectangle", 2, "test.sn", &arena);

    Type *rect_type = ast_create_struct_type(&arena, "Rectangle", rect_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, rect_tok, rect_type);

    Stmt *rect_decl = ast_create_struct_decl_stmt(&arena, rect_tok, rect_fields, 2, NULL, 0, false, false, false, NULL, &rect_tok);
    ast_module_add_statement(&arena, &module, rect_decl);

    /* Type check the module - should pass as Point is properly registered */
    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    /* Verify Rectangle has correct field types */
    Symbol *rect_symbol = symbol_table_lookup_type(&table, rect_tok);
    assert(rect_symbol != NULL);
    assert(rect_symbol->type->as.struct_type.field_count == 2);
    assert(rect_symbol->type->as.struct_type.fields[0].type->kind == TYPE_STRUCT);
    assert(rect_symbol->type->as.struct_type.fields[1].type->kind == TYPE_STRUCT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_lookup_for_later_use");
}

/* Test: looking up non-existent struct returns NULL */
static void test_struct_symbol_table_lookup_not_found()
{
    DEBUG_INFO("Starting test_struct_symbol_table_lookup_not_found");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a lookup token for non-existent struct */
    Token nonexistent_tok;
    setup_token(&nonexistent_tok, TOKEN_IDENTIFIER, "NonExistent", 1, "test.sn", &arena);

    /* Look up should return NULL */
    Symbol *symbol = symbol_table_lookup_type(&table, nonexistent_tok);
    assert(symbol == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_symbol_table_lookup_not_found");
}

/* ============================================================================
 * Struct Literal Field Initialization Tracking Tests
 * ============================================================================
 * These tests verify that struct literal field initialization tracking works:
 * - fields_initialized array is allocated and populated
 * - ast_struct_literal_field_initialized helper function works correctly
 * ============================================================================ */

/* Test: struct literal with all fields initialized */
static void test_struct_literal_all_fields_initialized()
{
    DEBUG_INFO("Starting test_struct_literal_all_fields_initialized");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with both fields: Point { x: 1.0, y: 2.0 } */
    FieldInitializer inits[2];
    Token x_tok, y_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    setup_token(&y_tok, TOKEN_IDENTIFIER, "y", 2, "test.sn", &arena);

    LiteralValue val1, val2;
    val1.double_value = 1.0;
    val2.double_value = 2.0;

    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_tok);
    inits[1].name = y_tok;
    inits[1].value = ast_create_literal_expr(&arena, val2, double_type, false, &y_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 2, &struct_name_tok);

    /* Create a function with var p: Point = Point { x: 1.0, y: 2.0 } */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass */

    /* Verify fields_initialized array is populated */
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* x is initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* y is initialized */

    /* Test helper function */
    assert(ast_struct_literal_field_initialized(struct_lit, 0) == true);
    assert(ast_struct_literal_field_initialized(struct_lit, 1) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_all_fields_initialized");
}

/* Test: struct literal with partial field initialization tracking (before required field check)
 * Note: This test has all fields with defaults, so partial init is allowed */
static void test_struct_literal_partial_initialization()
{
    DEBUG_INFO("Starting test_struct_literal_partial_initialization");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with all fields having defaults - so partial init is OK */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Create default values */
    Token retries_def_tok, verbose_def_tok;
    setup_token(&retries_def_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    setup_token(&verbose_def_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue retries_def_val, verbose_def_val;
    retries_def_val.int_value = 3;
    verbose_def_val.bool_value = false;
    Expr *retries_default = ast_create_literal_expr(&arena, retries_def_val, int_type, false, &retries_def_tok);
    Expr *verbose_default = ast_create_literal_expr(&arena, verbose_def_val, bool_type, false, &verbose_def_tok);

    StructField fields[3];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* Required field */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "retries");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = retries_default;  /* Optional - has default */
    fields[1].c_alias = NULL;
    fields[2].name = arena_strdup(&arena, "verbose");
    fields[2].type = bool_type;
    fields[2].offset = 0;
    fields[2].default_value = verbose_default;  /* Optional - has default */
    fields[2].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 3, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only one required field: Config { timeout: 60 } */
    FieldInitializer inits[1];
    Token timeout_tok;
    setup_token(&timeout_tok, TOKEN_IDENTIFIER, "timeout", 2, "test.sn", &arena);

    LiteralValue val;
    val.int_value = 60;

    inits[0].name = timeout_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, int_type, false, &timeout_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function with var c: Config = Config { timeout: 60 } */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - required field provided, others have defaults */

    /* Verify all fields are initialized (explicit + defaults applied) */
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 3);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);   /* timeout explicitly */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);   /* retries via default */
    assert(struct_lit->as.struct_literal.fields_initialized[2] == true);   /* verbose via default */

    /* Verify field_count includes defaults */
    assert(struct_lit->as.struct_literal.field_count == 3);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_partial_initialization");
}

/* Test: struct literal with empty initialization - struct with all defaults should pass */
static void test_struct_literal_empty_initialization()
{
    DEBUG_INFO("Starting test_struct_literal_empty_initialization");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double = 0.0, y: double = 0.0 (all defaults) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value expressions */
    Token x_def_tok, y_def_tok;
    setup_token(&x_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    setup_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue x_def_val, y_def_val;
    x_def_val.double_value = 0.0;
    y_def_val.double_value = 0.0;
    Expr *x_default = ast_create_literal_expr(&arena, x_def_val, double_type, false, &x_def_tok);
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = x_default;  /* Has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;  /* Has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with no fields: Point {} - should pass since all have defaults */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Create a function with var p: Point = Point {} */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all fields have defaults */

    /* Verify defaults were applied - field_count should now be 2 */
    assert(struct_lit->as.struct_literal.field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* x via default */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* y via default */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_empty_initialization");
}

/* Test: helper function returns false for invalid/edge cases */
static void test_struct_literal_field_init_helper_edge_cases()
{
    DEBUG_INFO("Starting test_struct_literal_field_init_helper_edge_cases");

    Arena arena;
    arena_init(&arena, 4096);

    /* Test NULL expression */
    assert(ast_struct_literal_field_initialized(NULL, 0) == false);

    /* Test non-struct-literal expression */
    LiteralValue val;
    val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok;
    setup_token(&tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *int_lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    assert(ast_struct_literal_field_initialized(int_lit, 0) == false);

    /* Test struct literal before type checking (fields_initialized is NULL) */
    Token struct_tok;
    setup_token(&struct_tok, TOKEN_IDENTIFIER, "TestStruct", 1, "test.sn", &arena);
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_tok, NULL, 0, &struct_tok);
    /* Before type checking, fields_initialized should be NULL */
    assert(struct_lit->as.struct_literal.fields_initialized == NULL);
    assert(ast_struct_literal_field_initialized(struct_lit, 0) == false);

    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_field_init_helper_edge_cases");
}

/* Test: helper function with invalid field index */
static void test_struct_literal_field_init_invalid_index()
{
    DEBUG_INFO("Starting test_struct_literal_field_init_invalid_index");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double = 0.0 (y has a default) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value for y */
    Token y_def_tok;
    setup_literal_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_def_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* x is required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;  /* y has a default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal: Point { x: 1.0 } - y gets default value */
    FieldInitializer inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);

    LiteralValue val;
    val.double_value = 1.0;

    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, double_type, false, &x_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    /* Test invalid indices return false */
    assert(ast_struct_literal_field_initialized(struct_lit, -1) == false);  /* Negative index */
    assert(ast_struct_literal_field_initialized(struct_lit, 2) == false);   /* Index out of bounds */
    assert(ast_struct_literal_field_initialized(struct_lit, 100) == false); /* Way out of bounds */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_field_init_invalid_index");
}

/* ============================================================================
 * Default Value Application Tests
 * ============================================================================
 * These tests verify that default values are applied to uninitialized fields:
 * - Synthetic field initializers are created for fields with defaults
 * - Default values are type-checked
 * - The struct literal's field_count is updated to include defaults
 * ============================================================================ */

/* Test: default value is applied when field is not explicitly initialized */
static void test_struct_default_value_applied()
{
    DEBUG_INFO("Starting test_struct_default_value_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with:
     * - timeout: int = 60 (has default)
     * - retries: int (no default)
     */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default value expression: 60 */
    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = default_expr;  /* Has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "retries");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal that only specifies retries: Config { retries: 3 } */
    FieldInitializer inits[1];
    Token retries_tok;
    setup_token(&retries_tok, TOKEN_IDENTIFIER, "retries", 2, "test.sn", &arena);

    LiteralValue val;
    val.int_value = 3;
    inits[0].name = retries_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, int_type, false, &retries_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Verify initial field count is 1 */
    assert(struct_lit->as.struct_literal.field_count == 1);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass */

    /* Verify that field_count is now 2 (1 explicit + 1 default) */
    assert(struct_lit->as.struct_literal.field_count == 2);

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* timeout via default */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* retries via explicit */

    /* Verify the synthetic initializer was added for timeout */
    bool found_timeout = false;
    for (int i = 0; i < struct_lit->as.struct_literal.field_count; i++)
    {
        if (strncmp(struct_lit->as.struct_literal.fields[i].name.start, "timeout", 7) == 0)
        {
            found_timeout = true;
            /* Verify the value is the default expression */
            assert(struct_lit->as.struct_literal.fields[i].value == default_expr);
            break;
        }
    }
    assert(found_timeout);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_default_value_applied");
}

/* Test: multiple default values are applied */
static void test_struct_multiple_defaults_applied()
{
    DEBUG_INFO("Starting test_struct_multiple_defaults_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with all fields having defaults:
     * - host: str = "localhost"
     * - port: int = 8080
     * - debug: bool = false
     */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Create default value expressions */
    Token host_default_tok, port_default_tok, debug_default_tok;
    setup_token(&host_default_tok, TOKEN_STRING_LITERAL, "localhost", 1, "test.sn", &arena);
    setup_token(&port_default_tok, TOKEN_INT_LITERAL, "8080", 1, "test.sn", &arena);
    setup_token(&debug_default_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);

    LiteralValue host_val, port_val, debug_val;
    host_val.string_value = "localhost";
    port_val.int_value = 8080;
    debug_val.bool_value = false;

    Expr *host_default = ast_create_literal_expr(&arena, host_val, str_type, false, &host_default_tok);
    Expr *port_default = ast_create_literal_expr(&arena, port_val, int_type, false, &port_default_tok);
    Expr *debug_default = ast_create_literal_expr(&arena, debug_val, bool_type, false, &debug_default_tok);

    StructField fields[3];
    fields[0].name = arena_strdup(&arena, "host");
    fields[0].type = str_type;
    fields[0].offset = 0;
    fields[0].default_value = host_default;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "port");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = port_default;
    fields[1].c_alias = NULL;
    fields[2].name = arena_strdup(&arena, "debug");
    fields[2].type = bool_type;
    fields[2].offset = 0;
    fields[2].default_value = debug_default;
    fields[2].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "ServerConfig", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "ServerConfig", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 3, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create empty struct literal: ServerConfig {} */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Verify initial field count is 0 */
    assert(struct_lit->as.struct_literal.field_count == 0);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "cfg", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass */

    /* Verify that field_count is now 3 (all 3 defaults applied) */
    assert(struct_lit->as.struct_literal.field_count == 3);

    /* Verify all fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* host */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* port */
    assert(struct_lit->as.struct_literal.fields_initialized[2] == true);  /* debug */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_multiple_defaults_applied");
}

/* Test: explicit value overrides default value */
static void test_struct_explicit_overrides_default()
{
    DEBUG_INFO("Starting test_struct_explicit_overrides_default");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with timeout: int = 60 */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[1];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = default_expr;
    fields[0].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal that explicitly sets timeout: Config { timeout: 120 } */
    FieldInitializer inits[1];
    Token timeout_tok;
    setup_token(&timeout_tok, TOKEN_IDENTIFIER, "timeout", 2, "test.sn", &arena);

    LiteralValue explicit_val;
    explicit_val.int_value = 120;
    Expr *explicit_expr = ast_create_literal_expr(&arena, explicit_val, int_type, false, &timeout_tok);

    inits[0].name = timeout_tok;
    inits[0].value = explicit_expr;

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass */

    /* Verify field_count is still 1 (explicit value used, default not added) */
    assert(struct_lit->as.struct_literal.field_count == 1);

    /* Verify the explicit value is used, not the default */
    assert(struct_lit->as.struct_literal.fields[0].value == explicit_expr);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_explicit_overrides_default");
}

/* Test: missing required fields causes error */
static void test_struct_missing_required_fields_error()
{
    DEBUG_INFO("Starting test_struct_missing_required_fields_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create empty struct literal: Point {} - should fail because x and y are required */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - required fields x and y not initialized */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_missing_required_fields_error");
}

/* Test: missing single required field causes error */
static void test_struct_missing_one_required_field_error()
{
    DEBUG_INFO("Starting test_struct_missing_one_required_field_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only x: Point { x: 1.0 } - missing y */
    FieldInitializer inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);

    LiteralValue val;
    val.double_value = 1.0;
    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, double_type, false, &x_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - required field y not initialized */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_missing_one_required_field_error");
}

/* Test: all fields provided for struct with required fields - should pass */
static void test_struct_all_required_fields_provided()
{
    DEBUG_INFO("Starting test_struct_all_required_fields_provided");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with both fields: Point { x: 1.0, y: 2.0 } */
    FieldInitializer inits[2];
    Token x_tok, y_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    setup_token(&y_tok, TOKEN_IDENTIFIER, "y", 2, "test.sn", &arena);

    LiteralValue val1, val2;
    val1.double_value = 1.0;
    val2.double_value = 2.0;
    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_tok);
    inits[1].name = y_tok;
    inits[1].value = ast_create_literal_expr(&arena, val2, double_type, false, &y_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 2, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all required fields are initialized */

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_all_required_fields_provided");
}

/* Test: optional fields (with defaults) don't need to be provided */
static void test_struct_optional_fields_not_required()
{
    DEBUG_INFO("Starting test_struct_optional_fields_not_required");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with a mix of required and optional fields */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default value for timeout */
    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "port");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "timeout");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = default_expr;  /* Has default - optional */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only required field: Config { port: 8080 } */
    FieldInitializer inits[1];
    Token port_tok;
    setup_token(&port_tok, TOKEN_IDENTIFIER, "port", 2, "test.sn", &arena);

    LiteralValue port_val;
    port_val.int_value = 8080;
    inits[0].name = port_tok;
    inits[0].value = ast_create_literal_expr(&arena, port_val, int_type, false, &port_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - required field provided, optional has default */

    /* Verify field_count is 2 (required + default applied) */
    assert(struct_lit->as.struct_literal.field_count == 2);

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_optional_fields_not_required");
}

/* ============================================================================
 * Nested Struct Initialization Tests
 * ============================================================================
 * These tests verify that nested struct literals are properly validated:
 * - Inner struct literals are type-checked recursively
 * - Inner struct defaults are applied correctly
 * - Inner struct required fields are enforced
 * ============================================================================ */

/* Test: 2-level nesting with all required fields provided */
static void test_nested_struct_all_fields_provided()
{
    DEBUG_INFO("Starting test_nested_struct_all_fields_provided");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = NULL;
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Rect struct: struct Rect => origin: Point, size: Point */
    StructField rect_fields[2];
    rect_fields[0].name = arena_strdup(&arena, "origin");
    rect_fields[0].type = point_type;
    rect_fields[0].offset = 0;
    rect_fields[0].default_value = NULL;
    rect_fields[0].c_alias = NULL;
    rect_fields[1].name = arena_strdup(&arena, "size");
    rect_fields[1].type = point_type;
    rect_fields[1].offset = 16;
    rect_fields[1].default_value = NULL;
    rect_fields[1].c_alias = NULL;

    Token rect_tok;
    setup_token(&rect_tok, TOKEN_IDENTIFIER, "Rect", 2, "test.sn", &arena);

    Type *rect_type = ast_create_struct_type(&arena, "Rect", rect_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, rect_tok, rect_type);

    Stmt *rect_decl = ast_create_struct_decl_stmt(&arena, rect_tok, rect_fields, 2, NULL, 0, false, false, false, NULL, &rect_tok);
    ast_module_add_statement(&arena, &module, rect_decl);

    /* Create nested struct literal: Rect { origin: Point { x: 0.0, y: 0.0 }, size: Point { x: 100.0, y: 50.0 } } */

    /* Inner Point literal for origin: Point { x: 0.0, y: 0.0 } */
    FieldInitializer origin_point_inits[2];
    Token x_tok, y_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    setup_token(&y_tok, TOKEN_IDENTIFIER, "y", 3, "test.sn", &arena);

    LiteralValue zero_val = {.double_value = 0.0};
    origin_point_inits[0].name = x_tok;
    origin_point_inits[0].value = ast_create_literal_expr(&arena, zero_val, double_type, false, &x_tok);
    origin_point_inits[1].name = y_tok;
    origin_point_inits[1].value = ast_create_literal_expr(&arena, zero_val, double_type, false, &y_tok);

    Expr *origin_lit = ast_create_struct_literal_expr(&arena, point_tok, origin_point_inits, 2, &point_tok);

    /* Inner Point literal for size: Point { x: 100.0, y: 50.0 } */
    FieldInitializer size_point_inits[2];
    Token x2_tok, y2_tok;
    setup_token(&x2_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    setup_token(&y2_tok, TOKEN_IDENTIFIER, "y", 3, "test.sn", &arena);

    LiteralValue width_val = {.double_value = 100.0};
    LiteralValue height_val = {.double_value = 50.0};
    size_point_inits[0].name = x2_tok;
    size_point_inits[0].value = ast_create_literal_expr(&arena, width_val, double_type, false, &x2_tok);
    size_point_inits[1].name = y2_tok;
    size_point_inits[1].value = ast_create_literal_expr(&arena, height_val, double_type, false, &y2_tok);

    Expr *size_lit = ast_create_struct_literal_expr(&arena, point_tok, size_point_inits, 2, &point_tok);

    /* Outer Rect literal: Rect { origin: Point {...}, size: Point {...} } */
    FieldInitializer rect_inits[2];
    Token origin_tok, size_tok;
    setup_token(&origin_tok, TOKEN_IDENTIFIER, "origin", 3, "test.sn", &arena);
    setup_token(&size_tok, TOKEN_IDENTIFIER, "size", 3, "test.sn", &arena);

    rect_inits[0].name = origin_tok;
    rect_inits[0].value = origin_lit;
    rect_inits[1].name = size_tok;
    rect_inits[1].value = size_lit;

    Expr *rect_lit = ast_create_struct_literal_expr(&arena, rect_tok, rect_inits, 2, &rect_tok);

    /* Create function to trigger type checking */
    Token fn_tok, r_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, r_tok, rect_type, rect_lit, &r_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all fields provided at all levels */

    /* Verify outer struct fields are initialized */
    assert(rect_lit->as.struct_literal.fields_initialized[0] == true);
    assert(rect_lit->as.struct_literal.fields_initialized[1] == true);

    /* Verify inner struct fields are initialized */
    assert(origin_lit->as.struct_literal.fields_initialized[0] == true);
    assert(origin_lit->as.struct_literal.fields_initialized[1] == true);
    assert(size_lit->as.struct_literal.fields_initialized[0] == true);
    assert(size_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_all_fields_provided");
}

/* Test: nested struct with missing required field in inner struct */
static void test_nested_struct_inner_missing_required()
{
    DEBUG_INFO("Starting test_nested_struct_inner_missing_required");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double (both required) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = NULL;
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Wrapper struct: struct Wrapper => pt: Point */
    StructField wrapper_fields[1];
    wrapper_fields[0].name = arena_strdup(&arena, "pt");
    wrapper_fields[0].type = point_type;
    wrapper_fields[0].offset = 0;
    wrapper_fields[0].default_value = NULL;
    wrapper_fields[0].c_alias = NULL;

    Token wrapper_tok;
    setup_token(&wrapper_tok, TOKEN_IDENTIFIER, "Wrapper", 2, "test.sn", &arena);

    Type *wrapper_type = ast_create_struct_type(&arena, "Wrapper", wrapper_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, wrapper_tok, wrapper_type);

    Stmt *wrapper_decl = ast_create_struct_decl_stmt(&arena, wrapper_tok, wrapper_fields, 1, NULL, 0, false, false, false, NULL, &wrapper_tok);
    ast_module_add_statement(&arena, &module, wrapper_decl);

    /* Create nested literal: Wrapper { pt: Point { x: 1.0 } } - MISSING y! */
    FieldInitializer point_inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);

    LiteralValue x_val = {.double_value = 1.0};
    point_inits[0].name = x_tok;
    point_inits[0].value = ast_create_literal_expr(&arena, x_val, double_type, false, &x_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, point_tok, point_inits, 1, &point_tok);

    /* Outer Wrapper literal */
    FieldInitializer wrapper_inits[1];
    Token pt_tok;
    setup_token(&pt_tok, TOKEN_IDENTIFIER, "pt", 3, "test.sn", &arena);

    wrapper_inits[0].name = pt_tok;
    wrapper_inits[0].value = point_lit;

    Expr *wrapper_lit = ast_create_struct_literal_expr(&arena, wrapper_tok, wrapper_inits, 1, &wrapper_tok);

    /* Create function to trigger type checking */
    Token fn_tok, w_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&w_tok, TOKEN_IDENTIFIER, "w", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, w_tok, wrapper_type, wrapper_lit, &w_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - inner Point is missing required field 'y' */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_inner_missing_required");
}

/* Test: nested struct with defaults applied at inner level */
static void test_nested_struct_inner_defaults_applied()
{
    DEBUG_INFO("Starting test_nested_struct_inner_defaults_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double = 0.0 */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value for y */
    Token y_def_tok;
    setup_literal_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_def_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;  /* x is required */
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = y_default;  /* y has default */
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Wrapper struct: struct Wrapper => pt: Point */
    StructField wrapper_fields[1];
    wrapper_fields[0].name = arena_strdup(&arena, "pt");
    wrapper_fields[0].type = point_type;
    wrapper_fields[0].offset = 0;
    wrapper_fields[0].default_value = NULL;
    wrapper_fields[0].c_alias = NULL;

    Token wrapper_tok;
    setup_token(&wrapper_tok, TOKEN_IDENTIFIER, "Wrapper", 2, "test.sn", &arena);

    Type *wrapper_type = ast_create_struct_type(&arena, "Wrapper", wrapper_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, wrapper_tok, wrapper_type);

    Stmt *wrapper_decl = ast_create_struct_decl_stmt(&arena, wrapper_tok, wrapper_fields, 1, NULL, 0, false, false, false, NULL, &wrapper_tok);
    ast_module_add_statement(&arena, &module, wrapper_decl);

    /* Create nested literal: Wrapper { pt: Point { x: 5.0 } } - y gets default */
    FieldInitializer point_inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);

    LiteralValue x_val = {.double_value = 5.0};
    point_inits[0].name = x_tok;
    point_inits[0].value = ast_create_literal_expr(&arena, x_val, double_type, false, &x_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, point_tok, point_inits, 1, &point_tok);

    /* Outer Wrapper literal */
    FieldInitializer wrapper_inits[1];
    Token pt_tok;
    setup_token(&pt_tok, TOKEN_IDENTIFIER, "pt", 3, "test.sn", &arena);

    wrapper_inits[0].name = pt_tok;
    wrapper_inits[0].value = point_lit;

    Expr *wrapper_lit = ast_create_struct_literal_expr(&arena, wrapper_tok, wrapper_inits, 1, &wrapper_tok);

    /* Create function to trigger type checking */
    Token fn_tok, w_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&w_tok, TOKEN_IDENTIFIER, "w", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, w_tok, wrapper_type, wrapper_lit, &w_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - y gets its default value */

    /* Verify outer Wrapper is fully initialized */
    assert(wrapper_lit->as.struct_literal.fields_initialized[0] == true);

    /* Verify inner Point: x was explicit, y got default applied */
    assert(point_lit->as.struct_literal.fields_initialized[0] == true);  /* x */
    assert(point_lit->as.struct_literal.fields_initialized[1] == true);  /* y (from default) */

    /* Verify field_count was updated to include the default */
    assert(point_lit->as.struct_literal.field_count == 2);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_inner_defaults_applied");
}

/* Test: 3-level nesting to ensure deep recursion works */
static void test_nested_struct_three_levels()
{
    DEBUG_INFO("Starting test_nested_struct_three_levels");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level 1: struct Inner => val: int = 42 */
    Token inner_def_tok;
    setup_literal_token(&inner_def_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue inner_def_val = {.int_value = 42};
    Expr *inner_default = ast_create_literal_expr(&arena, inner_def_val, int_type, false, &inner_def_tok);

    StructField inner_fields[1];
    inner_fields[0].name = arena_strdup(&arena, "val");
    inner_fields[0].type = int_type;
    inner_fields[0].offset = 0;
    inner_fields[0].default_value = inner_default;
    inner_fields[0].c_alias = NULL;

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Level 2: struct Middle => inner: Inner */
    StructField middle_fields[1];
    middle_fields[0].name = arena_strdup(&arena, "inner");
    middle_fields[0].type = inner_type;
    middle_fields[0].offset = 0;
    middle_fields[0].default_value = NULL;
    middle_fields[0].c_alias = NULL;

    Token middle_tok;
    setup_token(&middle_tok, TOKEN_IDENTIFIER, "Middle", 2, "test.sn", &arena);

    Type *middle_type = ast_create_struct_type(&arena, "Middle", middle_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, middle_tok, middle_type);

    Stmt *middle_decl = ast_create_struct_decl_stmt(&arena, middle_tok, middle_fields, 1, NULL, 0, false, false, false, NULL, &middle_tok);
    ast_module_add_statement(&arena, &module, middle_decl);

    /* Level 3: struct Outer => mid: Middle */
    StructField outer_fields[1];
    outer_fields[0].name = arena_strdup(&arena, "mid");
    outer_fields[0].type = middle_type;
    outer_fields[0].offset = 0;
    outer_fields[0].default_value = NULL;
    outer_fields[0].c_alias = NULL;

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 3, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create 3-level nested literal: Outer { mid: Middle { inner: Inner {} } }
     * Inner {} uses default for 'val' */

    /* Innermost: Inner {} */
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, NULL, 0, &inner_tok);

    /* Middle: Middle { inner: Inner {} } */
    FieldInitializer middle_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);

    middle_inits[0].name = inner_field_tok;
    middle_inits[0].value = inner_lit;

    Expr *middle_lit = ast_create_struct_literal_expr(&arena, middle_tok, middle_inits, 1, &middle_tok);

    /* Outermost: Outer { mid: Middle {...} } */
    FieldInitializer outer_inits[1];
    Token mid_field_tok;
    setup_token(&mid_field_tok, TOKEN_IDENTIFIER, "mid", 4, "test.sn", &arena);

    outer_inits[0].name = mid_field_tok;
    outer_inits[0].value = middle_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* Create function to trigger type checking */
    Token fn_tok, o_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 6, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - Inner.val gets default 42 */

    /* Verify all levels are initialized */
    assert(outer_lit->as.struct_literal.fields_initialized[0] == true);
    assert(middle_lit->as.struct_literal.fields_initialized[0] == true);
    assert(inner_lit->as.struct_literal.fields_initialized[0] == true);  /* val got default */

    /* Verify Inner's field_count was updated to include default */
    assert(inner_lit->as.struct_literal.field_count == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_three_levels");
}

/* Test: 3-level nesting with missing required field at deepest level */
static void test_nested_struct_three_levels_missing_required()
{
    DEBUG_INFO("Starting test_nested_struct_three_levels_missing_required");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level 1: struct Inner => val: int (REQUIRED - no default) */
    StructField inner_fields[1];
    inner_fields[0].name = arena_strdup(&arena, "val");
    inner_fields[0].type = int_type;
    inner_fields[0].offset = 0;
    inner_fields[0].default_value = NULL;  /* Required! */
    inner_fields[0].c_alias = NULL;

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Level 2: struct Middle => inner: Inner */
    StructField middle_fields[1];
    middle_fields[0].name = arena_strdup(&arena, "inner");
    middle_fields[0].type = inner_type;
    middle_fields[0].offset = 0;
    middle_fields[0].default_value = NULL;
    middle_fields[0].c_alias = NULL;

    Token middle_tok;
    setup_token(&middle_tok, TOKEN_IDENTIFIER, "Middle", 2, "test.sn", &arena);

    Type *middle_type = ast_create_struct_type(&arena, "Middle", middle_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, middle_tok, middle_type);

    Stmt *middle_decl = ast_create_struct_decl_stmt(&arena, middle_tok, middle_fields, 1, NULL, 0, false, false, false, NULL, &middle_tok);
    ast_module_add_statement(&arena, &module, middle_decl);

    /* Level 3: struct Outer => mid: Middle */
    StructField outer_fields[1];
    outer_fields[0].name = arena_strdup(&arena, "mid");
    outer_fields[0].type = middle_type;
    outer_fields[0].offset = 0;
    outer_fields[0].default_value = NULL;
    outer_fields[0].c_alias = NULL;

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 3, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create 3-level nested literal: Outer { mid: Middle { inner: Inner {} } }
     * Inner {} is MISSING required field 'val'! */

    /* Innermost: Inner {} - MISSING 'val' */
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, NULL, 0, &inner_tok);

    /* Middle: Middle { inner: Inner {} } */
    FieldInitializer middle_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);

    middle_inits[0].name = inner_field_tok;
    middle_inits[0].value = inner_lit;

    Expr *middle_lit = ast_create_struct_literal_expr(&arena, middle_tok, middle_inits, 1, &middle_tok);

    /* Outermost: Outer { mid: Middle {...} } */
    FieldInitializer outer_inits[1];
    Token mid_field_tok;
    setup_token(&mid_field_tok, TOKEN_IDENTIFIER, "mid", 4, "test.sn", &arena);

    outer_inits[0].name = mid_field_tok;
    outer_inits[0].value = middle_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* Create function to trigger type checking */
    Token fn_tok, o_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 6, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - Inner.val is missing */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_three_levels_missing_required");
}

/* Test: scope depth is correctly set during member access type checking */
static void test_member_access_scope_depth_propagation()
{
    DEBUG_INFO("Starting test_member_access_scope_depth_propagation");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);
    fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create function containing: var p: Point = Point { x: 1.0, y: 2.0 }; var v: double = p.x */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Point literal: Point { x: 1.0, y: 2.0 } */
    FieldInitializer inits[2];
    Token x_field_tok, y_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);
    setup_token(&y_field_tok, TOKEN_IDENTIFIER, "y", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    LiteralValue val2 = {.double_value = 2.0};
    inits[0].name = x_field_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);
    inits[1].name = y_field_tok;
    inits[1].value = ast_create_literal_expr(&arena, val2, double_type, false, &y_field_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 2, &struct_name_tok);

    /* var p: Point = Point { ... } */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 7, "test.sn", &arena);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, point_lit, &p_tok);

    /* Create member access: p.x */
    Expr *p_var = ast_create_variable_expr(&arena, p_tok, &p_tok);
    Expr *member_access = ast_create_member_access_expr(&arena, p_var, x_field_tok, &x_field_tok);

    /* Verify initial state before type checking */
    assert(member_access->as.member_access.scope_depth == 0);
    assert(member_access->as.member_access.escaped == false);
    assert(member_access->as.member_access.field_index == -1);

    /* var v: double = p.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 8, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, member_access, &v_tok);

    /* Create function body with both declarations */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, member access should have:
     * - field_index set (found the field)
     * - scope_depth set to the function body scope depth (typically 2: global=1, function=2) */
    assert(member_access->as.member_access.field_index == 0);  /* x is first field */
    assert(member_access->as.member_access.scope_depth >= 1);  /* Inside a scope */
    DEBUG_INFO("Member access scope_depth after type checking: %d", member_access->as.member_access.scope_depth);

    /* Verify the member access inherits proper context */
    assert(member_access->as.member_access.escaped == false);  /* Not escaping in this case */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_scope_depth_propagation");
}

/* Test: scope depth is correctly set for nested scope member access */
static void test_member_access_nested_scope_depth()
{
    DEBUG_INFO("Starting test_member_access_nested_scope_depth");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create function with nested block: fn test_fn() { { var p: Point = ...; var v = p.x } } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Point literal: Point { x: 1.0 } */
    FieldInitializer inits[1];
    Token x_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    inits[0].name = x_field_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* var p: Point = Point { ... } */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 7, "test.sn", &arena);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, point_lit, &p_tok);

    /* Create member access: p.x */
    Expr *p_var = ast_create_variable_expr(&arena, p_tok, &p_tok);
    Expr *member_access = ast_create_member_access_expr(&arena, p_var, x_field_tok, &x_field_tok);

    /* var v: double = p.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 8, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, member_access, &v_tok);

    /* Create inner block { var p; var v = p.x } */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = p_decl;
    inner_body[1] = v_decl;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 6, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body with the block */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    fn_body[0] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, member access inside nested block should have deeper scope_depth */
    assert(member_access->as.member_access.field_index == 0);  /* x is first field */
    assert(member_access->as.member_access.scope_depth >= 2);  /* Inside function + block */
    DEBUG_INFO("Member access in nested block scope_depth: %d", member_access->as.member_access.scope_depth);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_nested_scope_depth");
}

/* Test: scope depth propagates through nested field access chains (outer.inner.x) */
static void test_member_access_chain_scope_depth()
{
    DEBUG_INFO("Starting test_member_access_chain_scope_depth");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with x: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "x", double_type, NULL);

    Token inner_struct_tok;
    setup_token(&inner_struct_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_struct_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_struct_tok, inner_struct_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_struct_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_struct_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_struct_type, NULL);

    Token outer_struct_tok;
    setup_token(&outer_struct_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_struct_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_struct_tok, outer_struct_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_struct_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_struct_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function: fn test_fn() { var o: Outer = ...; var v = o.inner.x } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { x: 1.0 } */
    FieldInitializer inner_inits[1];
    Token x_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    inner_inits[0].name = x_field_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_struct_tok, inner_inits, 1, &inner_struct_tok);

    /* Create Outer literal: Outer { inner: Inner { x: 1.0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_struct_tok, outer_inits, 1, &outer_struct_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_struct_type, outer_lit, &o_tok);

    /* Create nested member access: o.inner (first level) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);
    Expr *o_inner = ast_create_member_access_expr(&arena, o_var, inner_field_tok, &inner_field_tok);

    /* Create nested member access: o.inner.x (second level) */
    Expr *o_inner_x = ast_create_member_access_expr(&arena, o_inner, x_field_tok, &x_field_tok);

    /* var v: double = o.inner.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 9, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, o_inner_x, &v_tok);

    /* Create function body */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = o_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking:
     * - o.inner should have scope_depth from 'o' (declaration scope depth)
     * - o.inner.x should also have scope_depth from 'o' (propagated through chain) */
    assert(o_inner->as.member_access.field_index == 0);  /* inner is first field of Outer */
    assert(o_inner_x->as.member_access.field_index == 0);  /* x is first field of Inner */

    /* Both should have the same scope depth (from base variable 'o') */
    assert(o_inner->as.member_access.scope_depth == o_inner_x->as.member_access.scope_depth);
    DEBUG_INFO("o.inner scope_depth: %d, o.inner.x scope_depth: %d",
               o_inner->as.member_access.scope_depth, o_inner_x->as.member_access.scope_depth);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_chain_scope_depth");
}

/* Test: scope depth propagates through three-level nested chains (a.b.c.d) */
static void test_member_access_chain_three_levels()
{
    DEBUG_INFO("Starting test_member_access_chain_three_levels");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Level3 struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField level3_fields[1];
    level3_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token level3_tok;
    setup_token(&level3_tok, TOKEN_IDENTIFIER, "Level3", 1, "test.sn", &arena);

    Type *level3_type = ast_create_struct_type(&arena, "Level3", level3_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level3_tok, level3_type);

    Stmt *level3_decl = ast_create_struct_decl_stmt(&arena, level3_tok, level3_fields, 1, NULL, 0, false, false, false, NULL, &level3_tok);
    ast_module_add_statement(&arena, &module, level3_decl);

    /* Create Level2 struct with c: Level3 */
    StructField level2_fields[1];
    level2_fields[0] = create_test_field(&arena, "c", level3_type, NULL);

    Token level2_tok;
    setup_token(&level2_tok, TOKEN_IDENTIFIER, "Level2", 2, "test.sn", &arena);

    Type *level2_type = ast_create_struct_type(&arena, "Level2", level2_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level2_tok, level2_type);

    Stmt *level2_decl = ast_create_struct_decl_stmt(&arena, level2_tok, level2_fields, 1, NULL, 0, false, false, false, NULL, &level2_tok);
    ast_module_add_statement(&arena, &module, level2_decl);

    /* Create Level1 struct with b: Level2 */
    StructField level1_fields[1];
    level1_fields[0] = create_test_field(&arena, "b", level2_type, NULL);

    Token level1_tok;
    setup_token(&level1_tok, TOKEN_IDENTIFIER, "Level1", 3, "test.sn", &arena);

    Type *level1_type = ast_create_struct_type(&arena, "Level1", level1_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level1_tok, level1_type);

    Stmt *level1_decl = ast_create_struct_decl_stmt(&arena, level1_tok, level1_fields, 1, NULL, 0, false, false, false, NULL, &level1_tok);
    ast_module_add_statement(&arena, &module, level1_decl);

    /* Create function: fn test_fn() { var a: Level1 = ...; var v = a.b.c.val } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Level3 literal: Level3 { val: 42 } */
    FieldInitializer level3_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_42 = {.int_value = 42};
    level3_inits[0].name = val_tok;
    level3_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *level3_lit = ast_create_struct_literal_expr(&arena, level3_tok, level3_inits, 1, &level3_tok);

    /* Create Level2 literal: Level2 { c: Level3 { ... } } */
    FieldInitializer level2_inits[1];
    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 7, "test.sn", &arena);

    level2_inits[0].name = c_tok;
    level2_inits[0].value = level3_lit;

    Expr *level2_lit = ast_create_struct_literal_expr(&arena, level2_tok, level2_inits, 1, &level2_tok);

    /* Create Level1 literal: Level1 { b: Level2 { ... } } */
    FieldInitializer level1_inits[1];
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "b", 8, "test.sn", &arena);

    level1_inits[0].name = b_tok;
    level1_inits[0].value = level2_lit;

    Expr *level1_lit = ast_create_struct_literal_expr(&arena, level1_tok, level1_inits, 1, &level1_tok);

    /* var a: Level1 = Level1 { ... } */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "a", 9, "test.sn", &arena);
    Stmt *a_decl = ast_create_var_decl_stmt(&arena, a_tok, level1_type, level1_lit, &a_tok);

    /* Create nested member accesses:
     * a.b      -> first level
     * a.b.c    -> second level
     * a.b.c.val -> third level */
    Expr *a_var = ast_create_variable_expr(&arena, a_tok, &a_tok);
    Expr *a_b = ast_create_member_access_expr(&arena, a_var, b_tok, &b_tok);
    Expr *a_b_c = ast_create_member_access_expr(&arena, a_b, c_tok, &c_tok);
    Expr *a_b_c_val = ast_create_member_access_expr(&arena, a_b_c, val_tok, &val_tok);

    /* var v: int = a.b.c.val */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 10, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, int_type, a_b_c_val, &v_tok);

    /* Create function body */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = a_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, all member accesses in the chain should have the same scope_depth
     * (propagated from base variable 'a') */
    int base_depth = a_b->as.member_access.scope_depth;
    assert(a_b_c->as.member_access.scope_depth == base_depth);
    assert(a_b_c_val->as.member_access.scope_depth == base_depth);

    DEBUG_INFO("Three-level chain scope depths: a.b=%d, a.b.c=%d, a.b.c.val=%d",
               a_b->as.member_access.scope_depth,
               a_b_c->as.member_access.scope_depth,
               a_b_c_val->as.member_access.scope_depth);

    /* Verify field indices are correct */
    assert(a_b->as.member_access.field_index == 0);      /* b is first field of Level1 */
    assert(a_b_c->as.member_access.field_index == 0);    /* c is first field of Level2 */
    assert(a_b_c_val->as.member_access.field_index == 0); /* val is first field of Level3 */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_chain_three_levels");
}

/* Test: escape detection for field assignment - RHS escapes to LHS field */
static void test_field_assign_escape_detection()
{
    DEBUG_INFO("Starting test_field_assign_escape_detection");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = Outer { inner: Inner { val: 0 } }
     *     {
     *         var local: Inner = Inner { val: 42 }
     *         o.inner = local  // escape: local escapes to outer scope via field
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { val: 0 } */
    FieldInitializer inner_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    /* Create Outer literal: Outer { inner: Inner { val: 0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Inner block: { var local: Inner = ...; o.inner = local } */
    /* Create Inner literal for local: Inner { val: 42 } */
    LiteralValue val_42 = {.int_value = 42};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    /* var local: Inner = Inner { val: 42 } */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* Create o.inner (member access) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create o.inner = local (member assignment) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 10, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = o_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should be marked as escaping
     * because 'local' is from a deeper scope than 'o' */
    assert(local_var->escape_info.escapes_scope == true);
    DEBUG_INFO("Field assign escape test: RHS escape_info.escapes_scope = %d", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_escape_detection");
}

/* Test: no escape when RHS and LHS are in the same scope */
static void test_field_assign_same_scope_no_escape()
{
    DEBUG_INFO("Starting test_field_assign_same_scope_no_escape");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = Outer { inner: Inner { val: 0 } }
     *     var local: Inner = Inner { val: 42 }
     *     o.inner = local  // NO escape: both in same scope
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { val: 0 } */
    FieldInitializer inner_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    /* Create Outer literal: Outer { inner: Inner { val: 0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Create Inner literal for local: Inner { val: 42 } */
    LiteralValue val_42 = {.int_value = 42};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    /* var local: Inner = Inner { val: 42 } (same scope as o) */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* Create o.inner (member access) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create o.inner = local (member assignment) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Create function body - all in same scope */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 3);
    fn_body[0] = o_decl;
    fn_body[1] = local_decl;
    fn_body[2] = assign_stmt;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 3, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should NOT be marked as escaping
     * because 'local' and 'o' are in the same scope */
    assert(local_var->escape_info.escapes_scope == false);
    DEBUG_INFO("Field assign same scope test: RHS escape_info.escapes_scope = %d (should be 0)", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_same_scope_no_escape");
}

/* Test: escape detection for nested field access chain LHS (o.a.b = inner_val) */
static void test_field_assign_chain_escape_detection()
{
    DEBUG_INFO("Starting test_field_assign_chain_escape_detection");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Level2 struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField level2_fields[1];
    level2_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token level2_tok;
    setup_token(&level2_tok, TOKEN_IDENTIFIER, "Level2", 1, "test.sn", &arena);

    Type *level2_type = ast_create_struct_type(&arena, "Level2", level2_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level2_tok, level2_type);

    Stmt *level2_decl = ast_create_struct_decl_stmt(&arena, level2_tok, level2_fields, 1, NULL, 0, false, false, false, NULL, &level2_tok);
    ast_module_add_statement(&arena, &module, level2_decl);

    /* Create Level1 struct with l2: Level2 */
    StructField level1_fields[1];
    level1_fields[0] = create_test_field(&arena, "l2", level2_type, NULL);

    Token level1_tok;
    setup_token(&level1_tok, TOKEN_IDENTIFIER, "Level1", 2, "test.sn", &arena);

    Type *level1_type = ast_create_struct_type(&arena, "Level1", level1_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level1_tok, level1_type);

    Stmt *level1_decl = ast_create_struct_decl_stmt(&arena, level1_tok, level1_fields, 1, NULL, 0, false, false, false, NULL, &level1_tok);
    ast_module_add_statement(&arena, &module, level1_decl);

    /* Create Root struct with l1: Level1 */
    StructField root_fields[1];
    root_fields[0] = create_test_field(&arena, "l1", level1_type, NULL);

    Token root_tok;
    setup_token(&root_tok, TOKEN_IDENTIFIER, "Root", 3, "test.sn", &arena);

    Type *root_type = ast_create_struct_type(&arena, "Root", root_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, root_tok, root_type);

    Stmt *root_decl = ast_create_struct_decl_stmt(&arena, root_tok, root_fields, 1, NULL, 0, false, false, false, NULL, &root_tok);
    ast_module_add_statement(&arena, &module, root_decl);

    /* Create function:
     * fn test_fn() {
     *     var r: Root = Root { l1: Level1 { l2: Level2 { val: 0 } } }
     *     {
     *         var local: Level2 = Level2 { val: 99 }
     *         r.l1.l2 = local  // escape through nested chain
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Level2 literal: Level2 { val: 0 } */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer l2_inits[1];
    l2_inits[0].name = val_tok;
    l2_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *l2_lit = ast_create_struct_literal_expr(&arena, level2_tok, l2_inits, 1, &level2_tok);

    /* Create Level1 literal: Level1 { l2: Level2 { ... } } */
    Token l2_field_tok;
    setup_token(&l2_field_tok, TOKEN_IDENTIFIER, "l2", 7, "test.sn", &arena);

    FieldInitializer l1_inits[1];
    l1_inits[0].name = l2_field_tok;
    l1_inits[0].value = l2_lit;

    Expr *l1_lit = ast_create_struct_literal_expr(&arena, level1_tok, l1_inits, 1, &level1_tok);

    /* Create Root literal: Root { l1: Level1 { ... } } */
    Token l1_field_tok;
    setup_token(&l1_field_tok, TOKEN_IDENTIFIER, "l1", 8, "test.sn", &arena);

    FieldInitializer root_inits[1];
    root_inits[0].name = l1_field_tok;
    root_inits[0].value = l1_lit;

    Expr *root_lit = ast_create_struct_literal_expr(&arena, root_tok, root_inits, 1, &root_tok);

    /* var r: Root = Root { ... } */
    Token r_tok;
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 9, "test.sn", &arena);
    Stmt *r_decl = ast_create_var_decl_stmt(&arena, r_tok, root_type, root_lit, &r_tok);

    /* Inner block: { var local: Level2 = ...; r.l1.l2 = local } */
    /* Create Level2 literal for local: Level2 { val: 99 } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, level2_tok, local_inits, 1, &level2_tok);

    /* var local: Level2 = Level2 { val: 99 } */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 10, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, level2_type, local_lit, &local_tok);

    /* Create r.l1 (member access) */
    Expr *r_var = ast_create_variable_expr(&arena, r_tok, &r_tok);
    Expr *r_l1 = ast_create_member_access_expr(&arena, r_var, l1_field_tok, &l1_field_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create r.l1.l2 = local (member assignment to nested chain) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, r_l1, l2_field_tok, local_var, &l2_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &l2_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 11, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = r_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should be marked as escaping
     * because 'local' is from a deeper scope than 'r' */
    assert(local_var->escape_info.escapes_scope == true);

    /* Also verify that the LHS member access chain (r.l1) should be marked as escaped */
    assert(r_l1->as.member_access.escaped == true);
    DEBUG_INFO("Field assign chain escape test: LHS r.l1 escaped = %d, RHS escapes_scope = %d",
               r_l1->as.member_access.escaped, local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_chain_escape_detection");
}

/* Test: escape detection marks ALL nodes in deeply nested LHS chain (r.a.b.c = local) */
static void test_field_assign_deep_chain_all_nodes_escaped()
{
    DEBUG_INFO("Starting test_field_assign_deep_chain_all_nodes_escaped");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create a 4-level struct hierarchy: Root -> A -> B -> C(val: int) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level C: struct C { val: int } */
    StructField c_fields[1];
    c_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "C", 1, "test.sn", &arena);

    Type *c_type = ast_create_struct_type(&arena, "C", c_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, c_tok, c_type);

    Stmt *c_decl = ast_create_struct_decl_stmt(&arena, c_tok, c_fields, 1, NULL, 0, false, false, false, NULL, &c_tok);
    ast_module_add_statement(&arena, &module, c_decl);

    /* Level B: struct B { c: C } */
    StructField b_fields[1];
    b_fields[0] = create_test_field(&arena, "c", c_type, NULL);

    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "B", 2, "test.sn", &arena);

    Type *b_type = ast_create_struct_type(&arena, "B", b_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, b_tok, b_type);

    Stmt *b_decl = ast_create_struct_decl_stmt(&arena, b_tok, b_fields, 1, NULL, 0, false, false, false, NULL, &b_tok);
    ast_module_add_statement(&arena, &module, b_decl);

    /* Level A: struct A { b: B } */
    StructField a_fields[1];
    a_fields[0] = create_test_field(&arena, "b", b_type, NULL);

    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "A", 3, "test.sn", &arena);

    Type *a_type = ast_create_struct_type(&arena, "A", a_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, a_tok, a_type);

    Stmt *a_decl = ast_create_struct_decl_stmt(&arena, a_tok, a_fields, 1, NULL, 0, false, false, false, NULL, &a_tok);
    ast_module_add_statement(&arena, &module, a_decl);

    /* Root: struct Root { a: A } */
    StructField root_fields[1];
    root_fields[0] = create_test_field(&arena, "a", a_type, NULL);

    Token root_tok;
    setup_token(&root_tok, TOKEN_IDENTIFIER, "Root", 4, "test.sn", &arena);

    Type *root_type = ast_create_struct_type(&arena, "Root", root_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, root_tok, root_type);

    Stmt *root_decl = ast_create_struct_decl_stmt(&arena, root_tok, root_fields, 1, NULL, 0, false, false, false, NULL, &root_tok);
    ast_module_add_statement(&arena, &module, root_decl);

    /* Create function:
     * fn test_fn() {
     *     var r: Root = Root { a: A { b: B { c: C { val: 0 } } } }
     *     {
     *         var local: C = C { val: 99 }
     *         r.a.b.c = local  // escape through 3-level chain
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 10, "test.sn", &arena);

    /* Build nested literals from inside out */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 11, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer c_inits[1];
    c_inits[0].name = val_tok;
    c_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);
    Expr *c_lit = ast_create_struct_literal_expr(&arena, c_tok, c_inits, 1, &c_tok);

    Token c_field_tok;
    setup_token(&c_field_tok, TOKEN_IDENTIFIER, "c", 12, "test.sn", &arena);
    FieldInitializer b_inits[1];
    b_inits[0].name = c_field_tok;
    b_inits[0].value = c_lit;
    Expr *b_lit = ast_create_struct_literal_expr(&arena, b_tok, b_inits, 1, &b_tok);

    Token b_field_tok;
    setup_token(&b_field_tok, TOKEN_IDENTIFIER, "b", 13, "test.sn", &arena);
    FieldInitializer a_inits[1];
    a_inits[0].name = b_field_tok;
    a_inits[0].value = b_lit;
    Expr *a_lit = ast_create_struct_literal_expr(&arena, a_tok, a_inits, 1, &a_tok);

    Token a_field_tok;
    setup_token(&a_field_tok, TOKEN_IDENTIFIER, "a", 14, "test.sn", &arena);
    FieldInitializer root_inits[1];
    root_inits[0].name = a_field_tok;
    root_inits[0].value = a_lit;
    Expr *root_lit = ast_create_struct_literal_expr(&arena, root_tok, root_inits, 1, &root_tok);

    /* var r: Root = Root { ... } */
    Token r_tok;
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 15, "test.sn", &arena);
    Stmt *r_decl = ast_create_var_decl_stmt(&arena, r_tok, root_type, root_lit, &r_tok);

    /* Inner block: { var local: C = ...; r.a.b.c = local } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);
    Expr *local_lit = ast_create_struct_literal_expr(&arena, c_tok, local_inits, 1, &c_tok);

    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 16, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, c_type, local_lit, &local_tok);

    /* Build r.a.b.c chain from bottom up:
     * r_var -> r.a -> r.a.b -> r.a.b.c = local */
    Expr *r_var = ast_create_variable_expr(&arena, r_tok, &r_tok);
    Expr *r_a = ast_create_member_access_expr(&arena, r_var, a_field_tok, &a_field_tok);
    Expr *r_a_b = ast_create_member_access_expr(&arena, r_a, b_field_tok, &b_field_tok);

    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create r.a.b.c = local (c is the field being assigned) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, r_a_b, c_field_tok, local_var, &c_field_tok);

    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &c_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 17, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = r_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* Verify that ALL nodes in the LHS chain are marked as escaped:
     * - r.a (first level)
     * - r.a.b (second level, object_expr of the assignment) */
    assert(local_var->escape_info.escapes_scope == true);
    assert(r_a->as.member_access.escaped == true);
    assert(r_a_b->as.member_access.escaped == true);

    DEBUG_INFO("Deep chain escape test: r.a escaped = %d, r.a.b escaped = %d, RHS escapes_scope = %d",
               r_a->as.member_access.escaped, r_a_b->as.member_access.escaped,
               local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_deep_chain_all_nodes_escaped");
}

/* Test: LHS base scope is used for comparison, not intermediate scope */
static void test_field_assign_uses_base_scope()
{
    DEBUG_INFO("Starting test_field_assign_uses_base_scope");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = ...
     *     {
     *         {
     *             var deep_local: Inner = Inner { val: 99 }
     *             o.inner = deep_local  // RHS is 2 scopes deeper than base 'o'
     *         }
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal for outer */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer inner_inits[1];
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);
    FieldInitializer outer_inits[1];
    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;
    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = ... */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Deep inner block: { { var deep_local: Inner = ...; o.inner = deep_local } } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);
    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "deep_local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* o.inner = deep_local */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Deep inner block */
    Stmt **deepest_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    deepest_body[0] = local_decl;
    deepest_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 10, "test.sn", &arena);
    Stmt *deepest_block = ast_create_block_stmt(&arena, deepest_body, 2, &block_tok);

    /* Middle block (just wraps the deepest) */
    Stmt **middle_body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    middle_body[0] = deepest_block;
    Stmt *middle_block = ast_create_block_stmt(&arena, middle_body, 1, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = o_decl;
    fn_body[1] = middle_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* deep_local is 2 scopes deeper than 'o', so escape should be detected
     * using BASE scope of LHS (which is 'o'), not some intermediate. */
    assert(local_var->escape_info.escapes_scope == true);
    DEBUG_INFO("Base scope test: RHS escapes_scope = %d (should be 1)", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_uses_base_scope");
}

/* ============================================================================
 * Struct Type Equality Tests
 * ============================================================================
 * These tests verify ast_type_equals for struct types handles all edge cases.
 * ============================================================================ */

/* Test: struct type equality with matching names */
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

/* ============================================================================
 * Large Struct Tests
 * ============================================================================
 * These tests verify struct handling with many fields.
 * ============================================================================ */

/* Test: large struct with 20 fields */
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

void test_type_checker_struct_main(void)
{
    TEST_SECTION("Struct Type Checker");

    TEST_RUN("struct_primitive_fields", test_struct_primitive_fields);
    TEST_RUN("struct_all_primitive_types", test_struct_all_primitive_types);
    TEST_RUN("struct_nested_struct_type", test_struct_nested_struct_type);
    TEST_RUN("struct_array_field", test_struct_array_field);
    TEST_RUN("struct_default_value_valid", test_struct_default_value_valid);
    TEST_RUN("struct_default_value_type_mismatch", test_struct_default_value_type_mismatch);
    TEST_RUN("native_struct_pointer_field", test_native_struct_pointer_field);
    TEST_RUN("non_native_struct_pointer_field_error", test_non_native_struct_pointer_field_error);
    TEST_RUN("struct_empty", test_struct_empty);
    TEST_RUN("struct_opaque_field", test_struct_opaque_field);
    TEST_RUN("struct_null_field_type_error", test_struct_null_field_type_error);

    /* Circular dependency detection tests */
    TEST_RUN("struct_direct_circular_dependency", test_struct_direct_circular_dependency);
    TEST_RUN("struct_indirect_circular_dependency", test_struct_indirect_circular_dependency);
    TEST_RUN("struct_multi_level_circular_chain", test_struct_multi_level_circular_chain);
    TEST_RUN("struct_pointer_breaks_cycle", test_struct_pointer_breaks_cycle);
    TEST_RUN("struct_array_of_self_circular", test_struct_array_of_self_circular);
    TEST_RUN("circular_dependency_detection_direct", test_circular_dependency_detection_direct);

    /* Native struct context tests */
    TEST_RUN("native_struct_in_native_fn_context", test_native_struct_in_native_fn_context);
    TEST_RUN("native_struct_in_regular_fn_error", test_native_struct_in_regular_fn_error);
    TEST_RUN("regular_struct_in_regular_fn", test_regular_struct_in_regular_fn);

    /* Struct layout calculation tests */
    TEST_RUN("struct_layout_all_8byte_fields", test_struct_layout_all_8byte_fields);
    TEST_RUN("struct_layout_byte_int_padding", test_struct_layout_byte_int_padding);
    TEST_RUN("struct_layout_trailing_padding", test_struct_layout_trailing_padding);
    TEST_RUN("struct_layout_4byte_fields", test_struct_layout_4byte_fields);
    TEST_RUN("struct_layout_mixed_alignment", test_struct_layout_mixed_alignment);
    TEST_RUN("struct_layout_all_1byte_fields", test_struct_layout_all_1byte_fields);
    TEST_RUN("struct_layout_empty", test_struct_layout_empty);
    TEST_RUN("struct_layout_nested", test_struct_layout_nested);

    /* Packed struct layout tests */
    TEST_RUN("struct_layout_packed_mixed", test_struct_layout_packed_mixed);
    TEST_RUN("struct_layout_packed_binary_header", test_struct_layout_packed_binary_header);
    TEST_RUN("struct_layout_packed_vs_unpacked", test_struct_layout_packed_vs_unpacked);

    /* Symbol table registration tests */
    TEST_RUN("struct_symbol_table_registration", test_struct_symbol_table_registration);
    TEST_RUN("struct_symbol_table_metadata", test_struct_symbol_table_metadata);
    TEST_RUN("struct_symbol_table_native_metadata", test_struct_symbol_table_native_metadata);
    TEST_RUN("struct_symbol_table_size_alignment", test_struct_symbol_table_size_alignment);
    TEST_RUN("struct_symbol_table_lookup_for_later_use", test_struct_symbol_table_lookup_for_later_use);
    TEST_RUN("struct_symbol_table_lookup_not_found", test_struct_symbol_table_lookup_not_found);

    /* Struct literal field initialization tracking tests */
    TEST_RUN("struct_literal_all_fields_initialized", test_struct_literal_all_fields_initialized);
    TEST_RUN("struct_literal_partial_initialization", test_struct_literal_partial_initialization);
    TEST_RUN("struct_literal_empty_initialization", test_struct_literal_empty_initialization);
    TEST_RUN("struct_literal_field_init_helper_edge_cases", test_struct_literal_field_init_helper_edge_cases);
    TEST_RUN("struct_literal_field_init_invalid_index", test_struct_literal_field_init_invalid_index);

    /* Default value application tests */
    TEST_RUN("struct_default_value_applied", test_struct_default_value_applied);
    TEST_RUN("struct_multiple_defaults_applied", test_struct_multiple_defaults_applied);
    TEST_RUN("struct_explicit_overrides_default", test_struct_explicit_overrides_default);

    /* Required field enforcement tests */
    TEST_RUN("struct_missing_required_fields_error", test_struct_missing_required_fields_error);
    TEST_RUN("struct_missing_one_required_field_error", test_struct_missing_one_required_field_error);
    TEST_RUN("struct_all_required_fields_provided", test_struct_all_required_fields_provided);
    TEST_RUN("struct_optional_fields_not_required", test_struct_optional_fields_not_required);

    /* Nested struct initialization tests */
    TEST_RUN("nested_struct_all_fields_provided", test_nested_struct_all_fields_provided);
    TEST_RUN("nested_struct_inner_missing_required", test_nested_struct_inner_missing_required);
    TEST_RUN("nested_struct_inner_defaults_applied", test_nested_struct_inner_defaults_applied);
    TEST_RUN("nested_struct_three_levels", test_nested_struct_three_levels);
    TEST_RUN("nested_struct_three_levels_missing_required", test_nested_struct_three_levels_missing_required);

    /* Member access scope depth propagation tests */
    TEST_RUN("member_access_scope_depth_propagation", test_member_access_scope_depth_propagation);
    TEST_RUN("member_access_nested_scope_depth", test_member_access_nested_scope_depth);
    TEST_RUN("member_access_chain_scope_depth", test_member_access_chain_scope_depth);
    TEST_RUN("member_access_chain_three_levels", test_member_access_chain_three_levels);

    /* Field assignment escape detection tests */
    TEST_RUN("field_assign_escape_detection", test_field_assign_escape_detection);
    TEST_RUN("field_assign_same_scope_no_escape", test_field_assign_same_scope_no_escape);
    TEST_RUN("field_assign_chain_escape_detection", test_field_assign_chain_escape_detection);
    TEST_RUN("field_assign_deep_chain_all_nodes_escaped", test_field_assign_deep_chain_all_nodes_escaped);
    TEST_RUN("field_assign_uses_base_scope", test_field_assign_uses_base_scope);

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
