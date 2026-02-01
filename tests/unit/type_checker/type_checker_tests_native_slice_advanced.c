// type_checker_tests_native_slice_advanced.c
// Advanced tests for slice error handling and type inference

/* Test that slicing a non-pointer/non-array type produces error */
static void test_slice_invalid_type_error(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_int(): int (forward declaration) */
    Token get_int_tok;
    setup_test_token(&get_int_tok, TOKEN_IDENTIFIER, "get_int", 1, "test.sn", &arena);
    Stmt *get_int_decl = ast_create_function_stmt(&arena, get_int_tok, NULL, 0, int_type, NULL, 0, &get_int_tok);
    get_int_decl->as.function.is_native = true;

    /* Create call expression: get_int() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_int", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    /* Create slice expression: get_int()[0..10] -- INVALID: int is not sliceable! */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val' */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_int()[0..10] as val -- SHOULD FAIL */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_int_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: cannot slice an int */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that *int slice produces int[] */
static void test_int_pointer_slice_as_val_type_inference(void)
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
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: native fn get_ints(): *int (forward declaration) */
    Token get_ints_tok;
    setup_test_token(&get_ints_tok, TOKEN_IDENTIFIER, "get_ints", 1, "test.sn", &arena);
    Stmt *get_ints_decl = ast_create_function_stmt(&arena, get_ints_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ints_tok);
    get_ints_decl->as.function.is_native = true;

    /* Create call expression: get_ints() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_ints", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    /* Create slice expression: get_ints()[0..5] */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_ints()[0..5] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: int[] = get_ints()[0..5] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ints_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED */

    /* Verify type inference */
    assert(call_expr->expr_type != NULL);
    assert(call_expr->expr_type->kind == TYPE_POINTER);
    assert(call_expr->expr_type->as.pointer.base_type->kind == TYPE_INT);

    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice with step parameter is rejected */
static void test_pointer_slice_with_step_fails(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_data(): *byte (forward declaration) */
    Token get_data_tok;
    setup_test_token(&get_data_tok, TOKEN_IDENTIFIER, "get_data", 1, "test.sn", &arena);
    Stmt *get_data_decl = ast_create_function_stmt(&arena, get_data_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_data_tok);
    get_data_decl->as.function.is_native = true;

    /* Create slice expression: get_data()[0..10:2] -- with step parameter */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_data", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token step_tok;
    setup_test_token(&step_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue step_val = {.int_value = 2};
    Expr *step_expr = ast_create_literal_expr(&arena, step_val, int_type, false, &step_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    /* Create slice with step: ptr[0..10:2] */
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, step_expr, &bracket_tok);

    /* Wrap in as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_data()[0..10:2] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: ptr[0..10:2] with step not allowed */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

