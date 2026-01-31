// tests/type_checker_tests_struct_field_assign.c
// Field assignment escape detection tests

/* ============================================================================
 * Field Assignment Escape Detection Tests
 * ============================================================================ */

static void test_field_assign_escape_detection()
{
    DEBUG_INFO("Starting test_field_assign_escape_detection");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = Outer { inner: Inner { val: 0 } }
     *     {
     *         var local: Inner = Inner { val: 42 }
     *         o.inner = local  // escape: local escapes to outer scope via field
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { val: 0 } */
    FieldInitializer inner_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    /* Create Outer literal: Outer { inner: Inner { val: 0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Inner block: { var local: Inner = ...; o.inner = local } */
    /* Create Inner literal for local: Inner { val: 42 } */
    LiteralValue val_42 = {.int_value = 42};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    /* var local: Inner = Inner { val: 42 } */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* Create o.inner (member access) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create o.inner = local (member assignment) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 10, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = o_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should be marked as escaping
     * because 'local' is from a deeper scope than 'o' */
    assert(local_var->escape_info.escapes_scope == true);
    DEBUG_INFO("Field assign escape test: RHS escape_info.escapes_scope = %d", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_escape_detection");
}

/* Test: no escape when RHS and LHS are in the same scope */
static void test_field_assign_same_scope_no_escape()
{
    DEBUG_INFO("Starting test_field_assign_same_scope_no_escape");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = Outer { inner: Inner { val: 0 } }
     *     var local: Inner = Inner { val: 42 }
     *     o.inner = local  // NO escape: both in same scope
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal: Inner { val: 0 } */
    FieldInitializer inner_inits[1];
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    /* Create Outer literal: Outer { inner: Inner { val: 0 } } */
    FieldInitializer outer_inits[1];
    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);

    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;

    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = Outer { ... } */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Create Inner literal for local: Inner { val: 42 } */
    LiteralValue val_42 = {.int_value = 42};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_42, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    /* var local: Inner = Inner { val: 42 } (same scope as o) */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* Create o.inner (member access) */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create o.inner = local (member assignment) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Create function body - all in same scope */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 3);
    fn_body[0] = o_decl;
    fn_body[1] = local_decl;
    fn_body[2] = assign_stmt;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 3, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should NOT be marked as escaping
     * because 'local' and 'o' are in the same scope */
    assert(local_var->escape_info.escapes_scope == false);
    DEBUG_INFO("Field assign same scope test: RHS escape_info.escapes_scope = %d (should be 0)", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_same_scope_no_escape");
}

/* Test: escape detection for nested field access chain LHS (o.a.b = inner_val) */
static void test_field_assign_chain_escape_detection()
{
    DEBUG_INFO("Starting test_field_assign_chain_escape_detection");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Level2 struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField level2_fields[1];
    level2_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token level2_tok;
    setup_token(&level2_tok, TOKEN_IDENTIFIER, "Level2", 1, "test.sn", &arena);

    Type *level2_type = ast_create_struct_type(&arena, "Level2", level2_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level2_tok, level2_type);

    Stmt *level2_decl = ast_create_struct_decl_stmt(&arena, level2_tok, level2_fields, 1, NULL, 0, false, false, false, NULL, &level2_tok);
    ast_module_add_statement(&arena, &module, level2_decl);

    /* Create Level1 struct with l2: Level2 */
    StructField level1_fields[1];
    level1_fields[0] = create_test_field(&arena, "l2", level2_type, NULL);

    Token level1_tok;
    setup_token(&level1_tok, TOKEN_IDENTIFIER, "Level1", 2, "test.sn", &arena);

    Type *level1_type = ast_create_struct_type(&arena, "Level1", level1_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, level1_tok, level1_type);

    Stmt *level1_decl = ast_create_struct_decl_stmt(&arena, level1_tok, level1_fields, 1, NULL, 0, false, false, false, NULL, &level1_tok);
    ast_module_add_statement(&arena, &module, level1_decl);

    /* Create Root struct with l1: Level1 */
    StructField root_fields[1];
    root_fields[0] = create_test_field(&arena, "l1", level1_type, NULL);

    Token root_tok;
    setup_token(&root_tok, TOKEN_IDENTIFIER, "Root", 3, "test.sn", &arena);

    Type *root_type = ast_create_struct_type(&arena, "Root", root_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, root_tok, root_type);

    Stmt *root_decl = ast_create_struct_decl_stmt(&arena, root_tok, root_fields, 1, NULL, 0, false, false, false, NULL, &root_tok);
    ast_module_add_statement(&arena, &module, root_decl);

    /* Create function:
     * fn test_fn() {
     *     var r: Root = Root { l1: Level1 { l2: Level2 { val: 0 } } }
     *     {
     *         var local: Level2 = Level2 { val: 99 }
     *         r.l1.l2 = local  // escape through nested chain
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Level2 literal: Level2 { val: 0 } */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer l2_inits[1];
    l2_inits[0].name = val_tok;
    l2_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);

    Expr *l2_lit = ast_create_struct_literal_expr(&arena, level2_tok, l2_inits, 1, &level2_tok);

    /* Create Level1 literal: Level1 { l2: Level2 { ... } } */
    Token l2_field_tok;
    setup_token(&l2_field_tok, TOKEN_IDENTIFIER, "l2", 7, "test.sn", &arena);

    FieldInitializer l1_inits[1];
    l1_inits[0].name = l2_field_tok;
    l1_inits[0].value = l2_lit;

    Expr *l1_lit = ast_create_struct_literal_expr(&arena, level1_tok, l1_inits, 1, &level1_tok);

    /* Create Root literal: Root { l1: Level1 { ... } } */
    Token l1_field_tok;
    setup_token(&l1_field_tok, TOKEN_IDENTIFIER, "l1", 8, "test.sn", &arena);

    FieldInitializer root_inits[1];
    root_inits[0].name = l1_field_tok;
    root_inits[0].value = l1_lit;

    Expr *root_lit = ast_create_struct_literal_expr(&arena, root_tok, root_inits, 1, &root_tok);

    /* var r: Root = Root { ... } */
    Token r_tok;
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 9, "test.sn", &arena);
    Stmt *r_decl = ast_create_var_decl_stmt(&arena, r_tok, root_type, root_lit, &r_tok);

    /* Inner block: { var local: Level2 = ...; r.l1.l2 = local } */
    /* Create Level2 literal for local: Level2 { val: 99 } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);

    Expr *local_lit = ast_create_struct_literal_expr(&arena, level2_tok, local_inits, 1, &level2_tok);

    /* var local: Level2 = Level2 { val: 99 } */
    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 10, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, level2_type, local_lit, &local_tok);

    /* Create r.l1 (member access) */
    Expr *r_var = ast_create_variable_expr(&arena, r_tok, &r_tok);
    Expr *r_l1 = ast_create_member_access_expr(&arena, r_var, l1_field_tok, &l1_field_tok);

    /* Create local variable expression */
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create r.l1.l2 = local (member assignment to nested chain) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, r_l1, l2_field_tok, local_var, &l2_field_tok);

    /* Wrap member assignment in expression statement */
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &l2_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 11, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = r_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* After type checking, the RHS (local_var) should be marked as escaping
     * because 'local' is from a deeper scope than 'r' */
    assert(local_var->escape_info.escapes_scope == true);

    /* Also verify that the LHS member access chain (r.l1) should be marked as escaped */
    assert(r_l1->as.member_access.escaped == true);
    DEBUG_INFO("Field assign chain escape test: LHS r.l1 escaped = %d, RHS escapes_scope = %d",
               r_l1->as.member_access.escaped, local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_chain_escape_detection");
}

/* Test: escape detection marks ALL nodes in deeply nested LHS chain (r.a.b.c = local) */
static void test_field_assign_deep_chain_all_nodes_escaped()
{
    DEBUG_INFO("Starting test_field_assign_deep_chain_all_nodes_escaped");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create a 4-level struct hierarchy: Root -> A -> B -> C(val: int) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Level C: struct C { val: int } */
    StructField c_fields[1];
    c_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token c_tok;
    setup_token(&c_tok, TOKEN_IDENTIFIER, "C", 1, "test.sn", &arena);

    Type *c_type = ast_create_struct_type(&arena, "C", c_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, c_tok, c_type);

    Stmt *c_decl = ast_create_struct_decl_stmt(&arena, c_tok, c_fields, 1, NULL, 0, false, false, false, NULL, &c_tok);
    ast_module_add_statement(&arena, &module, c_decl);

    /* Level B: struct B { c: C } */
    StructField b_fields[1];
    b_fields[0] = create_test_field(&arena, "c", c_type, NULL);

    Token b_tok;
    setup_token(&b_tok, TOKEN_IDENTIFIER, "B", 2, "test.sn", &arena);

    Type *b_type = ast_create_struct_type(&arena, "B", b_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, b_tok, b_type);

    Stmt *b_decl = ast_create_struct_decl_stmt(&arena, b_tok, b_fields, 1, NULL, 0, false, false, false, NULL, &b_tok);
    ast_module_add_statement(&arena, &module, b_decl);

    /* Level A: struct A { b: B } */
    StructField a_fields[1];
    a_fields[0] = create_test_field(&arena, "b", b_type, NULL);

    Token a_tok;
    setup_token(&a_tok, TOKEN_IDENTIFIER, "A", 3, "test.sn", &arena);

    Type *a_type = ast_create_struct_type(&arena, "A", a_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, a_tok, a_type);

    Stmt *a_decl = ast_create_struct_decl_stmt(&arena, a_tok, a_fields, 1, NULL, 0, false, false, false, NULL, &a_tok);
    ast_module_add_statement(&arena, &module, a_decl);

    /* Root: struct Root { a: A } */
    StructField root_fields[1];
    root_fields[0] = create_test_field(&arena, "a", a_type, NULL);

    Token root_tok;
    setup_token(&root_tok, TOKEN_IDENTIFIER, "Root", 4, "test.sn", &arena);

    Type *root_type = ast_create_struct_type(&arena, "Root", root_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, root_tok, root_type);

    Stmt *root_decl = ast_create_struct_decl_stmt(&arena, root_tok, root_fields, 1, NULL, 0, false, false, false, NULL, &root_tok);
    ast_module_add_statement(&arena, &module, root_decl);

    /* Create function:
     * fn test_fn() {
     *     var r: Root = Root { a: A { b: B { c: C { val: 0 } } } }
     *     {
     *         var local: C = C { val: 99 }
     *         r.a.b.c = local  // escape through 3-level chain
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 10, "test.sn", &arena);

    /* Build nested literals from inside out */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 11, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer c_inits[1];
    c_inits[0].name = val_tok;
    c_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);
    Expr *c_lit = ast_create_struct_literal_expr(&arena, c_tok, c_inits, 1, &c_tok);

    Token c_field_tok;
    setup_token(&c_field_tok, TOKEN_IDENTIFIER, "c", 12, "test.sn", &arena);
    FieldInitializer b_inits[1];
    b_inits[0].name = c_field_tok;
    b_inits[0].value = c_lit;
    Expr *b_lit = ast_create_struct_literal_expr(&arena, b_tok, b_inits, 1, &b_tok);

    Token b_field_tok;
    setup_token(&b_field_tok, TOKEN_IDENTIFIER, "b", 13, "test.sn", &arena);
    FieldInitializer a_inits[1];
    a_inits[0].name = b_field_tok;
    a_inits[0].value = b_lit;
    Expr *a_lit = ast_create_struct_literal_expr(&arena, a_tok, a_inits, 1, &a_tok);

    Token a_field_tok;
    setup_token(&a_field_tok, TOKEN_IDENTIFIER, "a", 14, "test.sn", &arena);
    FieldInitializer root_inits[1];
    root_inits[0].name = a_field_tok;
    root_inits[0].value = a_lit;
    Expr *root_lit = ast_create_struct_literal_expr(&arena, root_tok, root_inits, 1, &root_tok);

    /* var r: Root = Root { ... } */
    Token r_tok;
    setup_token(&r_tok, TOKEN_IDENTIFIER, "r", 15, "test.sn", &arena);
    Stmt *r_decl = ast_create_var_decl_stmt(&arena, r_tok, root_type, root_lit, &r_tok);

    /* Inner block: { var local: C = ...; r.a.b.c = local } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);
    Expr *local_lit = ast_create_struct_literal_expr(&arena, c_tok, local_inits, 1, &c_tok);

    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "local", 16, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, c_type, local_lit, &local_tok);

    /* Build r.a.b.c chain from bottom up:
     * r_var -> r.a -> r.a.b -> r.a.b.c = local */
    Expr *r_var = ast_create_variable_expr(&arena, r_tok, &r_tok);
    Expr *r_a = ast_create_member_access_expr(&arena, r_var, a_field_tok, &a_field_tok);
    Expr *r_a_b = ast_create_member_access_expr(&arena, r_a, b_field_tok, &b_field_tok);

    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);

    /* Create r.a.b.c = local (c is the field being assigned) */
    Expr *member_assign = ast_create_member_assign_expr(&arena, r_a_b, c_field_tok, local_var, &c_field_tok);

    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &c_field_tok);

    /* Create inner block */
    Stmt **inner_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    inner_body[0] = local_decl;
    inner_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 17, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_body, 2, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = r_decl;
    fn_body[1] = inner_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* Verify that ALL nodes in the LHS chain are marked as escaped:
     * - r.a (first level)
     * - r.a.b (second level, object_expr of the assignment) */
    assert(local_var->escape_info.escapes_scope == true);
    assert(r_a->as.member_access.escaped == true);
    assert(r_a_b->as.member_access.escaped == true);

    DEBUG_INFO("Deep chain escape test: r.a escaped = %d, r.a.b escaped = %d, RHS escapes_scope = %d",
               r_a->as.member_access.escaped, r_a_b->as.member_access.escaped,
               local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_deep_chain_all_nodes_escaped");
}

/* Test: LHS base scope is used for comparison, not intermediate scope */
static void test_field_assign_uses_base_scope()
{
    DEBUG_INFO("Starting test_field_assign_uses_base_scope");

    Arena arena;
    arena_init(&arena, 16384);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    /* Create Inner struct with val: int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    StructField inner_fields[1];
    inner_fields[0] = create_test_field(&arena, "val", int_type, NULL);

    Token inner_tok;
    setup_token(&inner_tok, TOKEN_IDENTIFIER, "Inner", 1, "test.sn", &arena);

    Type *inner_type = ast_create_struct_type(&arena, "Inner", inner_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, inner_tok, inner_type);

    Stmt *inner_decl = ast_create_struct_decl_stmt(&arena, inner_tok, inner_fields, 1, NULL, 0, false, false, false, NULL, &inner_tok);
    ast_module_add_statement(&arena, &module, inner_decl);

    /* Create Outer struct with inner: Inner */
    StructField outer_fields[1];
    outer_fields[0] = create_test_field(&arena, "inner", inner_type, NULL);

    Token outer_tok;
    setup_token(&outer_tok, TOKEN_IDENTIFIER, "Outer", 2, "test.sn", &arena);

    Type *outer_type = ast_create_struct_type(&arena, "Outer", outer_fields, 1, NULL, 0, false, false, false, NULL);
    symbol_table_add_type(&table, outer_tok, outer_type);

    Stmt *outer_decl = ast_create_struct_decl_stmt(&arena, outer_tok, outer_fields, 1, NULL, 0, false, false, false, NULL, &outer_tok);
    ast_module_add_statement(&arena, &module, outer_decl);

    /* Create function:
     * fn test_fn() {
     *     var o: Outer = ...
     *     {
     *         {
     *             var deep_local: Inner = Inner { val: 99 }
     *             o.inner = deep_local  // RHS is 2 scopes deeper than base 'o'
     *         }
     *     }
     * }
     */
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 5, "test.sn", &arena);

    /* Create Inner literal for outer */
    Token val_tok;
    setup_token(&val_tok, TOKEN_IDENTIFIER, "val", 6, "test.sn", &arena);

    LiteralValue val_0 = {.int_value = 0};
    FieldInitializer inner_inits[1];
    inner_inits[0].name = val_tok;
    inner_inits[0].value = ast_create_literal_expr(&arena, val_0, int_type, false, &val_tok);
    Expr *inner_lit = ast_create_struct_literal_expr(&arena, inner_tok, inner_inits, 1, &inner_tok);

    Token inner_field_tok;
    setup_token(&inner_field_tok, TOKEN_IDENTIFIER, "inner", 7, "test.sn", &arena);
    FieldInitializer outer_inits[1];
    outer_inits[0].name = inner_field_tok;
    outer_inits[0].value = inner_lit;
    Expr *outer_lit = ast_create_struct_literal_expr(&arena, outer_tok, outer_inits, 1, &outer_tok);

    /* var o: Outer = ... */
    Token o_tok;
    setup_token(&o_tok, TOKEN_IDENTIFIER, "o", 8, "test.sn", &arena);
    Stmt *o_decl = ast_create_var_decl_stmt(&arena, o_tok, outer_type, outer_lit, &o_tok);

    /* Deep inner block: { { var deep_local: Inner = ...; o.inner = deep_local } } */
    LiteralValue val_99 = {.int_value = 99};
    FieldInitializer local_inits[1];
    local_inits[0].name = val_tok;
    local_inits[0].value = ast_create_literal_expr(&arena, val_99, int_type, false, &val_tok);
    Expr *local_lit = ast_create_struct_literal_expr(&arena, inner_tok, local_inits, 1, &inner_tok);

    Token local_tok;
    setup_token(&local_tok, TOKEN_IDENTIFIER, "deep_local", 9, "test.sn", &arena);
    Stmt *local_decl = ast_create_var_decl_stmt(&arena, local_tok, inner_type, local_lit, &local_tok);

    /* o.inner = deep_local */
    Expr *o_var = ast_create_variable_expr(&arena, o_tok, &o_tok);
    Expr *local_var = ast_create_variable_expr(&arena, local_tok, &local_tok);
    Expr *member_assign = ast_create_member_assign_expr(&arena, o_var, inner_field_tok, local_var, &inner_field_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, member_assign, &inner_field_tok);

    /* Deep inner block */
    Stmt **deepest_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    deepest_body[0] = local_decl;
    deepest_body[1] = assign_stmt;

    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 10, "test.sn", &arena);
    Stmt *deepest_block = ast_create_block_stmt(&arena, deepest_body, 2, &block_tok);

    /* Middle block (just wraps the deepest) */
    Stmt **middle_body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    middle_body[0] = deepest_block;
    Stmt *middle_block = ast_create_block_stmt(&arena, middle_body, 1, &block_tok);

    /* Create function body */
    Stmt **fn_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    fn_body[0] = o_decl;
    fn_body[1] = middle_block;

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, fn_body, 2, &fn_tok);
    fn_stmt->as.function.is_native = false;
    ast_module_add_statement(&arena, &module, fn_stmt);

    type_checker_reset_error();
    int no_error = type_check_module(&module, &table);

    assert(no_error == 1);  /* Should pass type checking */

    /* deep_local is 2 scopes deeper than 'o', so escape should be detected
     * using BASE scope of LHS (which is 'o'), not some intermediate. */
    assert(local_var->escape_info.escapes_scope == true);
    DEBUG_INFO("Base scope test: RHS escapes_scope = %d (should be 1)", local_var->escape_info.escapes_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_field_assign_uses_base_scope");
}


void test_type_checker_struct_field_assign_main(void)
{
    TEST_SECTION("Struct Type Checker - Field Assignment");

    TEST_RUN("field_assign_escape_detection", test_field_assign_escape_detection);
    TEST_RUN("field_assign_same_scope_no_escape", test_field_assign_same_scope_no_escape);
    TEST_RUN("field_assign_chain_escape_detection", test_field_assign_chain_escape_detection);
    TEST_RUN("field_assign_deep_chain_all_nodes_escaped", test_field_assign_deep_chain_all_nodes_escaped);
    TEST_RUN("field_assign_uses_base_scope", test_field_assign_uses_base_scope);
}
