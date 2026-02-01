// tests/code_gen_tests_stmt_control.c
// Statement code generation tests - control flow statements

static void test_code_gen_block_statement(void)
{
    DEBUG_INFO("Starting test_code_gen_block_statement");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "block_var");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, &var_tok);

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt *));
    *stmts = var_decl;
    int count = 1;

    Token block_tok;
    setup_basic_token(&block_tok, TOKEN_LEFT_BRACE, "{");
    Stmt *block = ast_create_block_stmt(&arena, stmts, count, &block_tok);

    ast_module_add_statement(&arena, &module, block);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtManagedArena *__local_arena__ = rt_managed_arena_create();\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    {\n"
                                  "        long long __sn__block_var = 0;\n"
                                  "    }\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_managed_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_block_statement");
}

static void test_code_gen_if_statement(void)
{
    DEBUG_INFO("Starting test_code_gen_if_statement");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token if_tok;
    setup_basic_token(&if_tok, TOKEN_IF, "if");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token cond_tok;
    setup_basic_token(&cond_tok, TOKEN_BOOL_LITERAL, "true");
    token_set_bool_literal(&cond_tok, 1);
    LiteralValue bval = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, bval, bool_type, false, &cond_tok);
    cond->expr_type = bool_type;

    Token then_tok;
    setup_basic_token(&then_tok, TOKEN_IDENTIFIER, "print");
    Expr *dummy_expr = ast_create_variable_expr(&arena, then_tok, &then_tok);
    dummy_expr->expr_type = bool_type;

    Stmt *then_stmt = ast_create_expr_stmt(&arena, dummy_expr, &then_tok);

    Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then_stmt, NULL, &if_tok);

    ast_module_add_statement(&arena, &module, if_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtManagedArena *__local_arena__ = rt_managed_arena_create();\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    if (1L) {\n"
                                  "        __sn__print;\n"
                                  "    }\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_managed_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_if_statement");
}

static void test_code_gen_while_statement(void)
{
    DEBUG_INFO("Starting test_code_gen_while_statement");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token while_tok;
    setup_basic_token(&while_tok, TOKEN_WHILE, "while");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token cond_tok;
    setup_basic_token(&cond_tok, TOKEN_BOOL_LITERAL, "true");
    token_set_bool_literal(&cond_tok, 1);
    LiteralValue bval = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, bval, bool_type, false, &cond_tok);
    cond->expr_type = bool_type;

    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Expr *body_expr = ast_create_variable_expr(&arena, body_tok, &body_tok);
    body_expr->expr_type = bool_type;

    Stmt *body = ast_create_expr_stmt(&arena, body_expr, &body_tok);

    Stmt *while_stmt = ast_create_while_stmt(&arena, cond, body, &while_tok);

    ast_module_add_statement(&arena, &module, while_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtManagedArena *__local_arena__ = rt_managed_arena_create();\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    while (1L) {\n"
                                  "        __sn__print;\n"
                                  "    }\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_managed_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_while_statement");
}

static void test_code_gen_for_statement(void)
{
    DEBUG_INFO("Starting test_code_gen_for_statement");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token for_tok;
    setup_basic_token(&for_tok, TOKEN_FOR, "for");

    // Initializer: var k: int = 0
    Token init_var_tok;
    setup_basic_token(&init_var_tok, TOKEN_IDENTIFIER, "k");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token init_val_tok;
    setup_basic_token(&init_val_tok, TOKEN_INT_LITERAL, "0");
    token_set_int_literal(&init_val_tok, 0);
    LiteralValue ival = {.int_value = 0};
    Expr *init_val = ast_create_literal_expr(&arena, ival, int_type, false, &init_val_tok);
    init_val->expr_type = int_type;

    Stmt *init_stmt = ast_create_var_decl_stmt(&arena, init_var_tok, int_type, init_val, &init_var_tok);

    // Condition: k < 5
    Token cond_left_tok;
    setup_basic_token(&cond_left_tok, TOKEN_IDENTIFIER, "k");
    Expr *cond_left = ast_create_variable_expr(&arena, cond_left_tok, &cond_left_tok);
    cond_left->expr_type = int_type;
    Token cond_right_tok;
    setup_basic_token(&cond_right_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&cond_right_tok, 5);
    LiteralValue cval = {.int_value = 5};
    Expr *cond_right = ast_create_literal_expr(&arena, cval, int_type, false, &cond_right_tok);
    cond_right->expr_type = int_type;
    Token cond_op_tok;
    setup_basic_token(&cond_op_tok, TOKEN_LESS, "<");
    Expr *cond = ast_create_binary_expr(&arena, cond_left, TOKEN_LESS, cond_right, &cond_op_tok);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    cond->expr_type = bool_type;

    // Increment: k++
    Token inc_tok;
    setup_basic_token(&inc_tok, TOKEN_IDENTIFIER, "k");
    Expr *inc_var = ast_create_variable_expr(&arena, inc_tok, &inc_tok);
    inc_var->expr_type = int_type;
    Expr *inc_expr = ast_create_increment_expr(&arena, inc_var, &inc_tok);
    inc_expr->expr_type = int_type;

    // Body: print(k)
    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Expr *callee_print = ast_create_variable_expr(&arena, body_tok, &body_tok);
    Type *print_param_types[1] = {int_type};
    Type *print_func_type = ast_create_function_type(&arena, void_type, print_param_types, 1);
    callee_print->expr_type = print_func_type;

    Token arg_k_tok;
    setup_basic_token(&arg_k_tok, TOKEN_IDENTIFIER, "k");
    Expr *arg_k = ast_create_variable_expr(&arena, arg_k_tok, &arg_k_tok);
    arg_k->expr_type = int_type;

    Expr *print_call = ast_create_call_expr(&arena, callee_print, &arg_k, 1, &body_tok);
    print_call->expr_type = void_type;

    Stmt *body = ast_create_expr_stmt(&arena, print_call, &body_tok);

    Stmt *for_stmt = ast_create_for_stmt(&arena, init_stmt, cond, inc_expr, body, &for_tok);

    ast_module_add_statement(&arena, &module, for_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    RtManagedArena *__local_arena__ = rt_managed_arena_create();\n"
                                  "    __main_arena__ = __local_arena__;\n"
                                  "    int _return_value = 0;\n"
                                  "    {\n"
                                  "        long long __sn__k = 0LL;\n"
                                  "        while (rt_lt_long(__sn__k, 5LL)) {\n"
                                  "            rt_print_long(__sn__k);\n"
                                  "        __for_continue_0__:;\n"
                                  "            rt_post_inc_long(&__sn__k);\n"
                                  "        }\n"
                                  "    }\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_managed_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_for_statement");
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_code_gen_stmt_control_main(void)
{
    TEST_SECTION("Code Gen Statement Tests - Control Flow");
    TEST_RUN("code_gen_block_statement", test_code_gen_block_statement);
    TEST_RUN("code_gen_if_statement", test_code_gen_if_statement);
    TEST_RUN("code_gen_while_statement", test_code_gen_while_statement);
    TEST_RUN("code_gen_for_statement", test_code_gen_for_statement);
}
