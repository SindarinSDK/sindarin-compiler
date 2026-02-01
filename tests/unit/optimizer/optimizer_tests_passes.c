// optimizer_tests_passes.c
// Tests for unreachable code removal, variable tracking, and full optimization passes

/* ============================================================================
 * Test: remove_unreachable_statements
 * ============================================================================ */

static void test_remove_unreachable_after_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: return 0; x = 5; (x = 5 should be removed) */
    Stmt **stmts = arena_alloc(&arena, 3 * sizeof(Stmt *));
    stmts[0] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    stmts[2] = create_expr_stmt(&arena, create_variable_expr(&arena, "y"));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 2);
    assert(count == 1);
    assert(stmts[0]->type == STMT_RETURN);

    arena_free(&arena);
    DEBUG_INFO("Finished test_remove_unreachable_after_return");
}

static void test_remove_unreachable_after_break(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: break; x = 5; (x = 5 should be removed) */
    Stmt **stmts = arena_alloc(&arena, 2 * sizeof(Stmt *));
    stmts[0] = arena_alloc(&arena, sizeof(Stmt));
    stmts[0]->type = STMT_BREAK;
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    int count = 2;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 1);
    assert(count == 1);
    assert(stmts[0]->type == STMT_BREAK);

    arena_free(&arena);
    DEBUG_INFO("Finished test_remove_unreachable_after_break");
}

static void test_no_unreachable_statements(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create: x = 5; y = 10; return 0; (no dead code) */
    Stmt **stmts = arena_alloc(&arena, 3 * sizeof(Stmt *));
    stmts[0] = create_expr_stmt(&arena, create_variable_expr(&arena, "x"));
    stmts[1] = create_expr_stmt(&arena, create_variable_expr(&arena, "y"));
    stmts[2] = create_return_stmt(&arena, create_int_literal(&arena, 0));
    int count = 3;

    int removed = remove_unreachable_statements(&opt, &stmts, &count);

    assert(removed == 0);
    assert(count == 3);

    arena_free(&arena);
    DEBUG_INFO("Finished test_no_unreachable_statements");
}

/* ============================================================================
 * Test: Variable usage tracking
 * ============================================================================ */

static void test_collect_used_variables(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: x + y */
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
    DEBUG_INFO("Finished test_collect_used_variables");
}

static void test_is_variable_used(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token vars[2];
    setup_basic_token(&vars[0], TOKEN_IDENTIFIER, "x");
    setup_basic_token(&vars[1], TOKEN_IDENTIFIER, "y");

    Token x_tok, z_tok;
    setup_basic_token(&x_tok, TOKEN_IDENTIFIER, "x");
    setup_basic_token(&z_tok, TOKEN_IDENTIFIER, "z");

    assert(is_variable_used(vars, 2, x_tok) == true);
    assert(is_variable_used(vars, 2, z_tok) == false);

    arena_free(&arena);
    DEBUG_INFO("Finished test_is_variable_used");
}

/* ============================================================================
 * Test: Full optimization passes
 * ============================================================================ */

static void test_optimizer_dead_code_elimination_function(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a function with:
       - var unused: int = 0  (unused variable - should be removed)
       - var x: int = 5       (used in return)
       - return x
       - var unreachable = 0  (unreachable - should be removed)
     */
    Stmt **body = arena_alloc(&arena, 4 * sizeof(Stmt *));
    body[0] = create_var_decl(&arena, "unused", create_int_literal(&arena, 0));
    body[1] = create_var_decl(&arena, "x", create_int_literal(&arena, 5));
    body[2] = create_return_stmt(&arena, create_variable_expr(&arena, "x"));
    body[3] = create_var_decl(&arena, "unreachable", create_int_literal(&arena, 0));

    FunctionStmt fn = {
        .body = body,
        .body_count = 4,
        .param_count = 0,
        .params = NULL,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "test_fn");

    optimizer_eliminate_dead_code_function(&opt, &fn);

    /* Should have removed unreachable code and unused variable */
    int stmts_removed, vars_removed, noops_removed;
    optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);

    assert(stmts_removed >= 1);  /* unreachable statement */
    assert(vars_removed >= 1);   /* unused variable */

    /* Final body should have 2 statements: var x and return x */
    assert(fn.body_count == 2);
    assert(fn.body[0]->type == STMT_VAR_DECL);
    assert(fn.body[1]->type == STMT_RETURN);

    arena_free(&arena);
    DEBUG_INFO("Finished test_optimizer_dead_code_elimination_function");
}

static void test_optimizer_noop_simplification(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a function with:
       - var x: int = y + 0   (should simplify to y)
       - return x
     */
    Expr *y = create_variable_expr(&arena, "y");
    Expr *zero = create_int_literal(&arena, 0);
    Expr *add = create_binary_expr(&arena, y, TOKEN_PLUS, zero);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));
    body[0] = create_var_decl(&arena, "x", add);
    body[1] = create_return_stmt(&arena, create_variable_expr(&arena, "x"));

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 0,
        .params = NULL,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "test_fn");

    optimizer_eliminate_dead_code_function(&opt, &fn);

    int stmts_removed, vars_removed, noops_removed;
    optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);

    assert(noops_removed >= 1);

    /* The initializer should now be simplified to just y */
    Expr *init = fn.body[0]->as.var_decl.initializer;
    assert(init->type == EXPR_VARIABLE);

    arena_free(&arena);
    DEBUG_INFO("Finished test_optimizer_noop_simplification");
}

