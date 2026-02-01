// tests/unit/parser/parser_tests_expr_call.c
// Parser tests for call and array expressions

/* ============================================================================
 * Call Expression Tests
 * ============================================================================ */

static void test_parser_call_no_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "foo()\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_CALL);
    assert(stmt->as.expression.expression->as.call.arg_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_call_one_arg(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "foo(42)\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_CALL);
    assert(stmt->as.expression.expression->as.call.arg_count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_call_multiple_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "foo(1, 2, 3)\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_CALL);
    assert(stmt->as.expression.expression->as.call.arg_count == 3);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_call_expression_args(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "foo(1 + 2, x * y)\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_CALL);
    assert(stmt->as.expression.expression->as.call.arg_count == 2);
    assert(stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_nested_calls(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "foo(bar(x))\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    Expr *call = stmt->as.expression.expression;
    assert(call->type == EXPR_CALL);
    assert(call->as.call.arguments[0]->type == EXPR_CALL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Array Expression Tests
 * ============================================================================ */

static void test_parser_array_empty(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var arr: int[] = []\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY);
    assert(stmt->as.var_decl.initializer->as.array.element_count == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_single_element(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var arr: int[] = [42]\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY);
    assert(stmt->as.var_decl.initializer->as.array.element_count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_multiple_elements(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var arr: int[] = [1, 2, 3, 4, 5]\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY);
    assert(stmt->as.var_decl.initializer->as.array.element_count == 5);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_access(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = arr[0]\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_ARRAY_ACCESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_nested_access(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = matrix[i][j]\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_ARRAY_ACCESS);
    assert(expr->as.array_access.array->type == EXPR_ARRAY_ACCESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
