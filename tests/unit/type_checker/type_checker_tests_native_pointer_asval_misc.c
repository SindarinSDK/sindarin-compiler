/**
 * type_checker_tests_native_pointer_asval_misc.c - as_val rejection and cstr tests
 *
 * Tests for as_val rejecting non-pointers and *char to str conversion.
 */

/* Test that 'as val' rejects non-pointer operand (int as val should error) */
static void test_as_val_rejects_non_pointer(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: var n: int = 42 */
    Token n_tok;
    setup_test_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, lit, NULL);

    /* Create: var x: int = n as val (THIS SHOULD FAIL - n is int, not *int) */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token n_ref_tok;
    setup_test_token(&n_ref_tok, TOKEN_IDENTIFIER, "n", 2, "test.sn", &arena);
    Expr *n_ref = ast_create_variable_expr(&arena, n_ref_tok, &n_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, n_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {n_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: int as val is not allowed */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *char as val converts to str (null-terminated string) */
static void test_as_val_char_pointer_to_str(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_char_type = ast_create_pointer_type(&arena, char_type);

    /* Create: var p: *char = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_char_type, nil_lit, NULL);

    /* Create: var s: str = p as val */
    Token s_tok;
    setup_test_token(&s_tok, TOKEN_IDENTIFIER, "s", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Stmt *s_decl = ast_create_var_decl_stmt(&arena, s_tok, str_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, s_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *char as val => str */

    /* Verify the expression type is str */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_STRING);

    /* Verify the metadata flag is set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *int as val does NOT set is_cstr_to_str flag */
static void test_as_val_int_pointer_no_cstr_flag(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create: var x: int = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int as val => int */

    /* Verify the expression type is int */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_INT);

    /* Verify the metadata flag is NOT set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_native_pointer_asval_misc_main(void)
{
    TEST_RUN("as_val_rejects_non_pointer", test_as_val_rejects_non_pointer);
    TEST_RUN("as_val_char_pointer_to_str", test_as_val_char_pointer_to_str);
    TEST_RUN("as_val_int_pointer_no_cstr_flag", test_as_val_int_pointer_no_cstr_flag);
}
