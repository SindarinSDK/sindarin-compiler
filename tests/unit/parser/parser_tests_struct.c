// tests/parser_tests_struct.c
// Struct declaration parser tests

static void test_struct_decl_empty_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "struct Point =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Point", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 0);
    assert(stmt->as.struct_decl.fields == NULL);
    assert(stmt->as.struct_decl.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_struct_decl_empty_parsing()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "native struct ZStream =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "ZStream", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 0);
    assert(stmt->as.struct_decl.fields == NULL);
    assert(stmt->as.struct_decl.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_decl_name_only()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "struct Rectangle =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Rectangle", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.is_native == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_multiple_struct_decls()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "struct Rectangle =>\n"
        "native struct Buffer =>\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 3);

    /* First struct: Point (not native) */
    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt1->as.struct_decl.name.start, "Point", stmt1->as.struct_decl.name.length) == 0);
    assert(stmt1->as.struct_decl.is_native == false);

    /* Second struct: Rectangle (not native) */
    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt2->as.struct_decl.name.start, "Rectangle", stmt2->as.struct_decl.name.length) == 0);
    assert(stmt2->as.struct_decl.is_native == false);

    /* Third struct: Buffer (native) */
    Stmt *stmt3 = module->statements[2];
    assert(stmt3->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt3->as.struct_decl.name.start, "Buffer", stmt3->as.struct_decl.name.length) == 0);
    assert(stmt3->as.struct_decl.is_native == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_two_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "    y: double\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Point", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 2);
    assert(stmt->as.struct_decl.fields != NULL);
    assert(stmt->as.struct_decl.is_native == false);

    /* Check first field: x: double */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "x") == 0);
    assert(stmt->as.struct_decl.fields[0].type != NULL);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[0].default_value == NULL);

    /* Check second field: y: double */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "y") == 0);
    assert(stmt->as.struct_decl.fields[1].type != NULL);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[1].default_value == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_three_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    timeout: int\n"
        "    retries: int\n"
        "    verbose: bool\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Config", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 3);
    assert(stmt->as.struct_decl.fields != NULL);

    /* Check first field: timeout: int */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "timeout") == 0);
    assert(stmt->as.struct_decl.fields[0].type != NULL);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);

    /* Check second field: retries: int */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "retries") == 0);
    assert(stmt->as.struct_decl.fields[1].type != NULL);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);

    /* Check third field: verbose: bool */
    assert(strcmp(stmt->as.struct_decl.fields[2].name, "verbose") == 0);
    assert(stmt->as.struct_decl.fields[2].type != NULL);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_BOOL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_with_various_types()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Mixed =>\n"
        "    name: str\n"
        "    count: long\n"
        "    flag: byte\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 3);

    /* Check field types */
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_STRING);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_LONG);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_BYTE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_native_struct_with_fields()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "native struct Buffer =>\n"
        "    length: int\n"
        "    capacity: int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(strncmp(stmt->as.struct_decl.name.start, "Buffer", stmt->as.struct_decl.name.length) == 0);
    assert(stmt->as.struct_decl.field_count == 2);
    assert(stmt->as.struct_decl.is_native == true);

    /* Check fields */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "length") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "capacity") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_field_with_default_int()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    timeout: int = 30\n"
        "    retries: int = 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 2);

    /* Check first field: timeout: int = 30 */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "timeout") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);
    assert(stmt->as.struct_decl.fields[0].default_value != NULL);
    assert(stmt->as.struct_decl.fields[0].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[0].default_value->as.literal.value.int_value == 30);

    /* Check second field: retries: int = 3 */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "retries") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);
    assert(stmt->as.struct_decl.fields[1].default_value != NULL);
    assert(stmt->as.struct_decl.fields[1].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[1].default_value->as.literal.value.int_value == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_field_with_default_bool()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Options =>\n"
        "    verbose: bool = false\n"
        "    debug: bool = true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 2);

    /* Check verbose: bool = false */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "verbose") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_BOOL);
    assert(stmt->as.struct_decl.fields[0].default_value != NULL);
    assert(stmt->as.struct_decl.fields[0].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[0].default_value->as.literal.value.bool_value == false);

    /* Check debug: bool = true */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "debug") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_BOOL);
    assert(stmt->as.struct_decl.fields[1].default_value != NULL);
    assert(stmt->as.struct_decl.fields[1].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[1].default_value->as.literal.value.bool_value == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_field_with_default_string()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct ServerConfig =>\n"
        "    host: str = \"localhost\"\n"
        "    port: int = 8080\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 2);

    /* Check host: str = "localhost" */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "host") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_STRING);
    assert(stmt->as.struct_decl.fields[0].default_value != NULL);
    assert(stmt->as.struct_decl.fields[0].default_value->type == EXPR_LITERAL);
    assert(strcmp(stmt->as.struct_decl.fields[0].default_value->as.literal.value.string_value, "localhost") == 0);

    /* Check port: int = 8080 */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "port") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);
    assert(stmt->as.struct_decl.fields[1].default_value != NULL);
    assert(stmt->as.struct_decl.fields[1].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[1].default_value->as.literal.value.int_value == 8080);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_mixed_defaults_and_no_defaults()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Mixed =>\n"
        "    required: int\n"
        "    optional: int = 42\n"
        "    name: str\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 3);

    /* Check required: int (no default) */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "required") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_INT);
    assert(stmt->as.struct_decl.fields[0].default_value == NULL);

    /* Check optional: int = 42 */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "optional") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_INT);
    assert(stmt->as.struct_decl.fields[1].default_value != NULL);
    assert(stmt->as.struct_decl.fields[1].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[1].default_value->as.literal.value.int_value == 42);

    /* Check name: str (no default) */
    assert(strcmp(stmt->as.struct_decl.fields[2].name, "name") == 0);
    assert(stmt->as.struct_decl.fields[2].type->kind == TYPE_STRING);
    assert(stmt->as.struct_decl.fields[2].default_value == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_field_with_default_double()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double = 0.0\n"
        "    y: double = 0.0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_STRUCT_DECL);
    assert(stmt->as.struct_decl.field_count == 2);

    /* Check x: double = 0.0 */
    assert(strcmp(stmt->as.struct_decl.fields[0].name, "x") == 0);
    assert(stmt->as.struct_decl.fields[0].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[0].default_value != NULL);
    assert(stmt->as.struct_decl.fields[0].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[0].default_value->as.literal.value.double_value == 0.0);

    /* Check y: double = 0.0 */
    assert(strcmp(stmt->as.struct_decl.fields[1].name, "y") == 0);
    assert(stmt->as.struct_decl.fields[1].type->kind == TYPE_DOUBLE);
    assert(stmt->as.struct_decl.fields[1].default_value != NULL);
    assert(stmt->as.struct_decl.fields[1].default_value->type == EXPR_LITERAL);
    assert(stmt->as.struct_decl.fields[1].default_value->as.literal.value.double_value == 0.0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

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

/* Error handling tests */

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

/* Struct literal tests */

static void test_struct_literal_empty()
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
        "var p: Point = Point {}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);  /* struct decl + var decl */

    /* Check the var declaration has a struct literal initializer */
    Stmt *var_stmt = module->statements[1];
    assert(var_stmt->type == STMT_VAR_DECL);
    assert(var_stmt->as.var_decl.initializer != NULL);
    assert(var_stmt->as.var_decl.initializer->type == EXPR_STRUCT_LITERAL);
    assert(strncmp(var_stmt->as.var_decl.initializer->as.struct_literal.struct_name.start,
                   "Point", 5) == 0);
    assert(var_stmt->as.var_decl.initializer->as.struct_literal.field_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_with_fields()
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
        "var p: Point = Point { x: 1.0, y: 2.0 }\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the struct literal */
    Stmt *var_stmt = module->statements[1];
    assert(var_stmt->type == STMT_VAR_DECL);
    Expr *init = var_stmt->as.var_decl.initializer;
    assert(init != NULL);
    assert(init->type == EXPR_STRUCT_LITERAL);
    assert(init->as.struct_literal.field_count == 2);

    /* Check field names and values */
    assert(strncmp(init->as.struct_literal.fields[0].name.start, "x", 1) == 0);
    assert(init->as.struct_literal.fields[0].value->type == EXPR_LITERAL);

    assert(strncmp(init->as.struct_literal.fields[1].name.start, "y", 1) == 0);
    assert(init->as.struct_literal.fields[1].value->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_partial_init()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Config =>\n"
        "    timeout: int\n"
        "    retries: int\n"
        "    verbose: bool\n"
        "\n"
        "var cfg: Config = Config { timeout: 30 }\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    Stmt *var_stmt = module->statements[1];
    Expr *init = var_stmt->as.var_decl.initializer;
    assert(init->type == EXPR_STRUCT_LITERAL);
    assert(init->as.struct_literal.field_count == 1);  /* Only one field specified */
    assert(strncmp(init->as.struct_literal.fields[0].name.start, "timeout", 7) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Field access tests */

static void test_field_access_simple()
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
        "fn test(p: Point): double =>\n"
        "    return p.x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the function body has a return statement with member access */
    Stmt *fn_stmt = module->statements[1];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);
    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_RETURN);
    assert(body_stmt->as.return_stmt.value->type == EXPR_MEMBER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_access_nested()
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
        "    height: double\n"
        "\n"
        "fn test(r: Rectangle): double =>\n"
        "    return r.origin.x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 3);

    /* Check the function body has nested member access (EXPR_MEMBER) */
    Stmt *fn_stmt = module->statements[2];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);
    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_RETURN);

    /* r.origin.x should be EXPR_MEMBER with an EXPR_MEMBER object */
    Expr *member_expr = body_stmt->as.return_stmt.value;
    assert(member_expr->type == EXPR_MEMBER);
    assert(member_expr->as.member.object->type == EXPR_MEMBER);  /* r.origin is also EXPR_MEMBER */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_assignment()
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
        "fn test(p: Point): void =>\n"
        "    p.x = 5.0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 2);

    /* Check the function body has a member assignment expression */
    Stmt *fn_stmt = module->statements[1];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);

    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_EXPR);
    assert(body_stmt->as.expression.expression->type == EXPR_MEMBER_ASSIGN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_field_assignment_nested()
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
        "\n"
        "fn test(r: Rectangle): void =>\n"
        "    r.origin.x = 5.0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(parser.had_error == false);
    assert(module->count == 3);

    /* Check the function body has a nested member assignment expression */
    Stmt *fn_stmt = module->statements[2];
    assert(fn_stmt->type == STMT_FUNCTION);
    assert(fn_stmt->as.function.body_count == 1);

    Stmt *body_stmt = fn_stmt->as.function.body[0];
    assert(body_stmt->type == STMT_EXPR);
    assert(body_stmt->as.expression.expression->type == EXPR_MEMBER_ASSIGN);

    /* The object should be a member expression (r.origin) */
    Expr *member_assign = body_stmt->as.expression.expression;
    assert(member_assign->as.member_assign.object->type == EXPR_MEMBER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Error handling tests for struct literals */

static void test_struct_literal_missing_colon()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "\n"
        "var p: Point = Point { x 1.0 }\n";  /* Missing colon */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_struct_literal_invalid_field_name()
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "struct Point =>\n"
        "    x: double\n"
        "\n"
        "var p: Point = Point { 123: 1.0 }\n";  /* Number instead of field name */
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_parser_struct_main()
{
    TEST_SECTION("Parser Struct Declaration Tests");
    TEST_RUN("struct_decl_empty_parsing", test_struct_decl_empty_parsing);
    TEST_RUN("native_struct_decl_empty_parsing", test_native_struct_decl_empty_parsing);
    TEST_RUN("struct_decl_name_only", test_struct_decl_name_only);
    TEST_RUN("multiple_struct_decls", test_multiple_struct_decls);
    TEST_RUN("struct_with_two_fields", test_struct_with_two_fields);
    TEST_RUN("struct_with_three_fields", test_struct_with_three_fields);
    TEST_RUN("struct_with_various_types", test_struct_with_various_types);
    TEST_RUN("native_struct_with_fields", test_native_struct_with_fields);
    TEST_RUN("struct_field_with_default_int", test_struct_field_with_default_int);
    TEST_RUN("struct_field_with_default_bool", test_struct_field_with_default_bool);
    TEST_RUN("struct_field_with_default_string", test_struct_field_with_default_string);
    TEST_RUN("struct_mixed_defaults_and_no_defaults", test_struct_mixed_defaults_and_no_defaults);
    TEST_RUN("struct_field_with_default_double", test_struct_field_with_default_double);
    TEST_RUN("struct_with_nested_struct_field", test_struct_with_nested_struct_field);
    TEST_RUN("struct_with_mixed_primitive_and_struct_fields", test_struct_with_mixed_primitive_and_struct_fields);

    /* Error handling tests */
    TEST_RUN("struct_error_duplicate_field_names", test_struct_error_duplicate_field_names);
    TEST_RUN("struct_error_pointer_in_non_native", test_struct_error_pointer_in_non_native);
    TEST_RUN("native_struct_allows_pointer_fields", test_native_struct_allows_pointer_fields);
    TEST_RUN("struct_error_missing_arrow", test_struct_error_missing_arrow);
    TEST_RUN("struct_error_missing_field_type", test_struct_error_missing_field_type);

    /* Struct literal tests */
    TEST_SECTION("Parser Struct Literal Tests");
    TEST_RUN("struct_literal_empty", test_struct_literal_empty);
    TEST_RUN("struct_literal_with_fields", test_struct_literal_with_fields);
    TEST_RUN("struct_literal_partial_init", test_struct_literal_partial_init);

    /* Field access tests */
    TEST_SECTION("Parser Field Access Tests");
    TEST_RUN("field_access_simple", test_field_access_simple);
    TEST_RUN("field_access_nested", test_field_access_nested);
    TEST_RUN("field_assignment", test_field_assignment);
    TEST_RUN("field_assignment_nested", test_field_assignment_nested);

    /* Error handling tests for struct literals */
    TEST_RUN("struct_literal_missing_colon", test_struct_literal_missing_colon);
    TEST_RUN("struct_literal_invalid_field_name", test_struct_literal_invalid_field_name);
}
