// tests/code_gen_tests_array.c
// Array code generation tests

static void test_code_gen_array_literal(void)
{
    DEBUG_INFO("Starting test_code_gen_array_literal");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");
    Token token;
    setup_basic_token(&token, TOKEN_ARRAY_LITERAL, "{1,2}");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Elements: 1, 2
    Token elem1_tok;
    setup_basic_token(&elem1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&elem1_tok, 1);
    LiteralValue val1 = {.int_value = 1};
    Expr *elem1 = ast_create_literal_expr(&arena, val1, int_type, false, &elem1_tok);
    elem1->expr_type = int_type;

    Token elem2_tok;
    setup_basic_token(&elem2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&elem2_tok, 2);
    LiteralValue val2 = {.int_value = 2};
    Expr *elem2 = ast_create_literal_expr(&arena, val2, int_type, false, &elem2_tok);
    elem2->expr_type = int_type;

    Expr **elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    elements[0] = elem1;
    elements[1] = elem2;
    int elem_count = 2;

    Expr *arr_expr = ast_create_array_expr(&arena, elements, elem_count, &token);
    arr_expr->expr_type = arr_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // code_gen_array_expression generates rt_array_create_* for runtime arrays
    const char *expected = get_expected(&arena,
                                  "rt_array_create_long(NULL, 2, (long long[]){1LL, 2LL});\n"
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_literal");
}

static void test_code_gen_array_var_declaration_with_init(void)
{
    DEBUG_INFO("Starting test_code_gen_array_var_declaration_with_init");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Initializer: {3, 4}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{3,4}");

    Token elem3_tok;
    setup_basic_token(&elem3_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&elem3_tok, 3);
    LiteralValue val3 = {.int_value = 3};
    Expr *elem3 = ast_create_literal_expr(&arena, val3, int_type, false, &elem3_tok);
    elem3->expr_type = int_type;

    Token elem4_tok;
    setup_basic_token(&elem4_tok, TOKEN_INT_LITERAL, "4");
    token_set_int_literal(&elem4_tok, 4);
    LiteralValue val4 = {.int_value = 4};
    Expr *elem4 = ast_create_literal_expr(&arena, val4, int_type, false, &elem4_tok);
    elem4->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    init_elements[0] = elem3;
    init_elements[1] = elem4;
    int init_count = 2;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // Use the array in an expression to ensure it's generated
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * arr = rt_array_create_long(NULL, 2, (long long[]){3LL, 4LL}); arr;
    const char *expected = get_expected(&arena,
                                  "long long * arr = rt_array_create_long(NULL, 2, (long long[]){3LL, 4LL});\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_var_declaration_with_init");
}

static void test_code_gen_array_var_declaration_without_init(void)
{
    DEBUG_INFO("Starting test_code_gen_array_var_declaration_without_init");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "empty_arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // No initializer, should default to NULL
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, NULL, &var_tok);

    // Use in expression
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * empty_arr = NULL; empty_arr;
    const char *expected = get_expected(&arena,
                                  "long long * empty_arr = NULL;\n"
                                  "empty_arr;\n"
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_var_declaration_without_init");
}

static void test_code_gen_array_access(void)
{
    DEBUG_INFO("Starting test_code_gen_array_access");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr = {10, 20, 30}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{10,20,30}");

    Token e1_tok, e2_tok, e3_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "10");
    token_set_int_literal(&e1_tok, 10);
    LiteralValue v1 = {.int_value = 10};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "20");
    token_set_int_literal(&e2_tok, 20);
    LiteralValue v2 = {.int_value = 20};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    setup_basic_token(&e3_tok, TOKEN_INT_LITERAL, "30");
    token_set_int_literal(&e3_tok, 30);
    LiteralValue v3 = {.int_value = 30};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &e3_tok);
    e3->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 3 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2; init_elements[2] = e3;
    int init_count = 3;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // Access: arr[1] (should be 20)
    Token access_tok;
    setup_basic_token(&access_tok, TOKEN_LEFT_BRACKET, "[");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Token idx_tok;
    setup_basic_token(&idx_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&idx_tok, 1);
    LiteralValue idx_val = {.int_value = 1};
    Expr *index = ast_create_literal_expr(&arena, idx_val, int_type, false, &idx_tok);
    index->expr_type = int_type;

    Expr *access_expr = ast_create_array_access_expr(&arena, arr_var, index, &access_tok);
    access_expr->expr_type = int_type;

    Stmt *access_stmt = ast_create_expr_stmt(&arena, access_expr, &access_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, access_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * arr = rt_array_create_long(NULL, 3, (long long[]){10LL, 20LL, 30LL}); arr[1];
    const char *expected = get_expected(&arena,
                                  "long long * arr = rt_array_create_long(NULL, 3, (long long[]){10LL, 20LL, 30LL});\n"
                                  "arr[1LL];\n"
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_access");
}

static void test_code_gen_array_pop(void)
{
    DEBUG_INFO("Starting test_code_gen_array_pop");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1,2,3}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1,2,3}");

    Token e1_tok, e2_tok, e3_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&e2_tok, 2);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    setup_basic_token(&e3_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&e3_tok, 3);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &e3_tok);
    e3->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 3 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2; init_elements[2] = e3;
    int init_count = 3;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // var result: int = arr.pop()
    Token res_tok;
    setup_basic_token(&res_tok, TOKEN_IDENTIFIER, "result");

    Token pop_tok;
    setup_basic_token(&pop_tok, TOKEN_IDENTIFIER, "pop");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, pop_tok, &pop_tok);
    member->expr_type = ast_create_function_type(&arena, int_type, NULL, 0);

    Expr **no_args = NULL;
    int arg_count = 0;

    Expr *pop_call = ast_create_call_expr(&arena, member, no_args, arg_count, &pop_tok);
    pop_call->expr_type = int_type;

    Stmt *res_decl = ast_create_var_decl_stmt(&arena, res_tok, int_type, pop_call, &res_tok);

    // Use result and arr
    Expr *use_res = ast_create_variable_expr(&arena, res_tok, &res_tok);
    use_res->expr_type = int_type;
    Stmt *use_res_stmt = ast_create_expr_stmt(&arena, use_res, &res_tok);

    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_arr_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, res_decl);
    ast_module_add_statement(&arena, &module, use_res_stmt);
    ast_module_add_statement(&arena, &module, use_arr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long result = rt_array_pop_long(arr); result; arr;
    const char *expected = get_expected(&arena,
                                  "long long * arr = rt_array_create_long(NULL, 3, (long long[]){1LL, 2LL, 3LL});\n"
                                  "long long result = rt_array_pop_long(arr);\n"
                                  "result;\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_pop");
}

void test_code_gen_array_main(void)
{
    TEST_SECTION("Code Gen Array Tests");
    TEST_RUN("code_gen_array_literal", test_code_gen_array_literal);
    TEST_RUN("code_gen_array_var_declaration_with_init", test_code_gen_array_var_declaration_with_init);
    TEST_RUN("code_gen_array_var_declaration_without_init", test_code_gen_array_var_declaration_without_init);
    TEST_RUN("code_gen_array_access", test_code_gen_array_access);
    TEST_RUN("code_gen_array_pop", test_code_gen_array_pop);
}
