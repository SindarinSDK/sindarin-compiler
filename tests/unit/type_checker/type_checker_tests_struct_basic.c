// tests/type_checker_tests_struct_basic.c
// Basic struct declaration type checker tests

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

void test_type_checker_struct_basic_main(void)
{
    TEST_SECTION("Struct Type Checker - Basic");

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
}
