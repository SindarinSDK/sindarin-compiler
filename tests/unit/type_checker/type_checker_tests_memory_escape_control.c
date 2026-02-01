/**
 * type_checker_tests_memory_escape_control.c - Escape analysis tests for control structures
 *
 * Tests for escape detection in if blocks, while loops, and for loops.
 */

static void test_escape_struct_assign_in_if_block()
{
    // Test: struct assignment from inner if-block scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     if true =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Condition: true
    Token cond_tok;
    setup_literal_token(&cond_tok, TOKEN_BOOL_LITERAL, "true", 2, "test.sn", &arena);
    LiteralValue cond_v = {.bool_value = true};
    Expr *cond_expr = ast_create_literal_expr(&arena, cond_v, bool_type, false, &cond_tok);

    // Inside if block: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create then-block
    Stmt *then_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *then_block = ast_create_block_stmt(&arena, then_stmts, 2, &block_tok);

    // Create if statement
    Token if_tok;
    setup_token(&if_tok, TOKEN_IF, "if", 2, "test.sn", &arena);
    Stmt *if_stmt = ast_create_if_stmt(&arena, cond_expr, then_block, NULL, &if_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, if_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_struct_assign_in_while_loop()
{
    // Test: struct assignment from inner while-loop scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     while false =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // Condition: false (so loop body isn't executed at runtime, but still type-checked)
    Token cond_tok;
    setup_literal_token(&cond_tok, TOKEN_BOOL_LITERAL, "false", 2, "test.sn", &arena);
    LiteralValue cond_v = {.bool_value = false};
    Expr *cond_expr = ast_create_literal_expr(&arena, cond_v, bool_type, false, &cond_tok);

    // Inside while loop: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create loop body block
    Stmt *body_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *body_block = ast_create_block_stmt(&arena, body_stmts, 2, &block_tok);

    // Create while statement
    Token while_tok;
    setup_token(&while_tok, TOKEN_WHILE, "while", 2, "test.sn", &arena);
    Stmt *while_stmt = ast_create_while_stmt(&arena, cond_expr, body_block, &while_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, while_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_escape_struct_assign_in_for_loop()
{
    // Test: struct assignment from inner for-loop scope to outer scope
    // fn main(): void =>
    //     var outer: int = 0
    //     for var i: int = 0; i < 10; i = i + 1 =>
    //         var inner: int = 42
    //         outer = inner  // inner escapes to outer scope
    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var outer: int = 0
    Token outer_name_tok;
    setup_token(&outer_name_tok, TOKEN_IDENTIFIER, "outer", 1, "test.sn", &arena);
    Token outer_init_tok;
    setup_literal_token(&outer_init_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue outer_v = {.int_value = 0};
    Expr *outer_init = ast_create_literal_expr(&arena, outer_v, int_type, false, &outer_init_tok);
    Stmt *outer_decl = ast_create_var_decl_stmt(&arena, outer_name_tok, int_type, outer_init, NULL);

    // For initializer: var i: int = 0
    Token i_name_tok;
    setup_token(&i_name_tok, TOKEN_IDENTIFIER, "i", 2, "test.sn", &arena);
    Token i_init_tok;
    setup_literal_token(&i_init_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue i_v = {.int_value = 0};
    Expr *i_init = ast_create_literal_expr(&arena, i_v, int_type, false, &i_init_tok);
    Stmt *for_init = ast_create_var_decl_stmt(&arena, i_name_tok, int_type, i_init, NULL);

    // For condition: i < 10
    Token cond_tok;
    setup_token(&cond_tok, TOKEN_LESS, "<", 2, "test.sn", &arena);
    Token i_tok;
    setup_token(&i_tok, TOKEN_IDENTIFIER, "i", 2, "test.sn", &arena);
    Expr *i_var = ast_create_variable_expr(&arena, i_tok, &i_tok);
    Token ten_tok;
    setup_literal_token(&ten_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue ten_v = {.int_value = 10};
    Expr *ten_lit = ast_create_literal_expr(&arena, ten_v, int_type, false, &ten_tok);
    Expr *for_cond = ast_create_binary_expr(&arena, i_var, TOKEN_LESS, ten_lit, &cond_tok);

    // For increment: i = i + 1
    Token plus_tok;
    setup_token(&plus_tok, TOKEN_PLUS, "+", 2, "test.sn", &arena);
    Expr *i_var2 = ast_create_variable_expr(&arena, i_tok, &i_tok);
    Token one_tok;
    setup_literal_token(&one_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue one_v = {.int_value = 1};
    Expr *one_lit = ast_create_literal_expr(&arena, one_v, int_type, false, &one_tok);
    Expr *i_plus_1 = ast_create_binary_expr(&arena, i_var2, TOKEN_PLUS, one_lit, &plus_tok);
    Expr *for_incr = ast_create_assign_expr(&arena, i_tok, i_plus_1, &i_tok);

    // Inside for loop: var inner: int = 42
    Token inner_name_tok;
    setup_token(&inner_name_tok, TOKEN_IDENTIFIER, "inner", 3, "test.sn", &arena);
    Token inner_init_tok;
    setup_literal_token(&inner_init_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue inner_v = {.int_value = 42};
    Expr *inner_init = ast_create_literal_expr(&arena, inner_v, int_type, false, &inner_init_tok);
    Stmt *inner_decl = ast_create_var_decl_stmt(&arena, inner_name_tok, int_type, inner_init, NULL);

    // outer = inner
    Token assign_outer_tok;
    setup_token(&assign_outer_tok, TOKEN_IDENTIFIER, "outer", 4, "test.sn", &arena);
    Token inner_var_tok;
    setup_token(&inner_var_tok, TOKEN_IDENTIFIER, "inner", 4, "test.sn", &arena);
    Expr *inner_var_expr = ast_create_variable_expr(&arena, inner_var_tok, &inner_var_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_outer_tok, inner_var_expr, &assign_outer_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_outer_tok);

    // Create loop body block
    Stmt *body_stmts[2] = {inner_decl, assign_stmt};
    Token block_tok;
    setup_token(&block_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Stmt *body_block = ast_create_block_stmt(&arena, body_stmts, 2, &block_tok);

    // Create for statement
    Token for_tok;
    setup_token(&for_tok, TOKEN_FOR, "for", 2, "test.sn", &arena);
    Stmt *for_stmt = ast_create_for_stmt(&arena, for_init, for_cond, for_incr, body_block, &for_tok);

    // Create main function
    Stmt *main_body[2] = {outer_decl, for_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 1, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, NULL, 0, void_type, main_body, 2, &main_name_tok);

    ast_module_add_statement(&arena, &module, main_func);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify escape is detected
    assert(ast_expr_escapes_scope(inner_var_expr) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_type_checker_memory_escape_control_main()
{
    TEST_RUN("escape_struct_assign_in_if_block", test_escape_struct_assign_in_if_block);
    TEST_RUN("escape_struct_assign_in_while_loop", test_escape_struct_assign_in_while_loop);
    TEST_RUN("escape_struct_assign_in_for_loop", test_escape_struct_assign_in_for_loop);
}
