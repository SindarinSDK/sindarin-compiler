// parser_tests_struct_nested.c
// Nested struct and error handling parser tests

static void test_struct_with_nested_struct_field()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "\n"
        "struct Rectangle =>\n"
        "    origin: Point\n"
        "    width: double\n"
        "    height: double\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    /* First struct: Point */
    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt1->as.struct_decl.name.start, "Point", stmt1->as.struct_decl.name.length) == 0);
    assert(stmt1->as.struct_decl.field_count == 2);

    /* Second struct: Rectangle with nested Point */
    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt2->as.struct_decl.name.start, "Rectangle", stmt2->as.struct_decl.name.length) == 0);
    assert(stmt2->as.struct_decl.field_count == 3);

    /* Check origin: Point field - should be TYPE_STRUCT with name "Point" */
    assert(strcmp(stmt2->as.struct_decl.fields[0].name, "origin") == 0);
    assert(stmt2->as.struct_decl.fields[0].type != NULL);
    assert(stmt2->as.struct_decl.fields[0].type->kind == TYPE_STRUCT);
    assert(strcmp(stmt2->as.struct_decl.fields[0].type->as.struct_type.name, "Point") == 0);
    /* Point struct is registered in symbol table, so it resolves to the full type with 2 fields */
    assert(stmt2->as.struct_decl.fields[0].type->as.struct_type.field_count == 2);

    /* Check width: double field */
    assert(strcmp(stmt2->as.struct_decl.fields[1].name, "width") == 0);
    assert(stmt2->as.struct_decl.fields[1].type->kind == TYPE_DOUBLE);

    /* Check height: double field */
    assert(strcmp(stmt2->as.struct_decl.fields[2].name, "height") == 0);
    assert(stmt2->as.struct_decl.fields[2].type->kind == TYPE_DOUBLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_mixed_primitive_and_struct_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    name: str\n"
        "    server: ServerConfig\n"
        "    timeout: int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 3);

    /* Check name: str (primitive) */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "name") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_STRING);

    /* Check server: ServerConfig (struct reference) */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "server") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_STRUCT);
    assert(strcmp(stmt->as.struct_decl.fields[1].type->as.struct_type.name, "ServerConfig") == 0);

    /* Check timeout: int (primitive) */
    assert(strcmp(stmt->as.struct_decl.fields[2].name, "timeout") == 0);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

static void test_struct_error_duplicate_field_names()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n"
        "    x: int\n";  /* Duplicate field name 'x' */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser returns NULL on error, but had_error flag should be set */
    assert(module == NULL);
    assert(parser.had_error == true);  /* Error should be recorded */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_error_pointer_in_non_native()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Data =>\n"
        "    ptr: *int\n";  /* Pointer field in non-native struct */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser returns NULL on error, but had_error flag should be set */
    assert(module == NULL);
    assert(parser.had_error == true);  /* Error should be recorded */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_struct_allows_pointer_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "native struct Buffer =>\n"
        "    data: *byte\n"
        "    size: int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* No error - native structs can have pointer fields */
    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 1);

    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.is_native == true);
    assert(stmt->as.struct_decl.field_count == 2);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_POINTER);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_error_missing_arrow()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point\n"
        "    x: double\n";  /* Missing => after struct name */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should report error */
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_error_missing_field_type()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x:\n";  /* Missing type after colon */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should report error */
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
