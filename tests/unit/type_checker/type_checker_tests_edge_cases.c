// tests/type_checker_tests_edge_cases.c
// Edge case tests for type checker

// =====================================================
// Type Equality Edge Cases
// =====================================================

static void test_type_equality_same_primitives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int2 = ast_create_primitive_type(&arena, TYPE_INT);

    assert(ast_type_equals(int1, int2) == 1);

    arena_free(&arena);
}

static void test_type_equality_different_primitives(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    assert(ast_type_equals(int_type, bool_type) == 0);
    assert(ast_type_equals(int_type, char_type) == 0);
    assert(ast_type_equals(int_type, double_type) == 0);
    assert(ast_type_equals(int_type, string_type) == 0);
    assert(ast_type_equals(bool_type, char_type) == 0);

    arena_free(&arena);
}

static void test_type_equality_arrays_same_element(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *arr2 = ast_create_array_type(&arena, int_type);

    assert(ast_type_equals(arr1, arr2) == 1);

    arena_free(&arena);
}

static void test_type_equality_arrays_different_element(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *arr_int = ast_create_array_type(&arena, int_type);
    Type *arr_bool = ast_create_array_type(&arena, bool_type);

    assert(ast_type_equals(arr_int, arr_bool) == 0);

    arena_free(&arena);
}

static void test_type_equality_nested_arrays(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *nested_arr1 = ast_create_array_type(&arena, arr1);

    Type *arr2 = ast_create_array_type(&arena, int_type);
    Type *nested_arr2 = ast_create_array_type(&arena, arr2);

    assert(ast_type_equals(nested_arr1, nested_arr2) == 1);

    arena_free(&arena);
}

static void test_type_equality_function_types(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *params1[] = {int_type, int_type};
    Type *params2[] = {int_type, int_type};

    Type *fn1 = ast_create_function_type(&arena, int_type, params1, 2);
    Type *fn2 = ast_create_function_type(&arena, int_type, params2, 2);

    assert(ast_type_equals(fn1, fn2) == 1);

    // Different return type
    Type *fn3 = ast_create_function_type(&arena, void_type, params1, 2);
    assert(ast_type_equals(fn1, fn3) == 0);

    arena_free(&arena);
}

static void test_type_equality_function_different_params(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *params1[] = {int_type};
    Type *params2[] = {bool_type};
    Type *params3[] = {int_type, int_type};

    Type *fn1 = ast_create_function_type(&arena, int_type, params1, 1);
    Type *fn2 = ast_create_function_type(&arena, int_type, params2, 1);
    Type *fn3 = ast_create_function_type(&arena, int_type, params3, 2);

    // Different param type
    assert(ast_type_equals(fn1, fn2) == 0);
    // Different param count
    assert(ast_type_equals(fn1, fn3) == 0);

    arena_free(&arena);
}

static void test_type_equality_with_null(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    assert(ast_type_equals(NULL, NULL) == 0);
    assert(ast_type_equals(int_type, NULL) == 0);
    assert(ast_type_equals(NULL, int_type) == 0);

    arena_free(&arena);
}

// =====================================================
// Type Coercion Edge Cases
// =====================================================

static void test_coercion_int_to_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // int is coercible to double
    assert(is_type_coercible_to(int_type, double_type) == 1);

    arena_free(&arena);
}

static void test_coercion_double_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // double is NOT coercible to int
    assert(is_type_coercible_to(double_type, int_type) == 0);

    arena_free(&arena);
}

static void test_coercion_byte_to_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // byte is coercible to int
    assert(is_type_coercible_to(byte_type, int_type) == 1);

    arena_free(&arena);
}

static void test_coercion_char_to_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // char is coercible to int
    assert(is_type_coercible_to(char_type, int_type) == 1);

    arena_free(&arena);
}

static void test_coercion_same_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    // Same types are coercible
    assert(is_type_coercible_to(int_type, int_type) == 1);
    assert(is_type_coercible_to(double_type, double_type) == 1);
    assert(is_type_coercible_to(bool_type, bool_type) == 1);

    arena_free(&arena);
}

static void test_coercion_string_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // string is NOT coercible to int
    assert(is_type_coercible_to(string_type, int_type) == 0);

    arena_free(&arena);
}

static void test_coercion_bool_to_int_fails(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // bool is NOT coercible to int (in strict typing)
    assert(is_type_coercible_to(bool_type, int_type) == 0);

    arena_free(&arena);
}

// =====================================================
// Literal Expression Type Checking
// =====================================================

static void test_literal_int_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(lit->expr_type != NULL);
    assert(lit->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_literal_bool_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val = {.bool_value = true};
    Expr *lit = ast_create_literal_expr(&arena, val, bool_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "flag", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_literal_string_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_STRING_LITERAL, "hello", 1, "test.sn", &arena);
    LiteralValue val = {.string_value = "hello"};
    Expr *lit = ast_create_literal_expr(&arena, val, string_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "msg", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, string_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_literal_char_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_CHAR_LITERAL, "a", 1, "test.sn", &arena);
    LiteralValue val = {.char_value = 'a'};
    Expr *lit = ast_create_literal_expr(&arena, val, char_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "ch", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, char_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Binary Expression Edge Cases
// =====================================================

static void test_binary_logical_and(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val1 = {.bool_value = true};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, bool_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue val2 = {.bool_value = false};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, bool_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_AND, "&&", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_AND, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_logical_or(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val1 = {.bool_value = true};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, bool_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue val2 = {.bool_value = false};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, bool_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_OR, "||", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_OR, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_comparison_lt(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_LESS, "<", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_LESS, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_binary_modulo(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 10};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 3};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MODULO, "%", 1, "test.sn", &arena);
    Expr *binary = ast_create_binary_expr(&arena, lit1, TOKEN_MODULO, lit2, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, binary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(binary->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Unary Expression Edge Cases
// =====================================================

static void test_unary_not_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue val = {.bool_value = true};
    Expr *lit = ast_create_literal_expr(&arena, val, bool_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_BANG, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, bool_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_unary_negate_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MINUS, "-", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_MINUS, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_unary_negate_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "3.14", 1, "test.sn", &arena);
    LiteralValue val = {.double_value = 3.14};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);

    Token op_tok;
    setup_token(&op_tok, TOKEN_MINUS, "-", 1, "test.sn", &arena);
    Expr *unary = ast_create_unary_expr(&arena, TOKEN_MINUS, lit, &op_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, double_type, unary, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);
    assert(unary->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Variable Declaration Edge Cases
// =====================================================

static void test_var_decl_no_initializer(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_var_decl_mismatch_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var x: int = "hello" - type mismatch
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_STRING_LITERAL, "hello", 1, "test.sn", &arena);
    LiteralValue val = {.string_value = "hello"};
    Expr *lit = ast_create_literal_expr(&arena, val, string_type, false, &lit_tok);

    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit, NULL);

    Stmt *body[1] = {decl};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Type Size and Alignment
// =====================================================

static void test_type_size_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    assert(get_type_size(int_type) == 8);

    arena_free(&arena);
}

static void test_type_size_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(get_type_size(bool_type) == 1);

    arena_free(&arena);
}

static void test_type_size_char(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(get_type_size(char_type) == 1);

    arena_free(&arena);
}

static void test_type_size_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(get_type_size(double_type) == 8);

    arena_free(&arena);
}

static void test_type_size_string(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(get_type_size(string_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    assert(get_type_size(arr_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_pointer(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr_type = ast_create_pointer_type(&arena, int_type);
    assert(get_type_size(ptr_type) == 8);  // Pointer size

    arena_free(&arena);
}

static void test_type_size_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(get_type_size(void_type) == 0);

    arena_free(&arena);
}

static void test_type_size_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    assert(get_type_size(byte_type) == 1);

    arena_free(&arena);
}

static void test_type_size_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(get_type_size(long_type) == 8);

    arena_free(&arena);
}

// =====================================================
// Return Type Checking
// =====================================================

static void test_return_type_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret = ast_create_return_stmt(&arena, ret_tok, lit, &ret_tok);

    Stmt *body[1] = {ret};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "get_value", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, int_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_return_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret = ast_create_return_stmt(&arena, ret_tok, NULL, &ret_tok);

    Stmt *body[1] = {ret};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "do_nothing", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Assignment Type Checking
// =====================================================

static void test_assignment_same_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var x: int = 1
    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit1, NULL);

    // x = 2
    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);
    Token eq_tok;
    setup_token(&eq_tok, TOKEN_EQUAL, "=", 2, "test.sn", &arena);
    Expr *assign = ast_create_assign_expr(&arena, var_tok, lit2, &eq_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, &eq_tok);

    Stmt *body[2] = {decl, assign_stmt};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_edge_cases_main(void)
{
    TEST_SECTION("Type Checker Edge Cases");

    // Type equality
    TEST_RUN("type_equality_same_primitives", test_type_equality_same_primitives);
    TEST_RUN("type_equality_different_primitives", test_type_equality_different_primitives);
    TEST_RUN("type_equality_arrays_same_element", test_type_equality_arrays_same_element);
    TEST_RUN("type_equality_arrays_different_element", test_type_equality_arrays_different_element);
    TEST_RUN("type_equality_nested_arrays", test_type_equality_nested_arrays);
    TEST_RUN("type_equality_function_types", test_type_equality_function_types);
    TEST_RUN("type_equality_function_different_params", test_type_equality_function_different_params);
    TEST_RUN("type_equality_with_null", test_type_equality_with_null);

    // Type coercion
    TEST_RUN("coercion_int_to_double", test_coercion_int_to_double);
    TEST_RUN("coercion_double_to_int_fails", test_coercion_double_to_int_fails);
    TEST_RUN("coercion_byte_to_int", test_coercion_byte_to_int);
    TEST_RUN("coercion_char_to_int", test_coercion_char_to_int);
    TEST_RUN("coercion_same_type", test_coercion_same_type);
    TEST_RUN("coercion_string_to_int_fails", test_coercion_string_to_int_fails);
    TEST_RUN("coercion_bool_to_int_fails", test_coercion_bool_to_int_fails);

    // Literal expressions
    TEST_RUN("literal_int_type", test_literal_int_type);
    TEST_RUN("literal_bool_type", test_literal_bool_type);
    TEST_RUN("literal_string_type", test_literal_string_type);
    TEST_RUN("literal_char_type", test_literal_char_type);

    // Binary expressions
    TEST_RUN("binary_logical_and", test_binary_logical_and);
    TEST_RUN("binary_logical_or", test_binary_logical_or);
    TEST_RUN("binary_comparison_lt", test_binary_comparison_lt);
    TEST_RUN("binary_modulo", test_binary_modulo);

    // Unary expressions
    TEST_RUN("unary_not_bool", test_unary_not_bool);
    TEST_RUN("unary_negate_int", test_unary_negate_int);
    TEST_RUN("unary_negate_double", test_unary_negate_double);

    // Variable declarations
    TEST_RUN("var_decl_no_initializer", test_var_decl_no_initializer);
    TEST_RUN("var_decl_mismatch_type_error", test_var_decl_mismatch_type_error);

    // Type sizes
    TEST_RUN("type_size_int", test_type_size_int);
    TEST_RUN("type_size_bool", test_type_size_bool);
    TEST_RUN("type_size_char", test_type_size_char);
    TEST_RUN("type_size_double", test_type_size_double);
    TEST_RUN("type_size_string", test_type_size_string);
    TEST_RUN("type_size_array", test_type_size_array);
    TEST_RUN("type_size_pointer", test_type_size_pointer);
    TEST_RUN("type_size_void", test_type_size_void);
    TEST_RUN("type_size_byte", test_type_size_byte);
    TEST_RUN("type_size_long", test_type_size_long);

    // Return statements
    TEST_RUN("return_type_match", test_return_type_match);
    TEST_RUN("return_void", test_return_void);

    // Assignments
    TEST_RUN("assignment_same_type", test_assignment_same_type);
}
