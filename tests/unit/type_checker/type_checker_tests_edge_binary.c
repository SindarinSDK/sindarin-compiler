// tests/unit/type_checker/type_checker_tests_edge_binary.c
// Binary Expression Edge Cases

static void test_binary_logical_and(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val1 = {.bool_value = true};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, bool_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue val2 = {.bool_value = false};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, bool_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_AND, "&&", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_AND, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_logical_or(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val1 = {.bool_value = true};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, bool_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue val2 = {.bool_value = false};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, bool_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_OR, "||", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_OR, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_comparison_lt(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_LESS, "<", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_LESS, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_modulo(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 10};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 3};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MODULO, "%", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_MODULO, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
