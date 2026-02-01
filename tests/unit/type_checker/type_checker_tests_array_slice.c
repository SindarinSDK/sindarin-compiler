// type_checker_tests_array_slice.c
// Array slice type checker tests

static void test_type_check_array_slice_full()
{
    DEBUG_INFO("Starting test_type_check_array_slice_full");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // var arr:int[]
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    // var slice:int[] = arr[1..3]
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, start, end, NULL, &arr_tok);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *sym = symbol_table_lookup_symbol(&table, slice_tok);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_ARRAY);
    assert(sym->type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_full");
}

static void test_type_check_array_slice_from_start()
{
    DEBUG_INFO("Starting test_type_check_array_slice_from_start");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, NULL, end, NULL, &arr_tok);
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_from_start");
}

static void test_type_check_array_slice_to_end()
{
    DEBUG_INFO("Starting test_type_check_array_slice_to_end");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, start, NULL, NULL, &arr_tok);
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_to_end");
}

static void test_type_check_array_slice_non_array()
{
    DEBUG_INFO("Starting test_type_check_array_slice_non_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // var x:int = 5
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 5};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, lit, NULL);
    ast_module_add_statement(&arena, &module, x_decl);

    // Try to slice x[1..3] - should fail
    Expr *x_var = ast_create_variable_expr(&arena, x_tok, &x_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, x_var, start, end, NULL, &x_tok);
    Stmt *slice_stmt = ast_create_expr_stmt(&arena, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_non_array");
}
