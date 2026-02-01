// optimizer_tests_tail_call.c
// Tests for tail call detection and optimization

/* ============================================================================
 * Test: Tail Call Detection
 * ============================================================================ */

static void test_tail_call_detection_simple(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: return foo(x) */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_variable_expr(&arena, "x");
    Expr *call = create_call_expr(&arena, "foo", args, 1);
    Stmt *ret = create_return_stmt(&arena, call);

    /* Create a token for function name "foo" */
    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "foo");

    /* Should be a tail call */
    assert(is_tail_recursive_return(ret, func_name) == true);

    /* Not a tail call to a different function */
    Token other_name;
    setup_basic_token(&other_name, TOKEN_IDENTIFIER, "bar");
    assert(is_tail_recursive_return(ret, other_name) == false);

    arena_free(&arena);
}

static void test_tail_call_detection_not_tail(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create: return n * foo(x) - NOT a tail call */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_variable_expr(&arena, "x");
    Expr *call = create_call_expr(&arena, "foo", args, 1);
    Expr *n = create_variable_expr(&arena, "n");
    Expr *mul = create_binary_expr(&arena, n, TOKEN_STAR, call);
    Stmt *ret = create_return_stmt(&arena, mul);

    Token func_name;
    setup_basic_token(&func_name, TOKEN_IDENTIFIER, "foo");

    /* Should NOT be a tail call (wrapped in multiplication) */
    assert(is_tail_recursive_return(ret, func_name) == false);

    arena_free(&arena);
}

static void test_function_has_tail_recursion(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create a tail-recursive function:
       fn foo(n: int): int =>
           if n <= 0 => return 0
           return foo(n - 1)
     */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_binary_expr(&arena, create_variable_expr(&arena, "n"),
                                 TOKEN_MINUS, create_int_literal(&arena, 1));
    Expr *call = create_call_expr(&arena, "foo", args, 1);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));

    /* if statement */
    Stmt *if_stmt = arena_alloc(&arena, sizeof(Stmt));
    if_stmt->type = STMT_IF;
    if_stmt->as.if_stmt.condition = create_binary_expr(&arena,
        create_variable_expr(&arena, "n"), TOKEN_LESS_EQUAL, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.then_branch = create_return_stmt(&arena, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.else_branch = NULL;
    body[0] = if_stmt;

    /* return foo(n-1) */
    body[1] = create_return_stmt(&arena, call);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    setup_basic_token(&params[0].name, TOKEN_IDENTIFIER, "n");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 1,
        .params = params,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "foo");

    /* Should detect tail recursion */
    assert(function_has_tail_recursion(&fn) == true);

    arena_free(&arena);
}

static void test_tail_call_marking(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Optimizer opt;
    optimizer_init(&opt, &arena);

    /* Create a tail-recursive function */
    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = create_binary_expr(&arena, create_variable_expr(&arena, "n"),
                                 TOKEN_MINUS, create_int_literal(&arena, 1));
    Expr *call = create_call_expr(&arena, "foo", args, 1);

    /* Verify it's not marked yet */
    assert(call->as.call.is_tail_call == false);

    Stmt **body = arena_alloc(&arena, 2 * sizeof(Stmt *));

    Stmt *if_stmt = arena_alloc(&arena, sizeof(Stmt));
    if_stmt->type = STMT_IF;
    if_stmt->as.if_stmt.condition = create_binary_expr(&arena,
        create_variable_expr(&arena, "n"), TOKEN_LESS_EQUAL, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.then_branch = create_return_stmt(&arena, create_int_literal(&arena, 0));
    if_stmt->as.if_stmt.else_branch = NULL;
    body[0] = if_stmt;
    body[1] = create_return_stmt(&arena, call);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    setup_basic_token(&params[0].name, TOKEN_IDENTIFIER, "n");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;

    FunctionStmt fn = {
        .body = body,
        .body_count = 2,
        .param_count = 1,
        .params = params,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT)
    };
    setup_basic_token(&fn.name, TOKEN_IDENTIFIER, "foo");

    /* Mark tail calls */
    int marked = optimizer_mark_tail_calls(&opt, &fn);

    /* Should have marked 1 tail call */
    assert(marked == 1);
    assert(opt.tail_calls_optimized == 1);

    /* The call expression should now be marked */
    assert(call->as.call.is_tail_call == true);

    arena_free(&arena);
}

