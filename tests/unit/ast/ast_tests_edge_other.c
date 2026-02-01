// ast_tests_edge_other.c
// Unary, variable, statement, and module tests

/* ============================================================================
 * Unary Expression Tests
 * ============================================================================ */

static void test_ast_unary_negate(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 5};
    Token tok = create_dummy_token(&arena, "-");
    Expr *operand = ast_create_literal_expr(&arena, v, t, false, &tok);

    Expr *e = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_UNARY);
    assert(e->as.unary.operator == TOKEN_MINUS);
    assert(e->as.unary.operand == operand);

    cleanup_arena(&arena);
}

static void test_ast_unary_not(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_BOOL);
    LiteralValue v = {.bool_value = true};
    Token tok = create_dummy_token(&arena, "!");
    Expr *operand = ast_create_literal_expr(&arena, v, t, false, &tok);

    Expr *e = ast_create_unary_expr(&arena, TOKEN_BANG, operand, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_UNARY);
    assert(e->as.unary.operator == TOKEN_BANG);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Variable Expression Tests
 * ============================================================================ */

static void test_ast_variable_simple(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "x");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);
    assert(strncmp(e->as.variable.name.start, "x", 1) == 0);

    cleanup_arena(&arena);
}

static void test_ast_variable_underscore(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "_private");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);
    assert(strncmp(e->as.variable.name.start, "_private", 8) == 0);

    cleanup_arena(&arena);
}

static void test_ast_variable_long_name(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "very_long_variable_name_123");
    Expr *e = ast_create_variable_expr(&arena, tok, &tok);

    assert(e != NULL);
    assert(e->type == EXPR_VARIABLE);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Statement Tests
 * ============================================================================ */

static void test_ast_expr_stmt(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token tok = create_dummy_token(&arena, "42");
    Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);

    Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);

    assert(s != NULL);
    assert(s->type == STMT_EXPR);
    assert(s->as.expression.expression == expr);

    cleanup_arena(&arena);
}

static void test_ast_return_stmt_with_value(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 0};
    Token tok = create_dummy_token(&arena, "return");
    Expr *val = ast_create_literal_expr(&arena, v, t, false, &tok);

    Stmt *s = ast_create_return_stmt(&arena, tok, val, &tok);

    assert(s != NULL);
    assert(s->type == STMT_RETURN);
    assert(s->as.return_stmt.value == val);

    cleanup_arena(&arena);
}

static void test_ast_return_stmt_void(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "return");
    Stmt *s = ast_create_return_stmt(&arena, tok, NULL, &tok);

    assert(s != NULL);
    assert(s->type == STMT_RETURN);
    assert(s->as.return_stmt.value == NULL);

    cleanup_arena(&arena);
}

static void test_ast_block_stmt_empty(void)
{
    Arena arena;
    setup_arena(&arena);

    Token tok = create_dummy_token(&arena, "{");
    Stmt *s = ast_create_block_stmt(&arena, NULL, 0, &tok);

    assert(s != NULL);
    assert(s->type == STMT_BLOCK);
    assert(s->as.block.count == 0);

    cleanup_arena(&arena);
}

static void test_ast_block_stmt_with_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "{");
    Token name = create_dummy_token(&arena, "x");

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt *));
    stmts[0] = ast_create_var_decl_stmt(&arena, name, t, NULL, &name);

    Stmt *s = ast_create_block_stmt(&arena, stmts, 1, &tok);

    assert(s != NULL);
    assert(s->type == STMT_BLOCK);
    assert(s->as.block.count == 1);

    cleanup_arena(&arena);
}

static void test_ast_var_decl_no_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token name = create_dummy_token(&arena, "x");
    Stmt *s = ast_create_var_decl_stmt(&arena, name, t, NULL, &name);

    assert(s != NULL);
    assert(s->type == STMT_VAR_DECL);
    assert(s->as.var_decl.type == t);
    assert(s->as.var_decl.initializer == NULL);

    cleanup_arena(&arena);
}

static void test_ast_var_decl_with_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token name = create_dummy_token(&arena, "x");
    Expr *init = ast_create_literal_expr(&arena, v, t, false, &name);

    Stmt *s = ast_create_var_decl_stmt(&arena, name, t, init, &name);

    assert(s != NULL);
    assert(s->type == STMT_VAR_DECL);
    assert(s->as.var_decl.initializer == init);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Module Tests
 * ============================================================================ */

static void test_ast_module_init(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    assert(mod.filename != NULL);
    assert(strcmp(mod.filename, "test.sn") == 0);
    assert(mod.statement_count == 0);

    cleanup_arena(&arena);
}

static void test_ast_module_add_single_stmt(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue v = {.int_value = 42};
    Token tok = create_dummy_token(&arena, "42");
    Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);
    Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);

    ast_module_add_statement(&arena, &mod, s);

    assert(mod.statement_count == 1);
    assert(mod.statements[0] == s);

    cleanup_arena(&arena);
}

static void test_ast_module_add_multiple_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Type *t = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "42");
    for (int i = 0; i < 5; i++) {
        LiteralValue v = {.int_value = i};
        Expr *expr = ast_create_literal_expr(&arena, v, t, false, &tok);
        Stmt *s = ast_create_expr_stmt(&arena, expr, &tok);
        ast_module_add_statement(&arena, &mod, s);
    }

    assert(mod.statement_count == 5);

    cleanup_arena(&arena);
}
