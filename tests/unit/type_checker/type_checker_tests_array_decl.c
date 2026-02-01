// type_checker_tests_array_decl.c
// Array declaration type checker tests

static void test_type_check_array_decl_no_init()
{
    DEBUG_INFO("Starting test_type_check_array_decl_no_init");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, NULL, NULL);

    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *sym = symbol_table_lookup_symbol(&table, name_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_no_init");
}

static void test_type_check_array_decl_with_init_matching()
{
    DEBUG_INFO("Starting test_type_check_array_decl_with_init_matching");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, elem_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, elem_type, false, &lit2_tok);

    Expr *elements[2] = {lit1, lit2};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, arr_lit, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, elem_type));
    assert(ast_type_equals(arr_lit->expr_type, arr_type));

    Symbol *sym = symbol_table_lookup_symbol(&table, name_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_with_init_matching");
}

static void test_type_check_array_decl_with_init_mismatch()
{
    DEBUG_INFO("Starting test_type_check_array_decl_with_init_mismatch");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "1.5", 2, "test.sn", &arena);
    LiteralValue val = {.double_value = 1.5};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);

    Expr *elements[1] = {lit};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, arr_lit, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, double_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_with_init_mismatch");
}
