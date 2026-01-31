// tests/type_checker_tests_struct_literal.c
// Struct literal field initialization tests

/* ============================================================================
 * Struct Literal Field Initialization Tracking Tests
 * ============================================================================ */

static void test_struct_literal_all_fields_initialized()
{
    DEBUG_INFO("Starting test_struct_literal_all_fields_initialized");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;
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

    /* Create a function with var p: Point = Point { x: 1.0, y: 2.0 } */
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

    assert(no_error == 1);  /* Should pass */

    /* Verify fields_initialized array is populated */
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* x is initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* y is initialized */

    /* Test helper function */
    assert(ast_struct_literal_field_initialized(struct_lit, 0) == true);
    assert(ast_struct_literal_field_initialized(struct_lit, 1) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_all_fields_initialized");
}

/* Test: struct literal with partial field initialization tracking (before required field check)
 * Note: This test has all fields with defaults, so partial init is allowed */
static void test_struct_literal_partial_initialization()
{
    DEBUG_INFO("Starting test_struct_literal_partial_initialization");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with all fields having defaults - so partial init is OK */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Create default values */
    Token retries_def_tok, verbose_def_tok;
    setup_token(&retries_def_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    setup_token(&verbose_def_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);
    LiteralValue retries_def_val, verbose_def_val;
    retries_def_val.int_value = 3;
    verbose_def_val.bool_value = false;
    Expr *retries_default = ast_create_literal_expr(&arena, retries_def_val, int_type, false, &retries_def_tok);
    Expr *verbose_default = ast_create_literal_expr(&arena, verbose_def_val, bool_type, false, &verbose_def_tok);

    StructField fields[3];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* Required field */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "retries");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = retries_default;  /* Optional - has default */
    fields[1].c_alias = NULL;
    fields[2].name = arena_strdup(&arena, "verbose");
    fields[2].type = bool_type;
    fields[2].offset = 0;
    fields[2].default_value = verbose_default;  /* Optional - has default */
    fields[2].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 3, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with only one required field: Config { timeout: 60 } */
    FieldInitializer inits[1];
    Token timeout_tok;
    setup_token(&timeout_tok, TOKEN_IDENTIFIER, "timeout", 2, "test.sn", &arena);

    LiteralValue val;
    val.int_value = 60;

    inits[0].name = timeout_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, int_type, false, &timeout_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Create a function with var c: Config = Config { timeout: 60 } */
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

    assert(no_error == 1);  /* Should pass - required field provided, others have defaults */

    /* Verify all fields are initialized (explicit + defaults applied) */
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 3);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);   /* timeout explicitly */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);   /* retries via default */
    assert(struct_lit->as.struct_literal.fields_initialized[2] == true);   /* verbose via default */

    /* Verify field_count includes defaults */
    assert(struct_lit->as.struct_literal.field_count == 3);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_partial_initialization");
}

/* Test: struct literal with empty initialization - struct with all defaults should pass */
static void test_struct_literal_empty_initialization()
{
    DEBUG_INFO("Starting test_struct_literal_empty_initialization");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double = 0.0, y: double = 0.0 (all defaults) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value expressions */
    Token x_def_tok, y_def_tok;
    setup_token(&x_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    setup_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue x_def_val, y_def_val;
    x_def_val.double_value = 0.0;
    y_def_val.double_value = 0.0;
    Expr *x_default = ast_create_literal_expr(&arena, x_def_val, double_type, false, &x_def_tok);
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = x_default;  /* Has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;  /* Has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal with no fields: Point {} - should pass since all have defaults */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Create a function with var p: Point = Point {} */
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

    assert(no_error == 1);  /* Should pass - all fields have defaults */

    /* Verify defaults were applied - field_count should now be 2 */
    assert(struct_lit->as.struct_literal.field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized != NULL);
    assert(struct_lit->as.struct_literal.total_field_count == 2);
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* x via default */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* y via default */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_empty_initialization");
}

/* Test: helper function returns false for invalid/edge cases */
static void test_struct_literal_field_init_helper_edge_cases()
{
    DEBUG_INFO("Starting test_struct_literal_field_init_helper_edge_cases");

    Arena arena;
    arena_init(&arena, 4096);

    /* Test NULL expression */
    assert(ast_struct_literal_field_initialized(NULL, 0) == false);

    /* Test non-struct-literal expression */
    LiteralValue val;
    val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok;
    setup_token(&tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *int_lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    assert(ast_struct_literal_field_initialized(int_lit, 0) == false);

    /* Test struct literal before type checking (fields_initialized is NULL) */
    Token struct_tok;
    setup_token(&struct_tok, TOKEN_IDENTIFIER, "TestStruct", 1, "test.sn", &arena);
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_tok, NULL, 0, &struct_tok);
    /* Before type checking, fields_initialized should be NULL */
    assert(struct_lit->as.struct_literal.fields_initialized == NULL);
    assert(ast_struct_literal_field_initialized(struct_lit, 0) == false);

    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_field_init_helper_edge_cases");
}

/* Test: helper function with invalid field index */
static void test_struct_literal_field_init_invalid_index()
{
    DEBUG_INFO("Starting test_struct_literal_field_init_invalid_index");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double = 0.0 (y has a default) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value for y */
    Token y_def_tok;
    setup_literal_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_def_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = NULL;  /* x is required */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;  /* y has a default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal: Point { x: 1.0 } - y gets default value */
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
    assert(no_error == 1);

    /* Test invalid indices return false */
    assert(ast_struct_literal_field_initialized(struct_lit, -1) == false);  /* Negative index */
    assert(ast_struct_literal_field_initialized(struct_lit, 2) == false);   /* Index out of bounds */
    assert(ast_struct_literal_field_initialized(struct_lit, 100) == false); /* Way out of bounds */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_literal_field_init_invalid_index");
}


void test_type_checker_struct_literal_main(void)
{
    TEST_SECTION("Struct Type Checker - Struct Literals");

    TEST_RUN("struct_literal_all_fields_initialized", test_struct_literal_all_fields_initialized);
    TEST_RUN("struct_literal_partial_initialization", test_struct_literal_partial_initialization);
    TEST_RUN("struct_literal_empty_initialization", test_struct_literal_empty_initialization);
    TEST_RUN("struct_literal_field_init_helper_edge_cases", test_struct_literal_field_init_helper_edge_cases);
    TEST_RUN("struct_literal_field_init_invalid_index", test_struct_literal_field_init_invalid_index);
}
