/**
 * code_gen_tests_optimization_nonneg.c - is_provably_non_negative tests
 *
 * Tests for detecting provably non-negative expressions.
 */

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

static void test_code_gen_optimization_nonneg_main(void)
{
    TEST_RUN("is_provably_non_negative_int_literals", test_is_provably_non_negative_int_literals);
    TEST_RUN("is_provably_non_negative_long_literals", test_is_provably_non_negative_long_literals);
    TEST_RUN("is_provably_non_negative_negative_literals", test_is_provably_non_negative_negative_literals);
    TEST_RUN("is_provably_non_negative_untracked_variables", test_is_provably_non_negative_untracked_variables);
    TEST_RUN("is_provably_non_negative_tracked_loop_counters", test_is_provably_non_negative_tracked_loop_counters);
    TEST_RUN("is_provably_non_negative_other_expressions", test_is_provably_non_negative_other_expressions);
}
