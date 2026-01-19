// tests/code_gen_tests_stmt.c
// Statement code generation tests

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
                                  "rt_print_string(\"Hello, world!\");\n"
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
                                  "void myfn(RtArena *);\n\n"
                                  "void myfn(RtArena *__caller_arena__) {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);\n"
                                  "    goto myfn_return;\n"
                                  "myfn_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return;\n"
                                  "}\n\n"
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
                                  "long long add(RtArena *, long long);\n\n"
                                  "long long add(RtArena *__caller_arena__, long long a) {\n"
                                  "    RtArena *__local_arena__ = rt_arena_create(__caller_arena__);\n"
                                  "    long long _return_value = 0;\n"
                                  "    _return_value = a;\n"
                                  "    goto add_return;\n"
                                  "add_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n\n"
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
                                  "    RtArena *__local_arena__ = rt_arena_create(NULL);\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    rt_arena_destroy(__local_arena__);\n"
                                  "    return _return_value;\n"
                                  "}\n\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_main_function_special_case");
}

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
                                  "{\n"
                                  "    long long block_var = 0;\n"
                                  "}\n"
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
                                  "if (1L) {\n"
                                  "    print;\n"
                                  "}\n"
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
                                  "while (1L) {\n"
                                  "    print;\n"
                                  "}\n"
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
                                  "{\n"
                                  "    long long k = 0LL;\n"
                                  "    while (rt_lt_long(k, 5LL)) {\n"
                                  "        rt_print_long(k);\n"
                                  "    __for_continue_0__:;\n"
                                  "        rt_post_inc_long(&k);\n"
                                  "    }\n"
                                  "}\n"
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

    DEBUG_INFO("Finished test_code_gen_for_statement");
}

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

    const char *expected = get_expected(&arena,
                                  "{\n"
                                  "    char * s = rt_to_string_string(NULL, \"test\");\n"
                                  "    if (s) {\n"
                                  "        rt_free_string(s);\n"
                                  "    }\n"
                                  "}\n"
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
                                  "long long counter = 0;\n"
                                  "rt_post_inc_long(&counter);\n"
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

    DEBUG_INFO("Finished test_code_gen_module_no_main_adds_dummy");
}

void test_code_gen_stmt_main(void)
{
    TEST_SECTION("Code Gen Statement Tests");
    TEST_RUN("code_gen_call_expression_simple", test_code_gen_call_expression_simple);
    TEST_RUN("code_gen_function_simple_void", test_code_gen_function_simple_void);
    TEST_RUN("code_gen_function_with_params_and_return", test_code_gen_function_with_params_and_return);
    TEST_RUN("code_gen_main_function_special_case", test_code_gen_main_function_special_case);
    TEST_RUN("code_gen_block_statement", test_code_gen_block_statement);
    TEST_RUN("code_gen_if_statement", test_code_gen_if_statement);
    TEST_RUN("code_gen_while_statement", test_code_gen_while_statement);
    TEST_RUN("code_gen_for_statement", test_code_gen_for_statement);
    TEST_RUN("code_gen_string_free_in_block", test_code_gen_string_free_in_block);
    TEST_RUN("code_gen_increment_decrement", test_code_gen_increment_decrement);
    TEST_RUN("code_gen_null_expression", test_code_gen_null_expression);
    TEST_RUN("code_gen_new_label", test_code_gen_new_label);
    TEST_RUN("code_gen_module_no_main_adds_dummy", test_code_gen_module_no_main_adds_dummy);
}
