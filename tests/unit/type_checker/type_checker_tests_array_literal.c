// type_checker_tests_array_literal.c
// Array literal type checker tests

static void test_type_check_array_literal_empty()
{
    DEBUG_INFO("Starting test_type_check_array_literal_empty");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, NULL, 0, &arr_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_lit, &arr_tok);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *empty_arr_type = ast_create_array_type(&arena, nil_type);
    assert(ast_type_equals(arr_lit->expr_type, empty_arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_literal_empty");
}

