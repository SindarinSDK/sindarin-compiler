// parser_tests_edge_stmts.c
// Statement tests

/* ============================================================================
 * Statement Tests
 * ============================================================================ */

static void test_parse_multiple_var_decls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1\nvar y: int = 2\nvar z: int = 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 3);
    for (int i = 0; i < 3; i++) {
        assert(module->statements[i]->type == STMT_VAR_DECL);
    }

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_nested_if(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if a =>\n"
        "  if b =>\n"
        "    print(\"nested\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK);
    /* The inner if is inside the then_branch */
    Stmt *inner = if_stmt->as.if_stmt.then_branch->as.block.statements[0];
    assert(inner->type == STMT_IF);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_if_elif_else(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if a =>\n"
        "  print(\"a\")\n"
        "elif b =>\n"
        "  print(\"b\")\n"
        "else =>\n"
        "  print(\"c\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    /* Check that elif creates a nested if */
    assert(if_stmt->as.if_stmt.else_branch != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_while_loop(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while x > 0 =>\n"
        "  x = x - 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *while_stmt = module->statements[0];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.condition->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_for_range(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "for i in 0..10 =>\n"
        "  print(i)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *for_stmt = module->statements[0];
    assert(for_stmt->type == STMT_FOR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_return_value(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): int =>\n"
        "  return 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    Stmt *ret = fn->as.function.body[0];
    assert(ret->type == STMT_RETURN);
    assert(ret->as.return_stmt.value != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_return_void(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn foo(): void =>\n"
        "  return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *fn = module->statements[0];
    Stmt *ret = fn->as.function.body[0];
    assert(ret->type == STMT_RETURN);
    assert(ret->as.return_stmt.value == NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_break_statement(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while true =>\n"
        "  break\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *while_stmt = module->statements[0];
    Stmt *body = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(body->type == STMT_BREAK);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_continue_statement(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while true =>\n"
        "  continue\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *while_stmt = module->statements[0];
    Stmt *body = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(body->type == STMT_CONTINUE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
