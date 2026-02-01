// optimizer_tests_terminator.c
// Tests for statement terminator detection and no-op expression detection

/* ============================================================================
 * Test: stmt_is_terminator
 * ============================================================================ */

static void test_stmt_is_terminator_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *ret_stmt = create_return_stmt(&arena, create_int_literal(&arena, 0));
    assert(stmt_is_terminator(ret_stmt) == true);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_return");
}

static void test_stmt_is_terminator_break_continue(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *break_stmt = arena_alloc(&arena, sizeof(Stmt));
    break_stmt->type = STMT_BREAK;
    assert(stmt_is_terminator(break_stmt) == true);

    Stmt *continue_stmt = arena_alloc(&arena, sizeof(Stmt));
    continue_stmt->type = STMT_CONTINUE;
    assert(stmt_is_terminator(continue_stmt) == true);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_break_continue");
}

static void test_stmt_is_terminator_non_terminator(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *expr_stmt = create_expr_stmt(&arena, create_int_literal(&arena, 42));
    assert(stmt_is_terminator(expr_stmt) == false);

    Stmt *var_decl = create_var_decl(&arena, "x", create_int_literal(&arena, 5));
    assert(stmt_is_terminator(var_decl) == false);

    arena_free(&arena);
    DEBUG_INFO("Finished test_stmt_is_terminator_non_terminator");
}

/* ============================================================================
 * Test: expr_is_noop
 * ============================================================================ */

static void test_expr_is_noop_add_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, zero);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test 0 + x */
    Expr *add2 = create_binary_expr(&arena, zero, TOKEN_PLUS, x);
    is_noop = expr_is_noop(add2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_add_zero");
}

static void test_expr_is_noop_sub_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *sub = create_binary_expr(&arena, x, TOKEN_MINUS, zero);

    Expr *simplified;
    bool is_noop = expr_is_noop(sub, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_sub_zero");
}

static void test_expr_is_noop_mul_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *one = create_int_literal(&arena, 1);
    Expr *mul = create_binary_expr(&arena, x, TOKEN_STAR, one);

    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test 1 * x */
    Expr *mul2 = create_binary_expr(&arena, one, TOKEN_STAR, x);
    is_noop = expr_is_noop(mul2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_mul_one");
}

static void test_expr_is_noop_div_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *one = create_int_literal(&arena, 1);
    Expr *div = create_binary_expr(&arena, x, TOKEN_SLASH, one);

    Expr *simplified;
    bool is_noop = expr_is_noop(div, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_div_one");
}

static void test_expr_is_noop_double_negation(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *neg1 = create_unary_expr(&arena, TOKEN_BANG, x);
    Expr *neg2 = create_unary_expr(&arena, TOKEN_BANG, neg1);

    Expr *simplified;
    bool is_noop = expr_is_noop(neg2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    /* Also test -(-x) */
    Expr *y = create_variable_expr(&arena, "y");
    Expr *minus1 = create_unary_expr(&arena, TOKEN_MINUS, y);
    Expr *minus2 = create_unary_expr(&arena, TOKEN_MINUS, minus1);

    is_noop = expr_is_noop(minus2, &simplified);
    assert(is_noop == true);
    assert(simplified == y);

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_double_negation");
}

static void test_expr_is_noop_not_noop(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == false);
    assert(simplified == add);  /* Not simplified */

    arena_free(&arena);
    DEBUG_INFO("Finished test_expr_is_noop_not_noop");
}

