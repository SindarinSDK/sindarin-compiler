// parser_tests_edge_precedence.c
// Expression precedence and literal tests

/* ============================================================================
 * Expression Precedence Tests
 * ============================================================================ */

static void test_parse_precedence_mul_over_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 1 + 2 * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    /* Should parse as 1 + (2 * 3), so top-level is + */
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    assert(init->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_parens_override(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = (1 + 2) * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (1 + 2) * 3, so top-level is * */
    assert(init->as.binary.operator == TOKEN_STAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_comparison_lower(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = 1 + 2 > 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (1 + 2) > 3, so top-level is > */
    assert(init->as.binary.operator == TOKEN_GREATER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_and_lower_than_comparison(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a > b and c > d\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (a > b) and (c > d), so top-level is and */
    assert(init->as.binary.operator == TOKEN_AND);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_precedence_or_lower_than_and(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = a and b or c and d\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (a and b) or (c and d), so top-level is or */
    assert(init->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_unary_precedence(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = -2 * 3\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_BINARY);
    /* Should parse as (-2) * 3, top-level is * */
    assert(init->as.binary.operator == TOKEN_STAR);
    assert(init->as.binary.left->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_double_negation(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = --5\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);
    assert(init->as.unary.operator == TOKEN_MINUS);
    assert(init->as.unary.operand->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_not_and_comparison(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = not a > b\n";
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
 * Literal Tests
 * ============================================================================ */

static void test_parse_int_literal_zero(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 0\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.int_value == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_int_literal_negative(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = -42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_UNARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_double_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: double = 3.14\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_DOUBLE);
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_bool_true(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.bool_value == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_bool_false(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: bool = false\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(init->as.literal.value.bool_value == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_char_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: char = 'a'\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.type->kind == TYPE_CHAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_string_empty(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: str = \"\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(strcmp(init->as.literal.value.string_value, "") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parse_string_with_escapes(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: str = \"hello\\nworld\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *init = stmt->as.var_decl.initializer;
    assert(init->type == EXPR_LITERAL);
    assert(strstr(init->as.literal.value.string_value, "\n") != NULL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
