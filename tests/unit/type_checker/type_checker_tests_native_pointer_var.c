/**
 * type_checker_tests_native_pointer_var.c - Pointer variable and arithmetic tests
 *
 * Tests for pointer variable handling and arithmetic rejection.
 */

/* Suppress warning for operator name arrays used for readability but not in assertions */
#pragma clang diagnostic ignored "-Wunused-variable"

/* Test that pointer variables are REJECTED in regular (non-native) functions */
static void test_pointer_var_rejected_in_regular_function(void)
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

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a REGULAR function (not native) */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "regular_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* Regular function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - pointer vars not allowed in regular functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer variables are ACCEPTED in native functions */
static void test_pointer_var_accepted_in_native_function(void)
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

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a NATIVE function */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "native_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = true;  /* Native function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should PASS - pointer vars allowed in native functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test helper: create a binary expression with pointer and int */
static Stmt *create_pointer_arithmetic_stmt(Arena *arena, Type *ptr_type, Type *int_type, SnTokenType op)
{
    /* Create a pointer variable reference */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", arena);
    Expr *p_ref = ast_create_variable_expr(arena, p_tok, &p_tok);
    p_ref->expr_type = ptr_type;

    /* Create int literal 1 */
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", arena);
    LiteralValue val = {.int_value = 1};
    Expr *lit = ast_create_literal_expr(arena, val, int_type, false, &lit_tok);

    /* Create binary expression: p + 1 */
    Token op_tok;
    setup_test_token(&op_tok, TOKEN_PLUS, "+", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p_ref, op, lit, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer arithmetic is REJECTED for all operators (+, -, *, /, %) */
static void test_pointer_arithmetic_rejected(void)
{
    /* Test each arithmetic operator */
    SnTokenType operators[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO};
    const char *op_names[] = {"+", "-", "*", "/", "%%"};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p: *int = nil */
        Token p_tok;
        setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p + 1 (or p - 1, etc.) */
        Stmt *arith_stmt = create_pointer_arithmetic_stmt(&arena, ptr_int_type, int_type, operators[i]);

        /* Wrap in a native function (to allow pointer var declaration) */
        Stmt *body[2] = {p_decl, arith_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 0);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

static void test_type_checker_native_pointer_var_main(void)
{
    TEST_RUN("pointer_var_rejected_in_regular_function", test_pointer_var_rejected_in_regular_function);
    TEST_RUN("pointer_var_accepted_in_native_function", test_pointer_var_accepted_in_native_function);
    TEST_RUN("pointer_arithmetic_rejected", test_pointer_arithmetic_rejected);
}
