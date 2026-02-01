// tests/unit/parser/parser_tests_expr_unary.c
// Parser tests for unary expressions and precedence

/* ============================================================================
 * Unary Expression Tests
 * ============================================================================ */

static void test_parser_unary_minus(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = -y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_UNARY);
    assert(stmt->as.var_decl.initializer->as.unary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_unary_not(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = !cond\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_UNARY);
    assert(stmt->as.var_decl.initializer->as.unary.operator == TOKEN_BANG);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_double_negation(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = --y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_UNARY);
    Expr *inner = stmt->as.var_decl.initializer->as.unary.operand;
    assert(inner->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Precedence Tests
 * ============================================================================ */

static void test_parser_precedence_mul_over_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // 1 + 2 * 3 should parse as 1 + (2 * 3)
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 1 + 2 * 3\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_PLUS);
    assert(expr->as.binary.right->type == EXPR_BINARY);
    assert(expr->as.binary.right->as.binary.operator == TOKEN_STAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_precedence_div_over_sub(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // 10 - 6 / 2 should parse as 10 - (6 / 2)
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 10 - 6 / 2\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_MINUS);
    assert(expr->as.binary.right->type == EXPR_BINARY);
    assert(expr->as.binary.right->as.binary.operator == TOKEN_SLASH);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_precedence_comparison_over_logical(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // x < y and y < z should parse as (x < y) and (y < z)
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x < y and y < z\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_AND);
    assert(expr->as.binary.left->type == EXPR_BINARY);
    assert(expr->as.binary.left->as.binary.operator == TOKEN_LESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_left_associativity_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // 1 + 2 + 3 should parse as (1 + 2) + 3
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 1 + 2 + 3\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_PLUS);
    assert(expr->as.binary.left->type == EXPR_BINARY);
    assert(expr->as.binary.left->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_parentheses_override(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    // (1 + 2) * 3 should parse as (1 + 2) * 3
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = (1 + 2) * 3\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_STAR);
    assert(expr->as.binary.left->type == EXPR_BINARY);
    assert(expr->as.binary.left->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
