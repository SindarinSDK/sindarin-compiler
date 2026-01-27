// optimizer_tests_edge_cases.c
// Additional edge case tests for the optimizer

/* ============================================================================
 * Constant Folding Tests - Arithmetic
 * ============================================================================ */

static void test_const_fold_add_positives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, 10);
    Expr *right = create_int_literal(&arena, 20);
    Expr *add = create_binary_expr(&arena, left, TOKEN_PLUS, right);

    /* Both operands are literals - can be folded */
    assert(add->as.binary.left->type == EXPR_LITERAL);
    assert(add->as.binary.right->type == EXPR_LITERAL);

    arena_free(&arena);
}

static void test_const_fold_add_negatives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, -10);
    Expr *right = create_int_literal(&arena, -20);
    Expr *add = create_binary_expr(&arena, left, TOKEN_PLUS, right);

    assert(add->as.binary.left->as.literal.value.int_value == -10);
    assert(add->as.binary.right->as.literal.value.int_value == -20);

    arena_free(&arena);
}

static void test_const_fold_sub_positive(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, 30);
    Expr *right = create_int_literal(&arena, 10);
    Expr *sub = create_binary_expr(&arena, left, TOKEN_MINUS, right);

    assert(sub->as.binary.operator == TOKEN_MINUS);

    arena_free(&arena);
}

static void test_const_fold_mul_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, 100);
    Expr *zero = create_int_literal(&arena, 0);
    Expr *mul = create_binary_expr(&arena, left, TOKEN_STAR, zero);

    /* x * 0 pattern */
    assert(mul->as.binary.right->as.literal.value.int_value == 0);

    arena_free(&arena);
}

static void test_const_fold_mul_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, 42);
    Expr *one = create_int_literal(&arena, 1);
    Expr *mul = create_binary_expr(&arena, left, TOKEN_STAR, one);

    /* x * 1 is a no-op */
    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == true);

    arena_free(&arena);
}

static void test_const_fold_div_by_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = create_int_literal(&arena, 100);
    Expr *one = create_int_literal(&arena, 1);
    Expr *div = create_binary_expr(&arena, left, TOKEN_SLASH, one);

    /* x / 1 is a no-op */
    Expr *simplified;
    bool is_noop = expr_is_noop(div, &simplified);
    assert(is_noop == true);

    arena_free(&arena);
}

/* ============================================================================
 * Noop Detection Tests - More Cases
 * ============================================================================ */

static void test_noop_zero_plus_var(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *zero = create_int_literal(&arena, 0);
    Expr *x = create_variable_expr(&arena, "x");
    Expr *add = create_binary_expr(&arena, zero, TOKEN_PLUS, x);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
}

static void test_noop_var_plus_zero(void)
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

    arena_free(&arena);
}

static void test_noop_var_minus_zero(void)
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
}

static void test_noop_one_times_var(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *one = create_int_literal(&arena, 1);
    Expr *x = create_variable_expr(&arena, "x");
    Expr *mul = create_binary_expr(&arena, one, TOKEN_STAR, x);

    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
}

static void test_noop_var_times_one(void)
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

    arena_free(&arena);
}

static void test_noop_var_div_one(void)
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
}

static void test_noop_not_not_var(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *not1 = create_unary_expr(&arena, TOKEN_BANG, x);
    Expr *not2 = create_unary_expr(&arena, TOKEN_BANG, not1);

    Expr *simplified;
    bool is_noop = expr_is_noop(not2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
}

static void test_noop_neg_neg_var(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *neg1 = create_unary_expr(&arena, TOKEN_MINUS, x);
    Expr *neg2 = create_unary_expr(&arena, TOKEN_MINUS, neg1);

    Expr *simplified;
    bool is_noop = expr_is_noop(neg2, &simplified);
    assert(is_noop == true);
    assert(simplified == x);

    arena_free(&arena);
}

/* ============================================================================
 * Not A Noop Tests
 * ============================================================================ */

static void test_not_noop_add_nonzero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == false);
    assert(simplified == add);

    arena_free(&arena);
}

static void test_not_noop_sub_nonzero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *sub = create_binary_expr(&arena, x, TOKEN_MINUS, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(sub, &simplified);
    assert(is_noop == false);
    assert(simplified == sub);

    arena_free(&arena);
}

static void test_not_noop_mul_nonone(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *two = create_int_literal(&arena, 2);
    Expr *mul = create_binary_expr(&arena, x, TOKEN_STAR, two);

    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == false);
    assert(simplified == mul);

    arena_free(&arena);
}

static void test_not_noop_div_nonone(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *two = create_int_literal(&arena, 2);
    Expr *div = create_binary_expr(&arena, x, TOKEN_SLASH, two);

    Expr *simplified;
    bool is_noop = expr_is_noop(div, &simplified);
    assert(is_noop == false);
    assert(simplified == div);

    arena_free(&arena);
}

static void test_not_noop_single_negation(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *neg = create_unary_expr(&arena, TOKEN_MINUS, x);

    Expr *simplified;
    bool is_noop = expr_is_noop(neg, &simplified);
    assert(is_noop == false);
    assert(simplified == neg);

    arena_free(&arena);
}

static void test_not_noop_single_not(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *not_expr = create_unary_expr(&arena, TOKEN_BANG, x);

    Expr *simplified;
    bool is_noop = expr_is_noop(not_expr, &simplified);
    assert(is_noop == false);
    assert(simplified == not_expr);

    arena_free(&arena);
}

/* ============================================================================
 * Terminator Detection Tests
 * ============================================================================ */

static void test_terminator_return_value(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *ret = create_return_stmt(&arena, create_int_literal(&arena, 42));
    assert(stmt_is_terminator(ret) == true);

    arena_free(&arena);
}

static void test_terminator_return_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *ret = create_return_stmt(&arena, NULL);
    assert(stmt_is_terminator(ret) == true);

    arena_free(&arena);
}

static void test_terminator_break(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *stmt = arena_alloc(&arena, sizeof(Stmt));
    stmt->type = STMT_BREAK;
    assert(stmt_is_terminator(stmt) == true);

    arena_free(&arena);
}

static void test_terminator_continue(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *stmt = arena_alloc(&arena, sizeof(Stmt));
    stmt->type = STMT_CONTINUE;
    assert(stmt_is_terminator(stmt) == true);

    arena_free(&arena);
}

static void test_not_terminator_expr(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *stmt = create_expr_stmt(&arena, create_int_literal(&arena, 42));
    assert(stmt_is_terminator(stmt) == false);

    arena_free(&arena);
}

static void test_not_terminator_var_decl(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *stmt = create_var_decl(&arena, "x", create_int_literal(&arena, 0));
    assert(stmt_is_terminator(stmt) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Variable Usage Tracking Tests
 * ============================================================================ */

static void test_used_vars_literal(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *lit = create_int_literal(&arena, 42);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(lit, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 0);  /* Literals don't use variables */

    arena_free(&arena);
}

static void test_used_vars_single_var(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(x, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 1);
    assert(is_variable_used(used_vars, used_count, x->as.variable.name));

    arena_free(&arena);
}

static void test_used_vars_two_vars(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *y = create_variable_expr(&arena, "y");
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, y);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(add, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 2);
    assert(is_variable_used(used_vars, used_count, x->as.variable.name));
    assert(is_variable_used(used_vars, used_count, y->as.variable.name));

    arena_free(&arena);
}

static void test_used_vars_nested_expr(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *a = create_variable_expr(&arena, "a");
    Expr *b = create_variable_expr(&arena, "b");
    Expr *c = create_variable_expr(&arena, "c");
    Expr *add1 = create_binary_expr(&arena, a, TOKEN_PLUS, b);
    Expr *add2 = create_binary_expr(&arena, add1, TOKEN_PLUS, c);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(add2, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 3);
    assert(is_variable_used(used_vars, used_count, a->as.variable.name));
    assert(is_variable_used(used_vars, used_count, b->as.variable.name));
    assert(is_variable_used(used_vars, used_count, c->as.variable.name));

    arena_free(&arena);
}

static void test_used_vars_unary_expr(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *neg = create_unary_expr(&arena, TOKEN_MINUS, x);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    collect_used_variables(neg, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 1);
    assert(is_variable_used(used_vars, used_count, x->as.variable.name));

    arena_free(&arena);
}

static void test_is_var_used_empty(void)
{
    Token tok;
    setup_basic_token(&tok, TOKEN_IDENTIFIER, "x");

    assert(is_variable_used(NULL, 0, tok) == false);
}

static void test_is_var_used_not_found(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token vars[2];
    setup_basic_token(&vars[0], TOKEN_IDENTIFIER, "a");
    setup_basic_token(&vars[1], TOKEN_IDENTIFIER, "b");

    Token z_tok;
    setup_basic_token(&z_tok, TOKEN_IDENTIFIER, "z");

    assert(is_variable_used(vars, 2, z_tok) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Unreachable Code Tests
 * ============================================================================ */

static void test_unreachable_single_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, 1 * sizeof(Stmt *));
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    int count = 1;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 0);
    assert(count == 1);

    arena_free(&arena);
}

static void test_unreachable_two_returns(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, 2 * sizeof(Stmt *));
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    stmts[1] = create_return_stmt(&arena, create_int_literal(&arena, 1));
    int count = 2;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 1);
    assert(count == 1);

    arena_free(&arena);
}

static void test_unreachable_many_after_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, 5 * sizeof(Stmt *));
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "a"));
    stmts[2] = create_expr_stmt(&arena, create_variable_expr(&arena, "b"));
    stmts[3] = create_expr_stmt(&arena, create_variable_expr(&arena, "c"));
    stmts[4] = create_expr_stmt(&arena, create_variable_expr(&arena, "d"));
    int count = 5;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 4);
    assert(count == 1);

    arena_free(&arena);
}

static void test_unreachable_break_in_middle(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, 4 * sizeof(Stmt *));
    stmts[0] = create_expr_stmt(&arena, create_variable_expr(&arena, "a"));
    Stmt *break_stmt = arena_alloc(&arena, sizeof(Stmt));
    break_stmt->type = STMT_BREAK;
    stmts[1] = break_stmt;
    stmts[2] = create_expr_stmt(&arena, create_variable_expr(&arena, "b"));
    stmts[3] = create_expr_stmt(&arena, create_variable_expr(&arena, "c"));
    int count = 4;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 2);
    assert(count == 2);

    arena_free(&arena);
}

static void test_unreachable_no_terminator(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, 3 * sizeof(Stmt *));
    stmts[0] = create_expr_stmt(&arena, create_variable_expr(&arena, "a"));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "b"));
    stmts[2] = create_expr_stmt(&arena, create_variable_expr(&arena, "c"));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 0);
    assert(count == 3);

    arena_free(&arena);
}

/* ============================================================================
 * Optimizer Stats Tests
 * ============================================================================ */

static void test_optimizer_init_stats(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    int stmts, vars, noops;
    optimizer_get_stats(&opt, &stmts, &vars, &noops);

    assert(stmts == 0);
    assert(vars == 0);
    assert(noops == 0);

    arena_free(&arena);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void run_optimizer_edge_cases_tests(void)
{
    TEST_SECTION("Optimizer - Constant Folding");
    TEST_RUN("const_fold_add_positives", test_const_fold_add_positives);
    TEST_RUN("const_fold_add_negatives", test_const_fold_add_negatives);
    TEST_RUN("const_fold_sub_positive", test_const_fold_sub_positive);
    TEST_RUN("const_fold_mul_zero", test_const_fold_mul_zero);
    TEST_RUN("const_fold_mul_one", test_const_fold_mul_one);
    TEST_RUN("const_fold_div_by_one", test_const_fold_div_by_one);

    TEST_SECTION("Optimizer - Noop Detection");
    TEST_RUN("noop_zero_plus_var", test_noop_zero_plus_var);
    TEST_RUN("noop_var_plus_zero", test_noop_var_plus_zero);
    TEST_RUN("noop_var_minus_zero", test_noop_var_minus_zero);
    TEST_RUN("noop_one_times_var", test_noop_one_times_var);
    TEST_RUN("noop_var_times_one", test_noop_var_times_one);
    TEST_RUN("noop_var_div_one", test_noop_var_div_one);
    TEST_RUN("noop_not_not_var", test_noop_not_not_var);
    TEST_RUN("noop_neg_neg_var", test_noop_neg_neg_var);

    TEST_SECTION("Optimizer - Not A Noop");
    TEST_RUN("not_noop_add_nonzero", test_not_noop_add_nonzero);
    TEST_RUN("not_noop_sub_nonzero", test_not_noop_sub_nonzero);
    TEST_RUN("not_noop_mul_nonone", test_not_noop_mul_nonone);
    TEST_RUN("not_noop_div_nonone", test_not_noop_div_nonone);
    TEST_RUN("not_noop_single_negation", test_not_noop_single_negation);
    TEST_RUN("not_noop_single_not", test_not_noop_single_not);

    TEST_SECTION("Optimizer - Terminator Detection");
    TEST_RUN("terminator_return_value", test_terminator_return_value);
    TEST_RUN("terminator_return_void", test_terminator_return_void);
    TEST_RUN("terminator_break", test_terminator_break);
    TEST_RUN("terminator_continue", test_terminator_continue);
    TEST_RUN("not_terminator_expr", test_not_terminator_expr);
    TEST_RUN("not_terminator_var_decl", test_not_terminator_var_decl);

    TEST_SECTION("Optimizer - Variable Usage");
    TEST_RUN("used_vars_literal", test_used_vars_literal);
    TEST_RUN("used_vars_single_var", test_used_vars_single_var);
    TEST_RUN("used_vars_two_vars", test_used_vars_two_vars);
    TEST_RUN("used_vars_nested_expr", test_used_vars_nested_expr);
    TEST_RUN("used_vars_unary_expr", test_used_vars_unary_expr);
    TEST_RUN("is_var_used_empty", test_is_var_used_empty);
    TEST_RUN("is_var_used_not_found", test_is_var_used_not_found);

    TEST_SECTION("Optimizer - Unreachable Code");
    TEST_RUN("unreachable_single_return", test_unreachable_single_return);
    TEST_RUN("unreachable_two_returns", test_unreachable_two_returns);
    TEST_RUN("unreachable_many_after_return", test_unreachable_many_after_return);
    TEST_RUN("unreachable_break_in_middle", test_unreachable_break_in_middle);
    TEST_RUN("unreachable_no_terminator", test_unreachable_no_terminator);

    TEST_SECTION("Optimizer - Stats");
    TEST_RUN("optimizer_init_stats", test_optimizer_init_stats);
}
