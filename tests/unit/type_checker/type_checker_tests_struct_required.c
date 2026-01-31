// tests/type_checker_tests_struct_required.c
// Required field enforcement tests for structs

/* ============================================================================
 * Required Field Enforcement Tests
 * ============================================================================ */

static void test_struct_missing_required_fields_error()
{
    DEBUG_INFO("Starting test_struct_missing_required_fields_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create empty struct literal: Point {} - should fail because x and y are required */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - required fields x and y not initialized */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_missing_required_fields_error");
}

/* Test: missing single required field causes error */
static void test_struct_missing_one_required_field_error()
{
    DEBUG_INFO("Starting test_struct_missing_one_required_field_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only x: Point { x: 1.0 } - missing y */
    FieldInitializer inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);

    LiteralValue val;
    val.double_value = 1.0;
    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, double_type, false, &x_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should fail - required field y not initialized */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_missing_one_required_field_error");
}

/* Test: all fields provided for struct with required fields - should pass */
static void test_struct_all_required_fields_provided()
{
    DEBUG_INFO("Starting test_struct_all_required_fields_provided");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with NO default values - all fields are required */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default - required */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with both fields: Point { x: 1.0, y: 2.0 } */
    FieldInitializer inits[2];
    Token x_tok, y_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    setup_token(&y_tok, TOKEN_IDENTIFIER, "y", 2, "test.sn", &arena);

    LiteralValue val1, val2;
    val1.double_value = 1.0;
    val2.double_value = 2.0;
    inits[0].name = x_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_tok);
    inits[1].name = y_tok;
    inits[1].value = ast_create_literal_expr(&arena, val2, double_type, false, &y_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 2, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, p_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all required fields are initialized */

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_all_required_fields_provided");
}

/* Test: optional fields (with defaults) don't need to be provided */
static void test_struct_optional_fields_not_required()
{
    DEBUG_INFO("Starting test_struct_optional_fields_not_required");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with a mix of required and optional fields */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default value for timeout */
    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "port");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* No default - required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "timeout");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = default_expr;  /* Has default - optional */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only required field: Config { port: 8080 } */
    FieldInitializer inits[1];
    Token port_tok;
    setup_token(&port_tok, TOKEN_IDENTIFIER, "port", 2, "test.sn", &arena);

    LiteralValue port_val;
    port_val.int_value = 8080;
    inits[0].name = port_tok;
    inits[0].value = ast_create_literal_expr(&arena, port_val, int_type, false, &port_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - required field provided, optional has default */

    /* Verify field_count is 2 (required + default applied) */
    assert(struct_lit->as.struct_literal.field_count == 2);

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_optional_fields_not_required");
}

void test_type_checker_struct_required_main(void)
{
    TEST_SECTION("Struct Type Checker - Required Fields");

    TEST_RUN("struct_missing_required_fields_error", test_struct_missing_required_fields_error);
    TEST_RUN("struct_missing_one_required_field_error", test_struct_missing_one_required_field_error);
    TEST_RUN("struct_all_required_fields_provided", test_struct_all_required_fields_provided);
    TEST_RUN("struct_optional_fields_not_required", test_struct_optional_fields_not_required);
}
