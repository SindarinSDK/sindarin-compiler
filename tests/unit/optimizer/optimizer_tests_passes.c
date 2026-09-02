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

static void test_unused_checked_arithmetic_is_not_removed(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer checked_opt;
    optimizer_init(&checked_opt, &arena);

    SnTokenType arithmetic_ops[] = {
        TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO
    };
    for (int i = 0; i < 5; i++)
    {
        Stmt *checked_stmt = create_var_decl(&arena, "checked",
            create_binary_expr(&arena, create_int_literal(&arena, 1), arithmetic_ops[i],
                create_int_literal(&arena, 1)));
        Stmt *checked_stmts[] = { checked_stmt };
        int checked_count = 1;

        assert(remove_unused_variables(&checked_opt, checked_stmts, &checked_count) == 0);
        assert(checked_count == 1);
    }

    Stmt *pure_stmt = create_var_decl(&arena, "pure", create_int_literal(&arena, 1));
    Stmt *pure_stmts[] = { pure_stmt };
    int pure_count = 1;

    assert(remove_unused_variables(&checked_opt, pure_stmts, &pure_count) == 1);
    assert(pure_count == 0);

    Expr *double_left = create_int_literal(&arena, 1);
    Expr *double_right = create_int_literal(&arena, 1);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    double_left->expr_type = double_type;
    double_right->expr_type = double_type;
    Expr *double_add = create_binary_expr(&arena, double_left, TOKEN_PLUS, double_right);
    double_add->expr_type = double_type;
    Stmt *double_stmt = create_var_decl(&arena, "double", double_add);
    Stmt *double_stmts[] = { double_stmt };
    int double_count = 1;

    assert(remove_unused_variables(&checked_opt, double_stmts, &double_count) == 1);
    assert(double_count == 0);

    Stmt *unchecked_stmt = create_var_decl(&arena, "unchecked",
        create_binary_expr(&arena, create_int_literal(&arena, 1), TOKEN_PLUS,
            create_int_literal(&arena, 1)));
    Stmt *unchecked_stmts[] = { unchecked_stmt };
    int unchecked_count = 1;
    Optimizer unchecked_opt;
    optimizer_init(&unchecked_opt, &arena);
    optimizer_set_checked_arithmetic(&unchecked_opt, false);

    assert(remove_unused_variables(&unchecked_opt, unchecked_stmts, &unchecked_count) == 1);
    assert(unchecked_count == 0);

    arena_free(&arena);
    DEBUG_INFO("Finished test_unused_checked_arithmetic_is_not_removed");
}

static void test_nested_observable_effects_are_not_removed(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer unchecked_opt;
    optimizer_init(&unchecked_opt, &arena);
    optimizer_set_checked_arithmetic(&unchecked_opt, false);

    Expr *nested_call = create_binary_expr(&arena,
        create_call_expr(&arena, "side_effect", NULL, 0), TOKEN_PLUS,
        create_int_literal(&arena, 1));
    Stmt *call_stmt = create_var_decl(&arena, "call", nested_call);
    Stmt *call_stmts[] = { call_stmt };
    int call_count = 1;
    assert(remove_unused_variables(&unchecked_opt, call_stmts, &call_count) == 0);
    assert(call_count == 1);

    Expr *static_call = arena_alloc(&arena, sizeof(Expr));
    memset(static_call, 0, sizeof(Expr));
    static_call->type = EXPR_STATIC_CALL;
    static_call->expr_type = ast_create_primitive_type(&arena, TYPE_INT);
    static_call->as.static_call.arguments = NULL;
    static_call->as.static_call.arg_count = 0;
    Expr *nested_static_call = create_binary_expr(&arena, static_call, TOKEN_PLUS,
        create_int_literal(&arena, 1));
    Stmt *static_stmt = create_var_decl(&arena, "static_call", nested_static_call);
    Stmt *static_stmts[] = { static_stmt };
    int static_count = 1;
    assert(remove_unused_variables(&unchecked_opt, static_stmts, &static_count) == 0);
    assert(static_count == 1);

    Expr *method_call = arena_alloc(&arena, sizeof(Expr));
    memset(method_call, 0, sizeof(Expr));
    method_call->type = EXPR_METHOD_CALL;
    method_call->expr_type = ast_create_primitive_type(&arena, TYPE_INT);
    method_call->as.method_call.object = NULL;
    method_call->as.method_call.args = NULL;
    method_call->as.method_call.arg_count = 0;
    Expr *nested_method_call = create_binary_expr(&arena, method_call, TOKEN_PLUS,
        create_int_literal(&arena, 1));
    Stmt *method_stmt = create_var_decl(&arena, "method_call", nested_method_call);
    Stmt *method_stmts[] = { method_stmt };
    int method_count = 1;
    assert(remove_unused_variables(&unchecked_opt, method_stmts, &method_count) == 0);
    assert(method_count == 1);

    StructMethod overloaded_method = {0};
    Expr *overloaded = create_binary_expr(&arena, create_int_literal(&arena, 1), TOKEN_PLUS,
        create_int_literal(&arena, 1));
    overloaded->as.binary.operator_method = &overloaded_method;
    Stmt *overloaded_stmt = create_var_decl(&arena, "overloaded", overloaded);
    Stmt *overloaded_stmts[] = { overloaded_stmt };
    int overloaded_count = 1;
    assert(remove_unused_variables(&unchecked_opt, overloaded_stmts, &overloaded_count) == 0);
    assert(overloaded_count == 1);

    arena_free(&arena);
    DEBUG_INFO("Finished test_nested_observable_effects_are_not_removed");
}
