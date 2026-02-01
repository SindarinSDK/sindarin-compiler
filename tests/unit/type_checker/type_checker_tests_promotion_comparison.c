// type_checker_tests_promotion_comparison.c
// Type checker tests for comparison type promotion

static void test_type_check_int_double_comparison()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 5 < 5.5 (int < double)
    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "5.5", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 5.5};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token less_tok;
    setup_token(&less_tok, TOKEN_LESS, "<", 1, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, int_lit, TOKEN_LESS, double_lit, &less_tok);

    // var result: bool = 5 < 5.5
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int/double comparison is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_double_int_equality()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 5.0 == 5 (double == int)
    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "5.0", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 5.0};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token eq_tok;
    setup_token(&eq_tok, TOKEN_EQUAL_EQUAL, "==", 1, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, double_lit, TOKEN_EQUAL_EQUAL, int_lit, &eq_tok);

    // var result: bool = 5.0 == 5
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - double/int equality is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int_double_greater()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create variable declarations
    Token i_tok;
    setup_token(&i_tok, TOKEN_IDENTIFIER, "i", 1, "test.sn", &arena);
    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_init = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);
    Stmt *i_decl = ast_create_var_decl_stmt(&arena, i_tok, int_type, int_init, NULL);

    Token d_tok;
    setup_token(&d_tok, TOKEN_IDENTIFIER, "d", 2, "test.sn", &arena);
    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "2.5", 2, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 2.5};
    Expr *double_init = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);
    Stmt *d_decl = ast_create_var_decl_stmt(&arena, d_tok, double_type, double_init, NULL);

    // Create comparison: i > d
    Expr *i_var = ast_create_variable_expr(&arena, i_tok, NULL);
    Expr *d_var = ast_create_variable_expr(&arena, d_tok, NULL);

    Token gt_tok;
    setup_token(&gt_tok, TOKEN_GREATER, ">", 3, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, i_var, TOKEN_GREATER, d_var, &gt_tok);

    // var result: bool = i > d
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 3, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[3] = {i_decl, d_decl, result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 3, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int/double comparison is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

