// tests/code_gen_tests_expr.c
// Expression code generation tests

static void test_code_gen_literal_expression(void)
{
    DEBUG_INFO("Starting test_code_gen_literal_expression");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_INT_LITERAL, "42");
    token_set_int_literal(&token, 42);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue val = {.int_value = 42};
    Expr *lit_expr = ast_create_literal_expr(&arena, val, int_type, false, &token);
    lit_expr->expr_type = int_type;
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, lit_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "42LL;\n"
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

    DEBUG_INFO("Finished test_code_gen_literal_expression");
}

static void test_code_gen_variable_expression(void)
{
    DEBUG_INFO("Starting test_code_gen_variable_expression");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_token;
    setup_basic_token(&var_token, TOKEN_IDENTIFIER, "x");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *init_expr = NULL; // Default 0
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_token, int_type, init_expr, &var_token);

    Expr *var_expr = ast_create_variable_expr(&arena, var_token, &var_token);
    var_expr->expr_type = int_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_token);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "long long x = 0;\n"
                                  "x;\n"
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

    DEBUG_INFO("Finished test_code_gen_variable_expression");
}

static void test_code_gen_binary_expression_int_add(void)
{
    DEBUG_INFO("Starting test_code_gen_binary_expression_int_add");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_PLUS, "+");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token left_tok, right_tok;
    setup_basic_token(&left_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&left_tok, 1);
    LiteralValue lval = {.int_value = 1};
    Expr *left = ast_create_literal_expr(&arena, lval, int_type, false, &left_tok);
    left->expr_type = int_type;

    setup_basic_token(&right_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&right_tok, 2);
    LiteralValue rval = {.int_value = 2};
    Expr *right = ast_create_literal_expr(&arena, rval, int_type, false, &right_tok);
    right->expr_type = int_type;

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
    bin_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, bin_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* Constant folding optimization: 1 + 2 is folded to 3L at compile time */
    const char *expected = get_expected(&arena,
                                  "3LL;\n"
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

    DEBUG_INFO("Finished test_code_gen_binary_expression_int_add");
}

static void test_code_gen_binary_expression_string_concat(void)
{
    DEBUG_INFO("Starting test_code_gen_binary_expression_string_concat");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_PLUS, "+");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token left_tok;
    setup_basic_token(&left_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    token_set_string_literal(&left_tok, "hello");
    LiteralValue lval = {.string_value = "hello"};
    Expr *left = ast_create_literal_expr(&arena, lval, str_type, false, &left_tok);
    left->expr_type = str_type;

    Token right_tok;
    setup_basic_token(&right_tok, TOKEN_STRING_LITERAL, "\"world\"");
    token_set_string_literal(&right_tok, "world");
    LiteralValue rval = {.string_value = "world"};
    Expr *right = ast_create_literal_expr(&arena, rval, str_type, false, &right_tok);
    right->expr_type = str_type;

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
    bin_expr->expr_type = str_type;
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, bin_expr, &token);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "{\n"
                                  "    char *_tmp = rt_str_concat(NULL, \"hello\", \"world\");\n"
                                  "    (void)_tmp;\n"
                                  "    rt_free_string(_tmp);\n"
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

    DEBUG_INFO("Finished test_code_gen_binary_expression_string_concat");
}

static void test_code_gen_unary_expression_negate(void)
{
    DEBUG_INFO("Starting test_code_gen_unary_expression_negate");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_MINUS, "-");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token op_tok;
    setup_basic_token(&op_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&op_tok, 5);
    LiteralValue val = {.int_value = 5};
    Expr *operand = ast_create_literal_expr(&arena, val, int_type, false, &op_tok);
    operand->expr_type = int_type;

    Expr *unary_expr = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &token);
    unary_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, unary_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* Constant folding optimization: -5 is folded to -5L at compile time */
    const char *expected = get_expected(&arena,
                                  "-5LL;\n"
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

    DEBUG_INFO("Finished test_code_gen_unary_expression_negate");
}

static void test_code_gen_assign_expression(void)
{
    DEBUG_INFO("Starting test_code_gen_assign_expression");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_basic_token(&name_tok, TOKEN_IDENTIFIER, "x");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *init_expr = NULL;
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, name_tok, int_type, init_expr, &name_tok);

    Token val_tok;
    setup_basic_token(&val_tok, TOKEN_INT_LITERAL, "10");
    token_set_int_literal(&val_tok, 10);
    LiteralValue val = {.int_value = 10};

    Expr *value = ast_create_literal_expr(&arena, val, int_type, false, &val_tok);
    value->expr_type = int_type;

    Expr *assign_expr = ast_create_assign_expr(&arena, name_tok, value, &name_tok);
    assign_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, assign_expr, &name_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "long long x = 0;\n"
                                  "(x = 10LL);\n"
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

    DEBUG_INFO("Finished test_code_gen_assign_expression");
}

/**
 * Test that *int as val generates C dereference: *(ptr)
 * This tests the primitive pointer unwrapping case in code_gen_as_val_expression.
 */
static void test_code_gen_as_val_int_pointer(void)
{
    DEBUG_INFO("Starting test_code_gen_as_val_int_pointer");

    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create pointer to int type: *int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create variable 'ptr' of type *int */
    Token ptr_token;
    setup_basic_token(&ptr_token, TOKEN_IDENTIFIER, "ptr");
    Expr *ptr_init = NULL;
    Stmt *ptr_decl = ast_create_var_decl_stmt(&arena, ptr_token, ptr_int_type, ptr_init, &ptr_token);

    /* Create 'ptr as val' expression */
    Expr *ptr_expr = ast_create_variable_expr(&arena, ptr_token, &ptr_token);
    ptr_expr->expr_type = ptr_int_type;

    Token as_val_token;
    setup_basic_token(&as_val_token, TOKEN_AS, "as");
    Expr *as_val_expr = ast_create_as_val_expr(&arena, ptr_expr, &as_val_token);
    as_val_expr->expr_type = int_type;
    as_val_expr->as.as_val.is_noop = false;
    as_val_expr->as.as_val.is_cstr_to_str = false;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, as_val_expr, &as_val_token);

    ast_module_add_statement(&arena, &module, ptr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* The key assertion: *int as val should generate *(ptr) */
    const char *expected = get_expected(&arena,
                                  "long long* ptr = 0;\n"
                                  "(*(ptr));\n"
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

    DEBUG_INFO("Finished test_code_gen_as_val_int_pointer");
}

/**
 * Test that *double as val generates C dereference: *(ptr)
 * This tests the primitive pointer unwrapping case for double type.
 */
static void test_code_gen_as_val_double_pointer(void)
{
    DEBUG_INFO("Starting test_code_gen_as_val_double_pointer");

    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create pointer to double type: *double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *ptr_double_type = ast_create_pointer_type(&arena, double_type);

    /* Create variable 'dptr' of type *double */
    Token dptr_token;
    setup_basic_token(&dptr_token, TOKEN_IDENTIFIER, "dptr");
    Expr *dptr_init = NULL;
    Stmt *dptr_decl = ast_create_var_decl_stmt(&arena, dptr_token, ptr_double_type, dptr_init, &dptr_token);

    /* Create 'dptr as val' expression */
    Expr *dptr_expr = ast_create_variable_expr(&arena, dptr_token, &dptr_token);
    dptr_expr->expr_type = ptr_double_type;

    Token as_val_token;
    setup_basic_token(&as_val_token, TOKEN_AS, "as");
    Expr *as_val_expr = ast_create_as_val_expr(&arena, dptr_expr, &as_val_token);
    as_val_expr->expr_type = double_type;
    as_val_expr->as.as_val.is_noop = false;
    as_val_expr->as.as_val.is_cstr_to_str = false;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, as_val_expr, &as_val_token);

    ast_module_add_statement(&arena, &module, dptr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* The key assertion: *double as val should generate *(dptr) */
    const char *expected = get_expected(&arena,
                                  "double* dptr = 0;\n"
                                  "(*(dptr));\n"
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

    DEBUG_INFO("Finished test_code_gen_as_val_double_pointer");
}

/**
 * Test that *char as val generates rt_arena_strdup call with NULL pointer fallback.
 * This tests the C string conversion case in code_gen_as_val_expression.
 * The generated code should:
 * 1. Check if the pointer is NULL
 * 2. If not NULL, call rt_arena_strdup with the pointer
 * 3. If NULL, return empty string via rt_arena_strdup(arena, "")
 */
static void test_code_gen_as_val_char_pointer(void)
{
    DEBUG_INFO("Starting test_code_gen_as_val_char_pointer");

    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create pointer to char type: *char */
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *ptr_char_type = ast_create_pointer_type(&arena, char_type);

    /* Create variable 'cptr' of type *char */
    Token cptr_token;
    setup_basic_token(&cptr_token, TOKEN_IDENTIFIER, "cptr");
    Expr *cptr_init = NULL;
    Stmt *cptr_decl = ast_create_var_decl_stmt(&arena, cptr_token, ptr_char_type, cptr_init, &cptr_token);

    /* Create 'cptr as val' expression */
    Expr *cptr_expr = ast_create_variable_expr(&arena, cptr_token, &cptr_token);
    cptr_expr->expr_type = ptr_char_type;

    Token as_val_token;
    setup_basic_token(&as_val_token, TOKEN_AS, "as");
    Expr *as_val_expr = ast_create_as_val_expr(&arena, cptr_expr, &as_val_token);
    /* Result type is str (TYPE_STRING), not char */
    as_val_expr->expr_type = ast_create_primitive_type(&arena, TYPE_STRING);
    as_val_expr->as.as_val.is_noop = false;
    as_val_expr->as.as_val.is_cstr_to_str = true;  /* This is the key flag for C string conversion */

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, as_val_expr, &as_val_token);

    ast_module_add_statement(&arena, &module, cptr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    /* The key assertions:
     * 1. *char as val emits rt_arena_strdup call
     * 2. NULL pointer is handled with empty string fallback
     * Generated pattern: ((cptr) ? rt_arena_strdup(arena, cptr) : rt_arena_strdup(arena, ""))
     * At global scope, arena is NULL */
    const char *expected = get_expected(&arena,
                                  "char* cptr = 0;\n"
                                  "((cptr) ? rt_arena_strdup(NULL, cptr) : rt_arena_strdup(NULL, \"\"));\n"
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

    DEBUG_INFO("Finished test_code_gen_as_val_char_pointer");
}

void test_code_gen_expr_main(void)
{
    TEST_SECTION("Code Gen Expression Tests");
    TEST_RUN("code_gen_literal_expression", test_code_gen_literal_expression);
    TEST_RUN("code_gen_variable_expression", test_code_gen_variable_expression);
    TEST_RUN("code_gen_binary_expression_int_add", test_code_gen_binary_expression_int_add);
    TEST_RUN("code_gen_binary_expression_string_concat", test_code_gen_binary_expression_string_concat);
    TEST_RUN("code_gen_unary_expression_negate", test_code_gen_unary_expression_negate);
    TEST_RUN("code_gen_assign_expression", test_code_gen_assign_expression);
    TEST_RUN("code_gen_as_val_int_pointer", test_code_gen_as_val_int_pointer);
    TEST_RUN("code_gen_as_val_double_pointer", test_code_gen_as_val_double_pointer);
    TEST_RUN("code_gen_as_val_char_pointer", test_code_gen_as_val_char_pointer);
}
