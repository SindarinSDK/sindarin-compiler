/**
 * type_checker_tests_native_pointer_return.c - Pointer return value tests
 *
 * Tests for pointer return handling with and without 'as val'.
 */

/* Test that pointer return from native fn WITHOUT 'as val' fails in regular function */
static void test_pointer_return_without_as_val_fails_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() -- missing 'as val', should fail */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, get_ptr_call, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: pointer return without as val in regular function */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer return from native fn WITH 'as val' succeeds in regular function */
static void test_pointer_return_with_as_val_succeeds_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() as val -- with 'as val', should succeed */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 5, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, get_ptr_call, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: pointer return with as val in regular function */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native functions can store pointer return values without 'as val' */
static void test_native_fn_can_store_pointer_return(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var p: *int = get_ptr() -- allowed in native function */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, get_ptr_call, NULL);

    /* Wrap in native function */
    Stmt *native_body[1] = {p_decl};
    Token native_tok;
    setup_test_token(&native_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Stmt *native_func = ast_create_function_stmt(&arena, native_tok, NULL, 0, void_type, native_body, 1, &native_tok);
    native_func->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, native_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: native function can store pointer returns */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_native_pointer_return_main(void)
{
    TEST_RUN("pointer_return_without_as_val_fails_in_regular_fn", test_pointer_return_without_as_val_fails_in_regular_fn);
    TEST_RUN("pointer_return_with_as_val_succeeds_in_regular_fn", test_pointer_return_with_as_val_succeeds_in_regular_fn);
    TEST_RUN("native_fn_can_store_pointer_return", test_native_fn_can_store_pointer_return);
}
