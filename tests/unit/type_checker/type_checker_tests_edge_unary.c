// tests/unit/type_checker/type_checker_tests_edge_unary.c
// Unary Expression Edge Cases

static void test_unary_not_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val = {.bool_value = true};
    Expr *lit = ast_create_literal_expr(&arena, val, bool_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_BANG, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_unary_negate_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MINUS, "-", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_MINUS, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_unary_negate_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "3.14", 1, "test.sn", &arena);
    LiteralValue val = {.double_value = 3.14};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MINUS, "-", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_MINUS, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, double_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Variable Declaration Edge Cases
// =====================================================

static void test_var_decl_no_initializer(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_var_decl_mismatch_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var x: int = "hello" - type mismatch
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_STRING_LITERAL, "hello", 1, "test.sn", &arena);
    LiteralValue val = {.string_value = "hello"};
    Expr *lit = ast_create_literal_expr(&arena, val, string_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
