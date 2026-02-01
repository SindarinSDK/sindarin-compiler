// type_checker_tests_promotion_interop.c
// Type checker tests for interop type declarations (int32, uint, float, etc.)

static void test_type_check_int32_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: int32 = 42 (using int literal, which should be compatible)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int32_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_uint_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: uint = 42 (using int literal, which should be compatible)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, uint_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_uint32_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: uint32 = 42
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, uint32_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_float_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: float = 3.14 (using double literal)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "3.14", 1, "test.sn", &arena);
    LiteralValue val = {.double_value = 3.14};
    Expr *init = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, float_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

