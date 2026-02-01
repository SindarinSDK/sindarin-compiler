/**
 * code_gen_tests_optimization_fold_overflow.c - Constant folding overflow tests
 *
 * Tests for integer overflow, underflow, and division by zero in constant folding.
 */

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

static void test_code_gen_optimization_fold_overflow_main(void)
{
    TEST_RUN("constant_fold_int_overflow", test_constant_fold_int_overflow);
    TEST_RUN("constant_fold_int_underflow", test_constant_fold_int_underflow);
    TEST_RUN("constant_fold_mul_overflow", test_constant_fold_mul_overflow);
    TEST_RUN("constant_fold_div_by_zero_int", test_constant_fold_div_by_zero_int);
    TEST_RUN("constant_fold_mod_by_zero", test_constant_fold_mod_by_zero);
    TEST_RUN("constant_fold_div_by_zero_double", test_constant_fold_div_by_zero_double);
}
