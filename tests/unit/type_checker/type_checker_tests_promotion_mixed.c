// type_checker_tests_promotion_mixed.c
// Type checker tests for mixed type operations and pointer types

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

