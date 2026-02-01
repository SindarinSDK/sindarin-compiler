// type_checker_tests_native_slice_basic.c
// Basic pointer and array slice tests

/* Test that pointer slice *byte[0..10] produces byte[] */
static void test_pointer_slice_byte_to_byte_array(void)
{
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: var p: *byte = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_byte_type, nil_lit, NULL);

    /* Create slice expression: p[0..10] */
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, p_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: byte[] = p[0..10] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, slice_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *byte[0..10] => byte[] */

    /* Verify the slice expression type is byte[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice *int[0..5] produces int[] */
static void test_pointer_slice_int_to_int_array(void)
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
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create slice expression: p[0..5] */
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, p_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: int[] = p[0..5] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int[0..5] => int[] */

    /* Verify the slice expression type is int[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that slicing a non-array, non-pointer type fails (e.g., int[0..5]) */
static void test_slice_non_array_non_pointer_fails(void)
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

    /* Create: var n: int = 42 */
    Token n_tok;
    setup_test_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, lit, NULL);

    /* Create slice expression: n[0..5] - THIS SHOULD FAIL, n is int not array/pointer */
    Token n_ref_tok;
    setup_test_token(&n_ref_tok, TOKEN_IDENTIFIER, "n", 2, "test.sn", &arena);
    Expr *n_ref = ast_create_variable_expr(&arena, n_ref_tok, &n_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, n_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: int[] = n[0..5] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {n_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: cannot slice int */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that array slicing still works correctly (regression test) */
static void test_array_slice_still_works(void)
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

    /* Create: var arr: int[] = {1, 2, 3, 4, 5} */
    Token arr_tok;
    setup_test_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    LiteralValue v1 = {.int_value = 1};
    LiteralValue v2 = {.int_value = 2};
    LiteralValue v3 = {.int_value = 3};
    Token lit1_tok; setup_test_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    Token lit2_tok; setup_test_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    Token lit3_tok; setup_test_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);
    Expr *elements[3] = {e1, e2, e3};

    Token brace_tok;
    setup_test_token(&brace_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 3, &brace_tok);

    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, int_array_type, arr_lit, NULL);

    /* Create slice expression: arr[1..3] */
    Token arr_ref_tok;
    setup_test_token(&arr_ref_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *arr_ref = ast_create_variable_expr(&arena, arr_ref_tok, &arr_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var slice: int[] = arr[1..3] */
    Token slice_tok;
    setup_test_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {arr_decl, slice_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: array slicing still works */

    /* Verify the slice expression type is int[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
