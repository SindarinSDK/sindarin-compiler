// tests/code_gen_tests_stmt_func.c
// Statement code generation tests - function related tests

static void test_code_gen_call_expression_simple(void)
{
    DEBUG_INFO("Starting test_code_gen_call_expression_simple");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token callee_tok;
    setup_basic_token(&callee_tok, TOKEN_IDENTIFIER, "print");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Expr *callee = ast_create_variable_expr(&arena, callee_tok, &callee_tok);
    callee->expr_type = void_type;

    Token string_tok;
    setup_basic_token(&string_tok, TOKEN_STR, "\"Hello, world!\"");
    LiteralValue str_value;
    str_value.string_value = "Hello, world!";
    Expr *string_expr = ast_create_literal_expr(&arena, str_value, string_type, false, &string_tok);
    string_expr->expr_type = string_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr*) * 1);
    args[0] = string_expr;
    int arg_count = 1;

    Expr *call_expr = ast_create_call_expr(&arena, callee, args, arg_count, &callee_tok);
    call_expr->expr_type = void_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, call_expr, &callee_tok);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    rt_print_string_v2(rt_arena_v2_strdup(__local_arena__, \"Hello, world!\"));\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_call_expression_simple");
}

static void test_code_gen_function_simple_void(void)
{
    DEBUG_INFO("Starting test_code_gen_function_simple_void");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token fn_tok;
    setup_basic_token(&fn_tok, TOKEN_IDENTIFIER, "myfn");

    Parameter *params = NULL;
    int param_count = 0;
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt **body = NULL;
    int body_count = 0;

    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, params, param_count, void_type, body, body_count, &fn_tok);

    ast_module_add_statement(&arena, &module, fn_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* All non-main functions receive arena as first parameter */
    const char *expected = get_expected(&arena,
                                  "void __sn__myfn(RtArenaV2 *);\n\n"
                                  "void __sn__myfn(RtArenaV2 *__caller_arena__) {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_DEFAULT, \"func\");\n"
                                  "    goto __sn__myfn_return;\n"
                                  "__sn__myfn_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return;\n"
                                  "}\n\n"
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_function_simple_void");
}

static void test_code_gen_function_with_params_and_return(void)
{
    DEBUG_INFO("Starting test_code_gen_function_with_params_and_return");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token fn_tok;
    setup_basic_token(&fn_tok, TOKEN_IDENTIFIER, "add");

    // Params
    Token param_tok;
    setup_basic_token(&param_tok, TOKEN_IDENTIFIER, "a");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Parameter param = {.name = param_tok, .type = int_type};

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    *params = param;
    int param_count = 1;

    // Return type int
    Type *ret_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Body: return a;
    Token ret_tok;
    setup_basic_token(&ret_tok, TOKEN_RETURN, "return");

    Expr *var_expr = ast_create_variable_expr(&arena, param_tok, &param_tok);
    var_expr->expr_type = int_type;

    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, var_expr, &ret_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    *body = ret_stmt;
    int body_count = 1;

    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, params, param_count, ret_type, body, body_count, &fn_tok);

    ast_module_add_statement(&arena, &module, fn_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* All non-main functions receive arena as first parameter */
    const char *expected = get_expected(&arena,
                                  "long long __sn__add(RtArenaV2 *, long long);\n\n"
                                  "long long __sn__add(RtArenaV2 *__caller_arena__, long long __sn__a) {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_DEFAULT, \"func\");\n"
                                  "    long long _return_value = 0;\n"
                                  "    _return_value = __sn__a;\n"
                                  "    goto __sn__add_return;\n"
                                  "__sn__add_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n\n"
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_function_with_params_and_return");
}

static void test_code_gen_main_function_special_case(void)
{
    DEBUG_INFO("Starting test_code_gen_main_function_special_case");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token main_tok;
    setup_basic_token(&main_tok, TOKEN_IDENTIFIER, "main");

    Parameter *params = NULL;
    int param_count = 0;
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt **body = NULL;
    int body_count = 0;

    Stmt *main_stmt = ast_create_function_stmt(&arena, main_tok, params, param_count, void_type, body, body_count, &main_tok);

    ast_module_add_statement(&arena, &module, main_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_v2_condemn(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_main_function_special_case");
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_stmt_func_main(void)
{
    TEST_SECTION("Code Gen Statement Tests - Functions");
    TEST_RUN("code_gen_call_expression_simple", test_code_gen_call_expression_simple);
    TEST_RUN("code_gen_function_simple_void", test_code_gen_function_simple_void);
    TEST_RUN("code_gen_function_with_params_and_return", test_code_gen_function_with_params_and_return);
    TEST_RUN("code_gen_main_function_special_case", test_code_gen_main_function_special_case);
}
