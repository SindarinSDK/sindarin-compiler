// tests/type_checker_tests_struct_defaults.c
// Default value application tests for structs

/* ============================================================================
 * Default Value Application Tests
 * ============================================================================ */

static void test_struct_default_value_applied()
{
    DEBUG_INFO("Starting test_struct_default_value_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with:
     * - timeout: int = 60 (has default)
     * - retries: int (no default)
     */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default value expression: 60 */
    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = default_expr;  /* Has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "retries");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = NULL;  /* No default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal that only specifies retries: Config { retries: 3 } */
    FieldInitializer inits[1];
    Token retries_tok;
    setup_token(&retries_tok, TOKEN_IDENTIFIER, "retries", 2, "test.sn", &arena);

    LiteralValue val;
    val.int_value = 3;
    inits[0].name = retries_tok;
    inits[0].value = ast_create_literal_expr(&arena, val, int_type, false, &retries_tok);

    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* Verify initial field count is 1 */
    assert(struct_lit->as.struct_literal.field_count == 1);

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

    assert(no_error == 1);  /* Should pass */

    /* Verify that field_count is now 2 (1 explicit + 1 default) */
    assert(struct_lit->as.struct_literal.field_count == 2);

    /* Verify both fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* timeout via default */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* retries via explicit */

    /* Verify the synthetic initializer was added for timeout */
    bool found_timeout = false;
    for (int i = 0; i < struct_lit->as.struct_literal.field_count; i++)
    {
        if (strncmp(struct_lit->as.struct_literal.fields[i].name.start, "timeout", 7) == 0)
        {
            found_timeout = true;
            /* Verify the value is the default expression */
            assert(struct_lit->as.struct_literal.fields[i].value == default_expr);
            break;
        }
    }
    assert(found_timeout);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_default_value_applied");
}

/* Test: multiple default values are applied */
static void test_struct_multiple_defaults_applied()
{
    DEBUG_INFO("Starting test_struct_multiple_defaults_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with all fields having defaults:
     * - host: str = "localhost"
     * - port: int = 8080
     * - debug: bool = false
     */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Create default value expressions */
    Token host_default_tok, port_default_tok, debug_default_tok;
    setup_token(&host_default_tok, TOKEN_STRING_LITERAL, "localhost", 1, "test.sn", &arena);
    setup_token(&port_default_tok, TOKEN_INT_LITERAL, "8080", 1, "test.sn", &arena);
    setup_token(&debug_default_tok, TOKEN_BOOL_LITERAL, "false", 1, "test.sn", &arena);

    LiteralValue host_val, port_val, debug_val;
    host_val.string_value = "localhost";
    port_val.int_value = 8080;
    debug_val.bool_value = false;

    Expr *host_default = ast_create_literal_expr(&arena, host_val, str_type, false, &host_default_tok);
    Expr *port_default = ast_create_literal_expr(&arena, port_val, int_type, false, &port_default_tok);
    Expr *debug_default = ast_create_literal_expr(&arena, debug_val, bool_type, false, &debug_default_tok);

    StructField fields[3];
    fields[0].name = arena_strdup(&arena, "host");
    fields[0].type = str_type;
    fields[0].offset = 0;
    fields[0].default_value = host_default;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "port");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = port_default;
    fields[1].c_alias = NULL;
    fields[2].name = arena_strdup(&arena, "debug");
    fields[2].type = bool_type;
    fields[2].offset = 0;
    fields[2].default_value = debug_default;
    fields[2].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "ServerConfig", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "ServerConfig", fields, 3, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 3, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create empty struct literal: ServerConfig {} */
    Expr *struct_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, NULL, 0, &struct_name_tok);

    /* Verify initial field count is 0 */
    assert(struct_lit->as.struct_literal.field_count == 0);

    /* Create a function to trigger type checking */
    Token fn_tok, c_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    setup_token(&c_tok, TOKEN_IDENTIFIER, "cfg", 3, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, c_tok, struct_type, struct_lit, &c_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass */

    /* Verify that field_count is now 3 (all 3 defaults applied) */
    assert(struct_lit->as.struct_literal.field_count == 3);

    /* Verify all fields are marked as initialized */
    assert(struct_lit->as.struct_literal.fields_initialized[0] == true);  /* host */
    assert(struct_lit->as.struct_literal.fields_initialized[1] == true);  /* port */
    assert(struct_lit->as.struct_literal.fields_initialized[2] == true);  /* debug */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_multiple_defaults_applied");
}

/* Test: explicit value overrides default value */
static void test_struct_explicit_overrides_default()
{
    DEBUG_INFO("Starting test_struct_explicit_overrides_default");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Config struct with timeout: int = 60 */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token default_tok;
    setup_token(&default_tok, TOKEN_INT_LITERAL, "60", 1, "test.sn", &arena);
    LiteralValue default_val;
    default_val.int_value = 60;
    Expr *default_expr = ast_create_literal_expr(&arena, default_val, int_type, false, &default_tok);

    StructField fields[1];
    fields[0].name = arena_strdup(&arena, "timeout");
    fields[0].type = int_type;
    fields[0].offset = 0;
    fields[0].default_value = default_expr;
    fields[0].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Config", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Config", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create struct literal that explicitly sets timeout: Config { timeout: 120 } */
    FieldInitializer inits[1];
    Token timeout_tok;
    setup_token(&timeout_tok, TOKEN_IDENTIFIER, "timeout", 2, "test.sn", &arena);

    LiteralValue explicit_val;
    explicit_val.int_value = 120;
    Expr *explicit_expr = ast_create_literal_expr(&arena, explicit_val, int_type, false, &timeout_tok);

    inits[0].name = timeout_tok;
    inits[0].value = explicit_expr;

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

    assert(no_error == 1);  /* Should pass */

    /* Verify field_count is still 1 (explicit value used, default not added) */
    assert(struct_lit->as.struct_literal.field_count == 1);

    /* Verify the explicit value is used, not the default */
    assert(struct_lit->as.struct_literal.fields[0].value == explicit_expr);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_struct_explicit_overrides_default");
}


void test_type_checker_struct_defaults_main(void)
{
    TEST_SECTION("Struct Type Checker - Default Values");

    TEST_RUN("struct_default_value_applied", test_struct_default_value_applied);
    TEST_RUN("struct_multiple_defaults_applied", test_struct_multiple_defaults_applied);
    TEST_RUN("struct_explicit_overrides_default", test_struct_explicit_overrides_default);
}
