// tests/unit/type_checker/type_checker_tests_edge_literal.c
// Literal Expression Type Checking

static void test_literal_int_type(void)
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

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(lit->expr_type != NULL);
    assert(lit->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_literal_bool_type(void)
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

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "flag", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, lit, NULL);

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

static void test_literal_string_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_STRING_LITERAL, "hello", 1, "test.sn", &arena);
    LiteralValue val = {.string_value = "hello"};
    Expr *lit = ast_create_literal_expr(&arena, val, string_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "msg", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, string_type, lit, NULL);

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

static void test_literal_char_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_CHAR_LITERAL, "a", 1, "test.sn", &arena);
    LiteralValue val = {.char_value = 'a'};
    Expr *lit = ast_create_literal_expr(&arena, val, char_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "ch", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, char_type, lit, NULL);

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
