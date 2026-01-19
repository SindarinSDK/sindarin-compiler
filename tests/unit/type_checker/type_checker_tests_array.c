// tests/type_checker_tests_array.c
// Array declaration, literal, access, and assignment type checker tests

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

static void test_type_check_array_access_valid()
{
    DEBUG_INFO("Starting test_type_check_array_access_valid");

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
    Token lit1_tok, lit2_tok, lit3_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    setup_literal_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);
    Expr *elements[3] = {e1, e2, e3};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 3, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, idx, &access_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, access, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, x_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(access->expr_type != NULL);
    assert(ast_type_equals(access->expr_type, int_type));
    assert(var_arr->expr_type != NULL);
    assert(ast_type_equals(var_arr->expr_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_valid");
}

static void test_type_check_array_access_non_array()
{
    DEBUG_INFO("Starting test_type_check_array_access_non_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token num_tok;
    setup_token(&num_tok, TOKEN_IDENTIFIER, "num", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 5};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *num_decl = ast_create_var_decl_stmt(&arena, num_tok, int_type, lit, NULL);

    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_num = ast_create_variable_expr(&arena, num_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_num, idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, num_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_non_array");
}

static void test_type_check_array_access_invalid_index()
{
    DEBUG_INFO("Starting test_type_check_array_access_invalid_index");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"foo\"", 2, "test.sn", &arena);
    LiteralValue str_val = {.string_value = arena_strdup(&arena, "foo")};
    Expr *str_idx = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, str_idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_invalid_index");
}

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

static void test_type_check_array_slice_full()
{
    DEBUG_INFO("Starting test_type_check_array_slice_full");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // var arr:int[]
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    // var slice:int[] = arr[1..3]
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, start, end, NULL, &arr_tok);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    Symbol *sym = symbol_table_lookup_symbol(&table, slice_tok);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_ARRAY);
    assert(sym->type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_full");
}

static void test_type_check_array_slice_from_start()
{
    DEBUG_INFO("Starting test_type_check_array_slice_from_start");

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
    ast_module_add_statement(&arena, &module, arr_decl);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, NULL, end, NULL, &arr_tok);
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_from_start");
}

static void test_type_check_array_slice_to_end()
{
    DEBUG_INFO("Starting test_type_check_array_slice_to_end");

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
    ast_module_add_statement(&arena, &module, arr_decl);

    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_var, start, NULL, NULL, &arr_tok);
    Token slice_tok;
    setup_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, arr_type, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_to_end");
}

static void test_type_check_array_slice_non_array()
{
    DEBUG_INFO("Starting test_type_check_array_slice_non_array");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // var x:int = 5
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 5};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, lit, NULL);
    ast_module_add_statement(&arena, &module, x_decl);

    // Try to slice x[1..3] - should fail
    Expr *x_var = ast_create_variable_expr(&arena, x_tok, &x_tok);
    Token start_tok;
    setup_literal_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);
    Token end_tok;
    setup_literal_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Expr *slice_expr = ast_create_array_slice_expr(&arena, x_var, start, end, NULL, &x_tok);
    Stmt *slice_stmt = ast_create_expr_stmt(&arena, slice_expr, NULL);
    ast_module_add_statement(&arena, &module, slice_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_slice_non_array");
}

// Sized array allocation tests

static void test_type_check_sized_array_alloc_basic()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_basic");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create size expression: 10
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 10};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create sized array alloc: int[10]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[10]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_basic");
}

static void test_type_check_sized_array_alloc_with_default()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_with_default");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Create size expression: 5
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 5};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create default value: 0
    Token default_tok;
    setup_literal_token(&default_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue default_val = {.int_value = 0};
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    // Create sized array alloc: int[5] = 0
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, default_expr, &alloc_tok);

    // var arr: int[] = int[5] = 0
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_with_default");
}

static void test_type_check_sized_array_alloc_mismatch_default()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_mismatch_default");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    // Create size expression: 5
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue size_val = {.int_value = 5};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, int_type, false, &size_tok);

    // Create default value: true (wrong type)
    Token default_tok;
    setup_literal_token(&default_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue default_val = {.bool_value = 1};
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, bool_type, false, &default_tok);

    // Create sized array alloc: int[5] = true (type mismatch)
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, default_expr, &alloc_tok);

    // var arr: int[] = int[5] = true
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_mismatch_default");
}

static void test_type_check_sized_array_alloc_runtime_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_runtime_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // var n: int = 20
    Token n_tok;
    setup_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token n_val_tok;
    setup_literal_token(&n_val_tok, TOKEN_INT_LITERAL, "20", 1, "test.sn", &arena);
    LiteralValue n_val = {.int_value = 20};
    Expr *n_init = ast_create_literal_expr(&arena, n_val, int_type, false, &n_val_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, n_init, NULL);
    ast_module_add_statement(&arena, &module, n_decl);

    // Create size expression: n (variable reference)
    Expr *size_expr = ast_create_variable_expr(&arena, n_tok, &n_tok);

    // Create sized array alloc: int[n]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[n]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_runtime_size");
}

static void test_type_check_sized_array_alloc_invalid_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_invalid_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    // Create size expression: "bad" (wrong type)
    Token size_tok;
    setup_literal_token(&size_tok, TOKEN_STRING_LITERAL, "\"bad\"", 1, "test.sn", &arena);
    LiteralValue size_val = {.string_value = arena_strdup(&arena, "bad")};
    Expr *size_expr = ast_create_literal_expr(&arena, size_val, str_type, false, &size_tok);

    // Create sized array alloc: int["bad"] (invalid size type)
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int["bad"]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_invalid_size");
}

static void test_type_check_sized_array_alloc_long_size()
{
    DEBUG_INFO("Starting test_type_check_sized_array_alloc_long_size");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);

    // var n: long = 20
    Token n_tok;
    setup_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token n_val_tok;
    setup_literal_token(&n_val_tok, TOKEN_INT_LITERAL, "20", 1, "test.sn", &arena);
    LiteralValue n_val = {.int_value = 20};
    Expr *n_init = ast_create_literal_expr(&arena, n_val, long_type, false, &n_val_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, long_type, n_init, NULL);
    ast_module_add_statement(&arena, &module, n_decl);

    // Create size expression: n (variable reference of type long)
    Expr *size_expr = ast_create_variable_expr(&arena, n_tok, &n_tok);

    // Create sized array alloc: int[n]
    Token alloc_tok;
    setup_token(&alloc_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *sized_alloc = ast_create_sized_array_alloc_expr(&arena, int_type, size_expr, NULL, &alloc_tok);

    // var arr: int[] = int[n]
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, alloc_tok, arr_type, sized_alloc, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify the sized alloc expression has correct type
    assert(sized_alloc->expr_type != NULL);
    assert(sized_alloc->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(sized_alloc->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_sized_array_alloc_long_size");
}

void test_type_checker_array_main()
{
    TEST_SECTION("Type Checker Arrays");

    TEST_RUN("array_decl_no_init", test_type_check_array_decl_no_init);
    TEST_RUN("array_decl_with_init_matching", test_type_check_array_decl_with_init_matching);
    TEST_RUN("array_decl_with_init_mismatch", test_type_check_array_decl_with_init_mismatch);
    TEST_RUN("array_literal_empty", test_type_check_array_literal_empty);
    TEST_RUN("array_literal_heterogeneous", test_type_check_array_literal_heterogeneous);
    TEST_RUN("array_access_valid", test_type_check_array_access_valid);
    TEST_RUN("array_access_non_array", test_type_check_array_access_non_array);
    TEST_RUN("array_access_invalid_index", test_type_check_array_access_invalid_index);
    TEST_RUN("array_assignment_matching", test_type_check_array_assignment_matching);
    TEST_RUN("array_assignment_mismatch", test_type_check_array_assignment_mismatch);
    TEST_RUN("nested_array", test_type_check_nested_array);
    // Slice tests
    TEST_RUN("array_slice_full", test_type_check_array_slice_full);
    TEST_RUN("array_slice_from_start", test_type_check_array_slice_from_start);
    TEST_RUN("array_slice_to_end", test_type_check_array_slice_to_end);
    TEST_RUN("array_slice_non_array", test_type_check_array_slice_non_array);
    // Sized array allocation tests
    TEST_RUN("sized_array_alloc_basic", test_type_check_sized_array_alloc_basic);
    TEST_RUN("sized_array_alloc_with_default", test_type_check_sized_array_alloc_with_default);
    TEST_RUN("sized_array_alloc_mismatch_default", test_type_check_sized_array_alloc_mismatch_default);
    TEST_RUN("sized_array_alloc_runtime_size", test_type_check_sized_array_alloc_runtime_size);
    TEST_RUN("sized_array_alloc_invalid_size", test_type_check_sized_array_alloc_invalid_size);
    TEST_RUN("sized_array_alloc_long_size", test_type_check_sized_array_alloc_long_size);
}
