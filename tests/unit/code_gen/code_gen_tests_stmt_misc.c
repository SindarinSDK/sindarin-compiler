// tests/code_gen_tests_stmt_misc.c
// Statement code generation tests - miscellaneous statements

static void test_code_gen_string_free_in_block(void)
{
    DEBUG_INFO("Starting test_code_gen_string_free_in_block");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token str_tok;
    setup_basic_token(&str_tok, TOKEN_IDENTIFIER, "s");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_STRING_LITERAL, "\"test\"");
    token_set_string_literal(&init_tok, "test");
    LiteralValue sval = {.string_value = "test"};
    Expr *init = ast_create_literal_expr(&arena, sval, str_type, false, &init_tok);
    Stmt *str_decl = ast_create_var_decl_stmt(&arena, str_tok, str_type, init, &str_tok);

    Stmt **block_stmts = arena_alloc(&arena, sizeof(Stmt *));
    *block_stmts = str_decl;
    int bcount = 1;

    Token block_tok;
    setup_basic_token(&block_tok, TOKEN_LEFT_BRACE, "{");
    Stmt *block = ast_create_block_stmt(&arena, block_stmts, bcount, &block_tok);

    ast_module_add_statement(&arena, &module, block);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Module-level blocks don't emit global declarations for handle-type variables.
    // Instead, they use deferred initialization in main() because C doesn't allow
    // function calls in global initializers. The block body is emitted inside main().
    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    {\n"
                                  "        RtHandleV2 *__s_pending__ = NULL;\n"
                                  "        RtHandleV2 * __sn__s = rt_arena_v2_strdup(__local_arena__, \"test\");\n"
                                  "    }\n"
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

    DEBUG_INFO("Finished test_code_gen_string_free_in_block");
}

static void test_code_gen_increment_decrement(void)
{
    DEBUG_INFO("Starting test_code_gen_increment_decrement");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "counter");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, &var_tok);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = int_type;
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &var_tok);
    inc_expr->expr_type = int_type;
    Stmt *inc_stmt = ast_create_expr_stmt(&arena, inc_expr, &var_tok);

    ast_module_add_statement(&arena, &module, decl);
    ast_module_add_statement(&arena, &module, inc_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    RtHandleV2 *__counter_pending__ = NULL;\n"
                                  "    long long __sn__counter;\n"
                                  "    rt_post_inc_long(&__sn__counter);\n"
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

    DEBUG_INFO("Finished test_code_gen_increment_decrement");
}

static void test_code_gen_null_expression(void)
{
    DEBUG_INFO("Starting test_code_gen_null_expression");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token null_tok;
    setup_basic_token(&null_tok, TOKEN_NIL, "nil");
    Stmt *null_stmt = ast_create_expr_stmt(&arena, NULL, &null_tok);

    ast_module_add_statement(&arena, &module, null_stmt);

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
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_null_expression");
}

static void test_code_gen_new_label(void)
{
    DEBUG_INFO("Starting test_code_gen_new_label");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    int label1 = code_gen_new_label(&gen);
    int label2 = code_gen_new_label(&gen);

    assert(label1 == 0);
    assert(label2 == 1);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    remove_test_file(test_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_new_label");
}

static void test_code_gen_module_no_main_adds_dummy(void)
{
    DEBUG_INFO("Starting test_code_gen_module_no_main_adds_dummy");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

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
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_module_no_main_adds_dummy");
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_stmt_misc_main(void)
{
    TEST_SECTION("Code Gen Statement Tests - Miscellaneous");
    TEST_RUN("code_gen_string_free_in_block", test_code_gen_string_free_in_block);
    TEST_RUN("code_gen_increment_decrement", test_code_gen_increment_decrement);
    TEST_RUN("code_gen_null_expression", test_code_gen_null_expression);
    TEST_RUN("code_gen_new_label", test_code_gen_new_label);
    TEST_RUN("code_gen_module_no_main_adds_dummy", test_code_gen_module_no_main_adds_dummy);
}
