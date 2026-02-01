/**
 * type_checker_tests_native_pointer_compare.c - Pointer comparison tests
 *
 * Tests for pointer comparison with nil and other pointers.
 */

/* Test helper: create a comparison expression with two pointers */
static Stmt *create_pointer_comparison_stmt(Arena *arena, Type *ptr_type, SnTokenType op, bool use_nil_as_right)
{
    /* Create a pointer variable reference */
    Token p1_tok;
    setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", arena);
    Expr *p1_ref = ast_create_variable_expr(arena, p1_tok, &p1_tok);
    p1_ref->expr_type = ptr_type;

    Expr *right_operand;
    if (use_nil_as_right)
    {
        /* Create nil literal */
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", arena);
        Type *nil_type = ast_create_primitive_type(arena, TYPE_NIL);
        LiteralValue nil_val = {.int_value = 0};
        right_operand = ast_create_literal_expr(arena, nil_val, nil_type, false, &nil_tok);
    }
    else
    {
        /* Create second pointer variable reference */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", arena);
        right_operand = ast_create_variable_expr(arena, p2_tok, &p2_tok);
        right_operand->expr_type = ptr_type;
    }

    /* Create binary expression: p1 == p2 or p1 == nil */
    Token op_tok;
    setup_test_token(&op_tok, op == TOKEN_EQUAL_EQUAL ? TOKEN_EQUAL_EQUAL : TOKEN_BANG_EQUAL,
                     op == TOKEN_EQUAL_EQUAL ? "==" : "!=", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p1_ref, op, right_operand, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer equality (==, !=) with nil is ALLOWED */
static void test_pointer_nil_comparison_allowed(void)
{
    SnTokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
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

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p1 == nil or p1 != nil */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], true);

        /* Wrap in a native function */
        Stmt *body[2] = {p1_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 1);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

/* Test that pointer-to-pointer equality (==, !=) is ALLOWED */
static void test_pointer_pointer_comparison_allowed(void)
{
    SnTokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
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

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok1;
        setup_test_token(&nil_tok1, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit1 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok1);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit1, NULL);

        /* Create: var p2: *int = nil */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", &arena);
        Token nil_tok2;
        setup_test_token(&nil_tok2, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        Expr *nil_lit2 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok2);
        Stmt *p2_decl = ast_create_var_decl_stmt(&arena, p2_tok, ptr_int_type, nil_lit2, NULL);

        /* Create: p1 == p2 or p1 != p2 */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], false);

        /* Wrap in a native function */
        Stmt *body[3] = {p1_decl, p2_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 3, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        assert(no_error == 1);

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

}

static void test_type_checker_native_pointer_compare_main(void)
{
    TEST_RUN("pointer_nil_comparison_allowed", test_pointer_nil_comparison_allowed);
    TEST_RUN("pointer_pointer_comparison_allowed", test_pointer_pointer_comparison_allowed);
}
