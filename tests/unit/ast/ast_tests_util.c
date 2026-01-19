// tests/ast_tests_util.c
// Utility, module, and print AST tests

static void test_ast_init_module()
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");
    assert(mod.count == 0);
    assert(mod.capacity == 8);
    assert(mod.statements != NULL);
    assert(strcmp(mod.filename, "test.sn") == 0);

    // NULL module
    ast_init_module(&arena, NULL, "test.sn"); // Should do nothing

    // NULL filename (code allows, but filename is const char*)
    Module mod_null_file;
    ast_init_module(&arena, &mod_null_file, NULL);
    assert(mod_null_file.filename == NULL);

    cleanup_arena(&arena);
}

static void test_ast_module_add_statement()
{
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *s1 = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc), loc);
    ast_module_add_statement(&arena, &mod, s1);
    assert(mod.count == 1);
    assert(mod.statements[0] == s1);

    // Add more to trigger resize
    int old_capacity = mod.capacity;
    for (int i = 1; i < 10; i++)
    {
        Stmt *s = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "y"), loc), loc);
        ast_module_add_statement(&arena, &mod, s);
        assert(mod.statements[i] == s);
    }
    assert(mod.count == 10);
    assert(mod.capacity > old_capacity); // Resized

    // NULL module or stmt
    ast_module_add_statement(&arena, NULL, s1);   // Nothing
    ast_module_add_statement(&arena, &mod, NULL); // Nothing, count unchanged
    assert(mod.count == 10);

    cleanup_arena(&arena);
}

static void test_ast_clone_token()
{
    Arena arena;
    setup_arena(&arena);

    Token orig = create_dummy_token(&arena, "token");
    Token *clone = ast_clone_token(&arena, &orig);
    assert(clone != NULL);
    assert(clone != &orig);
    assert(strcmp(clone->start, "token") == 0);
    assert(clone->length == 5);
    assert(clone->type == TOKEN_IDENTIFIER);
    assert(clone->line == 1);
    assert(strcmp(clone->filename, "test.sn") == 0);

    // NULL
    assert(ast_clone_token(&arena, NULL) == NULL);

    // Empty string
    Token empty_orig = create_dummy_token(&arena, "");
    Token *empty_clone = ast_clone_token(&arena, &empty_orig);
    assert(empty_clone != NULL);
    assert(empty_clone->length == 0);
    assert(strcmp(empty_clone->start, "") == 0);

    // Different type
    Token diff_type = orig;
    diff_type.type = TOKEN_STRING_LITERAL;
    Token *clone_diff = ast_clone_token(&arena, &diff_type);
    assert(clone_diff->type == TOKEN_STRING_LITERAL);

    cleanup_arena(&arena);
}

static void test_ast_print()
{
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *expr = ast_create_binary_expr(&arena,
                                        ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc),
                                        TOKEN_PLUS,
                                        ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc),
                                        loc);
    ast_print_expr(&arena, expr, 0);

    Stmt *stmt = ast_create_if_stmt(&arena,
                                    expr,
                                    ast_create_block_stmt(&arena, NULL, 0, loc),
                                    NULL,
                                    loc);
    ast_print_stmt(&arena, stmt, 0);

    // NULL
    ast_print_expr(&arena, NULL, 0);
    ast_print_stmt(&arena, NULL, 0);

    // More complex expr
    Expr *lit = ast_create_literal_expr(&arena, (LiteralValue){.string_value = "test"}, ast_create_primitive_type(&arena, TYPE_STRING), true, loc);
    ast_print_expr(&arena, lit, 0);

    // Complex stmt
    Stmt *func = ast_create_function_stmt(&arena, create_dummy_token(&arena, "func"), NULL, 0, ast_create_primitive_type(&arena, TYPE_VOID), NULL, 0, loc);
    ast_print_stmt(&arena, func, 0);

    // Test member access printing
    Token arr_tok = create_dummy_token(&arena, "arr");
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, loc);
    Token push_tok = create_dummy_token(&arena, "push");
    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, loc);
    ast_print_expr(&arena, member, 0);  // Should print "Member Access: push" and recurse on "arr"

    // Member with NULL object (should not crash, but skipped in print)
    Expr *member_null = ast_create_member_expr(&arena, NULL, push_tok, loc);
    ast_print_expr(&arena, member_null, 0);  // Prints only member name, no object

    cleanup_arena(&arena);
}

void test_ast_util_main()
{
    TEST_SECTION("AST Utility Tests");
    TEST_RUN("ast_init_module", test_ast_init_module);
    TEST_RUN("ast_module_add_statement", test_ast_module_add_statement);
    TEST_RUN("ast_clone_token", test_ast_clone_token);
    TEST_RUN("ast_print", test_ast_print);
}
