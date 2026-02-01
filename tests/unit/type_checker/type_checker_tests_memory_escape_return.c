/**
 * type_checker_tests_memory_escape_return.c - Escape analysis tests for return statements
 *
 * Tests for escape detection in return statements.
 */

static void test_escape_return_local_variable()
{
    // Test: function returns local variable
    // Expected: return expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn getValue(): int =>
    //   var local: int = 42
    //   return local  // local escapes via return

    // var local: int = 42
    Token local_name_tok;
    setup_token(&local_name_tok, TOKEN_IDENTIFIER, "local", 1, "test.sn", &arena);
    Token local_init_tok;
    setup_literal_token(&local_init_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue local_v = {.int_value = 42};
    Expr *local_init = ast_create_literal_expr(&arena, local_v, int_type, false, &local_init_tok);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_name_tok, int_type, local_init, NULL);

    // return local
    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token local_var_tok;
    setup_token(&local_var_tok, TOKEN_IDENTIFIER, "local", 2, "test.sn", &arena);
    Expr *local_var_expr = ast_create_variable_expr(&arena, local_var_tok, &local_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, local_var_expr, &return_tok);

    // Create function body
    Stmt *func_body[2] = {local_decl, return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getValue", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 2, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that local_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(local_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_parameter_no_escape()
{
    // Test: function returns parameter variable
    // Expected: return expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn identity(x: int): int =>
    //   return x  // parameter, no escape

    // Create parameter
    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    // return x
    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token x_var_tok;
    setup_token(&x_var_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Expr *x_var_expr = ast_create_variable_expr(&arena, x_var_tok, &x_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, x_var_expr, &return_tok);

    // Create function body
    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "identity", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, params, 1, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that x_var_expr does NOT have escapes_scope set (it's a parameter)
    assert(ast_expr_escapes_scope(x_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_global_no_escape()
{
    // Test: function returns global variable
    // Expected: return expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create global:
    // var global: int = 100

    Token global_name_tok;
    setup_token(&global_name_tok, TOKEN_IDENTIFIER, "globalVal", 1, "test.sn", &arena);
    Token global_init_tok;
    setup_literal_token(&global_init_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue global_v = {.int_value = 100};
    Expr *global_init = ast_create_literal_expr(&arena, global_v, int_type, false, &global_init_tok);
    Stmt *global_decl = ast_create_var_decl_stmt(&arena, global_name_tok, int_type, global_init, NULL);

    // Create function:
    // fn getGlobal(): int =>
    //   return globalVal  // global, no escape

    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 3, "test.sn", &arena);
    Token global_var_tok;
    setup_token(&global_var_tok, TOKEN_IDENTIFIER, "globalVal", 3, "test.sn", &arena);
    Expr *global_var_expr = ast_create_variable_expr(&arena, global_var_tok, &global_var_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, global_var_expr, &return_tok);

    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getGlobal", 2, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, global_decl);
    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that global_var_expr does NOT have escapes_scope set (it's a global)
    assert(ast_expr_escapes_scope(global_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_return_literal_no_escape()
{
    // Test: function returns literal expression
    // Expected: return expression should NOT have escapes_scope (not a variable)
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create function:
    // fn getConstant(): int =>
    //   return 42  // literal, not a variable

    Token return_tok;
    setup_token(&return_tok, TOKEN_RETURN, "return", 2, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue lit_v = {.int_value = 42};
    Expr *lit_expr = ast_create_literal_expr(&arena, lit_v, int_type, false, &lit_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, lit_expr, &return_tok);

    Stmt *func_body[1] = {return_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "getConstant", 1, "test.sn", &arena);
    Stmt *func = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, int_type, func_body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that lit_expr does NOT have escapes_scope set (it's a literal)
    assert(ast_expr_escapes_scope(lit_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_memory_escape_return_main()
{
    TEST_RUN("escape_return_local_variable", test_escape_return_local_variable);
    TEST_RUN("escape_return_parameter_no_escape", test_escape_return_parameter_no_escape);
    TEST_RUN("escape_return_global_no_escape", test_escape_return_global_no_escape);
    TEST_RUN("escape_return_literal_no_escape", test_escape_return_literal_no_escape);
}
