// tests/type_checker_tests_struct_native.c
// Native struct context tests

static void test_native_struct_in_native_fn_context()
{
    DEBUG_INFO("Starting test_native_struct_in_native_fn_context");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct with pointer field - give all fields defaults for this test */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default values for fields - nil for pointer, 0 for length */
    Token nil_tok;
    setup_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    LiteralValue nil_val = {0};
    Expr *nil_expr = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    Token len_def_tok;
    setup_token(&len_def_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue len_val;
    len_val.int_value = 0;
    Expr *len_default = ast_create_literal_expr(&arena, len_val, int_type, false, &len_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "data");
    fields[0].type = ptr_byte;
    fields[0].offset = 0;
    fields[0].default_value = nil_expr;  /* has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "length");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = len_default;  /* has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);  /* native struct */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create native function that uses the native struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var buf: Buffer = Buffer {} */
    Token buf_tok;
    setup_token(&buf_tok, TOKEN_IDENTIFIER, "buf", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, buf_tok, struct_type, struct_lit, &buf_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = true;  /* Mark as native function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - native struct in native fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_native_struct_in_native_fn_context");
}

/* Test: native struct used in regular fn context - should fail */
static void test_native_struct_in_regular_fn_error()
{
    DEBUG_INFO("Starting test_native_struct_in_regular_fn_error");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create native struct with pointer field - give all fields defaults for this test */
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *ptr_byte = ast_create_pointer_type(&arena, byte_type);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create default values for fields */
    Token nil_tok;
    setup_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    LiteralValue nil_val = {0};
    Expr *nil_expr = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    Token len_def_tok;
    setup_token(&len_def_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue len_val;
    len_val.int_value = 0;
    Expr *len_default = ast_create_literal_expr(&arena, len_val, int_type, false, &len_def_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "data");
    fields[0].type = ptr_byte;
    fields[0].offset = 0;
    fields[0].default_value = nil_expr;  /* has default */
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "length");
    fields[1].type = int_type;
    fields[1].offset = 0;
    fields[1].default_value = len_default;  /* has default */
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Buffer", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Buffer", fields, 2, NULL, 0, true, false, false, NULL);  /* native struct */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, true, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create REGULAR function that tries to use the native struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var buf: Buffer = Buffer {} */
    Token buf_tok;
    setup_token(&buf_tok, TOKEN_IDENTIFIER, "buf", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, buf_tok, struct_type, struct_lit, &buf_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;  /* NOT native function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - native struct in regular fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_native_struct_in_regular_fn_error");
}

/* Test: regular struct can be used anywhere - should pass */
static void test_regular_struct_in_regular_fn()
{
    DEBUG_INFO("Starting test_regular_struct_in_regular_fn");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create regular struct (not native) with primitive fields */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default values (0.0) for fields */
    Token x_lit_tok;
    setup_literal_token(&x_lit_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue x_val = {.double_value = 0.0};
    Expr *x_default = ast_create_literal_expr(&arena, x_val, double_type, false, &x_lit_tok);

    Token y_lit_tok;
    setup_literal_token(&y_lit_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_val, double_type, false, &y_lit_tok);

    StructField fields[2];
    fields[0].name = arena_strdup(&arena, "x");
    fields[0].type = double_type;
    fields[0].offset = 0;
    fields[0].default_value = x_default;
    fields[0].c_alias = NULL;
    fields[1].name = arena_strdup(&arena, "y");
    fields[1].type = double_type;
    fields[1].offset = 0;
    fields[1].default_value = y_default;
    fields[1].c_alias = NULL;

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);  /* NOT native */
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    /* Create struct declaration */
    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create regular function that uses the regular struct */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);

    /* Function body: var p: Point = Point {} */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);

    /* Create struct literal expression */
    Expr *struct_lit = arena_alloc(&arena, sizeof(Expr));
    memset(struct_lit, 0, sizeof(Expr));
    struct_lit->type = EXPR_STRUCT_LITERAL;
    struct_lit->as.struct_literal.struct_name = struct_name_tok;
    struct_lit->as.struct_literal.fields = NULL;
    struct_lit->as.struct_literal.field_count = 0;
    struct_lit->as.struct_literal.struct_type = NULL;

    Token var_tok;
    setup_token(&var_tok, TOKEN_VAR, "var", 3, "test.sn", &arena);
    struct_lit->token = &var_tok;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, struct_lit, &p_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;  /* Regular function */
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - regular struct in regular fn context */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_regular_struct_in_regular_fn");
}

void test_type_checker_struct_native_main(void)
{
    TEST_SECTION("Struct Type Checker - Native Context");

    TEST_RUN("native_struct_in_native_fn_context", test_native_struct_in_native_fn_context);
    TEST_RUN("native_struct_in_regular_fn_error", test_native_struct_in_regular_fn_error);
    TEST_RUN("regular_struct_in_regular_fn", test_regular_struct_in_regular_fn);
}
