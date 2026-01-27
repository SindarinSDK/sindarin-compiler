// tests/unit/parser/parser_tests_expressions.c
// Parser tests for expressions - various expression types and precedence

/* ============================================================================
 * Literal Expression Tests
 * ============================================================================ */

static void test_parser_int_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = 42\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.int_value == 42);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_negative_int_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = -42\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_UNARY);
    assert(stmt->as.var_decl.initializer->as.unary.operator == TOKEN_MINUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_long_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: long = 9223372036854775807\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_double_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: double = 3.14159\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_string_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var s: str = \"hello\"\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(strcmp(stmt->as.var_decl.initializer->as.literal.value.string_value, "hello") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_bool_true_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = true\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.bool_value == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_bool_false_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = false\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.bool_value == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_char_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var c: char = 'A'\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.char_value == 'A');

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_byte_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: byte = 255\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_nil_literal(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var p: *int = nil\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Binary Expression Tests
 * ============================================================================ */

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

/* ============================================================================
 * Assignment Expression Tests
 * ============================================================================ */

static void test_parser_assign_simple(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "x = 42\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_ASSIGN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_assign_expression(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "x = y + z\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_ASSIGN);
    assert(stmt->as.expression.expression->as.assign.value->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_array_assign(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "arr[0] = 42\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_INDEX_ASSIGN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Complex Expression Tests
 * ============================================================================ */

static void test_parser_complex_arithmetic(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = (a + b) * (c - d) / e\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_complex_logical(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = (x > 0 and y > 0) or (x < 0 and y < 0)\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_OR);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_mixed_expression(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var b: bool = arr[i] + foo(x) > 10\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_BINARY);
    assert(expr->as.binary.operator == TOKEN_GREATER);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Increment/Decrement Tests
 * ============================================================================ */

static void test_parser_increment(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "x++\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_INCREMENT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_decrement(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "x--\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_DECREMENT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Member Access Tests
 * ============================================================================ */

static void test_parser_member_access(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = obj.field\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->as.var_decl.initializer->type == EXPR_MEMBER_ACCESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_chained_member_access(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "var x: int = a.b.c\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    Expr *expr = stmt->as.var_decl.initializer;
    assert(expr->type == EXPR_MEMBER_ACCESS);
    assert(expr->as.member_access.object->type == EXPR_MEMBER_ACCESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

static void test_parser_method_call(void)
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "obj.method()\n");

    Module *module = parser_execute(&parser, "test.sn");
    assert(module != NULL);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression->type == EXPR_CALL);
    assert(stmt->as.expression.expression->as.call.callee->type == EXPR_MEMBER_ACCESS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_parser_expressions_main(void)
{
    TEST_SECTION("Parser Literal Expressions");
    TEST_RUN("parser_int_literal", test_parser_int_literal);
    TEST_RUN("parser_negative_int_literal", test_parser_negative_int_literal);
    TEST_RUN("parser_long_literal", test_parser_long_literal);
    TEST_RUN("parser_double_literal", test_parser_double_literal);
    TEST_RUN("parser_string_literal", test_parser_string_literal);
    TEST_RUN("parser_bool_true_literal", test_parser_bool_true_literal);
    TEST_RUN("parser_bool_false_literal", test_parser_bool_false_literal);
    TEST_RUN("parser_char_literal", test_parser_char_literal);
    TEST_RUN("parser_byte_literal", test_parser_byte_literal);
    TEST_RUN("parser_nil_literal", test_parser_nil_literal);

    TEST_SECTION("Parser Binary Expressions");
    TEST_RUN("parser_binary_add", test_parser_binary_add);
    TEST_RUN("parser_binary_subtract", test_parser_binary_subtract);
    TEST_RUN("parser_binary_multiply", test_parser_binary_multiply);
    TEST_RUN("parser_binary_divide", test_parser_binary_divide);
    TEST_RUN("parser_binary_modulo", test_parser_binary_modulo);
    TEST_RUN("parser_binary_equal", test_parser_binary_equal);
    TEST_RUN("parser_binary_not_equal", test_parser_binary_not_equal);
    TEST_RUN("parser_binary_less", test_parser_binary_less);
    TEST_RUN("parser_binary_greater", test_parser_binary_greater);
    TEST_RUN("parser_binary_less_equal", test_parser_binary_less_equal);
    TEST_RUN("parser_binary_greater_equal", test_parser_binary_greater_equal);
    TEST_RUN("parser_binary_and", test_parser_binary_and);
    TEST_RUN("parser_binary_or", test_parser_binary_or);

    TEST_SECTION("Parser Unary Expressions");
    TEST_RUN("parser_unary_minus", test_parser_unary_minus);
    TEST_RUN("parser_unary_not", test_parser_unary_not);
    TEST_RUN("parser_double_negation", test_parser_double_negation);

    TEST_SECTION("Parser Precedence");
    TEST_RUN("parser_precedence_mul_over_add", test_parser_precedence_mul_over_add);
    TEST_RUN("parser_precedence_div_over_sub", test_parser_precedence_div_over_sub);
    TEST_RUN("parser_precedence_comparison_over_logical", test_parser_precedence_comparison_over_logical);
    TEST_RUN("parser_left_associativity_add", test_parser_left_associativity_add);
    TEST_RUN("parser_parentheses_override", test_parser_parentheses_override);

    TEST_SECTION("Parser Call Expressions");
    TEST_RUN("parser_call_no_args", test_parser_call_no_args);
    TEST_RUN("parser_call_one_arg", test_parser_call_one_arg);
    TEST_RUN("parser_call_multiple_args", test_parser_call_multiple_args);
    TEST_RUN("parser_call_expression_args", test_parser_call_expression_args);
    TEST_RUN("parser_nested_calls", test_parser_nested_calls);

    TEST_SECTION("Parser Array Expressions");
    TEST_RUN("parser_array_empty", test_parser_array_empty);
    TEST_RUN("parser_array_single_element", test_parser_array_single_element);
    TEST_RUN("parser_array_multiple_elements", test_parser_array_multiple_elements);
    TEST_RUN("parser_array_access", test_parser_array_access);
    TEST_RUN("parser_array_nested_access", test_parser_array_nested_access);

    TEST_SECTION("Parser Assignment Expressions");
    TEST_RUN("parser_assign_simple", test_parser_assign_simple);
    TEST_RUN("parser_assign_expression", test_parser_assign_expression);
    TEST_RUN("parser_array_assign", test_parser_array_assign);

    TEST_SECTION("Parser Complex Expressions");
    TEST_RUN("parser_complex_arithmetic", test_parser_complex_arithmetic);
    TEST_RUN("parser_complex_logical", test_parser_complex_logical);
    TEST_RUN("parser_mixed_expression", test_parser_mixed_expression);

    TEST_SECTION("Parser Increment/Decrement");
    TEST_RUN("parser_increment", test_parser_increment);
    TEST_RUN("parser_decrement", test_parser_decrement);

    TEST_SECTION("Parser Member Access");
    TEST_RUN("parser_member_access", test_parser_member_access);
    TEST_RUN("parser_chained_member_access", test_parser_chained_member_access);
    TEST_RUN("parser_method_call", test_parser_method_call);
}
