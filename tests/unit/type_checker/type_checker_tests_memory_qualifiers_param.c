/**
 * type_checker_tests_memory_qualifiers_param.c - Parameter memory qualifier tests
 *
 * Tests for param as ref/val and edge case handling.
 */

static void test_type_check_param_as_ref_error()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);  // 'as ref' on arrays is invalid (they are already references)
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = arr_type;
    params[0].mem_qualifier = MEM_AS_REF;  // Invalid for array parameters - arrays are already references

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0); // Should have error

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_param_as_ref_primitive()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "counter", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_AS_REF;  // Valid for primitive parameters - enables pass-by-reference

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "increment", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - 'as ref' is valid on primitives

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_param_as_val()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = arr_type;
    params[0].mem_qualifier = MEM_AS_VAL;  // Copy semantics for array param

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_null_stmt_handling()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create a function with a NULL statement in the body (edge case)
    // This tests that type_check_stmt handles null properly
    Stmt *body[2];

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    body[0] = ast_create_return_stmt(&arena, ret_tok, NULL, &ret_tok);
    body[1] = NULL;  // Null statement in body

    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_null", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    // Should not crash even with null statement in body
    int no_error = type_check_module(&module, &table);
    // No assertion on result, just checking it doesn't crash
    (void)no_error;

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_function_with_null_param_type()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create a function with a parameter that has NULL type (edge case)
    Parameter *params = (Parameter *)arena_alloc(&arena, sizeof(Parameter));
    Token param_name_tok;
    setup_token(&param_name_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    params[0].name = param_name_tok;
    params[0].type = NULL;  // NULL type - edge case
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt **body = NULL;
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_null_param", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, body, 0, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    // Should not crash even with null param type
    int no_error = type_check_module(&module, &table);
    // No assertion on result, just checking it doesn't crash
    (void)no_error;

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_memory_qualifiers_param_main()
{
    TEST_RUN("param_as_ref_error", test_type_check_param_as_ref_error);
    TEST_RUN("param_as_ref_primitive", test_type_check_param_as_ref_primitive);
    TEST_RUN("param_as_val", test_type_check_param_as_val);
    TEST_RUN("null_stmt_handling", test_type_check_null_stmt_handling);
    TEST_RUN("function_with_null_param_type", test_type_check_function_with_null_param_type);
}
