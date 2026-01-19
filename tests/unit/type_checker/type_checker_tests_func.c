// tests/type_checker_tests_func.c
// Function return array and complex type checker tests

static void test_type_check_function_return_array()
{
    DEBUG_INFO("Starting test_type_check_function_return_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Parameter *params = NULL;
    int param_count = 0;

    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);

    Expr *elements[2] = {e1, e2};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, arr_lit, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "create_arr", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, param_count, arr_type, body, 1, &func_name_tok);

    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);

    Token call_name_tok;
    setup_token(&call_name_tok, TOKEN_IDENTIFIER, "create_arr", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_name_tok, NULL);
    Expr **args = NULL;
    int arg_count = 0;
    Expr *call = ast_create_call_expr(&arena, callee, args, arg_count, &call_name_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, call, NULL);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, var_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_name_tok);
    assert(func_sym != NULL);
    assert(func_sym->type != NULL);
    assert(func_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(func_sym->type->as.function.return_type, arr_type));
    assert(func_sym->type->as.function.param_count == 0);

    Symbol *var_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(var_sym != NULL);
    assert(ast_type_equals(var_sym->type, arr_type));

    assert(call->expr_type != NULL);
    assert(ast_type_equals(call->expr_type, arr_type));

    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_function_return_array");
}

static void test_type_check_var_decl_function_call_array()
{
    DEBUG_INFO("Starting test_type_check_var_decl_function_call_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token print_tok;
    setup_token(&print_tok, TOKEN_IDENTIFIER, "print", 5, "test.sn", &arena);

    Type *print_arg_types[1] = {string_type};
    Type *print_func_type = ast_create_function_type(&arena, void_type, print_arg_types, 1);
    symbol_table_add_symbol_with_kind(&table, print_tok, print_func_type, SYMBOL_LOCAL);

    Parameter *declare_params = NULL;
    int declare_param_count = 0;

    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);

    Token lit3_tok;
    setup_literal_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);

    Expr *elements[3] = {e1, e2, e3};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 3, &arr_lit_tok);

    Token int_arr_tok;
    setup_token(&int_arr_tok, TOKEN_IDENTIFIER, "int_arr", 7, "test.sn", &arena);
    Stmt *int_arr_decl = ast_create_var_decl_stmt(&arena, int_arr_tok, arr_type, arr_lit, NULL);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 6, "test.sn", &arena);
    Expr *int_arr_var = ast_create_variable_expr(&arena, int_arr_tok, NULL);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, int_arr_var, &ret_tok);

    Stmt *declare_body[2] = {int_arr_decl, ret_stmt};
    Token declare_name_tok;
    setup_token(&declare_name_tok, TOKEN_IDENTIFIER, "declare_basic_int_array", 22, "test.sn", &arena);
    Stmt *declare_func = ast_create_function_stmt(&arena, declare_name_tok, declare_params, declare_param_count, arr_type, declare_body, 2, &declare_name_tok);

    int print_param_count = 1;
    Parameter *print_params = (Parameter *)arena_alloc(&arena, sizeof(Parameter) * print_param_count);
    print_params[0].name.type = TOKEN_IDENTIFIER;
    print_params[0].name.start = arena_strdup(&arena, "arr");
    print_params[0].name.length = 3;
    print_params[0].name.line = 5;
    print_params[0].name.filename = "test.sn";
    print_params[0].type = arr_type;

    Token str_lit_tok;
    setup_literal_token(&str_lit_tok, TOKEN_STRING_LITERAL, "\"Int Array: \"", 13, "test.sn", &arena);
    LiteralValue str_val = {.string_value = arena_strdup(&arena, "Int Array: ")};
    Expr *str_part = ast_create_literal_expr(&arena, str_val, string_type, false, &str_lit_tok);

    Token interp_tok;
    setup_token(&interp_tok, TOKEN_INTERPOL_STRING, "$\"Int Array: {arr}\"", 18, "test.sn", &arena);
    Expr *arr_param_var = ast_create_variable_expr(&arena, print_params[0].name, NULL);
    Expr *interp_parts[2] = {str_part, arr_param_var};
    char *interp_fmts[2] = {NULL, NULL};
    Expr *interp = ast_create_interpolated_expr(&arena, interp_parts, interp_fmts, 2, &interp_tok);

    Expr *print_callee = ast_create_variable_expr(&arena, print_tok, NULL);
    Expr *print_args[1] = {interp};
    Stmt *print_call_stmt = ast_create_expr_stmt(&arena, ast_create_call_expr(&arena, print_callee, print_args, 1, &print_tok), &print_tok);

    Stmt *print_body[1] = {print_call_stmt};
    Token print_name_tok;
    setup_token(&print_name_tok, TOKEN_IDENTIFIER, "print_basic_int_array", 20, "test.sn", &arena);
    Stmt *print_func = ast_create_function_stmt(&arena, print_name_tok, print_params, print_param_count, void_type, print_body, 1, &print_name_tok);

    Parameter *main_params = NULL;
    int main_param_count = 0;

    Token main_arr_tok;
    setup_token(&main_arr_tok, TOKEN_IDENTIFIER, "arr", 3, "test.sn", &arena);
    Token main_call_name_tok;
    setup_token(&main_call_name_tok, TOKEN_IDENTIFIER, "declare_basic_int_array", 22, "test.sn", &arena);
    Expr *main_callee = ast_create_variable_expr(&arena, main_call_name_tok, NULL);
    Expr **main_call_args = NULL;
    int main_call_arg_count = 0;
    Expr *main_call = ast_create_call_expr(&arena, main_callee, main_call_args, main_call_arg_count, &main_call_name_tok);
    Stmt *main_arr_decl = ast_create_var_decl_stmt(&arena, main_arr_tok, arr_type, main_call, NULL);

    Token main_print_name_tok;
    setup_token(&main_print_name_tok, TOKEN_IDENTIFIER, "print_basic_int_array", 20, "test.sn", &arena);
    Expr *main_print_callee = ast_create_variable_expr(&arena, main_print_name_tok, NULL);
    Expr *main_print_args[1] = {ast_create_variable_expr(&arena, main_arr_tok, NULL)};
    Expr *main_print_call = ast_create_call_expr(&arena, main_print_callee, main_print_args, 1, &main_print_name_tok);
    Stmt *main_print_stmt = ast_create_expr_stmt(&arena, main_print_call, &main_print_name_tok);

    Stmt *main_body[2] = {main_arr_decl, main_print_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 4, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, main_params, main_param_count, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, declare_func);
    ast_module_add_statement(&arena, &module, print_func);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *declare_sym = symbol_table_lookup_symbol(&table, declare_name_tok);
    assert(declare_sym != NULL);
    assert(declare_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(declare_sym->type->as.function.return_type, arr_type));
    assert(declare_sym->type->as.function.param_count == 0);

    Symbol *print_sym = symbol_table_lookup_symbol(&table, print_name_tok);
    assert(print_sym != NULL);
    assert(print_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(print_sym->type->as.function.return_type, void_type));
    assert(print_sym->type->as.function.param_count == 1);
    assert(ast_type_equals(print_sym->type->as.function.param_types[0], arr_type));

    Symbol *main_sym = symbol_table_lookup_symbol(&table, main_name_tok);
    assert(main_sym != NULL);
    assert(main_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(main_sym->type->as.function.return_type, void_type));
    assert(main_sym->type->as.function.param_count == 0);

    assert(main_call->expr_type != NULL);
    assert(ast_type_equals(main_call->expr_type, arr_type));

    assert(interp->expr_type != NULL);
    assert(interp->expr_type->kind == TYPE_STRING);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_var_decl_function_call_array");
}

void test_type_checker_func_main()
{
    TEST_SECTION("Type Checker Functions");

    TEST_RUN("function_return_array", test_type_check_function_return_array);
    TEST_RUN("var_decl_function_call_array", test_type_check_var_decl_function_call_array);
}
