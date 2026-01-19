// tests/unit/type_checker/type_checker_tests_native_slice.c
// Tests for pointer slicing and array slicing in native functions
// Note: setup_test_token helper is defined in type_checker_tests_native.c

#include "../test_harness.h"

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

/* ============================================================================
 * Main entry point for slice tests
 * ============================================================================ */

void test_type_checker_native_slice_main(void)
{
    TEST_SECTION("Native Slice");

    TEST_RUN("pointer_slice_byte_to_byte_array", test_pointer_slice_byte_to_byte_array);
    TEST_RUN("pointer_slice_int_to_int_array", test_pointer_slice_int_to_int_array);
    TEST_RUN("slice_non_array_non_pointer_fails", test_slice_non_array_non_pointer_fails);
    TEST_RUN("array_slice_still_works", test_array_slice_still_works);
    TEST_RUN("as_val_context_tracking", test_as_val_context_tracking);
    TEST_RUN("pointer_slice_with_as_val_in_regular_fn", test_pointer_slice_with_as_val_in_regular_fn);
    TEST_RUN("pointer_slice_without_as_val_in_regular_fn_fails", test_pointer_slice_without_as_val_in_regular_fn_fails);
    TEST_RUN("as_val_on_array_type_is_noop", test_as_val_on_array_type_is_noop);
    TEST_RUN("get_buffer_slice_as_val_type_inference", test_get_buffer_slice_as_val_type_inference);
    TEST_RUN("slice_invalid_type_error", test_slice_invalid_type_error);
    TEST_RUN("int_pointer_slice_as_val_type_inference", test_int_pointer_slice_as_val_type_inference);
    TEST_RUN("pointer_slice_with_step_fails", test_pointer_slice_with_step_fails);
}
