// tests/unit/parser/parser_tests_expressions.c
// Parser tests for expressions - various expression types and precedence

/* Include split modules */
#include "parser_tests_expr_literal.c"
#include "parser_tests_expr_binary.c"
#include "parser_tests_expr_unary.c"
#include "parser_tests_expr_call.c"

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
