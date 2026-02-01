// tests/unit/parser/parser_tests_expr_binary.c
// Parser tests for binary expressions

static void test_parser_binary_add(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 1 + 2\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_subtract(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 5 - 3\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_multiply(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 4 * 5\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_STAR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_divide(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 10 / 2\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_SLASH);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_modulo(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 10 % 3\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_MODULO);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x == y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_EQUAL_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_not_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x != y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_BANG_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_less(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x < y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_LESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_greater(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x > y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_GREATER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_less_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x <= y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_LESS_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_greater_equal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x >= y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_GREATER_EQUAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_and(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x and y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_AND);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_binary_or(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = x or y\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_BINARY);
    assert(stmt->as.var_decl.initializer->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}
