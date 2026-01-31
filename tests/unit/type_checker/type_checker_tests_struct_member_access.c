// tests/type_checker_tests_struct_member_access.c
// Member access scope depth propagation tests

/* ============================================================================
 * Member Access Scope Depth Propagation Tests
 * ============================================================================ */

static void test_member_access_scope_depth_propagation()
{
    DEBUG_INFO("Starting test_member_access_scope_depth_propagation");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double, y: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[2];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);
    fields[1] = create_test_field(&arena, "y", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 2, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 2, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create function containing: var p: Point = Point { x: 1.0, y: 2.0 }; var v: double = p.x */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Point literal: Point { x: 1.0, y: 2.0 } */
    FieldInitializer inits[2];
    Token x_field_tok, y_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);
    setup_token(&y_field_tok, TOKEN_IDENTIFIER, "y", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    LiteralValue val2 = {.double_value = 2.0};
    inits[0].name = x_field_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);
    inits[1].name = y_field_tok;
    inits[1].value = ast_create_literal_expr(&arena, val2, double_type, false, &y_field_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 2, &struct_name_tok);

    /* var p: Point = Point { ... } */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 7, "test.sn", &arena);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, point_lit, &p_tok);

    /* Create member access: p.x */
    Expr *p_var = ast_create_variable_expr(&arena, p_tok, &p_tok);
    Expr *member_access = ast_create_member_access_expr(&arena, p_var, x_field_tok, &x_field_tok);

    /* Verify initial state before type checking */
    assert(member_access->as.member_access.scope_depth == 0);
    assert(member_access->as.member_access.escaped == false);
    assert(member_access->as.member_access.field_index == -1);

    /* var v: double = p.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 8, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, member_access, &v_tok);

    /* Create function body with both declarations */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = p_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, member access should have:
     * - field_index set (found the field)
     * - scope_depth set to the function body scope depth (typically 2: global=1, function=2) */
    assert(member_access->as.member_access.field_index == 0);  /* x is first field */
    assert(member_access->as.member_access.scope_depth >= 1);  /* Inside a scope */
    DEBUG_INFO("Member access scope_depth after type checking: %d", member_access->as.member_access.scope_depth);

    /* Verify the member access inherits proper context */
    assert(member_access->as.member_access.escaped == false);  /* Not escaping in this case */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_scope_depth_propagation");
}

/* Test: scope depth is correctly set for nested scope member access */
static void test_member_access_nested_scope_depth()
{
    DEBUG_INFO("Starting test_member_access_nested_scope_depth");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Point struct with x: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField fields[1];
    fields[0] = create_test_field(&arena, "x", double_type, NULL);

    Token struct_name_tok;
    setup_token(&struct_name_tok, TOKEN_IDENTIFIER, "Point", 1, "test.sn", &arena);

    Type *struct_type = ast_create_struct_type(&arena, "Point", fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, struct_name_tok, struct_type);

    Stmt *struct_decl = ast_create_struct_decl_stmt(&arena, struct_name_tok, fields, 1, NULL, 0, false, false, false, NULL, &struct_name_tok);
    ast_module_add_statement(&arena, &module, struct_decl);

    /* Create function with nested block: fn test_fn() { { var p: Point = ...; var v = p.x } } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Point literal: Point { x: 1.0 } */
    FieldInitializer inits[1];
    Token x_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    inits[0].name = x_field_tok;
    inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);

    Expr *point_lit = ast_create_struct_literal_expr(&arena, struct_name_tok, inits, 1, &struct_name_tok);

    /* var p: Point = Point { ... } */
    Token p_tok;
    setup_token(&p_tok, TOKEN_IDENTIFIER, "p", 7, "test.sn", &arena);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, struct_type, point_lit, &p_tok);

    /* Create member access: p.x */
    Expr *p_var = ast_create_variable_expr(&arena, p_tok, &p_tok);
    Expr *member_access = ast_create_member_access_expr(&arena, p_var, x_field_tok, &x_field_tok);

    /* var v: double = p.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 8, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, member_access, &v_tok);

    /* Create inner block { var p; var v = p.x } */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = p_decl;
    inner_body[1] = v_decl;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 6, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body with the block */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    fn_body[0] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 1, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, member access inside nested block should have deeper scope_depth */
    assert(member_access->as.member_access.field_index == 0);  /* x is first field */
    assert(member_access->as.member_access.scope_depth >= 2);  /* Inside function + block */
    DEBUG_INFO("Member access in nested block scope_depth: %d", member_access->as.member_access.scope_depth);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_nested_scope_depth");
}

/* Test: scope depth propagates through nested field access chains (outer.inner.x) */
static void test_member_access_chain_scope_depth()
{
    DEBUG_INFO("Starting test_member_access_chain_scope_depth");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with x: double */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "x", double_type, NULL);

    Token inner_struct_tok;
    setup_token(&inner_struct_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_struct_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_struct_tok, inner_struct_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_struct_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_struct_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_struct_type, NULL);

    Token outer_struct_tok;
    setup_token(&outer_struct_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_struct_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_struct_tok, outer_struct_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_struct_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_struct_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function: fn test_fn() { var o: Outer = ...; var v = o.inner.x } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { x: 1.0 } */
    FieldInitializer inner_inits[1];
    Token x_field_tok;
    setup_token(&x_field_tok, TOKEN_IDENTIFIER, "x", 6, "test.sn", &arena);

    LiteralValue val1 = {.double_value = 1.0};
    inner_inits[0].name = x_field_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val1, double_type, false, &x_field_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_struct_tok, inner_inits, 1, &inner_struct_tok);

    /* Create Outer literal: Outer { inner: Inner { x: 1.0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_struct_tok, outer_inits, 1, &outer_struct_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_struct_type, outer_lit, &o_tok);

    /* Create nested member access: o.inner (first level) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);
    Expr *o_inner = ast_create_member_access_expr(&arena, o_var, inner_field_tok, &inner_field_tok);

    /* Create nested member access: o.inner.x (second level) */
    Expr *o_inner_x = ast_create_member_access_expr(&arena, o_inner, x_field_tok, &x_field_tok);

    /* var v: double = o.inner.x */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 9, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, double_type, o_inner_x, &v_tok);

    /* Create function body */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = o_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking:
     * - o.inner should have scope_depth from 'o' (declaration scope depth)
     * - o.inner.x should also have scope_depth from 'o' (propagated through chain) */
    assert(o_inner->as.member_access.field_index == 0);  /* inner is first field of Outer */
    assert(o_inner_x->as.member_access.field_index == 0);  /* x is first field of Inner */

    /* Both should have the same scope depth (from base variable 'o') */
    assert(o_inner->as.member_access.scope_depth == o_inner_x->as.member_access.scope_depth);
    DEBUG_INFO("o.inner scope_depth: %d, o.inner.x scope_depth: %d",
               o_inner->as.member_access.scope_depth, o_inner_x->as.member_access.scope_depth);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_chain_scope_depth");
}

/* Test: scope depth propagates through three-level nested chains (a.b.c.d) */
static void test_member_access_chain_three_levels()
{
    DEBUG_INFO("Starting test_member_access_chain_three_levels");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Level3 struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField level3_fields[1];
    level3_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token level3_tok;
    setup_token(&level3_tok, TOKEN_IDENTIFIER, "Level3", 1, "test.sn", &arena);

    Type *level3_type = ast_create_struct_type(&arena, "Level3", level3_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level3_tok, level3_type);

    Stmt *level3_decl = ast_create_struct_decl_stmt(&arena, level3_tok, level3_fields, 1, NULL, 0, false, false, false, NULL, &level3_tok);
    ast_module_add_statement(&arena, &module, level3_decl);

    /* Create Level2 struct with c: Level3 */
    StructField level2_fields[1];
    level2_fields[0] = create_test_field(&arena, "c", level3_type, NULL);

    Token level2_tok;
    setup_token(&level2_tok, TOKEN_IDENTIFIER, "Level2", 2, "test.sn", &arena);

    Type *level2_type = ast_create_struct_type(&arena, "Level2", level2_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level2_tok, level2_type);

    Stmt *level2_decl = ast_create_struct_decl_stmt(&arena, level2_tok, level2_fields, 1, NULL, 0, false, false, false, NULL, &level2_tok);
    ast_module_add_statement(&arena, &module, level2_decl);

    /* Create Level1 struct with b: Level2 */
    StructField level1_fields[1];
    level1_fields[0] = create_test_field(&arena, "b", level2_type, NULL);

    Token level1_tok;
    setup_token(&level1_tok, TOKEN_IDENTIFIER, "Level1", 3, "test.sn", &arena);

    Type *level1_type = ast_create_struct_type(&arena, "Level1", level1_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level1_tok, level1_type);

    Stmt *level1_decl = ast_create_struct_decl_stmt(&arena, level1_tok, level1_fields, 1, NULL, 0, false, false, false, NULL, &level1_tok);
    ast_module_add_statement(&arena, &module, level1_decl);

    /* Create function: fn test_fn() { var a: Level1 = ...; var v = a.b.c.val } */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Level3 literal: Level3 { val: 42 } */
    FieldInitializer level3_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_42 = {.int_value = 42};
    level3_inits[0].name = val_tok;
    level3_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *level3_lit = ast_create_struct_literal_expr(&arena, level3_tok, level3_inits, 1, &level3_tok);

    /* Create Level2 literal: Level2 { c: Level3 { ... } } */
    FieldInitializer level2_inits[1];
    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "c", 7, "test.sn", &arena);

    level2_inits[0].name = c_tok;
    level2_inits[0].value = level3_lit;

    Expr *level2_lit = ast_create_struct_literal_expr(&arena, level2_tok, level2_inits, 1, &level2_tok);

    /* Create Level1 literal: Level1 { b: Level2 { ... } } */
    FieldInitializer level1_inits[1];
    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "b", 8, "test.sn", &arena);

    level1_inits[0].name = b_tok;
    level1_inits[0].value = level2_lit;

    Expr *level1_lit = ast_create_struct_literal_expr(&arena, level1_tok, level1_inits, 1, &level1_tok);

    /* var a: Level1 = Level1 { ... } */
    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "a", 9, "test.sn", &arena);
    Stmt *a_decl = ast_create_var_decl_stmt(&arena, a_tok, level1_type, level1_lit, &a_tok);

    /* Create nested member accesses:
     * a.b      -> first level
     * a.b.c    -> second level
     * a.b.c.val -> third level */
    Expr *a_var = ast_create_variable_expr(&arena, a_tok, &a_tok);
    Expr *a_b = ast_create_member_access_expr(&arena, a_var, b_tok, &b_tok);
    Expr *a_b_c = ast_create_member_access_expr(&arena, a_b, c_tok, &c_tok);
    Expr *a_b_c_val = ast_create_member_access_expr(&arena, a_b_c, val_tok, &val_tok);

    /* var v: int = a.b.c.val */
    Token v_tok;
    setup_token(&v_tok, TOKEN_IDENTIFIER, "v", 10, "test.sn", &arena);
    Stmt *v_decl = ast_create_var_decl_stmt(&arena, v_tok, int_type, a_b_c_val, &v_tok);

    /* Create function body */
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = a_decl;
    body[1] = v_decl;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, all member accesses in the chain should have the same scope_depth
     * (propagated from base variable 'a') */
    int base_depth = a_b->as.member_access.scope_depth;
    assert(a_b_c->as.member_access.scope_depth == base_depth);
    assert(a_b_c_val->as.member_access.scope_depth == base_depth);

    DEBUG_INFO("Three-level chain scope depths: a.b=%d, a.b.c=%d, a.b.c.val=%d",
               a_b->as.member_access.scope_depth,
               a_b_c->as.member_access.scope_depth,
               a_b_c_val->as.member_access.scope_depth);

    /* Verify field indices are correct */
    assert(a_b->as.member_access.field_index == 0);      /* b is first field of Level1 */
    assert(a_b_c->as.member_access.field_index == 0);    /* c is first field of Level2 */
    assert(a_b_c_val->as.member_access.field_index == 0); /* val is first field of Level3 */

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_member_access_chain_three_levels");
}

void test_type_checker_struct_member_access_main(void)
{
    TEST_SECTION("Struct Type Checker - Member Access");

    TEST_RUN("member_access_scope_depth_propagation", test_member_access_scope_depth_propagation);
    TEST_RUN("member_access_nested_scope_depth", test_member_access_nested_scope_depth);
    TEST_RUN("member_access_chain_scope_depth", test_member_access_chain_scope_depth);
    TEST_RUN("member_access_chain_three_levels", test_member_access_chain_three_levels);
}
