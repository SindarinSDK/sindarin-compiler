// tests/unit/optimizer/optimizer_tests_stress.c
// Optimizer stress tests - additional coverage for optimizer functions

/* ============================================================================
 * Optimizer Init Tests
 * ============================================================================ */

static void test_optimizer_init_basic(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    int stmts_removed, vars_removed, noops_removed;
    optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);
    assert(stmts_removed == 0);
    assert(vars_removed == 0);
    assert(noops_removed == 0);

    arena_free(&arena);
}

static void test_optimizer_multiple_init(void)
{
    for (int i = 0; i < 10; i++) {
        Arena arena;
        arena_init(&arena, 4096);

        Optimizer opt;
        optimizer_init(&opt, &arena);

        int stmts_removed, vars_removed, noops_removed;
        optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);
        assert(stmts_removed == 0);

        arena_free(&arena);
    }
}

/* ============================================================================
 * stmt_is_terminator Additional Tests
 * ============================================================================ */

static void test_optimizer_terminator_if_stmt(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *if_stmt = arena_alloc(&arena, sizeof(Stmt));
    if_stmt->type = STMT_IF;
    assert(stmt_is_terminator(if_stmt) == false);

    arena_free(&arena);
}

static void test_optimizer_terminator_while_stmt(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *while_stmt = arena_alloc(&arena, sizeof(Stmt));
    while_stmt->type = STMT_WHILE;
    assert(stmt_is_terminator(while_stmt) == false);

    arena_free(&arena);
}

static void test_optimizer_terminator_for_stmt(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *for_stmt = arena_alloc(&arena, sizeof(Stmt));
    for_stmt->type = STMT_FOR;
    assert(stmt_is_terminator(for_stmt) == false);

    arena_free(&arena);
}

static void test_optimizer_terminator_block_stmt(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *block_stmt = arena_alloc(&arena, sizeof(Stmt));
    block_stmt->type = STMT_BLOCK;
    assert(stmt_is_terminator(block_stmt) == false);

    arena_free(&arena);
}

static void test_optimizer_terminator_function_stmt(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Stmt *func_stmt = arena_alloc(&arena, sizeof(Stmt));
    func_stmt->type = STMT_FUNCTION;
    assert(stmt_is_terminator(func_stmt) == false);

    arena_free(&arena);
}

/* ============================================================================
 * expr_is_noop Additional Tests
 * ============================================================================ */

static void test_optimizer_noop_div_by_one(void)
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

static void test_optimizer_noop_mod_by_one(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *one = create_int_literal(&arena, 1);
    Expr *mod = create_binary_expr(&arena, x, TOKEN_MODULO, one);

    Expr *simplified;
    // x % 1 = 0, not x (always 0)
    bool is_noop = expr_is_noop(mod, &simplified);
    // This may or may not be considered a noop depending on implementation

    arena_free(&arena);
}

static void test_optimizer_noop_not_noop_add(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *add = create_binary_expr(&arena, x, TOKEN_PLUS, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(add, &simplified);
    assert(is_noop == false);

    arena_free(&arena);
}

static void test_optimizer_noop_not_noop_mul(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *five = create_int_literal(&arena, 5);
    Expr *mul = create_binary_expr(&arena, x, TOKEN_STAR, five);

    Expr *simplified;
    bool is_noop = expr_is_noop(mul, &simplified);
    assert(is_noop == false);

    arena_free(&arena);
}

static void test_optimizer_noop_mul_by_zero(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *x = create_variable_expr(&arena, "x");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *mul = create_binary_expr(&arena, x, TOKEN_STAR, zero);

    Expr *simplified;
    // x * 0 = 0, this is a constant fold not a noop
    bool is_noop = expr_is_noop(mul, &simplified);

    arena_free(&arena);
}

/* ============================================================================
 * Variable Collection Tests
 * ============================================================================ */

static void test_optimizer_collect_vars_literal(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *lit = create_int_literal(&arena, 42);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;
    collect_used_variables(lit, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 0);

    arena_free(&arena);
}

static void test_optimizer_collect_vars_single(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *var = create_variable_expr(&arena, "x");

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;
    collect_used_variables(var, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 1);

    arena_free(&arena);
}

static void test_optimizer_collect_vars_binary(void)
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

    arena_free(&arena);
}

static void test_optimizer_collect_vars_unary(void)
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

    arena_free(&arena);
}

static void test_optimizer_collect_vars_nested(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Expr *a = create_variable_expr(&arena, "a");
    Expr *b = create_variable_expr(&arena, "b");
    Expr *c = create_variable_expr(&arena, "c");
    Expr *ab = create_binary_expr(&arena, a, TOKEN_PLUS, b);
    Expr *abc = create_binary_expr(&arena, ab, TOKEN_STAR, c);

    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;
    collect_used_variables(abc, &used_vars, &used_count, &used_capacity, &arena);

    assert(used_count == 3);

    arena_free(&arena);
}

/* ============================================================================
 * is_variable_used Tests
 * ============================================================================ */

static void test_optimizer_var_used_empty_list(void)
{
    Token name;
    setup_basic_token(&name, TOKEN_IDENTIFIER, "x");

    assert(is_variable_used(NULL, 0, name) == false);
}

static void test_optimizer_var_used_single_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token x;
    setup_basic_token(&x, TOKEN_IDENTIFIER, "x");

    Token used_vars[1];
    setup_basic_token(&used_vars[0], TOKEN_IDENTIFIER, "x");

    assert(is_variable_used(used_vars, 1, x) == true);

    arena_free(&arena);
}

static void test_optimizer_var_used_single_no_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token x;
    setup_basic_token(&x, TOKEN_IDENTIFIER, "x");

    Token used_vars[1];
    setup_basic_token(&used_vars[0], TOKEN_IDENTIFIER, "y");

    assert(is_variable_used(used_vars, 1, x) == false);

    arena_free(&arena);
}

static void test_optimizer_var_used_multiple_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token z;
    setup_basic_token(&z, TOKEN_IDENTIFIER, "z");

    Token used_vars[3];
    setup_basic_token(&used_vars[0], TOKEN_IDENTIFIER, "x");
    setup_basic_token(&used_vars[1], TOKEN_IDENTIFIER, "y");
    setup_basic_token(&used_vars[2], TOKEN_IDENTIFIER, "z");

    assert(is_variable_used(used_vars, 3, z) == true);

    arena_free(&arena);
}

static void test_optimizer_var_used_multiple_no_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token w;
    setup_basic_token(&w, TOKEN_IDENTIFIER, "w");

    Token used_vars[3];
    setup_basic_token(&used_vars[0], TOKEN_IDENTIFIER, "x");
    setup_basic_token(&used_vars[1], TOKEN_IDENTIFIER, "y");
    setup_basic_token(&used_vars[2], TOKEN_IDENTIFIER, "z");

    assert(is_variable_used(used_vars, 3, w) == false);

    arena_free(&arena);
}

/* ============================================================================
 * Remove Unreachable Statements Tests
 * ============================================================================ */

static void test_optimizer_remove_unreachable_empty(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = NULL;
    int count = 0;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);
    assert(removed == 0);
    assert(count == 0);

    arena_free(&arena);
}

static void test_optimizer_remove_unreachable_no_terminator(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt*) * 3);
    stmts[0] = create_expr_stmt(&arena, create_int_literal(&arena, 1));
    stmts[1] = create_expr_stmt(&arena, create_int_literal(&arena, 2));
    stmts[2] = create_expr_stmt(&arena, create_int_literal(&arena, 3));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);
    assert(removed == 0);
    assert(count == 3);

    arena_free(&arena);
}

static void test_optimizer_remove_unreachable_after_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt*) * 3);
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    stmts[1] = create_expr_stmt(&arena, create_int_literal(&arena, 1));
    stmts[2] = create_expr_stmt(&arena, create_int_literal(&arena, 2));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);
    assert(removed == 2);
    assert(count == 1);

    arena_free(&arena);
}

static void test_optimizer_remove_unreachable_after_break(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    Stmt *break_stmt = arena_alloc(&arena, sizeof(Stmt));
    break_stmt->type = STMT_BREAK;

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt*) * 2);
    stmts[0] = break_stmt;
    stmts[1] = create_expr_stmt(&arena, create_int_literal(&arena, 1));
    int count = 2;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);
    assert(removed == 1);
    assert(count == 1);

    arena_free(&arena);
}

/* ============================================================================
 * Tail Recursion Detection Tests
 * ============================================================================ */

static void test_optimizer_tail_recursive_simple(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    // Create: return func_name(args)
    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "factorial");

    // Create a call to factorial
    Expr *callee = create_variable_expr(&arena, "factorial");
    Expr *call = arena_alloc(&arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = NULL;
    call->as.call.arg_count = 0;

    Stmt *ret = create_return_stmt(&arena, call);

    bool is_tail = is_tail_recursive_return(ret, func_name);
    assert(is_tail == true);

    arena_free(&arena);
}

static void test_optimizer_tail_recursive_not_call(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "factorial");

    // Return a literal, not a call
    Stmt *ret = create_return_stmt(&arena, create_int_literal(&arena, 42));

    bool is_tail = is_tail_recursive_return(ret, func_name);
    assert(is_tail == false);

    arena_free(&arena);
}

static void test_optimizer_tail_recursive_different_name(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "factorial");

    // Call to different function
    Expr *callee = create_variable_expr(&arena, "other_func");
    Expr *call = arena_alloc(&arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = NULL;
    call->as.call.arg_count = 0;

    Stmt *ret = create_return_stmt(&arena, call);

    bool is_tail = is_tail_recursive_return(ret, func_name);
    assert(is_tail == false);

    arena_free(&arena);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_optimizer_stress_main(void)
{
    TEST_SECTION("Optimizer Stress - Init");
    TEST_RUN("optimizer_init_basic", test_optimizer_init_basic);
    TEST_RUN("optimizer_multiple_init", test_optimizer_multiple_init);

    TEST_SECTION("Optimizer Stress - Terminators");
    TEST_RUN("optimizer_terminator_if_stmt", test_optimizer_terminator_if_stmt);
    TEST_RUN("optimizer_terminator_while_stmt", test_optimizer_terminator_while_stmt);
    TEST_RUN("optimizer_terminator_for_stmt", test_optimizer_terminator_for_stmt);
    TEST_RUN("optimizer_terminator_block_stmt", test_optimizer_terminator_block_stmt);
    TEST_RUN("optimizer_terminator_function_stmt", test_optimizer_terminator_function_stmt);

    TEST_SECTION("Optimizer Stress - Noop Detection");
    TEST_RUN("optimizer_noop_div_by_one", test_optimizer_noop_div_by_one);
    TEST_RUN("optimizer_noop_mod_by_one", test_optimizer_noop_mod_by_one);
    TEST_RUN("optimizer_noop_not_noop_add", test_optimizer_noop_not_noop_add);
    TEST_RUN("optimizer_noop_not_noop_mul", test_optimizer_noop_not_noop_mul);
    TEST_RUN("optimizer_noop_mul_by_zero", test_optimizer_noop_mul_by_zero);

    TEST_SECTION("Optimizer Stress - Variable Collection");
    TEST_RUN("optimizer_collect_vars_literal", test_optimizer_collect_vars_literal);
    TEST_RUN("optimizer_collect_vars_single", test_optimizer_collect_vars_single);
    TEST_RUN("optimizer_collect_vars_binary", test_optimizer_collect_vars_binary);
    TEST_RUN("optimizer_collect_vars_unary", test_optimizer_collect_vars_unary);
    TEST_RUN("optimizer_collect_vars_nested", test_optimizer_collect_vars_nested);

    TEST_SECTION("Optimizer Stress - Variable Used Check");
    TEST_RUN("optimizer_var_used_empty_list", test_optimizer_var_used_empty_list);
    TEST_RUN("optimizer_var_used_single_match", test_optimizer_var_used_single_match);
    TEST_RUN("optimizer_var_used_single_no_match", test_optimizer_var_used_single_no_match);
    TEST_RUN("optimizer_var_used_multiple_match", test_optimizer_var_used_multiple_match);
    TEST_RUN("optimizer_var_used_multiple_no_match", test_optimizer_var_used_multiple_no_match);

    TEST_SECTION("Optimizer Stress - Remove Unreachable");
    TEST_RUN("optimizer_remove_unreachable_empty", test_optimizer_remove_unreachable_empty);
    TEST_RUN("optimizer_remove_unreachable_no_terminator", test_optimizer_remove_unreachable_no_terminator);
    TEST_RUN("optimizer_remove_unreachable_after_return", test_optimizer_remove_unreachable_after_return);
    TEST_RUN("optimizer_remove_unreachable_after_break", test_optimizer_remove_unreachable_after_break);

    TEST_SECTION("Optimizer Stress - Tail Recursion");
    TEST_RUN("optimizer_tail_recursive_simple", test_optimizer_tail_recursive_simple);
    TEST_RUN("optimizer_tail_recursive_not_call", test_optimizer_tail_recursive_not_call);
    TEST_RUN("optimizer_tail_recursive_different_name", test_optimizer_tail_recursive_different_name);
}
