// tests/code_gen_tests_optimization.c
// Comprehensive tests for code generation optimizations
// Tests: constant folding, native operators, edge cases, and behavior verification

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include "../arena.h"
#include "../ast.h"
#include "../code_gen.h"
#include "../code_gen/code_gen_util.h"
#include "../code_gen/code_gen_stmt.h"
#include "../code_gen/code_gen_expr.h"
#include "../symbol_table.h"

/* Cross-platform null device */
#ifdef _WIN32
#define NULL_DEVICE "NUL"
#else
#define NULL_DEVICE "/dev/null"
#endif

/* Helper to set up a token */
static void init_token(Token *tok, SnTokenType type, const char *lexeme)
{
    tok->type = type;
    tok->start = lexeme;
    tok->length = strlen(lexeme);
    tok->line = 1;
    tok->filename = "test.sn";
}

/* Helper to create an int literal expression */
static Expr *make_int_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_INT_LITERAL, "0");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *int_type = ast_create_primitive_type(arena, TYPE_INT);
    return ast_create_literal_expr(arena, lit_val, int_type, false, &tok);
}

/* Helper to create a long literal expression */
static Expr *make_long_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_LONG_LITERAL, "0LL");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *long_type = ast_create_primitive_type(arena, TYPE_LONG);
    return ast_create_literal_expr(arena, lit_val, long_type, false, &tok);
}

/* Helper to create a double literal expression */
static Expr *make_double_literal(Arena *arena, double value)
{
    Token tok;
    init_token(&tok, TOKEN_DOUBLE_LITERAL, "0.0");

    LiteralValue lit_val;
    lit_val.double_value = value;
    Type *double_type = ast_create_primitive_type(arena, TYPE_DOUBLE);
    return ast_create_literal_expr(arena, lit_val, double_type, false, &tok);
}

/* Helper to create a bool literal expression */
static Expr *make_bool_literal(Arena *arena, bool value)
{
    Token tok;
    init_token(&tok, TOKEN_BOOL_LITERAL, value ? "true" : "false");

    LiteralValue lit_val;
    lit_val.bool_value = value ? 1 : 0;
    Type *bool_type = ast_create_primitive_type(arena, TYPE_BOOL);
    return ast_create_literal_expr(arena, lit_val, bool_type, false, &tok);
}

/* Helper to create a binary expression */
static Expr *make_binary_expr(Arena *arena, Expr *left, SnTokenType op, Expr *right)
{
    Token tok;
    init_token(&tok, op, "+");
    return ast_create_binary_expr(arena, left, op, right, &tok);
}

/* Helper to create a unary expression */
static Expr *make_unary_expr(Arena *arena, SnTokenType op, Expr *operand)
{
    Token tok;
    init_token(&tok, op, "-");
    return ast_create_unary_expr(arena, op, operand, &tok);
}

/* ============================================================================
 * CONSTANT FOLDING EDGE CASE TESTS
 * ============================================================================ */

/* Test integer overflow cases */
static void test_constant_fold_int_overflow(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create MAX + 1 - this will wrap around in C */
    Expr *left = make_long_literal(&arena, LLONG_MAX);
    Expr *right = make_long_literal(&arena, 1);
    Expr *add = make_binary_expr(&arena, left, TOKEN_PLUS, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Constant folding should succeed (C's undefined behavior is our undefined behavior) */
    bool success = try_fold_constant(add, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result wraps around in two's complement (this is implementation-defined) */
    assert(int_result == LLONG_MIN);  /* Wrapped around */

    arena_free(&arena);
}

/* Test integer underflow cases */
static void test_constant_fold_int_underflow(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create MIN - 1 - this will wrap around in C */
    Expr *left = make_long_literal(&arena, LLONG_MIN);
    Expr *right = make_long_literal(&arena, 1);
    Expr *sub = make_binary_expr(&arena, left, TOKEN_MINUS, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(sub, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result wraps around in two's complement */
    assert(int_result == LLONG_MAX);  /* Wrapped around */

    arena_free(&arena);
}

/* Test multiplication overflow */
static void test_constant_fold_mul_overflow(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create LLONG_MAX * 2 - will overflow */
    Expr *left = make_long_literal(&arena, LLONG_MAX);
    Expr *right = make_long_literal(&arena, 2);
    Expr *mul = make_binary_expr(&arena, left, TOKEN_STAR, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(mul, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result will have wrapped around */
    assert(int_result == -2);  /* LLONG_MAX * 2 overflows to -2 in two's complement */

    arena_free(&arena);
}

/* Test division by zero is NOT folded */
static void test_constant_fold_div_by_zero_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_int_literal(&arena, 10);
    Expr *right = make_int_literal(&arena, 0);
    Expr *div = make_binary_expr(&arena, left, TOKEN_SLASH, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Division by zero should NOT be folded */
    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test modulo by zero is NOT folded */
static void test_constant_fold_mod_by_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_int_literal(&arena, 10);
    Expr *right = make_int_literal(&arena, 0);
    Expr *mod = make_binary_expr(&arena, left, TOKEN_MODULO, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Modulo by zero should NOT be folded */
    bool success = try_fold_constant(mod, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test double division by zero is NOT folded */
static void test_constant_fold_div_by_zero_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_double_literal(&arena, 10.0);
    Expr *right = make_double_literal(&arena, 0.0);
    Expr *div = make_binary_expr(&arena, left, TOKEN_SLASH, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Division by zero should NOT be folded (even for doubles which would produce inf) */
    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test double edge cases */
static void test_constant_fold_double_edge_cases(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Test with DBL_MAX */
    Expr *max = make_double_literal(&arena, DBL_MAX);
    Expr *one = make_double_literal(&arena, 1.0);
    Expr *add = make_binary_expr(&arena, max, TOKEN_PLUS, one);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(add, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == DBL_MAX);  /* Adding 1 to DBL_MAX rounds back */

    /* Test with very small numbers */
    Expr *tiny = make_double_literal(&arena, DBL_MIN);
    Expr *two = make_double_literal(&arena, 2.0);
    Expr *div = make_binary_expr(&arena, tiny, TOKEN_SLASH, two);

    success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == DBL_MIN / 2.0);

    arena_free(&arena);
}

/* Test negative zero handling */
static void test_constant_fold_negative_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* -0.0 * positive = -0.0 */
    Expr *neg_zero = make_double_literal(&arena, -0.0);
    Expr *pos = make_double_literal(&arena, 5.0);
    Expr *mul = make_binary_expr(&arena, neg_zero, TOKEN_STAR, pos);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(mul, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    /* Result should be -0.0, which equals 0.0 numerically */
    assert(double_result == 0.0);

    arena_free(&arena);
}

/* Test deeply nested constant expressions */
static void test_constant_fold_deep_nesting(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Build ((((1 + 2) * 3) - 4) / 2) = ((3 * 3) - 4) / 2 = (9 - 4) / 2 = 5 / 2 = 2 */
    Expr *one = make_int_literal(&arena, 1);
    Expr *two = make_int_literal(&arena, 2);
    Expr *three = make_int_literal(&arena, 3);
    Expr *four = make_int_literal(&arena, 4);
    Expr *two2 = make_int_literal(&arena, 2);

    Expr *add = make_binary_expr(&arena, one, TOKEN_PLUS, two);       /* 1 + 2 = 3 */
    Expr *mul = make_binary_expr(&arena, add, TOKEN_STAR, three);     /* 3 * 3 = 9 */
    Expr *sub = make_binary_expr(&arena, mul, TOKEN_MINUS, four);     /* 9 - 4 = 5 */
    Expr *div = make_binary_expr(&arena, sub, TOKEN_SLASH, two2);     /* 5 / 2 = 2 */

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    assert(int_result == 2);

    arena_free(&arena);
}

/* Test logical operators in constant folding */
static void test_constant_fold_logical_operators(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Test true && true = true */
    Expr *t1 = make_bool_literal(&arena, true);
    Expr *t2 = make_bool_literal(&arena, true);
    Expr *and_tt = make_binary_expr(&arena, t1, TOKEN_AND, t2);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(and_tt, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    assert(int_result == 1);

    /* Test true && false = false */
    Expr *f1 = make_bool_literal(&arena, false);
    Expr *and_tf = make_binary_expr(&arena, t1, TOKEN_AND, f1);

    success = try_fold_constant(and_tf, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 0);

    /* Test false || true = true */
    Expr *or_ft = make_binary_expr(&arena, f1, TOKEN_OR, t1);
    success = try_fold_constant(or_ft, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 1);

    /* Test false || false = false */
    Expr *f2 = make_bool_literal(&arena, false);
    Expr *or_ff = make_binary_expr(&arena, f1, TOKEN_OR, f2);
    success = try_fold_constant(or_ff, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 0);

    arena_free(&arena);
}

/* Test unary negation edge cases */
static void test_constant_fold_unary_negation_edge(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Test -LLONG_MIN wraps to LLONG_MIN (undefined behavior in C, but we fold it) */
    Expr *min = make_long_literal(&arena, LLONG_MIN);
    Expr *neg = make_unary_expr(&arena, TOKEN_MINUS, min);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(neg, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* -LLONG_MIN overflows back to LLONG_MIN in two's complement */
    assert(int_result == LLONG_MIN);

    /* Test double negation */
    Expr *dbl = make_double_literal(&arena, -3.14);
    Expr *neg_dbl = make_unary_expr(&arena, TOKEN_MINUS, dbl);

    success = try_fold_constant(neg_dbl, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == 3.14);

    arena_free(&arena);
}

/* Test comparison operators */
static void test_constant_fold_comparisons(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *five = make_int_literal(&arena, 5);
    Expr *ten = make_int_literal(&arena, 10);
    Expr *five2 = make_int_literal(&arena, 5);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success;

    /* 5 < 10 = true */
    Expr *lt = make_binary_expr(&arena, five, TOKEN_LESS, ten);
    success = try_fold_constant(lt, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 <= 5 = true */
    Expr *le = make_binary_expr(&arena, five, TOKEN_LESS_EQUAL, five2);
    success = try_fold_constant(le, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 10 > 5 = true */
    Expr *gt = make_binary_expr(&arena, ten, TOKEN_GREATER, five);
    success = try_fold_constant(gt, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 >= 10 = false */
    Expr *ge = make_binary_expr(&arena, five, TOKEN_GREATER_EQUAL, ten);
    success = try_fold_constant(ge, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 0);

    /* 5 == 5 = true */
    Expr *eq = make_binary_expr(&arena, five, TOKEN_EQUAL_EQUAL, five2);
    success = try_fold_constant(eq, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 != 10 = true */
    Expr *ne = make_binary_expr(&arena, five, TOKEN_BANG_EQUAL, ten);
    success = try_fold_constant(ne, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    arena_free(&arena);
}

/* Test double comparisons with precision issues */
static void test_constant_fold_double_comparison_precision(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* 0.1 + 0.2 should be close to but not exactly 0.3 due to floating point */
    Expr *pt1 = make_double_literal(&arena, 0.1);
    Expr *pt2 = make_double_literal(&arena, 0.2);
    Expr *sum = make_binary_expr(&arena, pt1, TOKEN_PLUS, pt2);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(sum, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    /* The result is not exactly 0.3 due to IEEE 754 representation */
    assert(double_result > 0.29 && double_result < 0.31);

    arena_free(&arena);
}

/* ============================================================================
 * NATIVE OPERATOR TESTS
 * ============================================================================ */

/* Test native operator availability */
static void test_can_use_native_operator(void)
{
    /* Operators that can use native C */
    assert(can_use_native_operator(TOKEN_PLUS) == true);
    assert(can_use_native_operator(TOKEN_MINUS) == true);
    assert(can_use_native_operator(TOKEN_STAR) == true);
    assert(can_use_native_operator(TOKEN_EQUAL_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_BANG_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_LESS) == true);
    assert(can_use_native_operator(TOKEN_LESS_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_GREATER) == true);
    assert(can_use_native_operator(TOKEN_GREATER_EQUAL) == true);

    /* Operators that need runtime (division/modulo for zero check) */
    assert(can_use_native_operator(TOKEN_SLASH) == false);
    assert(can_use_native_operator(TOKEN_MODULO) == false);

    /* Unknown operators */
    assert(can_use_native_operator(TOKEN_DOT) == false);
    assert(can_use_native_operator(TOKEN_COMMA) == false);
}

/* Test get_native_c_operator returns correct strings */
static void test_get_native_c_operator(void)
{
    assert(strcmp(get_native_c_operator(TOKEN_PLUS), "+") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MINUS), "-") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_STAR), "*") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_SLASH), "/") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MODULO), "%") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_EQUAL_EQUAL), "==") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_BANG_EQUAL), "!=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS), "<") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS_EQUAL), "<=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER), ">") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER_EQUAL), ">=") == 0);

    /* Unknown operators return NULL */
    assert(get_native_c_operator(TOKEN_DOT) == NULL);
}

/* Test gen_native_arithmetic in unchecked mode */
static void test_gen_native_arithmetic_unchecked(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Test integer addition */
    char *result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_PLUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    /* Test integer subtraction */
    result = gen_native_arithmetic(&gen, "10LL", "4LL", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test integer multiplication */
    result = gen_native_arithmetic(&gen, "7LL", "6LL", TOKEN_STAR, int_type);
    assert(result != NULL);
    assert(strstr(result, "*") != NULL);

    /* Test division should return NULL (needs runtime for zero check) */
    result = gen_native_arithmetic(&gen, "20LL", "4LL", TOKEN_SLASH, int_type);
    assert(result == NULL);

    /* Test double addition */
    result = gen_native_arithmetic(&gen, "3.14", "2.0", TOKEN_PLUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_arithmetic in checked mode returns NULL */
static void test_gen_native_arithmetic_checked(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_CHECKED;  /* Default mode */

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* In checked mode, all operations should return NULL */
    char *result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_PLUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_MINUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5LL", "3LL", TOKEN_STAR, int_type);
    assert(result == NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_unary */
static void test_gen_native_unary(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Test integer negation */
    char *result = gen_native_unary(&gen, "42LL", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test double negation */
    result = gen_native_unary(&gen, "3.14", TOKEN_MINUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test logical not */
    result = gen_native_unary(&gen, "true", TOKEN_BANG, bool_type);
    assert(result != NULL);
    assert(strstr(result, "!") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * ARENA REQUIREMENT ANALYSIS TESTS
 * ============================================================================ */

/* Test type_needs_arena through function_needs_arena */
static void test_function_needs_arena_primitives_only(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create a simple function with only int parameters and return */
    Parameter params[2];
    Token param_name1, param_name2;
    init_token(&param_name1, TOKEN_IDENTIFIER, "a");
    init_token(&param_name2, TOKEN_IDENTIFIER, "b");
    params[0].name = param_name1;
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;
    params[1].name = param_name2;
    params[1].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "add");

    /* Simple return a + b */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Expr *a_var = arena_alloc(&arena, sizeof(Expr));
    a_var->type = EXPR_VARIABLE;
    a_var->as.variable.name = param_name1;
    a_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *b_var = arena_alloc(&arena, sizeof(Expr));
    b_var->type = EXPR_VARIABLE;
    b_var->as.variable.name = param_name2;
    b_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *add_expr = make_binary_expr(&arena, a_var, TOKEN_PLUS, b_var);
    add_expr->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = add_expr;

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = params,
        .param_count = 2,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function with only primitives should NOT need arena */
    assert(function_needs_arena(&fn) == false);

    arena_free(&arena);
}

/* Test function_needs_arena with string return type */
static void test_function_needs_arena_string_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "get_string");

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));

    /* return "hello" */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Token str_tok;
    init_token(&str_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    LiteralValue lit_val;
    lit_val.string_value = "hello";
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *str_lit = ast_create_literal_expr(&arena, lit_val, str_type, false, &str_tok);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = str_lit;
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = NULL,
        .param_count = 0,
        .return_type = ast_create_primitive_type(&arena, TYPE_STRING),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function returning string should need arena */
    assert(function_needs_arena(&fn) == true);

    arena_free(&arena);
}

/* Test expr_needs_arena for various expression types */
static void test_expr_needs_arena_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Literals don't need arena */
    Expr *int_lit = make_int_literal(&arena, 42);
    assert(expr_needs_arena(int_lit) == false);

    /* Variables don't need arena */
    Token var_name;
    init_token(&var_name, TOKEN_IDENTIFIER, "x");
    Expr *var_expr = ast_create_variable_expr(&arena, var_name, &var_name);
    assert(expr_needs_arena(var_expr) == false);

    /* Array literals need arena */
    Expr *arr = arena_alloc(&arena, sizeof(Expr));
    arr->type = EXPR_ARRAY;
    arr->as.array.elements = NULL;
    arr->as.array.element_count = 0;
    assert(expr_needs_arena(arr) == true);

    /* Interpolated strings need arena */
    Expr *interp = arena_alloc(&arena, sizeof(Expr));
    interp->type = EXPR_INTERPOLATED;
    interp->as.interpol.parts = NULL;
    interp->as.interpol.part_count = 0;
    assert(expr_needs_arena(interp) == true);

    /* Array slices need arena */
    Expr *slice = arena_alloc(&arena, sizeof(Expr));
    slice->type = EXPR_ARRAY_SLICE;
    assert(expr_needs_arena(slice) == true);

    /* Lambda expressions need arena */
    Expr *lambda = arena_alloc(&arena, sizeof(Expr));
    lambda->type = EXPR_LAMBDA;
    assert(expr_needs_arena(lambda) == true);

    arena_free(&arena);
}

/* ============================================================================
 * TAIL CALL MARKING VERIFICATION
 * ============================================================================ */

/* Test function_has_marked_tail_calls */
static void test_function_has_marked_tail_calls_detection(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "factorial");

    /* Create return factorial(n-1) with is_tail_call = true */
    Token var_tok;
    init_token(&var_tok, TOKEN_IDENTIFIER, "factorial");

    Expr *callee = arena_alloc(&arena, sizeof(Expr));
    callee->type = EXPR_VARIABLE;
    callee->as.variable.name = var_tok;

    Expr *call = arena_alloc(&arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = NULL;
    call->as.call.arg_count = 0;
    call->as.call.is_tail_call = true;  /* Marked as tail call */

    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = call;

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = NULL,
        .param_count = 0,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Should detect the marked tail call */
    assert(function_has_marked_tail_calls(&fn) == true);

    /* Now test without the mark */
    call->as.call.is_tail_call = false;
    assert(function_has_marked_tail_calls(&fn) == false);

    arena_free(&arena);
}

/* ============================================================================
 * CONSTANT FOLDING CODE GENERATION TESTS
 * ============================================================================ */

/* Test try_constant_fold_binary generates correct literals */
static void test_try_constant_fold_binary_output(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Test integer folding produces correct literal */
    Expr *left = make_int_literal(&arena, 5);
    Expr *right = make_int_literal(&arena, 3);

    BinaryExpr bin_expr;
    bin_expr.left = left;
    bin_expr.right = right;
    bin_expr.operator = TOKEN_PLUS;

    char *result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    assert(strcmp(result, "8LL") == 0);

    /* Test multiplication */
    bin_expr.operator = TOKEN_STAR;
    result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    assert(strcmp(result, "15LL") == 0);

    /* Test double folding */
    Expr *d_left = make_double_literal(&arena, 2.5);
    Expr *d_right = make_double_literal(&arena, 4.0);
    bin_expr.left = d_left;
    bin_expr.right = d_right;
    bin_expr.operator = TOKEN_STAR;

    result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    /* Should produce a double literal (10.0) */
    assert(strstr(result, "10") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test try_constant_fold_unary generates correct literals */
static void test_try_constant_fold_unary_output(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Test integer negation */
    Expr *operand = make_int_literal(&arena, 42);

    UnaryExpr unary_expr;
    unary_expr.operand = operand;
    unary_expr.operator = TOKEN_MINUS;

    char *result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "-42LL") == 0);

    /* Test logical not on true */
    Expr *bool_operand = make_bool_literal(&arena, true);
    unary_expr.operand = bool_operand;
    unary_expr.operator = TOKEN_BANG;

    result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "0LL") == 0);

    /* Test logical not on false */
    bool_operand = make_bool_literal(&arena, false);
    unary_expr.operand = bool_operand;

    result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "1LL") == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * LOOP COUNTER TRACKING TESTS
 * ============================================================================ */

/* Test basic push/pop/check for loop counter stack */
static void test_loop_counter_push_pop(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Initially empty - nothing tracked */
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == false);
    assert(is_tracked_loop_counter(&gen, "i") == false);
    assert(gen.loop_counter_count == 0);

    /* Push first counter */
    push_loop_counter(&gen, "__idx_0__");
    assert(gen.loop_counter_count == 1);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == false);

    /* Push second counter */
    push_loop_counter(&gen, "__idx_1__");
    assert(gen.loop_counter_count == 2);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == true);

    /* Pop second counter */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 1);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);
    assert(is_tracked_loop_counter(&gen, "__idx_1__") == false);

    /* Pop first counter */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 0);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == false);

    /* Pop on empty stack should be safe */
    pop_loop_counter(&gen);
    assert(gen.loop_counter_count == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test loop counter stack grows when capacity is exceeded */
static void test_loop_counter_stack_growth(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Initial capacity should be 0 */
    assert(gen.loop_counter_capacity == 0);
    assert(gen.loop_counter_count == 0);

    /* Push many counters to force stack growth */
    char name[32];
    for (int i = 0; i < 20; i++)
    {
        snprintf(name, sizeof(name), "__idx_%d__", i);
        push_loop_counter(&gen, name);
    }

    assert(gen.loop_counter_count == 20);
    assert(gen.loop_counter_capacity >= 20);  /* Should have grown */

    /* Verify all counters are tracked */
    for (int i = 0; i < 20; i++)
    {
        snprintf(name, sizeof(name), "__idx_%d__", i);
        assert(is_tracked_loop_counter(&gen, name) == true);
    }

    /* Counter not in stack */
    assert(is_tracked_loop_counter(&gen, "__idx_99__") == false);

    /* Pop all and verify */
    for (int i = 0; i < 20; i++)
    {
        pop_loop_counter(&gen);
    }
    assert(gen.loop_counter_count == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_tracked_loop_counter with NULL input */
static void test_loop_counter_null_check(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* NULL should always return false */
    assert(is_tracked_loop_counter(&gen, NULL) == false);

    /* Even with items in stack, NULL returns false */
    push_loop_counter(&gen, "__idx_0__");
    assert(is_tracked_loop_counter(&gen, NULL) == false);
    assert(is_tracked_loop_counter(&gen, "__idx_0__") == true);

    pop_loop_counter(&gen);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * IS_PROVABLY_NON_NEGATIVE TESTS
 * ============================================================================ */

/* Test is_provably_non_negative with non-negative integer literals */
static void test_is_provably_non_negative_int_literals(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Zero should be non-negative */
    Expr *zero = make_int_literal(&arena, 0);
    assert(is_provably_non_negative(&gen, zero) == true);

    /* Positive integers should be non-negative */
    Expr *positive = make_int_literal(&arena, 42);
    assert(is_provably_non_negative(&gen, positive) == true);

    /* Large positive should be non-negative */
    Expr *large = make_int_literal(&arena, 1000000);
    assert(is_provably_non_negative(&gen, large) == true);

    /* INT_MAX should be non-negative */
    Expr *int_max = make_int_literal(&arena, INT_MAX);
    assert(is_provably_non_negative(&gen, int_max) == true);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_provably_non_negative with non-negative long literals */
static void test_is_provably_non_negative_long_literals(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Zero should be non-negative */
    Expr *zero = make_long_literal(&arena, 0LL);
    assert(is_provably_non_negative(&gen, zero) == true);

    /* Positive longs should be non-negative */
    Expr *positive = make_long_literal(&arena, 42LL);
    assert(is_provably_non_negative(&gen, positive) == true);

    /* Large positive should be non-negative */
    Expr *large = make_long_literal(&arena, 9999999999L);
    assert(is_provably_non_negative(&gen, large) == true);

    /* LLONG_MAX should be non-negative */
    Expr *long_max = make_long_literal(&arena, LLONG_MAX);
    assert(is_provably_non_negative(&gen, long_max) == true);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_provably_non_negative with negative literals */
static void test_is_provably_non_negative_negative_literals(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Negative integers should NOT be non-negative */
    Expr *neg_int = make_int_literal(&arena, -1);
    assert(is_provably_non_negative(&gen, neg_int) == false);

    Expr *neg_int2 = make_int_literal(&arena, -42);
    assert(is_provably_non_negative(&gen, neg_int2) == false);

    /* INT_MIN should NOT be non-negative */
    Expr *int_min = make_int_literal(&arena, INT_MIN);
    assert(is_provably_non_negative(&gen, int_min) == false);

    /* Negative longs should NOT be non-negative */
    Expr *neg_long = make_long_literal(&arena, -1LL);
    assert(is_provably_non_negative(&gen, neg_long) == false);

    Expr *neg_long2 = make_long_literal(&arena, -9999999999L);
    assert(is_provably_non_negative(&gen, neg_long2) == false);

    /* LLONG_MIN should NOT be non-negative */
    Expr *long_min = make_long_literal(&arena, LLONG_MIN);
    assert(is_provably_non_negative(&gen, long_min) == false);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_provably_non_negative with variables (untracked) */
static void test_is_provably_non_negative_untracked_variables(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Untracked variable should NOT be non-negative */
    Token var_tok;
    init_token(&var_tok, TOKEN_IDENTIFIER, "x");
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    assert(is_provably_non_negative(&gen, var_expr) == false);

    /* Another untracked variable */
    Token idx_tok;
    init_token(&idx_tok, TOKEN_IDENTIFIER, "index");
    Expr *idx_expr = ast_create_variable_expr(&arena, idx_tok, &idx_tok);
    assert(is_provably_non_negative(&gen, idx_expr) == false);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_provably_non_negative with tracked loop counter variables */
static void test_is_provably_non_negative_tracked_loop_counters(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Push a loop counter */
    push_loop_counter(&gen, "__idx_0__");

    /* Tracked loop counter variable should be non-negative */
    Token idx_tok;
    init_token(&idx_tok, TOKEN_IDENTIFIER, "__idx_0__");
    Expr *idx_expr = ast_create_variable_expr(&arena, idx_tok, &idx_tok);
    assert(is_provably_non_negative(&gen, idx_expr) == true);

    /* Untracked variable still returns false */
    Token other_tok;
    init_token(&other_tok, TOKEN_IDENTIFIER, "__idx_1__");
    Expr *other_expr = ast_create_variable_expr(&arena, other_tok, &other_tok);
    assert(is_provably_non_negative(&gen, other_expr) == false);

    /* Pop the counter - now it should return false */
    pop_loop_counter(&gen);
    assert(is_provably_non_negative(&gen, idx_expr) == false);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test is_provably_non_negative with other expression types */
static void test_is_provably_non_negative_other_expressions(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, NULL_DEVICE);

    /* Double literals should return false (not valid array indices) */
    Expr *dbl = make_double_literal(&arena, 3.14);
    assert(is_provably_non_negative(&gen, dbl) == false);

    /* Bool literals should return false */
    Expr *bool_lit = make_bool_literal(&arena, true);
    assert(is_provably_non_negative(&gen, bool_lit) == false);

    /* Binary expressions should return false (even if operands are non-negative) */
    Expr *left = make_int_literal(&arena, 5);
    Expr *right = make_int_literal(&arena, 3);
    Expr *add = make_binary_expr(&arena, left, TOKEN_PLUS, right);
    assert(is_provably_non_negative(&gen, add) == false);

    /* Unary expressions should return false */
    Expr *operand = make_int_literal(&arena, 42);
    Expr *neg = make_unary_expr(&arena, TOKEN_MINUS, operand);
    assert(is_provably_non_negative(&gen, neg) == false);

    /* NULL expression should return false */
    assert(is_provably_non_negative(&gen, NULL) == false);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

void test_code_gen_optimization_main(void)
{
    TEST_SECTION("Code Gen Optimization Tests");

    /* Constant folding edge case tests */
    TEST_RUN("constant_fold_int_overflow", test_constant_fold_int_overflow);
    TEST_RUN("constant_fold_int_underflow", test_constant_fold_int_underflow);
    TEST_RUN("constant_fold_mul_overflow", test_constant_fold_mul_overflow);
    TEST_RUN("constant_fold_div_by_zero_int", test_constant_fold_div_by_zero_int);
    TEST_RUN("constant_fold_mod_by_zero", test_constant_fold_mod_by_zero);
    TEST_RUN("constant_fold_div_by_zero_double", test_constant_fold_div_by_zero_double);
    TEST_RUN("constant_fold_double_edge_cases", test_constant_fold_double_edge_cases);
    TEST_RUN("constant_fold_negative_zero", test_constant_fold_negative_zero);
    TEST_RUN("constant_fold_deep_nesting", test_constant_fold_deep_nesting);
    TEST_RUN("constant_fold_logical_operators", test_constant_fold_logical_operators);
    TEST_RUN("constant_fold_unary_negation_edge", test_constant_fold_unary_negation_edge);
    TEST_RUN("constant_fold_comparisons", test_constant_fold_comparisons);
    TEST_RUN("constant_fold_double_comparison_precision", test_constant_fold_double_comparison_precision);

    /* Native operator tests */
    TEST_RUN("can_use_native_operator", test_can_use_native_operator);
    TEST_RUN("get_native_c_operator", test_get_native_c_operator);
    TEST_RUN("gen_native_arithmetic_unchecked", test_gen_native_arithmetic_unchecked);
    TEST_RUN("gen_native_arithmetic_checked", test_gen_native_arithmetic_checked);
    TEST_RUN("gen_native_unary", test_gen_native_unary);

    /* Arena requirement tests */
    TEST_RUN("function_needs_arena_primitives_only", test_function_needs_arena_primitives_only);
    TEST_RUN("function_needs_arena_string_return", test_function_needs_arena_string_return);
    TEST_RUN("expr_needs_arena_types", test_expr_needs_arena_types);

    /* Tail call marking tests */
    TEST_RUN("function_has_marked_tail_calls_detection", test_function_has_marked_tail_calls_detection);

    /* Constant folding code generation tests */
    TEST_RUN("try_constant_fold_binary_output", test_try_constant_fold_binary_output);
    TEST_RUN("try_constant_fold_unary_output", test_try_constant_fold_unary_output);

    /* Loop counter tracking tests */
    TEST_RUN("loop_counter_push_pop", test_loop_counter_push_pop);
    TEST_RUN("loop_counter_stack_growth", test_loop_counter_stack_growth);
    TEST_RUN("loop_counter_null_check", test_loop_counter_null_check);

    /* is_provably_non_negative tests */
    TEST_RUN("is_provably_non_negative_int_literals", test_is_provably_non_negative_int_literals);
    TEST_RUN("is_provably_non_negative_long_literals", test_is_provably_non_negative_long_literals);
    TEST_RUN("is_provably_non_negative_negative_literals", test_is_provably_non_negative_negative_literals);
    TEST_RUN("is_provably_non_negative_untracked_variables", test_is_provably_non_negative_untracked_variables);
    TEST_RUN("is_provably_non_negative_tracked_loop_counters", test_is_provably_non_negative_tracked_loop_counters);
    TEST_RUN("is_provably_non_negative_other_expressions", test_is_provably_non_negative_other_expressions);
}
