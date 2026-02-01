// parser_tests_edge_types.c
// Type tests and complex expression tests

/* ============================================================================
 * Type Tests
 * ============================================================================ */

static void test_parse_array_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: [int] = [1, 2, 3]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_pointer_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: *int = null\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_POINTER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nullable_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int? = null\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_NULLABLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_long_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: long = 100000000000\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_LONG);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_byte_type(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: byte = 255\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_BYTE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Complex Expression Tests
 * ============================================================================ */

static void test_parse_chained_method_calls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x.foo().bar().baz()\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_array_indexing(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x[0]\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.expression.expression;
    assert(expr->type == EXPR_INDEX);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_compound_assignment(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x += 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
