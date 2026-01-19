// tests/code_gen_tests_constfold.c
// Tests for constant folding optimization in code generation

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../arena.h"
#include "../ast.h"
#include "../code_gen/code_gen_util.h"

/* Test is_constant_expr for literals */
static void test_is_constant_expr_literal(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "42";
    tok.length = 2;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Int literal should be constant */
    LiteralValue int_val = {.int_value = 42};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &tok);
    assert(is_constant_expr(int_lit) == true);

    /* Double literal should be constant */
    LiteralValue double_val = {.double_value = 3.14};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &tok);
    assert(is_constant_expr(double_lit) == true);

    /* Bool literal should be constant */
    LiteralValue bool_val = {.bool_value = 1};
    Expr *bool_lit = ast_create_literal_expr(&arena, bool_val, bool_type, false, &tok);
    assert(is_constant_expr(bool_lit) == true);

    /* String literal should NOT be constant (for folding purposes) */
    LiteralValue str_val = {.string_value = "hello"};
    Expr *str_lit = ast_create_literal_expr(&arena, str_val, string_type, false, &tok);
    assert(is_constant_expr(str_lit) == false);

    arena_free(&arena);
}

/* Test is_constant_expr for binary expressions */
static void test_is_constant_expr_binary(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "5";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 5 + 3 */
    LiteralValue left_val = {.int_value = 5};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 3};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *add_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &tok);
    assert(is_constant_expr(add_expr) == true);

    Expr *sub_expr = ast_create_binary_expr(&arena, left, TOKEN_MINUS, right, &tok);
    assert(is_constant_expr(sub_expr) == true);

    Expr *mul_expr = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, &tok);
    assert(is_constant_expr(mul_expr) == true);

    Expr *div_expr = ast_create_binary_expr(&arena, left, TOKEN_SLASH, right, &tok);
    assert(is_constant_expr(div_expr) == true);

    Expr *mod_expr = ast_create_binary_expr(&arena, left, TOKEN_MODULO, right, &tok);
    assert(is_constant_expr(mod_expr) == true);

    Expr *lt_expr = ast_create_binary_expr(&arena, left, TOKEN_LESS, right, &tok);
    assert(is_constant_expr(lt_expr) == true);

    arena_free(&arena);
}

/* Test is_constant_expr for unary expressions */
static void test_is_constant_expr_unary(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "5";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create -5 */
    LiteralValue val = {.int_value = 5};
    Expr *operand = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    Expr *neg_expr = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &tok);
    assert(is_constant_expr(neg_expr) == true);

    /* Create !true */
    LiteralValue bool_val = {.bool_value = 1};
    Expr *bool_operand = ast_create_literal_expr(&arena, bool_val, bool_type, false, &tok);
    Expr *not_expr = ast_create_unary_expr(&arena, TOKEN_BANG, bool_operand, &tok);
    assert(is_constant_expr(not_expr) == true);

    arena_free(&arena);
}

/* Test try_fold_constant for integer addition */
static void test_try_fold_constant_add(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "5";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 5 + 3 */
    LiteralValue left_val = {.int_value = 5};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 3};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *add_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(add_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == 8);

    arena_free(&arena);
}

/* Test try_fold_constant for multiplication */
static void test_try_fold_constant_mul(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "6";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 6 * 7 */
    LiteralValue left_val = {.int_value = 6};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 7};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *mul_expr = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(mul_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == 42);

    arena_free(&arena);
}

/* Test try_fold_constant for division */
static void test_try_fold_constant_div(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "100";
    tok.length = 3;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 100 / 5 */
    LiteralValue left_val = {.int_value = 100};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 5};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *div_expr = ast_create_binary_expr(&arena, left, TOKEN_SLASH, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(div_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == 20);

    arena_free(&arena);
}

/* Test try_fold_constant for division by zero (should not fold) */
static void test_try_fold_constant_div_zero(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "10";
    tok.length = 2;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 10 / 0 - should NOT fold */
    LiteralValue left_val = {.int_value = 10};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 0};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *div_expr = ast_create_binary_expr(&arena, left, TOKEN_SLASH, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(div_expr, &int_result, &double_result, &is_double);

    /* Division by zero should not be folded */
    assert(success == false);

    arena_free(&arena);
}

/* Test try_fold_constant for unary negation */
static void test_try_fold_constant_neg(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "42";
    tok.length = 2;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create -42 */
    LiteralValue val = {.int_value = 42};
    Expr *operand = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    Expr *neg_expr = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(neg_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == -42);

    arena_free(&arena);
}

/* Test try_fold_constant for double arithmetic */
static void test_try_fold_constant_double(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    Token tok;
    tok.type = TOKEN_DOUBLE_LITERAL;
    tok.start = "3.14";
    tok.length = 4;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 3.14 * 2.0 */
    LiteralValue left_val = {.double_value = 3.14};
    Expr *left = ast_create_literal_expr(&arena, left_val, double_type, false, &tok);

    LiteralValue right_val = {.double_value = 2.0};
    Expr *right = ast_create_literal_expr(&arena, right_val, double_type, false, &tok);

    Expr *mul_expr = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(mul_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == true);
    assert(double_result > 6.27 && double_result < 6.29);

    arena_free(&arena);
}

/* Test try_fold_constant for mixed int/double arithmetic */
static void test_try_fold_constant_mixed(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "5";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 5 + 3.5 (int + double) */
    LiteralValue left_val = {.int_value = 5};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.double_value = 3.5};
    Expr *right = ast_create_literal_expr(&arena, right_val, double_type, false, &tok);

    Expr *add_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(add_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == true);  /* Result should be promoted to double */
    assert(double_result == 8.5);

    arena_free(&arena);
}

/* Test try_fold_constant for comparison operators */
static void test_try_fold_constant_comparison(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "5";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create 5 < 10 */
    LiteralValue left_val = {.int_value = 5};
    Expr *left = ast_create_literal_expr(&arena, left_val, int_type, false, &tok);

    LiteralValue right_val = {.int_value = 10};
    Expr *right = ast_create_literal_expr(&arena, right_val, int_type, false, &tok);

    Expr *lt_expr = ast_create_binary_expr(&arena, left, TOKEN_LESS, right, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(lt_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);  /* Comparison result is integer (boolean) */
    assert(int_result == 1);     /* 5 < 10 is true */

    /* Create 10 < 5 */
    Expr *gt_expr = ast_create_binary_expr(&arena, right, TOKEN_LESS, left, &tok);
    success = try_fold_constant(gt_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == 0);  /* 10 < 5 is false */

    arena_free(&arena);
}

/* Test try_fold_constant for nested expressions */
static void test_try_fold_constant_nested(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token tok;
    tok.type = TOKEN_INT_LITERAL;
    tok.start = "2";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    /* Create (2 + 3) * 4 = 20 */
    LiteralValue val2 = {.int_value = 2};
    Expr *two = ast_create_literal_expr(&arena, val2, int_type, false, &tok);

    LiteralValue val3 = {.int_value = 3};
    Expr *three = ast_create_literal_expr(&arena, val3, int_type, false, &tok);

    Expr *add_expr = ast_create_binary_expr(&arena, two, TOKEN_PLUS, three, &tok);

    LiteralValue val4 = {.int_value = 4};
    Expr *four = ast_create_literal_expr(&arena, val4, int_type, false, &tok);

    Expr *mul_expr = ast_create_binary_expr(&arena, add_expr, TOKEN_STAR, four, &tok);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success = try_fold_constant(mul_expr, &int_result, &double_result, &is_double);

    assert(success == true);
    assert(is_double == false);
    assert(int_result == 20);

    arena_free(&arena);
}

/* Test that variable expressions are not constant */
static void test_is_constant_expr_variable(void)
{
    Arena arena;
    arena_init(&arena, 1024);

    Token tok;
    tok.type = TOKEN_IDENTIFIER;
    tok.start = "x";
    tok.length = 1;
    tok.line = 1;
    tok.filename = "test.sn";

    Expr *var_expr = ast_create_variable_expr(&arena, tok, &tok);
    assert(is_constant_expr(var_expr) == false);

    arena_free(&arena);
}

void test_code_gen_constfold_main(void)
{
    TEST_SECTION("Code Gen Constant Folding Tests");
    TEST_RUN("is_constant_expr_literal", test_is_constant_expr_literal);
    TEST_RUN("is_constant_expr_binary", test_is_constant_expr_binary);
    TEST_RUN("is_constant_expr_unary", test_is_constant_expr_unary);
    TEST_RUN("is_constant_expr_variable", test_is_constant_expr_variable);
    TEST_RUN("try_fold_constant_add", test_try_fold_constant_add);
    TEST_RUN("try_fold_constant_mul", test_try_fold_constant_mul);
    TEST_RUN("try_fold_constant_div", test_try_fold_constant_div);
    TEST_RUN("try_fold_constant_div_zero", test_try_fold_constant_div_zero);
    TEST_RUN("try_fold_constant_neg", test_try_fold_constant_neg);
    TEST_RUN("try_fold_constant_double", test_try_fold_constant_double);
    TEST_RUN("try_fold_constant_mixed", test_try_fold_constant_mixed);
    TEST_RUN("try_fold_constant_comparison", test_try_fold_constant_comparison);
    TEST_RUN("try_fold_constant_nested", test_try_fold_constant_nested);
}
