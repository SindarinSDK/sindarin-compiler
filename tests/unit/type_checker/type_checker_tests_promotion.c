// tests/type_checker_tests_promotion.c
// Type checker tests for numeric type promotion (int -> double)

static void test_type_check_int_double_addition()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var pi: double = 3.14
    Token pi_tok;
    setup_token(&pi_tok, TOKEN_IDENTIFIER, "pi", 1, "test.sn", &arena);
    Token pi_lit_tok;
    setup_literal_token(&pi_lit_tok, TOKEN_DOUBLE_LITERAL, "3.14", 1, "test.sn", &arena);
    LiteralValue pi_val = {.double_value = 3.14};
    Expr *pi_init = ast_create_literal_expr(&arena, pi_val, double_type, false, &pi_lit_tok);
    Stmt *pi_decl = ast_create_var_decl_stmt(&arena, pi_tok, double_type, pi_init, NULL);

    // Create: var result: double = pi * 2 (int literal)
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 2, "test.sn", &arena);

    Expr *pi_var = ast_create_variable_expr(&arena, pi_tok, NULL);

    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 2};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token star_tok;
    setup_token(&star_tok, TOKEN_STAR, "*", 2, "test.sn", &arena);
    Expr *mult = ast_create_binary_expr(&arena, pi_var, TOKEN_STAR, int_lit, &star_tok);

    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, double_type, mult, NULL);

    // Wrap in a function
    Stmt *body[2] = {pi_decl, result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int promotes to double

    // Verify the multiplication expression has double type
    assert(mult->expr_type != NULL);
    assert(mult->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int_double_subtraction()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 10.5 - 3 (double - int)
    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "10.5", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 10.5};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 3};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token minus_tok;
    setup_token(&minus_tok, TOKEN_MINUS, "-", 1, "test.sn", &arena);
    Expr *sub = ast_create_binary_expr(&arena, double_lit, TOKEN_MINUS, int_lit, &minus_tok);

    // var result: double = 10.5 - 3
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, double_type, sub, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int promotes to double

    // Verify the subtraction expression has double type
    assert(sub->expr_type != NULL);
    assert(sub->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int_int_no_promotion()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 5 + 3 (int + int)
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 5};
    Expr *int_lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 3};
    Expr *int_lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);

    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 1, "test.sn", &arena);
    Expr *add = ast_create_binary_expr(&arena, int_lit1, TOKEN_PLUS, int_lit2, &plus_tok);

    // var result: int = 5 + 3
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, int_type, add, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the addition expression has int type (no promotion)
    assert(add->expr_type != NULL);
    assert(add->expr_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int_double_comparison()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 5 < 5.5 (int < double)
    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "5.5", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 5.5};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token less_tok;
    setup_token(&less_tok, TOKEN_LESS, "<", 1, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, int_lit, TOKEN_LESS, double_lit, &less_tok);

    // var result: bool = 5 < 5.5
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int/double comparison is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_double_int_equality()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create binary expression: 5.0 == 5 (double == int)
    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "5.0", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 5.0};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);

    Token eq_tok;
    setup_token(&eq_tok, TOKEN_EQUAL_EQUAL, "==", 1, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, double_lit, TOKEN_EQUAL_EQUAL, int_lit, &eq_tok);

    // var result: bool = 5.0 == 5
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - double/int equality is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int_double_greater()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create variable declarations
    Token i_tok;
    setup_token(&i_tok, TOKEN_IDENTIFIER, "i", 1, "test.sn", &arena);
    Token int_lit_tok;
    setup_literal_token(&int_lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 5};
    Expr *int_init = ast_create_literal_expr(&arena, int_val, int_type, false, &int_lit_tok);
    Stmt *i_decl = ast_create_var_decl_stmt(&arena, i_tok, int_type, int_init, NULL);

    Token d_tok;
    setup_token(&d_tok, TOKEN_IDENTIFIER, "d", 2, "test.sn", &arena);
    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "2.5", 2, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 2.5};
    Expr *double_init = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);
    Stmt *d_decl = ast_create_var_decl_stmt(&arena, d_tok, double_type, double_init, NULL);

    // Create comparison: i > d
    Expr *i_var = ast_create_variable_expr(&arena, i_tok, NULL);
    Expr *d_var = ast_create_variable_expr(&arena, d_tok, NULL);

    Token gt_tok;
    setup_token(&gt_tok, TOKEN_GREATER, ">", 3, "test.sn", &arena);
    Expr *cmp = ast_create_binary_expr(&arena, i_var, TOKEN_GREATER, d_var, &gt_tok);

    // var result: bool = i > d
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 3, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, bool_type, cmp, NULL);

    // Wrap in a function
    Stmt *body[3] = {i_decl, d_decl, result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 3, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - int/double comparison is valid

    // Verify the comparison expression has bool type
    assert(cmp->expr_type != NULL);
    assert(cmp->expr_type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int32_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: int32 = 42 (using int literal, which should be compatible)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int32_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_uint_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: uint = 42 (using int literal, which should be compatible)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, uint_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_uint32_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: uint32 = 42
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *init = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, uint32_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_float_var_decl()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create: var x: float = 3.14 (using double literal)
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "3.14", 1, "test.sn", &arena);
    LiteralValue val = {.double_value = 3.14};
    Expr *init = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, float_type, init, NULL);

    // Wrap in a function
    Stmt *body[1] = {x_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_int32_addition()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create two int32 literals and add them
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 5};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int32_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 3};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int32_type, false, &lit2_tok);

    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 1, "test.sn", &arena);
    Expr *add = ast_create_binary_expr(&arena, lit1, TOKEN_PLUS, lit2, &plus_tok);

    // var result: int32 = 5 + 3
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, int32_type, add, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify the addition expression has int32 type
    assert(add->expr_type != NULL);
    assert(add->expr_type->kind == TYPE_INT32);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_float_double_promotion()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create float + double -> double
    Token float_lit_tok;
    setup_literal_token(&float_lit_tok, TOKEN_DOUBLE_LITERAL, "1.5", 1, "test.sn", &arena);
    LiteralValue float_val = {.double_value = 1.5};
    Expr *float_lit = ast_create_literal_expr(&arena, float_val, float_type, false, &float_lit_tok);

    Token double_lit_tok;
    setup_literal_token(&double_lit_tok, TOKEN_DOUBLE_LITERAL, "2.5", 1, "test.sn", &arena);
    LiteralValue double_val = {.double_value = 2.5};
    Expr *double_lit = ast_create_literal_expr(&arena, double_val, double_type, false, &double_lit_tok);

    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 1, "test.sn", &arena);
    Expr *add = ast_create_binary_expr(&arena, float_lit, TOKEN_PLUS, double_lit, &plus_tok);

    // var result: double = 1.5f + 2.5
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, double_type, add, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - float promotes to double

    // Verify the addition expression has double type
    assert(add->expr_type != NULL);
    assert(add->expr_type->kind == TYPE_DOUBLE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_interop_type_mismatch()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create two incompatible literals: int32 + uint should fail
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 5};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int32_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 3};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, uint_type, false, &lit2_tok);

    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 1, "test.sn", &arena);
    Expr *add = ast_create_binary_expr(&arena, lit1, TOKEN_PLUS, lit2, &plus_tok);

    // var result: int32 = int32 + uint (type mismatch)
    Token result_tok;
    setup_token(&result_tok, TOKEN_IDENTIFIER, "result", 1, "test.sn", &arena);
    Stmt *result_decl = ast_create_var_decl_stmt(&arena, result_tok, int32_type, add, NULL);

    // Wrap in a function
    Stmt *body[1] = {result_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail - int32 and uint are incompatible

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_pointer_nil_assignment()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    // Create: var p: *int = nil
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_literal_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    // Wrap in a NATIVE function (pointer variables only allowed in native functions)
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = true;  // Mark as native

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - nil is compatible with pointer type in native fn

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_double_pointer()
{
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);
    Type *ptr_ptr_int_type = ast_create_pointer_type(&arena, ptr_int_type);

    // Create: var pp: **int = nil
    Token pp_tok;
    setup_token(&pp_tok, TOKEN_IDENTIFIER, "pp", 1, "test.sn", &arena);
    Token nil_tok;
    setup_literal_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *pp_decl = ast_create_var_decl_stmt(&arena, pp_tok, ptr_ptr_int_type, nil_lit, NULL);

    // Wrap in a NATIVE function (pointer variables only allowed in native functions)
    Stmt *body[1] = {pp_decl};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = true;  // Mark as native

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass - nil is compatible with pointer type in native fn

    // Verify the pointer type structure
    assert(ptr_ptr_int_type->kind == TYPE_POINTER);
    assert(ptr_ptr_int_type->as.pointer.base_type->kind == TYPE_POINTER);
    assert(ptr_ptr_int_type->as.pointer.base_type->as.pointer.base_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_check_pointer_type_equality()
{
    Arena arena;
    arena_init(&arena, 4096);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *ptr_int_type1 = ast_create_pointer_type(&arena, int_type);
    Type *ptr_int_type2 = ast_create_pointer_type(&arena, int_type);
    Type *ptr_double_type = ast_create_pointer_type(&arena, double_type);

    // Same pointer types should be equal
    assert(ast_type_equals(ptr_int_type1, ptr_int_type2) == 1);

    // Different pointer types should not be equal
    assert(ast_type_equals(ptr_int_type1, ptr_double_type) == 0);

    arena_free(&arena);
}

void test_type_checker_promotion_main()
{
    TEST_SECTION("Type Checker Promotion");

    TEST_RUN("int_double_addition", test_type_check_int_double_addition);
    TEST_RUN("int_double_subtraction", test_type_check_int_double_subtraction);
    TEST_RUN("int_int_no_promotion", test_type_check_int_int_no_promotion);
    TEST_RUN("int_double_comparison", test_type_check_int_double_comparison);
    TEST_RUN("double_int_equality", test_type_check_double_int_equality);
    TEST_RUN("int_double_greater", test_type_check_int_double_greater);
    TEST_RUN("int32_var_decl", test_type_check_int32_var_decl);
    TEST_RUN("uint_var_decl", test_type_check_uint_var_decl);
    TEST_RUN("uint32_var_decl", test_type_check_uint32_var_decl);
    TEST_RUN("float_var_decl", test_type_check_float_var_decl);
    TEST_RUN("int32_addition", test_type_check_int32_addition);
    TEST_RUN("float_double_promotion", test_type_check_float_double_promotion);
    TEST_RUN("interop_type_mismatch", test_type_check_interop_type_mismatch);
    TEST_RUN("pointer_nil_assignment", test_type_check_pointer_nil_assignment);
    TEST_RUN("double_pointer", test_type_check_double_pointer);
    TEST_RUN("pointer_type_equality", test_type_check_pointer_type_equality);
}
