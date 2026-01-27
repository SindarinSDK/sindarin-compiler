// tests/parser_tests_operators.c
// Tests for parsing all operator types

/* ============================================================================
 * Binary Arithmetic Operator Tests
 * ============================================================================ */

static void test_parse_op_plus(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1 + 2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_minus(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 5 - 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_multiply(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 3 * 4\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_STAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_divide(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 10 / 2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_SLASH);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_modulo(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 10 % 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_PERCENT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Binary Comparison Operator Tests
 * ============================================================================ */

static void test_parse_op_equal_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a == b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_EQUAL_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_not_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a != b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_BANG_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_less(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a < b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_LESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_less_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a <= b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_LESS_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_greater(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a > b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_GREATER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_greater_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a >= b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_GREATER_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Binary Logical Operator Tests
 * ============================================================================ */

static void test_parse_op_and(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a and b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_AND);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_or(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a or b\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Unary Operator Tests
 * ============================================================================ */

static void test_parse_op_negate(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = -5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);
    assert(init->as.unary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_op_not(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = not true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);
    assert(init->as.unary.operator == TOKEN_NOT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Assignment Operator Tests
 * ============================================================================ */

static void test_parse_assign_plus_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x += 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_assign_minus_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x -= 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_assign_star_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x *= 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_assign_slash_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x /= 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_assign_percent_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "x %= 5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Chained Operator Tests
 * ============================================================================ */

static void test_parse_chained_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1 + 2 + 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Left-associative: ((1 + 2) + 3), top is + */
    assert(init->as.binary.operator == TOKEN_PLUS);
    assert(init->as.binary.left->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_chained_mul(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 2 * 3 * 4\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_STAR);
    assert(init->as.binary.left->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_mixed_operators(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1 + 2 * 3 - 4\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (1 + (2 * 3)) - 4 */
    assert(init->as.binary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_chained_comparison(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a > b and c < d\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_AND);
    assert(init->as.binary.left->type == EXPR_BINARY);
    assert(init->as.binary.right->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_chained_logical(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a or b or c\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_parser_operators_main(void)
{
    TEST_SECTION("Parser - Binary Arithmetic Operators");
    TEST_RUN("op_plus", test_parse_op_plus);
    TEST_RUN("op_minus", test_parse_op_minus);
    TEST_RUN("op_multiply", test_parse_op_multiply);
    TEST_RUN("op_divide", test_parse_op_divide);
    TEST_RUN("op_modulo", test_parse_op_modulo);

    TEST_SECTION("Parser - Binary Comparison Operators");
    TEST_RUN("op_equal_equal", test_parse_op_equal_equal);
    TEST_RUN("op_not_equal", test_parse_op_not_equal);
    TEST_RUN("op_less", test_parse_op_less);
    TEST_RUN("op_less_equal", test_parse_op_less_equal);
    TEST_RUN("op_greater", test_parse_op_greater);
    TEST_RUN("op_greater_equal", test_parse_op_greater_equal);

    TEST_SECTION("Parser - Binary Logical Operators");
    TEST_RUN("op_and", test_parse_op_and);
    TEST_RUN("op_or", test_parse_op_or);

    TEST_SECTION("Parser - Unary Operators");
    TEST_RUN("op_negate", test_parse_op_negate);
    TEST_RUN("op_not", test_parse_op_not);

    TEST_SECTION("Parser - Assignment Operators");
    TEST_RUN("assign_plus_equal", test_parse_assign_plus_equal);
    TEST_RUN("assign_minus_equal", test_parse_assign_minus_equal);
    TEST_RUN("assign_star_equal", test_parse_assign_star_equal);
    TEST_RUN("assign_slash_equal", test_parse_assign_slash_equal);
    TEST_RUN("assign_percent_equal", test_parse_assign_percent_equal);

    TEST_SECTION("Parser - Chained Operators");
    TEST_RUN("chained_add", test_parse_chained_add);
    TEST_RUN("chained_mul", test_parse_chained_mul);
    TEST_RUN("mixed_operators", test_parse_mixed_operators);
    TEST_RUN("chained_comparison", test_parse_chained_comparison);
    TEST_RUN("chained_logical", test_parse_chained_logical);
}
