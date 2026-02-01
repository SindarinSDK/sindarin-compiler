/**
 * type_checker_tests_native_pointer_asref.c - as_ref out-parameter tests
 *
 * Tests for 'as ref' parameter passing in native functions.
 */

/* Test that 'as ref' parameter on primitive types in native functions is valid */
static void test_as_ref_primitive_param_in_native_fn(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_dimensions(width: int as ref, height: int as ref): void */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "get_dimensions", 1, "test.sn", &arena);

    Token width_tok;
    setup_test_token(&width_tok, TOKEN_IDENTIFIER, "width", 1, "test.sn", &arena);

    Token height_tok;
    setup_test_token(&height_tok, TOKEN_IDENTIFIER, "height", 1, "test.sn", &arena);

    /* Create params array with 'as ref' qualifier */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 2);
    params[0].name = width_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_AS_REF;
    params[1].name = height_tok;
    params[1].type = int_type;
    params[1].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 2, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: as ref on int is valid */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as ref' on array parameter (non-primitive) is rejected */
static void test_as_ref_array_param_rejected(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: native fn process(data: int[] as ref): void -- this should fail */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);

    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 1, "test.sn", &arena);

    /* Create param with 'as ref' on array (invalid) */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = data_tok;
    params[0].type = int_array_type;
    params[0].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: as ref only applies to primitives */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that calling a native function with 'as ref' params works with regular vars */
static void test_as_ref_param_call_with_vars(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn set_value(out: int as ref): void */
    Token set_val_tok;
    setup_test_token(&set_val_tok, TOKEN_IDENTIFIER, "set_value", 1, "test.sn", &arena);

    Token out_tok;
    setup_test_token(&out_tok, TOKEN_IDENTIFIER, "out", 1, "test.sn", &arena);

    Parameter *native_params = arena_alloc(&arena, sizeof(Parameter) * 1);
    native_params[0].name = out_tok;
    native_params[0].type = int_type;
    native_params[0].mem_qualifier = MEM_AS_REF;

    /* Create the param_mem_quals array for the function type */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 1);
    param_types[0] = int_type;
    Type *func_type = ast_create_function_type(&arena, void_type, param_types, 1);
    func_type->as.function.param_mem_quals = arena_alloc(&arena, sizeof(MemoryQualifier) * 1);
    func_type->as.function.param_mem_quals[0] = MEM_AS_REF;

    Stmt *native_decl = ast_create_function_stmt(&arena, set_val_tok, native_params, 1, void_type, NULL, 0, &set_val_tok);
    native_decl->as.function.is_native = true;

    /* Create a regular function that calls set_value(x) */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* var x: int = 0 */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 3, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, zero_lit, NULL);

    /* set_value(x) */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "set_value", 4, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Token x_arg_tok;
    setup_test_token(&x_arg_tok, TOKEN_IDENTIFIER, "x", 4, "test.sn", &arena);
    Expr *x_arg = ast_create_variable_expr(&arena, x_arg_tok, &x_arg_tok);

    Expr **args = arena_alloc(&arena, sizeof(Expr *) * 1);
    args[0] = x_arg;
    Expr *call = ast_create_call_expr(&arena, callee, args, 1, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = x_decl;
    body[1] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 2, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, native_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_native_pointer_asref_main(void)
{
    TEST_RUN("as_ref_primitive_param_in_native_fn", test_as_ref_primitive_param_in_native_fn);
    TEST_RUN("as_ref_array_param_rejected", test_as_ref_array_param_rejected);
    TEST_RUN("as_ref_param_call_with_vars", test_as_ref_param_call_with_vars);
}
