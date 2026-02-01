// type_checker_tests_array_access.c
// Array access type checker tests

static void test_type_check_array_access_valid()
{
    DEBUG_INFO("Starting test_type_check_array_access_valid");

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
    Token lit1_tok, lit2_tok, lit3_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    setup_literal_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);
    Expr *elements[3] = {e1, e2, e3};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 3, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, idx, &access_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, access, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, x_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(access->expr_type != NULL);
    assert(ast_type_equals(access->expr_type, int_type));
    assert(var_arr->expr_type != NULL);
    assert(ast_type_equals(var_arr->expr_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_valid");
}

static void test_type_check_array_access_non_array()
{
    DEBUG_INFO("Starting test_type_check_array_access_non_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token num_tok;
    setup_token(&num_tok, TOKEN_IDENTIFIER, "num", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 5};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *num_decl = ast_create_var_decl_stmt(&arena, num_tok, int_type, lit, NULL);

    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_num = ast_create_variable_expr(&arena, num_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_num, idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, num_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_non_array");
}

static void test_type_check_array_access_invalid_index()
{
    DEBUG_INFO("Starting test_type_check_array_access_invalid_index");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"foo\"", 2, "test.sn", &arena);
    LiteralValue str_val = {.string_value = arena_strdup(&arena, "foo")};
    Expr *str_idx = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, str_idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_invalid_index");
}
