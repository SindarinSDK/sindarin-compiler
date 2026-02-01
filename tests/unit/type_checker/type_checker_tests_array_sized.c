// type_checker_tests_array_sized.c
// Sized array allocation type checker tests

static void test_type_check_sized_array_alloc_basic()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_basic");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create size expression: 10
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 10};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create sized array alloc: int[10]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[10]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_basic");
}

static void test_type_check_sized_array_alloc_with_default()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_with_default");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create size expression: 5
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 5};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create default value: 0
    Token default_tok;
    setup_literal_token(&default_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue default_val = {.int_value = 0};
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    // Create sized array alloc: int[5] = 0
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, default_expr, &alloc_tok);

    // var arr: int[] = int[5] = 0
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_with_default");
}

static void test_type_check_sized_array_alloc_mismatch_default()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_mismatch_default");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    // Create size expression: 5
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 5};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create default value: true (wrong type)
    Token default_tok;
    setup_literal_token(&default_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue default_val = {.bool_value = 1};
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, bool_type, false, &default_tok);

    // Create sized array alloc: int[5] = true (type mismatch)
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, default_expr, &alloc_tok);

    // var arr: int[] = int[5] = true
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_mismatch_default");
}

static void test_type_check_sized_array_alloc_runtime_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_runtime_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // var n: int = 20
    Token n_tok;
    setup_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token n_val_tok;
    setup_literal_token(&n_val_tok, TOKEN_INT_LITERAL, "20", 1, "test.sn", &arena);
    LiteralValue n_val = {.int_value = 20};
    Expr *n_init = ast_create_literal_expr(&arena, n_val, int_type, false, &n_val_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, n_init, NULL);
    ast_module_add_statement(&arena, &module, n_decl);

    // Create size expression: n (variable reference)
    Expr *size_expr = ast_create_variable_expr(&arena, n_tok, &n_tok);

    // Create sized array alloc: int[n]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[n]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_runtime_size");
}

static void test_type_check_sized_array_alloc_invalid_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_invalid_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    // Create size expression: "bad" (wrong type)
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_STRING_LITERAL, "\"bad\"", 1, "test.sn", &arena);
    LiteralValue size_val = {.string_value = arena_strdup(&arena, "bad")};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, str_type, false, &size_tok);

    // Create sized array alloc: int["bad"] (invalid size type)
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int["bad"]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_invalid_size");
}

static void test_type_check_sized_array_alloc_long_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_long_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);

    // var n: long = 20
    Token n_tok;
    setup_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token n_val_tok;
    setup_literal_token(&n_val_tok, TOKEN_INT_LITERAL, "20", 1, "test.sn", &arena);
    LiteralValue n_val = {.int_value = 20};
    Expr *n_init = ast_create_literal_expr(&arena, n_val, long_type, false, &n_val_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, long_type, n_init, NULL);
    ast_module_add_statement(&arena, &module, n_decl);

    // Create size expression: n (variable reference of type long)
    Expr *size_expr = ast_create_variable_expr(&arena, n_tok, &n_tok);

    // Create sized array alloc: int[n]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[n]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_long_size");
}
