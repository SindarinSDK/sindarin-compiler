// tests/type_checker_tests_struct_nested.c
// Nested struct initialization tests

/* ============================================================================
 * Nested Struct Initialization Tests
 * ============================================================================ */

static void test_nested_struct_all_fields_provided()
{
    DEBUG_INFO("Starting test_nested_struct_all_fields_provided");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = NULL;
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Rect struct: struct Rect => origin: Point, size: Point */
    StructField rect_fields[2];
    rect_fields[0].name = arena_strdup(&arena, "origin");
    rect_fields[0].type = point_type;
    rect_fields[0].offset = 0;
    rect_fields[0].default_value = NULL;
    rect_fields[0].c_alias = NULL;
    rect_fields[1].name = arena_strdup(&arena, "size");
    rect_fields[1].type = point_type;
    rect_fields[1].offset = 16;
    rect_fields[1].default_value = NULL;
    rect_fields[1].c_alias = NULL;

    Token rect_tok;
    setup_token(&rect_tok, TOKEN_IDENTIFIER, "Rect", 2, "test.sn", &arena);

    Type *rect_type = ast_create_struct_type(&arena, "Rect", rect_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, rect_tok, rect_type);

    Stmt *rect_decl = ast_create_struct_decl_stmt(&arena, rect_tok, rect_fields, 2, NULL, 0, false, false, false, NULL, &rect_tok);
    ast_module_add_statement(&arena, &module, rect_decl);

    /* Create nested struct literal: Rect { origin: Point { x: 0.0, y: 0.0 }, size: Point { x: 100.0, y: 50.0 } } */

    /* Inner Point literal for origin: Point { x: 0.0, y: 0.0 } */
    FieldInitializer origin_point_inits[2];
    Token x_tok, y_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    setup_token(&y_tok, TOKEN_IDENTIFIER, "y", 3, "test.sn", &arena);

    LiteralValue zero_val = {.double_value = 0.0};
    origin_point_inits[0].name = x_tok;
    origin_point_inits[0].value = ast_create_literal_expr(&arena, zero_val, double_type, false, &x_tok);
    origin_point_inits[1].name = y_tok;
    origin_point_inits[1].value = ast_create_literal_expr(&arena, zero_val, double_type, false, &y_tok);

    Expr *origin_lit = ast_create_struct_literal_expr(&arena, point_tok, origin_point_inits, 2, &point_tok);

    /* Inner Point literal for size: Point { x: 100.0, y: 50.0 } */
    FieldInitializer size_point_inits[2];
    Token x2_tok, y2_tok;
    setup_token(&x2_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    setup_token(&y2_tok, TOKEN_IDENTIFIER, "y", 3, "test.sn", &arena);

    LiteralValue width_val = {.double_value = 100.0};
    LiteralValue height_val = {.double_value = 50.0};
    size_point_inits[0].name = x2_tok;
    size_point_inits[0].value = ast_create_literal_expr(&arena, width_val, double_type, false, &x2_tok);
    size_point_inits[1].name = y2_tok;
    size_point_inits[1].value = ast_create_literal_expr(&arena, height_val, double_type, false, &y2_tok);

    Expr *size_lit = ast_create_struct_literal_expr(&arena, point_tok, size_point_inits, 2, &point_tok);

    /* Outer Rect literal: Rect { origin: Point {...}, size: Point {...} } */
    FieldInitializer rect_inits[2];
    Token origin_tok, size_tok;
    setup_token(&origin_tok, TOKEN_IDENTIFIER, "origin", 3, "test.sn", &arena);
    setup_token(&size_tok, TOKEN_IDENTIFIER, "size", 3, "test.sn", &arena);

    rect_inits[0].name = origin_tok;
    rect_inits[0].value = origin_lit;
    rect_inits[1].name = size_tok;
    rect_inits[1].value = size_lit;

    Expr *rect_lit = ast_create_struct_literal_expr(&arena, rect_tok, rect_inits, 2, &rect_tok);

    /* Create function to trigger type checking */
    Token fn_tok, r_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, r_tok, rect_type, rect_lit, &r_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - all fields provided at all levels */

    /* Verify outer struct fields are initialized */
    assert(rect_lit->as.struct_literal.fields_initialized[0] == true);
    assert(rect_lit->as.struct_literal.fields_initialized[1] == true);

    /* Verify inner struct fields are initialized */
    assert(origin_lit->as.struct_literal.fields_initialized[0] == true);
    assert(origin_lit->as.struct_literal.fields_initialized[1] == true);
    assert(size_lit->as.struct_literal.fields_initialized[0] == true);
    assert(size_lit->as.struct_literal.fields_initialized[1] == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_all_fields_provided");
}

/* Test: nested struct with missing required field in inner struct */
static void test_nested_struct_inner_missing_required()
{
    DEBUG_INFO("Starting test_nested_struct_inner_missing_required");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double (both required) */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = NULL;
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Wrapper struct: struct Wrapper => pt: Point */
    StructField wrapper_fields[1];
    wrapper_fields[0].name = arena_strdup(&arena, "pt");
    wrapper_fields[0].type = point_type;
    wrapper_fields[0].offset = 0;
    wrapper_fields[0].default_value = NULL;
    wrapper_fields[0].c_alias = NULL;

    Token wrapper_tok;
    setup_token(&wrapper_tok, TOKEN_IDENTIFIER, "Wrapper", 2, "test.sn", &arena);

    Type *wrapper_type = ast_create_struct_type(&arena, "Wrapper", wrapper_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, wrapper_tok, wrapper_type);

    Stmt *wrapper_decl = ast_create_struct_decl_stmt(&arena, wrapper_tok, wrapper_fields, 1, NULL, 0, false, false, false, NULL, &wrapper_tok);
    ast_module_add_statement(&arena, &module, wrapper_decl);

    /* Create nested literal: Wrapper { pt: Point { x: 1.0 } } - MISSING y! */
    FieldInitializer point_inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);

    LiteralValue x_val = {.double_value = 1.0};
    point_inits[0].name = x_tok;
    point_inits[0].value = ast_create_literal_expr(&arena, x_val, double_type, false, &x_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, point_tok, point_inits, 1, &point_tok);

    /* Outer Wrapper literal */
    FieldInitializer wrapper_inits[1];
    Token pt_tok;
    setup_token(&pt_tok, TOKEN_IDENTIFIER, "pt", 3, "test.sn", &arena);

    wrapper_inits[0].name = pt_tok;
    wrapper_inits[0].value = point_lit;

    Expr *wrapper_lit = ast_create_struct_literal_expr(&arena, wrapper_tok, wrapper_inits, 1, &wrapper_tok);

    /* Create function to trigger type checking */
    Token fn_tok, w_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&w_tok, TOKEN_IDENTIFIER, "w", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, w_tok, wrapper_type, wrapper_lit, &w_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - inner Point is missing required field 'y' */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_inner_missing_required");
}

/* Test: nested struct with defaults applied at inner level */
static void test_nested_struct_inner_defaults_applied()
{
    DEBUG_INFO("Starting test_nested_struct_inner_defaults_applied");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct: struct Point => x: double, y: double = 0.0 */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Create default value for y */
    Token y_def_tok;
    setup_literal_token(&y_def_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue y_def_val = {.double_value = 0.0};
    Expr *y_default = ast_create_literal_expr(&arena, y_def_val, double_type, false, &y_def_tok);

    StructField point_fields[2];
    point_fields[0].name = arena_strdup(&arena, "x");
    point_fields[0].type = double_type;
    point_fields[0].offset = 0;
    point_fields[0].default_value = NULL;  /* x is required */
    point_fields[0].c_alias = NULL;
    point_fields[1].name = arena_strdup(&arena, "y");
    point_fields[1].type = double_type;
    point_fields[1].offset = 8;
    point_fields[1].default_value = y_default;  /* y has default */
    point_fields[1].c_alias = NULL;

    Token point_tok;
    setup_token(&point_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *point_type = ast_create_struct_type(&arena, "Point", point_fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, point_tok, point_type);

    Stmt *point_decl = ast_create_struct_decl_stmt(&arena, point_tok, point_fields, 2, NULL, 0, false, false, false, NULL, &point_tok);
    ast_module_add_statement(&arena, &module, point_decl);

    /* Create Wrapper struct: struct Wrapper => pt: Point */
    StructField wrapper_fields[1];
    wrapper_fields[0].name = arena_strdup(&arena, "pt");
    wrapper_fields[0].type = point_type;
    wrapper_fields[0].offset = 0;
    wrapper_fields[0].default_value = NULL;
    wrapper_fields[0].c_alias = NULL;

    Token wrapper_tok;
    setup_token(&wrapper_tok, TOKEN_IDENTIFIER, "Wrapper", 2, "test.sn", &arena);

    Type *wrapper_type = ast_create_struct_type(&arena, "Wrapper", wrapper_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, wrapper_tok, wrapper_type);

    Stmt *wrapper_decl = ast_create_struct_decl_stmt(&arena, wrapper_tok, wrapper_fields, 1, NULL, 0, false, false, false, NULL, &wrapper_tok);
    ast_module_add_statement(&arena, &module, wrapper_decl);

    /* Create nested literal: Wrapper { pt: Point { x: 5.0 } } - y gets default */
    FieldInitializer point_inits[1];
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);

    LiteralValue x_val = {.double_value = 5.0};
    point_inits[0].name = x_tok;
    point_inits[0].value = ast_create_literal_expr(&arena, x_val, double_type, false, &x_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, point_tok, point_inits, 1, &point_tok);

    /* Outer Wrapper literal */
    FieldInitializer wrapper_inits[1];
    Token pt_tok;
    setup_token(&pt_tok, TOKEN_IDENTIFIER, "pt", 3, "test.sn", &arena);

    wrapper_inits[0].name = pt_tok;
    wrapper_inits[0].value = point_lit;

    Expr *wrapper_lit = ast_create_struct_literal_expr(&arena, wrapper_tok, wrapper_inits, 1, &wrapper_tok);

    /* Create function to trigger type checking */
    Token fn_tok, w_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 4, "test.sn", &arena);
    setup_token(&w_tok, TOKEN_IDENTIFIER, "w", 5, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, w_tok, wrapper_type, wrapper_lit, &w_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - y gets its default value */

    /* Verify outer Wrapper is fully initialized */
    assert(wrapper_lit->as.struct_literal.fields_initialized[0] == true);

    /* Verify inner Point: x was explicit, y got default applied */
    assert(point_lit->as.struct_literal.fields_initialized[0] == true);  /* x */
    assert(point_lit->as.struct_literal.fields_initialized[1] == true);  /* y (from default) */

    /* Verify field_count was updated to include the default */
    assert(point_lit->as.struct_literal.field_count == 2);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_inner_defaults_applied");
}

/* Test: 3-level nesting to ensure deep recursion works */
static void test_nested_struct_three_levels()
{
    DEBUG_INFO("Starting test_nested_struct_three_levels");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level 1: struct Inner => val: int = 42 */
    Token inner_def_tok;
    setup_literal_token(&inner_def_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue inner_def_val = {.int_value = 42};
    Expr *inner_default = ast_create_literal_expr(&arena, inner_def_val, int_type, false, &inner_def_tok);

    StructField inner_fields[1];
    inner_fields[0].name = arena_strdup(&arena, "val");
    inner_fields[0].type = int_type;
    inner_fields[0].offset = 0;
    inner_fields[0].default_value = inner_default;
    inner_fields[0].c_alias = NULL;

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Level 2: struct Middle => inner: Inner */
    StructField middle_fields[1];
    middle_fields[0].name = arena_strdup(&arena, "inner");
    middle_fields[0].type = inner_type;
    middle_fields[0].offset = 0;
    middle_fields[0].default_value = NULL;
    middle_fields[0].c_alias = NULL;

    Token middle_tok;
    setup_token(&middle_tok, TOKEN_IDENTIFIER, "Middle", 2, "test.sn", &arena);

    Type *middle_type = ast_create_struct_type(&arena, "Middle", middle_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, middle_tok, middle_type);

    Stmt *middle_decl = ast_create_struct_decl_stmt(&arena, middle_tok, middle_fields, 1, NULL, 0, false, false, false, NULL, &middle_tok);
    ast_module_add_statement(&arena, &module, middle_decl);

    /* Level 3: struct Outer => mid: Middle */
    StructField outer_fields[1];
    outer_fields[0].name = arena_strdup(&arena, "mid");
    outer_fields[0].type = middle_type;
    outer_fields[0].offset = 0;
    outer_fields[0].default_value = NULL;
    outer_fields[0].c_alias = NULL;

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 3, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create 3-level nested literal: Outer { mid: Middle { inner: Inner {} } }
     * Inner {} uses default for 'val' */

    /* Innermost: Inner {} */
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, NULL, 0, &inner_tok);

    /* Middle: Middle { inner: Inner {} } */
    FieldInitializer middle_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);

    middle_inits[0].name = inner_field_tok;
    middle_inits[0].value = inner_lit;

    Expr *middle_lit = ast_create_struct_literal_expr(&arena, middle_tok, middle_inits, 1, &middle_tok);

    /* Outermost: Outer { mid: Middle {...} } */
    FieldInitializer outer_inits[1];
    Token mid_field_tok;
    setup_token(&mid_field_tok, TOKEN_IDENTIFIER, "mid", 4, "test.sn", &arena);

    outer_inits[0].name = mid_field_tok;
    outer_inits[0].value = middle_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* Create function to trigger type checking */
    Token fn_tok, o_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 6, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass - Inner.val gets default 42 */

    /* Verify all levels are initialized */
    assert(outer_lit->as.struct_literal.fields_initialized[0] == true);
    assert(middle_lit->as.struct_literal.fields_initialized[0] == true);
    assert(inner_lit->as.struct_literal.fields_initialized[0] == true);  /* val got default */

    /* Verify Inner's field_count was updated to include default */
    assert(inner_lit->as.struct_literal.field_count == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_three_levels");
}

/* Test: 3-level nesting with missing required field at deepest level */
static void test_nested_struct_three_levels_missing_required()
{
    DEBUG_INFO("Starting test_nested_struct_three_levels_missing_required");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level 1: struct Inner => val: int (REQUIRED - no default) */
    StructField inner_fields[1];
    inner_fields[0].name = arena_strdup(&arena, "val");
    inner_fields[0].type = int_type;
    inner_fields[0].offset = 0;
    inner_fields[0].default_value = NULL;  /* Required! */
    inner_fields[0].c_alias = NULL;

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Level 2: struct Middle => inner: Inner */
    StructField middle_fields[1];
    middle_fields[0].name = arena_strdup(&arena, "inner");
    middle_fields[0].type = inner_type;
    middle_fields[0].offset = 0;
    middle_fields[0].default_value = NULL;
    middle_fields[0].c_alias = NULL;

    Token middle_tok;
    setup_token(&middle_tok, TOKEN_IDENTIFIER, "Middle", 2, "test.sn", &arena);

    Type *middle_type = ast_create_struct_type(&arena, "Middle", middle_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, middle_tok, middle_type);

    Stmt *middle_decl = ast_create_struct_decl_stmt(&arena, middle_tok, middle_fields, 1, NULL, 0, false, false, false, NULL, &middle_tok);
    ast_module_add_statement(&arena, &module, middle_decl);

    /* Level 3: struct Outer => mid: Middle */
    StructField outer_fields[1];
    outer_fields[0].name = arena_strdup(&arena, "mid");
    outer_fields[0].type = middle_type;
    outer_fields[0].offset = 0;
    outer_fields[0].default_value = NULL;
    outer_fields[0].c_alias = NULL;

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 3, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create 3-level nested literal: Outer { mid: Middle { inner: Inner {} } }
     * Inner {} is MISSING required field 'val'! */

    /* Innermost: Inner {} - MISSING 'val' */
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, NULL, 0, &inner_tok);

    /* Middle: Middle { inner: Inner {} } */
    FieldInitializer middle_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);

    middle_inits[0].name = inner_field_tok;
    middle_inits[0].value = inner_lit;

    Expr *middle_lit = ast_create_struct_literal_expr(&arena, middle_tok, middle_inits, 1, &middle_tok);

    /* Outermost: Outer { mid: Middle {...} } */
    FieldInitializer outer_inits[1];
    Token mid_field_tok;
    setup_token(&mid_field_tok, TOKEN_IDENTIFIER, "mid", 4, "test.sn", &arena);

    outer_inits[0].name = mid_field_tok;
    outer_inits[0].value = middle_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* Create function to trigger type checking */
    Token fn_tok, o_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 6, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = var_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 0);  /* Should FAIL - Inner.val is missing */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_nested_struct_three_levels_missing_required");
}

void test_type_checker_struct_nested_main(void)
{
    TEST_SECTION("Struct Type Checker - Nested Structs");

    TEST_RUN("nested_struct_all_fields_provided", test_nested_struct_all_fields_provided);
    TEST_RUN("nested_struct_inner_missing_required", test_nested_struct_inner_missing_required);
    TEST_RUN("nested_struct_inner_defaults_applied", test_nested_struct_inner_defaults_applied);
    TEST_RUN("nested_struct_three_levels", test_nested_struct_three_levels);
    TEST_RUN("nested_struct_three_levels_missing_required", test_nested_struct_three_levels_missing_required);
}
