/**
 * type_checker_tests_native_pointer_struct.c - Pointer-to-struct member access tests
 *
 * Tests for *struct member access in regular vs native functions.
 */

/* Test: *struct member access is REJECTED in regular (non-native) functions */
static void test_ptr_struct_member_rejected_in_regular_fn(void)
{
    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create native struct: native struct Point => x: int, y: int */
    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = int_type;
    fields[1].offset = 4;
    fields[1].default_value = NULL;
    fields[1].c_alias = NULL;

    Token struct_tok;
    setup_test_token(&struct_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_tok, point_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create: *Point type */
    Type *ptr_point_type = ast_create_pointer_type(&arena, point_type);

    /* Create native fn that returns *Point (forward declaration) */
    Token get_point_tok;
    setup_test_token(&get_point_tok, TOKEN_IDENTIFIER, "get_point", 2, "test.sn", &arena);
    Stmt *native_decl = ast_create_function_stmt(&arena, get_point_tok, NULL, 0, ptr_point_type, NULL, 0, &get_point_tok);
    native_decl->as.function.is_native = true;
    ast_module_add_statement(&arena, &module, native_decl);

    /* In regular function: var p: *Point = nil ... p.x - should FAIL */
    /* First create variable declaration for p */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_point_type, nil_lit, &p_tok);

    /* Create variable reference for p */
    Expr *p_ref = ast_create_variable_expr(&arena, p_tok, &p_tok);

    /* Create member access: p.x */
    Token x_field_tok;
    setup_test_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Expr *member_access = ast_create_member_expr(&arena, p_ref, x_field_tok, &x_field_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member_access, &x_field_tok);

    /* Create a REGULAR function (not native) with var decl and member access */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = expr_stmt;
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "regular_func", 4, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;  /* Regular function */

    ast_module_add_statement(&arena, &module, func_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    /* Should FAIL - either due to pointer var in regular fn or pointer member access */
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *struct member access is ACCEPTED in native functions */
static void test_ptr_struct_member_accepted_in_native_fn(void)
{
    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create native struct: native struct Point => x: int, y: int */
    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = int_type;
    fields[1].offset = 4;
    fields[1].default_value = NULL;
    fields[1].c_alias = NULL;

    Token struct_tok;
    setup_test_token(&struct_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, true, false, false, NULL);
    symbol_table_add_type(&table, struct_tok, point_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create: *Point type */
    Type *ptr_point_type = ast_create_pointer_type(&arena, point_type);

    /* Create native fn that returns *Point (forward declaration) */
    Token get_point_tok;
    setup_test_token(&get_point_tok, TOKEN_IDENTIFIER, "get_point", 2, "test.sn", &arena);
    Stmt *native_decl = ast_create_function_stmt(&arena, get_point_tok, NULL, 0, ptr_point_type, NULL, 0, &get_point_tok);
    native_decl->as.function.is_native = true;
    ast_module_add_statement(&arena, &module, native_decl);

    /* In NATIVE function: var p: *Point = nil ... p.x - should PASS */
    /* First create variable declaration for p */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_point_type, nil_lit, &p_tok);

    /* Create variable reference for p */
    Expr *p_ref = ast_create_variable_expr(&arena, p_tok, &p_tok);

    /* Create member access: p.x */
    Token x_field_tok;
    setup_test_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Expr *member_access = ast_create_member_expr(&arena, p_ref, x_field_tok, &x_field_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member_access, &x_field_tok);

    /* Create a NATIVE function with var decl and member access */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = expr_stmt;
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "native_func", 4, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;  /* Native function */

    ast_module_add_statement(&arena, &module, func_decl);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should PASS - *struct member access allowed in native fn */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_native_pointer_struct_main(void)
{
    TEST_RUN("ptr_struct_member_rejected_in_regular_fn", test_ptr_struct_member_rejected_in_regular_fn);
    TEST_RUN("ptr_struct_member_accepted_in_native_fn", test_ptr_struct_member_accepted_in_native_fn);
}
