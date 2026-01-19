// tests/ast_tests_stmt.c
// Statement-related AST tests

static void test_ast_create_expr_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *expr = ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc);
    Stmt *estmt = ast_create_expr_stmt(&arena, expr, loc);
    assert(estmt != NULL);
    assert(estmt->type == STMT_EXPR);
    assert(estmt->as.expression.expression == expr);
    assert(tokens_equal(estmt->token, loc));

    // NULL expr
    assert(ast_create_expr_stmt(&arena, NULL, loc) == NULL);

    // NULL loc
    Stmt *estmt_null_loc = ast_create_expr_stmt(&arena, expr, NULL);
    assert(estmt_null_loc != NULL);
    assert(estmt_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_var_decl_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "var");
    Token *loc = ast_clone_token(&arena, &name);
    Type *typ = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Expr *init = ast_create_literal_expr(&arena, (LiteralValue){.double_value = 3.14}, typ, false, loc);
    Stmt *decl = ast_create_var_decl_stmt(&arena, name, typ, init, loc);
    assert(decl != NULL);
    assert(decl->type == STMT_VAR_DECL);
    assert(strcmp(decl->as.var_decl.name.start, "var") == 0);
    assert(decl->as.var_decl.type == typ);
    assert(decl->as.var_decl.initializer == init);
    assert(tokens_equal(decl->token, loc));

    // No initializer
    Stmt *decl_no_init = ast_create_var_decl_stmt(&arena, name, typ, NULL, loc);
    assert(decl_no_init != NULL);
    assert(decl_no_init->as.var_decl.initializer == NULL);

    // NULL type is allowed for type inference
    Stmt *decl_null_type = ast_create_var_decl_stmt(&arena, name, NULL, init, loc);
    assert(decl_null_type != NULL);
    assert(decl_null_type->as.var_decl.type == NULL);
    assert(decl_null_type->as.var_decl.initializer == init);

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Stmt *decl_empty = ast_create_var_decl_stmt(&arena, empty_name, typ, init, loc);
    assert(decl_empty != NULL);
    assert(decl_empty->as.var_decl.name.length == 0);

    // NULL loc
    Stmt *decl_null_loc = ast_create_var_decl_stmt(&arena, name, typ, init, NULL);
    assert(decl_null_loc != NULL);
    assert(decl_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_function_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "func");
    Token *loc = ast_clone_token(&arena, &name);
    Parameter params[1];
    params[0].name = create_dummy_token(&arena, "p");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ret = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *body[1];
    body[0] = ast_create_return_stmt(&arena, create_dummy_token(&arena, "return"), NULL, loc);
    Stmt *fn = ast_create_function_stmt(&arena, name, params, 1, ret, body, 1, loc);
    assert(fn != NULL);
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "func") == 0);
    assert(fn->as.function.param_count == 1);
    assert(strcmp(fn->as.function.params[0].name.start, "p") == 0);
    assert(fn->as.function.params[0].type->kind == TYPE_INT);
    assert(fn->as.function.return_type == ret);
    assert(fn->as.function.body_count == 1);
    assert(fn->as.function.body[0] == body[0]);
    assert(tokens_equal(fn->token, loc));

    // Empty params and body
    Stmt *fn_empty = ast_create_function_stmt(&arena, name, NULL, 0, ret, NULL, 0, loc);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.params == NULL);
    assert(fn_empty->as.function.body_count == 0);
    assert(fn_empty->as.function.body == NULL);

    // NULL return type
    Stmt *fn_null_ret = ast_create_function_stmt(&arena, name, params, 1, NULL, body, 1, loc);
    assert(fn_null_ret != NULL);
    assert(fn_null_ret->as.function.return_type == NULL);

    // NULL params with count > 0 (code allocates and copies, but type could be NULL)
    Parameter null_params[1] = {{.name = create_dummy_token(&arena, "p"), .type = NULL}};
    Stmt *fn_null_param_type = ast_create_function_stmt(&arena, name, null_params, 1, ret, body, 1, loc);
    assert(fn_null_param_type != NULL);
    assert(fn_null_param_type->as.function.params[0].type == NULL);

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Stmt *fn_empty_name = ast_create_function_stmt(&arena, empty_name, params, 1, ret, body, 1, loc);
    assert(fn_empty_name != NULL);
    assert(fn_empty_name->as.function.name.length == 0);

    // NULL loc
    Stmt *fn_null_loc = ast_create_function_stmt(&arena, name, params, 1, ret, body, 1, NULL);
    assert(fn_null_loc != NULL);
    assert(fn_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_return_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token kw = create_dummy_token(&arena, "return");
    Token *loc = ast_clone_token(&arena, &kw);
    Expr *val = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *ret = ast_create_return_stmt(&arena, kw, val, loc);
    assert(ret != NULL);
    assert(ret->type == STMT_RETURN);
    assert(strcmp(ret->as.return_stmt.keyword.start, "return") == 0);
    assert(ret->as.return_stmt.value == val);
    assert(tokens_equal(ret->token, loc));

    // No value
    Stmt *ret_no_val = ast_create_return_stmt(&arena, kw, NULL, loc);
    assert(ret_no_val != NULL);
    assert(ret_no_val->as.return_stmt.value == NULL);

    // Empty keyword (though unlikely)
    Token empty_kw = create_dummy_token(&arena, "");
    Stmt *ret_empty_kw = ast_create_return_stmt(&arena, empty_kw, val, loc);
    assert(ret_empty_kw != NULL);
    assert(ret_empty_kw->as.return_stmt.keyword.length == 0);

    // NULL loc
    Stmt *ret_null_loc = ast_create_return_stmt(&arena, kw, val, NULL);
    assert(ret_null_loc != NULL);
    assert(ret_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_block_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *stmts[2];
    stmts[0] = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc), loc);
    stmts[1] = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "y"), loc), loc);
    Stmt *block = ast_create_block_stmt(&arena, stmts, 2, loc);
    assert(block != NULL);
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.count == 2);
    assert(block->as.block.statements[0] == stmts[0]);
    assert(block->as.block.statements[1] == stmts[1]);
    assert(tokens_equal(block->token, loc));

    // Empty block
    Stmt *block_empty = ast_create_block_stmt(&arena, NULL, 0, loc);
    assert(block_empty != NULL);
    assert(block_empty->as.block.count == 0);
    assert(block_empty->as.block.statements == NULL);

    // NULL statements with count > 0
    Stmt *block_null_stmts = ast_create_block_stmt(&arena, NULL, 2, loc);
    assert(block_null_stmts != NULL);
    assert(block_null_stmts->as.block.statements == NULL);
    assert(block_null_stmts->as.block.count == 2);

    // NULL loc
    Stmt *block_null_loc = ast_create_block_stmt(&arena, stmts, 2, NULL);
    assert(block_null_loc != NULL);
    assert(block_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_if_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *then = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *els = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then, els, loc);
    assert(if_stmt != NULL);
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition == cond);
    assert(if_stmt->as.if_stmt.then_branch == then);
    assert(if_stmt->as.if_stmt.else_branch == els);
    assert(tokens_equal(if_stmt->token, loc));

    // No else
    Stmt *if_no_else = ast_create_if_stmt(&arena, cond, then, NULL, loc);
    assert(if_no_else != NULL);
    assert(if_no_else->as.if_stmt.else_branch == NULL);

    // NULL cond or then
    assert(ast_create_if_stmt(&arena, NULL, then, els, loc) == NULL);
    assert(ast_create_if_stmt(&arena, cond, NULL, els, loc) == NULL);
    assert(ast_create_if_stmt(&arena, NULL, NULL, els, loc) == NULL);

    // NULL loc
    Stmt *if_null_loc = ast_create_if_stmt(&arena, cond, then, els, NULL);
    assert(if_null_loc != NULL);
    assert(if_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_while_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *body = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *wh = ast_create_while_stmt(&arena, cond, body, loc);
    assert(wh != NULL);
    assert(wh->type == STMT_WHILE);
    assert(wh->as.while_stmt.condition == cond);
    assert(wh->as.while_stmt.body == body);
    assert(tokens_equal(wh->token, loc));

    // NULL cond or body
    assert(ast_create_while_stmt(&arena, NULL, body, loc) == NULL);
    assert(ast_create_while_stmt(&arena, cond, NULL, loc) == NULL);
    assert(ast_create_while_stmt(&arena, NULL, NULL, loc) == NULL);

    // NULL loc
    Stmt *wh_null_loc = ast_create_while_stmt(&arena, cond, body, NULL);
    assert(wh_null_loc != NULL);
    assert(wh_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_for_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *init = ast_create_var_decl_stmt(&arena, create_dummy_token(&arena, "i"), ast_create_primitive_type(&arena, TYPE_INT), NULL, loc);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Expr *inc = ast_create_increment_expr(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc), loc);
    Stmt *body = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *fr = ast_create_for_stmt(&arena, init, cond, inc, body, loc);
    assert(fr != NULL);
    assert(fr->type == STMT_FOR);
    assert(fr->as.for_stmt.initializer == init);
    assert(fr->as.for_stmt.condition == cond);
    assert(fr->as.for_stmt.increment == inc);
    assert(fr->as.for_stmt.body == body);
    assert(tokens_equal(fr->token, loc));

    // Optional parts
    Stmt *fr_partial = ast_create_for_stmt(&arena, NULL, NULL, NULL, body, loc);
    assert(fr_partial != NULL);
    assert(fr_partial->as.for_stmt.initializer == NULL);
    assert(fr_partial->as.for_stmt.condition == NULL);
    assert(fr_partial->as.for_stmt.increment == NULL);

    // NULL body
    assert(ast_create_for_stmt(&arena, init, cond, inc, NULL, loc) == NULL);

    // NULL loc
    Stmt *fr_null_loc = ast_create_for_stmt(&arena, init, cond, inc, body, NULL);
    assert(fr_null_loc != NULL);
    assert(fr_null_loc->token == NULL);

    cleanup_arena(&arena);
}

static void test_ast_create_import_stmt()
{
    Arena arena;
    setup_arena(&arena);

    Token mod = create_dummy_token(&arena, "module");
    Token *loc = ast_clone_token(&arena, &mod);

    // Test import without namespace
    Stmt *imp = ast_create_import_stmt(&arena, mod, NULL, loc);
    assert(imp != NULL);
    assert(imp->type == STMT_IMPORT);
    assert(strcmp(imp->as.import.module_name.start, "module") == 0);
    assert(imp->as.import.module_name.length == 6);
    assert(imp->as.import.namespace == NULL);
    assert(tokens_equal(imp->token, loc));

    // Test import with namespace
    Token ns_token = create_dummy_token(&arena, "math");
    Stmt *imp_ns = ast_create_import_stmt(&arena, mod, &ns_token, loc);
    assert(imp_ns != NULL);
    assert(imp_ns->type == STMT_IMPORT);
    assert(strcmp(imp_ns->as.import.module_name.start, "module") == 0);
    assert(imp_ns->as.import.namespace != NULL);
    assert(strcmp(imp_ns->as.import.namespace->start, "math") == 0);
    assert(imp_ns->as.import.namespace->length == 4);

    // Empty module name
    Token empty_mod = create_dummy_token(&arena, "");
    Stmt *imp_empty = ast_create_import_stmt(&arena, empty_mod, NULL, loc);
    assert(imp_empty != NULL);
    assert(imp_empty->as.import.module_name.length == 0);
    assert(imp_empty->as.import.namespace == NULL);

    // NULL loc
    Stmt *imp_null_loc = ast_create_import_stmt(&arena, mod, NULL, NULL);
    assert(imp_null_loc != NULL);
    assert(imp_null_loc->token == NULL);

    cleanup_arena(&arena);
}

void test_ast_stmt_main()
{
    TEST_SECTION("AST Statement Tests");
    TEST_RUN("ast_create_expr_stmt", test_ast_create_expr_stmt);
    TEST_RUN("ast_create_var_decl_stmt", test_ast_create_var_decl_stmt);
    TEST_RUN("ast_create_function_stmt", test_ast_create_function_stmt);
    TEST_RUN("ast_create_return_stmt", test_ast_create_return_stmt);
    TEST_RUN("ast_create_block_stmt", test_ast_create_block_stmt);
    TEST_RUN("ast_create_if_stmt", test_ast_create_if_stmt);
    TEST_RUN("ast_create_while_stmt", test_ast_create_while_stmt);
    TEST_RUN("ast_create_for_stmt", test_ast_create_for_stmt);
    TEST_RUN("ast_create_import_stmt", test_ast_create_import_stmt);
}
