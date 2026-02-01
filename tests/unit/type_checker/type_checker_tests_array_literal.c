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

static void test_type_check_array_literal_heterogeneous()
{
    // Mixed-type array literals (truly incompatible types) return any[] type
    // Note: int and double are NOT mixed - they get promoted to double[]
    // Use int and str for truly incompatible types
    DEBUG_INFO("Starting test_type_check_array_literal_heterogeneous");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue val2 = {.string_value = "hello"};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, str_type, false, &lit2_tok);

    Expr *elements[2] = {lit1, lit2};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_lit, &arr_tok);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1); // Should succeed - mixed types produce any[]

    // Verify the result type is any[]
    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(arr_lit->expr_type->as.array.element_type != NULL);
    assert(arr_lit->expr_type->as.array.element_type->kind == TYPE_ANY);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_literal_heterogeneous");
}
