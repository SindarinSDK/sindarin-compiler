// type_checker_tests_native_slice_as_val.c
// Tests for as_val semantics with pointer slices

/* Test that as_val context tracking functions work */
static void test_as_val_context_tracking(void)
{
    /* Default: not active */
    assert(as_val_context_is_active() == false);

    /* Enter: active */
    as_val_context_enter();
    assert(as_val_context_is_active() == true);

    /* Nesting: still active */
    as_val_context_enter();
    assert(as_val_context_is_active() == true);

    /* Exit once: still active (nested) */
    as_val_context_exit();
    assert(as_val_context_is_active() == true);

    /* Exit again: inactive */
    as_val_context_exit();
    assert(as_val_context_is_active() == false);

}

/* Test that pointer slice with 'as val' works in regular function */
static void test_pointer_slice_with_as_val_in_regular_fn(void)
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

    /* Create slice expression: get_data()[0..10] */
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

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_data()[0..10] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_data()[0..10] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: ptr[0..10] as val is OK in regular fn */

    /* Verify the as_val expression type is byte[] */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    /* Verify is_noop is true (slice already produces array type) */
    assert(as_val_expr->as.as_val.is_noop == true);
    assert(as_val_expr->as.as_val.is_cstr_to_str == false);

    /* Verify is_from_pointer is true on the inner slice expression */
    assert(slice_expr->as.array_slice.is_from_pointer == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice WITHOUT 'as val' fails in regular function */
static void test_pointer_slice_without_as_val_in_regular_fn_fails(void)
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

    /* Create slice expression: get_data()[0..10] -- WITHOUT as val */
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

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: byte[] = get_data()[0..10] -- NO as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, slice_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: ptr[0..10] without as val not allowed in regular fn */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' on array types works (no-op) */
static void test_as_val_on_array_type_is_noop(void)
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

    /* Create: var arr: int[] = {1, 2, 3} */
    Token arr_tok;
    setup_test_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token one_tok, two_tok, three_tok;
    setup_test_token(&one_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    setup_test_token(&two_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    setup_test_token(&three_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue one_val = {.int_value = 1};
    LiteralValue two_val = {.int_value = 2};
    LiteralValue three_val = {.int_value = 3};
    Expr *one_lit = ast_create_literal_expr(&arena, one_val, int_type, false, &one_tok);
    Expr *two_lit = ast_create_literal_expr(&arena, two_val, int_type, false, &two_tok);
    Expr *three_lit = ast_create_literal_expr(&arena, three_val, int_type, false, &three_tok);
    Expr **elements = arena_alloc(&arena, sizeof(Expr *) * 3);
    elements[0] = one_lit;
    elements[1] = two_lit;
    elements[2] = three_lit;
    Expr *array_expr = ast_create_array_expr(&arena, elements, 3, &arr_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, int_array_type, array_expr, NULL);

    /* Create: arr as val */
    Token arr_ref_tok;
    setup_test_token(&arr_ref_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *arr_ref = ast_create_variable_expr(&arena, arr_ref_tok, &arr_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, arr_ref, &as_tok);

    /* Create: var copy: int[] = arr as val */
    Token copy_tok;
    setup_test_token(&copy_tok, TOKEN_IDENTIFIER, "copy", 2, "test.sn", &arena);
    Stmt *copy_decl = ast_create_var_decl_stmt(&arena, copy_tok, int_array_type, as_val_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {arr_decl, copy_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: 'as val' on array is no-op */

    /* Verify the as_val expression type is int[] */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that get_buffer()[0..len] as val correctly infers byte[] from *byte */
static void test_get_buffer_slice_as_val_type_inference(void)
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

    /* Create: native fn get_buffer(): *byte (forward declaration) */
    Token get_buffer_tok;
    setup_test_token(&get_buffer_tok, TOKEN_IDENTIFIER, "get_buffer", 1, "test.sn", &arena);
    Stmt *get_buffer_decl = ast_create_function_stmt(&arena, get_buffer_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_buffer_tok);
    get_buffer_decl->as.function.is_native = true;

    /* Create call expression: get_buffer() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_buffer", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds: 0 and len (a variable) */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token len_tok;
    setup_test_token(&len_tok, TOKEN_IDENTIFIER, "len", 2, "test.sn", &arena);
    Expr *len_expr = ast_create_variable_expr(&arena, len_tok, &len_tok);

    /* Create slice expression: get_buffer()[0..len] */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, len_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_buffer()[0..len] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var len: int = 10 (needed for type checking len variable) */
    Token len_decl_tok;
    setup_test_token(&len_decl_tok, TOKEN_IDENTIFIER, "len", 1, "test.sn", &arena);
    Token ten_tok;
    setup_test_token(&ten_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue ten_val = {.int_value = 10};
    Expr *ten_expr = ast_create_literal_expr(&arena, ten_val, int_type, false, &ten_tok);
    Stmt *len_decl = ast_create_var_decl_stmt(&arena, len_decl_tok, int_type, ten_expr, NULL);

    /* Create: var data: byte[] = get_buffer()[0..len] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[2] = {len_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_buffer_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED */

    /* Verify type inference:
     * - call_expr should be *byte
     * - slice_expr should be byte[] (slice extracts element type from pointer base)
     * - as_val_expr should be byte[] (as val on array is no-op)
     */
    assert(call_expr->expr_type != NULL);
    assert(call_expr->expr_type->kind == TYPE_POINTER);
    assert(call_expr->expr_type->as.pointer.base_type->kind == TYPE_BYTE);

    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

