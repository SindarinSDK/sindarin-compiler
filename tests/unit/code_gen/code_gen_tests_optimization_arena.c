/**
 * code_gen_tests_optimization_arena.c - Arena requirement analysis tests
 *
 * Tests for function and expression arena requirement detection.
 */

/* Test type_needs_arena through function_needs_arena */
static void test_function_needs_arena_primitives_only(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Create a simple function with only int parameters and return */
    Parameter params[2];
    Token param_name1, param_name2;
    init_token(&param_name1, TOKEN_IDENTIFIER, "a");
    init_token(&param_name2, TOKEN_IDENTIFIER, "b");
    params[0].name = param_name1;
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;
    params[1].name = param_name2;
    params[1].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "add");

    /* Simple return a + b */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Expr *a_var = arena_alloc(&arena, sizeof(Expr));
    a_var->type = EXPR_VARIABLE;
    a_var->as.variable.name = param_name1;
    a_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *b_var = arena_alloc(&arena, sizeof(Expr));
    b_var->type = EXPR_VARIABLE;
    b_var->as.variable.name = param_name2;
    b_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *add_expr = make_binary_expr(&arena, a_var, TOKEN_PLUS, b_var);
    add_expr->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = add_expr;

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = params,
        .param_count = 2,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function with only primitives should NOT need arena */
    assert(function_needs_arena(&fn) == false);

    arena_free(&arena);
}

/* Test function_needs_arena with string return type */
static void test_function_needs_arena_string_return(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "get_string");

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));

    /* return "hello" */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Token str_tok;
    init_token(&str_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    LiteralValue lit_val;
    lit_val.string_value = "hello";
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *str_lit = ast_create_literal_expr(&arena, lit_val, str_type, false, &str_tok);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = str_lit;
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = NULL,
        .param_count = 0,
        .return_type = ast_create_primitive_type(&arena, TYPE_STRING),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function returning string should need arena */
    assert(function_needs_arena(&fn) == true);

    arena_free(&arena);
}

/* Test expr_needs_arena for various expression types */
static void test_expr_needs_arena_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    /* Literals don't need arena */
    Expr *int_lit = make_int_literal(&arena, 42);
    assert(expr_needs_arena(int_lit) == false);

    /* Variables don't need arena */
    Token var_name;
    init_token(&var_name, TOKEN_IDENTIFIER, "x");
    Expr *var_expr = ast_create_variable_expr(&arena, var_name, &var_name);
    assert(expr_needs_arena(var_expr) == false);

    /* Array literals need arena */
    Expr *arr = arena_alloc(&arena, sizeof(Expr));
    arr->type = EXPR_ARRAY;
    arr->as.array.elements = NULL;
    arr->as.array.element_count = 0;
    assert(expr_needs_arena(arr) == true);

    /* Interpolated strings need arena */
    Expr *interp = arena_alloc(&arena, sizeof(Expr));
    interp->type = EXPR_INTERPOLATED;
    interp->as.interpol.parts = NULL;
    interp->as.interpol.part_count = 0;
    assert(expr_needs_arena(interp) == true);

    /* Array slices need arena */
    Expr *slice = arena_alloc(&arena, sizeof(Expr));
    slice->type = EXPR_ARRAY_SLICE;
    assert(expr_needs_arena(slice) == true);

    /* Lambda expressions need arena */
    Expr *lambda = arena_alloc(&arena, sizeof(Expr));
    lambda->type = EXPR_LAMBDA;
    assert(expr_needs_arena(lambda) == true);

    arena_free(&arena);
}

static void test_code_gen_optimization_arena_main(void)
{
    TEST_RUN("function_needs_arena_primitives_only", test_function_needs_arena_primitives_only);
    TEST_RUN("function_needs_arena_string_return", test_function_needs_arena_string_return);
    TEST_RUN("expr_needs_arena_types", test_expr_needs_arena_types);
}
