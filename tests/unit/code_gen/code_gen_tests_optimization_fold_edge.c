/**
 * code_gen_tests_optimization_fold_edge.c - Constant folding edge case tests
 *
 * Tests for double edge cases, negative zero, deep nesting, logical operators, etc.
 */

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

static void test_code_gen_optimization_fold_edge_main(void)
{
    TEST_RUN("constant_fold_double_edge_cases", test_constant_fold_double_edge_cases);
    TEST_RUN("constant_fold_negative_zero", test_constant_fold_negative_zero);
    TEST_RUN("constant_fold_deep_nesting", test_constant_fold_deep_nesting);
    TEST_RUN("constant_fold_logical_operators", test_constant_fold_logical_operators);
    TEST_RUN("constant_fold_unary_negation_edge", test_constant_fold_unary_negation_edge);
    TEST_RUN("constant_fold_comparisons", test_constant_fold_comparisons);
    TEST_RUN("constant_fold_double_comparison_precision", test_constant_fold_double_comparison_precision);
}
