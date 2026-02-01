// parser_tests_struct_defaults.c
// Struct field default value parser tests

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
