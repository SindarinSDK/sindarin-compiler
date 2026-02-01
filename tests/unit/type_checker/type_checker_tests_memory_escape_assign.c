/**
 * type_checker_tests_memory_escape_assign.c - Escape analysis tests for assignments
 *
 * Tests for escape detection in variable assignments.
 */

static void test_escape_detect_inner_to_outer_assignment()
{
    // Test: outer = inner (inner declared in deeper scope than outer)
    // Expected: inner expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 0     // scope depth 1 (in function)
    //   {
    //     var inner: int = 42  // scope depth 2 (in block)
    //     outer = inner        // inner escapes to outer scope
    //   }

    // Statement 1: var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Statement 2: Block containing inner decl and assignment
    // var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 2, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner (assignment expression)
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 3, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create block with inner_decl and assign_stmt
    Stmt *block_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *block_stmt = ast_create_block_stmt(&arena, block_stmts, 2, &block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, block_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that inner_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_same_scope_no_escape()
{
    // Test: a = b (both declared in same scope)
    // Expected: b expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var a: int = 0
    //   var b: int = 42
    //   a = b  // same scope, no escape

    // Statement 1: var a: int = 0
    Token a_name_tok;
    setup_token(&a_name_tok, TOKEN_IDENTIFIER, "a", 1, "test.sn", &arena);
    Token a_init_tok;
    setup_literal_token(&a_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue a_v = {.int_value = 0};
    Expr *a_init = ast_create_literal_expr(&arena, a_v, int_type, false, &a_init_tok);
    Stmt *a_decl = ast_create_var_decl_stmt(&arena, a_name_tok, int_type, a_init, NULL);

    // Statement 2: var b: int = 42
    Token b_name_tok;
    setup_token(&b_name_tok, TOKEN_IDENTIFIER, "b", 2, "test.sn", &arena);
    Token b_init_tok;
    setup_literal_token(&b_init_tok, TOKEN_INT_LITERAL, "42", 2, "test.sn", &arena);
    LiteralValue b_v = {.int_value = 42};
    Expr *b_init = ast_create_literal_expr(&arena, b_v, int_type, false, &b_init_tok);
    Stmt *b_decl = ast_create_var_decl_stmt(&arena, b_name_tok, int_type, b_init, NULL);

    // Statement 3: a = b
    Token assign_a_tok;
    setup_token(&assign_a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    Token b_var_tok;
    setup_token(&b_var_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    Expr *b_var_expr = ast_create_variable_expr(&arena, b_var_tok, &b_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_a_tok, b_var_expr, &assign_a_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_a_tok);

    // Create main function body
    Stmt *main_body[3] = {a_decl, b_decl, assign_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 3, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that b_var_expr does NOT have escapes_scope set
    assert(ast_expr_escapes_scope(b_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_outer_to_inner_no_escape()
{
    // Test: inner = outer (assigning from outer scope to inner)
    // Expected: outer expression should NOT have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 42    // scope depth 1
    //   {
    //     var inner: int = 0   // scope depth 2
    //     inner = outer        // outer does NOT escape (it's already in outer scope)
    //   }

    // Statement 1: var outer: int = 42
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 42};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Block: var inner: int = 0; inner = outer
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 2, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 0};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // inner = outer
    Token assign_inner_tok;
    setup_token(&assign_inner_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token outer_var_tok;
    setup_token(&outer_var_tok, TOKEN_IDENTIFIER, "outer", 3, "test.sn", &arena);
    Expr *outer_var_expr = ast_create_variable_expr(&arena, outer_var_tok, &outer_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_inner_tok, outer_var_expr, &assign_inner_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_inner_tok);

    // Create block
    Stmt *block_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *block_stmt = ast_create_block_stmt(&arena, block_stmts, 2, &block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, block_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that outer_var_expr does NOT have escapes_scope set (it's from outer scope)
    assert(ast_expr_escapes_scope(outer_var_expr) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_detect_nested_blocks()
{
    // Test: outer = deeply_inner (from multiple nesting levels)
    // Expected: deeply_inner expression should have escapes_scope = true
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Create main function:
    // fn main(): void =>
    //   var outer: int = 0     // scope depth 1
    //   {
    //     {
    //       var deep: int = 42 // scope depth 3
    //       outer = deep       // deep escapes to outer scope (3 > 1)
    //     }
    //   }

    // Statement 1: var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Inner block: var deep: int = 42; outer = deep
    Token deep_name_tok;
    setup_token(&deep_name_tok, TOKEN_IDENTIFIER, "deep", 3, "test.sn", &arena);
    Token deep_init_tok;
    setup_literal_token(&deep_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue deep_v = {.int_value = 42};
    Expr *deep_init = ast_create_literal_expr(&arena, deep_v, int_type, false, &deep_init_tok);
    Stmt *deep_decl = ast_create_var_decl_stmt(&arena, deep_name_tok, int_type, deep_init, NULL);

    // outer = deep
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token deep_var_tok;
    setup_token(&deep_var_tok, TOKEN_IDENTIFIER, "deep", 4, "test.sn", &arena);
    Expr *deep_var_expr = ast_create_variable_expr(&arena, deep_var_tok, &deep_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, deep_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create inner block
    Stmt *inner_block_stmts[2] = {deep_decl, assign_stmt};
    Token inner_block_tok;
    setup_token(&inner_block_tok, TOKEN_LEFT_BRACE, "{", 3, "test.sn", &arena);
    Stmt *inner_block = ast_create_block_stmt(&arena, inner_block_stmts, 2, &inner_block_tok);

    // Create outer block
    Stmt *outer_block_stmts[1] = {inner_block};
    Token outer_block_tok;
    setup_token(&outer_block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *outer_block = ast_create_block_stmt(&arena, outer_block_stmts, 1, &outer_block_tok);

    // Create main function body
    Stmt *main_body[2] = {outer_decl, outer_block};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check the module
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify that deep_var_expr has escapes_scope set to true
    assert(ast_expr_escapes_scope(deep_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_memory_escape_assign_main()
{
    TEST_RUN("escape_detect_inner_to_outer_assignment", test_escape_detect_inner_to_outer_assignment);
    TEST_RUN("escape_same_scope_no_escape", test_escape_same_scope_no_escape);
    TEST_RUN("escape_outer_to_inner_no_escape", test_escape_outer_to_inner_no_escape);
    TEST_RUN("escape_detect_nested_blocks", test_escape_detect_nested_blocks);
}
