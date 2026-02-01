/**
 * type_checker_tests_native_pointer_inline.c - Inline pointer passing tests
 *
 * Tests for inline pointer passing and basic as_val functionality.
 */

/* Test that inline pointer passing (e.g., use_ptr(get_ptr())) is allowed */
static void test_inline_pointer_passing_allowed(void)
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

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 2, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 2, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create call: get_ptr() */
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);

    /* Create call: use_ptr(get_ptr()) - inline pointer passing */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {get_ptr_call};
    Expr *inline_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, inline_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;  /* Regular function doing inline call */

    /* Add all to module */
    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test inline pointer passing with nil is allowed */
static void test_inline_nil_passing_allowed(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 1, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 1, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create nil literal */
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 5, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    /* Create call: use_ptr(nil) */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {nil_lit};
    Expr *nil_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, nil_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    /* Add all to module */
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *int to int */
static void test_as_val_unwraps_pointer_int(void)
{
    Arena arena;
    arena_init(&arena, 8192);

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

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *double to double */
static void test_as_val_unwraps_pointer_double(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_double_type = ast_create_pointer_type(&arena, double_type);

    /* Create: var p: *double = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_double_type, nil_lit, NULL);

    /* Create: var x: double = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, double_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *double as val => double */

    /* Verify the expression type is double */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *float to float */
static void test_as_val_unwraps_pointer_float(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_float_type = ast_create_pointer_type(&arena, float_type);

    /* Create: var p: *float = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_float_type, nil_lit, NULL);

    /* Create: var x: float = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, float_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *float as val => float */

    /* Verify the expression type is float */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_FLOAT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_native_pointer_inline_main(void)
{
    TEST_RUN("inline_pointer_passing_allowed", test_inline_pointer_passing_allowed);
    TEST_RUN("inline_nil_passing_allowed", test_inline_nil_passing_allowed);
    TEST_RUN("as_val_unwraps_pointer_int", test_as_val_unwraps_pointer_int);
    TEST_RUN("as_val_unwraps_pointer_double", test_as_val_unwraps_pointer_double);
    TEST_RUN("as_val_unwraps_pointer_float", test_as_val_unwraps_pointer_float);
}
