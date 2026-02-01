/**
 * code_gen_tests_optimization_tailcall.c - Tail call and constant fold code gen tests
 *
 * Tests for tail call marking and constant folding code generation.
 */

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

static void test_code_gen_optimization_tailcall_main(void)
{
    TEST_RUN("function_has_marked_tail_calls_detection", test_function_has_marked_tail_calls_detection);
    TEST_RUN("try_constant_fold_binary_output", test_try_constant_fold_binary_output);
    TEST_RUN("try_constant_fold_unary_output", test_try_constant_fold_unary_output);
}
