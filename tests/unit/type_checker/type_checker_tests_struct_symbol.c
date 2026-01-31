// tests/type_checker_tests_struct_symbol.c
// Symbol table registration tests for structs

/* ============================================================================
 * Symbol Table Registration Tests
 * ============================================================================ */

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


void test_type_checker_struct_symbol_main(void)
{
    TEST_SECTION("Struct Type Checker - Symbol Table");

    TEST_RUN("struct_symbol_table_registration", test_struct_symbol_table_registration);
    TEST_RUN("struct_symbol_table_metadata", test_struct_symbol_table_metadata);
    TEST_RUN("struct_symbol_table_native_metadata", test_struct_symbol_table_native_metadata);
    TEST_RUN("struct_symbol_table_size_alignment", test_struct_symbol_table_size_alignment);
    TEST_RUN("struct_symbol_table_lookup_for_later_use", test_struct_symbol_table_lookup_for_later_use);
    TEST_RUN("struct_symbol_table_lookup_not_found", test_struct_symbol_table_lookup_not_found);
}
