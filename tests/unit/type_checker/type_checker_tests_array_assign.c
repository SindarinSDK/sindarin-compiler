// type_checker_tests_array_assign.c
// Array assignment type checker tests

static void test_type_check_array_assignment_matching()
{
    DEBUG_INFO("Starting test_type_check_array_assignment_matching");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    Token lit4_tok;
    setup_literal_token(&lit4_tok, TOKEN_INT_LITERAL, "4", 2, "test.sn", &arena);
    LiteralValue v4 = {.int_value = 4};
    Expr *e4 = ast_create_literal_expr(&arena, v4, int_type, false, &lit4_tok);
    Token lit5_tok;
    setup_literal_token(&lit5_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue v5 = {.int_value = 5};
    Expr *e5 = ast_create_literal_expr(&arena, v5, int_type, false, &lit5_tok);
    Expr *new_elements[2] = {e4, e5};
    Token new_arr_tok;
    setup_token(&new_arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *new_arr = ast_create_array_expr(&arena, new_elements, 2, &new_arr_tok);
    Expr *assign = ast_create_assign_expr(&arena, arr_tok, new_arr, NULL);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, assign_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(assign->expr_type != NULL);
    assert(ast_type_equals(assign->expr_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_assignment_matching");
}

static void test_type_check_array_assignment_mismatch()
{
    DEBUG_INFO("Starting test_type_check_array_assignment_mismatch");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "1.5", 2, "test.sn", &arena);
    LiteralValue val = {.double_value = 1.5};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);
    Expr *elements[1] = {lit};
    Token new_arr_tok;
    setup_token(&new_arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *new_arr = ast_create_array_expr(&arena, elements, 1, &new_arr_tok);
    Expr *assign = ast_create_assign_expr(&arena, arr_tok, new_arr, NULL);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, assign_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_assignment_mismatch");
}

static void test_type_check_nested_array()
{
    DEBUG_INFO("Starting test_type_check_nested_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *inner_arr_type = ast_create_array_type(&arena, int_type);
    Type *outer_arr_type = ast_create_array_type(&arena, inner_arr_type);

    Token nested_tok;
    setup_token(&nested_tok, TOKEN_IDENTIFIER, "nested", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, nested_tok, outer_arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *sym = symbol_table_lookup_symbol(&table, nested_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, outer_arr_type));
    assert(sym->type->as.array.element_type->kind == TYPE_ARRAY);
    assert(sym->type->as.array.element_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_nested_array");
}
