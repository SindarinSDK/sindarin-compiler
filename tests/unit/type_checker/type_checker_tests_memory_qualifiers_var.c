/**
 * type_checker_tests_memory_qualifiers_var.c - Variable and function memory qualifier tests
 *
 * Tests for var as ref/val and private/shared function qualifiers.
 */

static void test_type_check_var_as_ref_primitive()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_REF;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_var_as_ref_array_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, arr_init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_REF;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_var_as_val_array()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Expr *elements[1] = {e1};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{1}", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, arr_init, NULL);
    var_decl->as.var_decl.mem_qualifier = MEM_AS_VAL;

    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_private_function_primitive_return()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue v = {.int_value = 42};
    Expr *ret_val = ast_create_literal_expr(&arena, v, int_type, false, &lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_PRIVATE;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_private_function_array_return_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *ret_val = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, arr_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_PRIVATE;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_shared_function()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{}", 1, "test.sn", &arena);
    Expr *ret_val = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, ret_val, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "helper", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, arr_type, body, 1, &func_name_tok);
    func_decl->as.function.modifier = FUNC_SHARED;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1); // Shared functions can return any type

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_memory_qualifiers_var_main()
{
    TEST_RUN("var_as_ref_primitive", test_type_check_var_as_ref_primitive);
    TEST_RUN("var_as_ref_array_error", test_type_check_var_as_ref_array_error);
    TEST_RUN("var_as_val_array", test_type_check_var_as_val_array);
    TEST_RUN("private_function_primitive_return", test_type_check_private_function_primitive_return);
    TEST_RUN("private_function_array_return_error", test_type_check_private_function_array_return_error);
    TEST_RUN("shared_function", test_type_check_shared_function);
}
