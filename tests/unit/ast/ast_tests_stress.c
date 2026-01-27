// tests/unit/ast/ast_tests_stress.c
// Stress tests for AST creation and manipulation

/* ============================================================================
 * Type Creation Stress Tests
 * ============================================================================ */

static void test_ast_create_many_primitives(void)
{
    Arena arena;
    setup_arena(&arena);

    for (int i = 0; i < 100; i++) {
        Type *t = ast_create_primitive_type(&arena, TYPE_INT);
        assert(t != NULL);
        assert(t->kind == TYPE_INT);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_all_primitive_types(void)
{
    Arena arena;
    setup_arena(&arena);

    TypeKind types[] = {
        TYPE_INT, TYPE_LONG, TYPE_DOUBLE, TYPE_STRING,
        TYPE_BOOL, TYPE_CHAR, TYPE_BYTE, TYPE_VOID, TYPE_ANY
    };

    for (int i = 0; i < 9; i++) {
        Type *t = ast_create_primitive_type(&arena, types[i]);
        assert(t != NULL);
        assert(t->kind == types[i]);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_nested_arrays(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *arr2 = ast_create_array_type(&arena, arr1);
    Type *arr3 = ast_create_array_type(&arena, arr2);

    assert(arr3 != NULL);
    assert(arr3->kind == TYPE_ARRAY);
    assert(arr3->data.array.element_type == arr2);

    cleanup_arena(&arena);
}

static void test_ast_create_many_pointers(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr = int_type;
    for (int i = 0; i < 10; i++) {
        ptr = ast_create_pointer_type(&arena, ptr);
        assert(ptr != NULL);
        assert(ptr->kind == TYPE_POINTER);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_array_of_arrays(void)
{
    Arena arena;
    setup_arena(&arena);

    TypeKind types[] = {
        TYPE_INT, TYPE_STRING, TYPE_DOUBLE, TYPE_BOOL
    };

    for (int i = 0; i < 4; i++) {
        Type *t = ast_create_primitive_type(&arena, types[i]);
        Type *arr = ast_create_array_type(&arena, t);
        Type *arr2 = ast_create_array_type(&arena, arr);
        assert(arr2 != NULL);
        assert(arr2->kind == TYPE_ARRAY);
    }

    cleanup_arena(&arena);
}

/* ============================================================================
 * Expression Creation Stress Tests
 * ============================================================================ */

static void test_ast_create_many_literals(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "42");

    for (int i = 0; i < 100; i++) {
        LiteralValue val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        assert(lit != NULL);
        assert(lit->kind == EXPR_LITERAL);
        assert(lit->as.literal.value.int_value == i);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_many_variables(void)
{
    Arena arena;
    setup_arena(&arena);

    char names[100][16];
    for (int i = 0; i < 100; i++) {
        snprintf(names[i], 16, "var_%d", i);
        Token tok = create_dummy_token(&arena, names[i]);
        Expr *var = ast_create_variable_expr(&arena, tok, &tok);
        assert(var != NULL);
        assert(var->kind == EXPR_VARIABLE);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_binary_chain(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "1");
    LiteralValue val = {.int_value = 1};
    Expr *expr = ast_create_literal_expr(&arena, val, int_type, false, &tok);

    // Create a chain of additions: 1 + 1 + 1 + ... (10 times)
    for (int i = 0; i < 10; i++) {
        Expr *right = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        expr = ast_create_binary_expr(&arena, expr, TOKEN_PLUS, right, &tok);
        assert(expr != NULL);
        assert(expr->kind == EXPR_BINARY);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_unary_chain(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "42");
    LiteralValue val = {.int_value = 42};
    Expr *expr = ast_create_literal_expr(&arena, val, int_type, false, &tok);

    // Create a chain of negations: - - - - 42 (10 times)
    for (int i = 0; i < 10; i++) {
        expr = ast_create_unary_expr(&arena, TOKEN_MINUS, expr, &tok);
        assert(expr != NULL);
        assert(expr->kind == EXPR_UNARY);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_all_binary_ops(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");
    LiteralValue val = {.int_value = 1};
    Expr *left = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    Expr *right = ast_create_literal_expr(&arena, val, int_type, false, &tok);

    SnTokenType ops[] = {
        TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO,
        TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL, TOKEN_LESS, TOKEN_GREATER,
        TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL, TOKEN_AND, TOKEN_OR
    };

    for (int i = 0; i < 13; i++) {
        Expr *bin = ast_create_binary_expr(&arena, left, ops[i], right, &tok);
        assert(bin != NULL);
        assert(bin->kind == EXPR_BINARY);
        assert(bin->as.binary.op == ops[i]);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_call_exprs(void)
{
    Arena arena;
    setup_arena(&arena);

    for (int i = 0; i < 50; i++) {
        char name[16];
        snprintf(name, 16, "func_%d", i);
        Token tok = create_dummy_token(&arena, name);
        Expr *callee = ast_create_variable_expr(&arena, tok, &tok);
        Expr *call = ast_create_call_expr(&arena, callee, NULL, 0, &tok);
        assert(call != NULL);
        assert(call->kind == EXPR_CALL);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_array_access_exprs(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token arr_tok = create_dummy_token(&arena, "arr");
    Expr *arr = ast_create_variable_expr(&arena, arr_tok, &arr_tok);

    for (int i = 0; i < 50; i++) {
        Token idx_tok = create_dummy_token(&arena, "idx");
        LiteralValue val = {.int_value = i};
        Expr *idx = ast_create_literal_expr(&arena, val, int_type, false, &idx_tok);
        Expr *access_expr = ast_create_array_access_expr(&arena, arr, idx, &arr_tok);
        assert(access_expr != NULL);
        assert(access_expr->kind == EXPR_ARRAY_ACCESS);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_member_exprs(void)
{
    Arena arena;
    setup_arena(&arena);

    Token obj_tok = create_dummy_token(&arena, "obj");
    Expr *obj = ast_create_variable_expr(&arena, obj_tok, &obj_tok);

    char members[50][16];
    for (int i = 0; i < 50; i++) {
        snprintf(members[i], 16, "field_%d", i);
        Token member_tok = create_dummy_token(&arena, members[i]);
        Expr *member = ast_create_member_expr(&arena, obj, member_tok, &obj_tok);
        assert(member != NULL);
        assert(member->kind == EXPR_MEMBER);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_assign_exprs(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");

    for (int i = 0; i < 50; i++) {
        LiteralValue val = {.int_value = i};
        Expr *value = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        Expr *assign = ast_create_assign_expr(&arena, tok, value, &tok);
        assert(assign != NULL);
        assert(assign->kind == EXPR_ASSIGN);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_comparison_exprs(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");

    LiteralValue val1 = {.int_value = 10};
    Expr *left = ast_create_literal_expr(&arena, val1, int_type, false, &tok);
    LiteralValue val2 = {.int_value = 20};
    Expr *right = ast_create_literal_expr(&arena, val2, int_type, false, &tok);

    SnTokenType comp_ops[] = {TOKEN_LESS, TOKEN_GREATER, TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL};
    for (int i = 0; i < 50; i++) {
        Expr *cmp = ast_create_comparison_expr(&arena, left, right, comp_ops[i % 4], &tok);
        assert(cmp != NULL);
        assert(cmp->kind == EXPR_COMPARISON);
    }

    cleanup_arena(&arena);
}

/* ============================================================================
 * Statement Creation Stress Tests
 * ============================================================================ */

static void test_ast_create_many_var_decls(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    char names[100][16];
    for (int i = 0; i < 100; i++) {
        snprintf(names[i], 16, "x_%d", i);
        Token tok = create_dummy_token(&arena, names[i]);
        Stmt *decl = ast_create_var_decl_stmt(&arena, tok, int_type, NULL, &tok);
        assert(decl != NULL);
        assert(decl->kind == STMT_VAR_DECL);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_many_expr_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");

    for (int i = 0; i < 100; i++) {
        LiteralValue val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        Stmt *stmt = ast_create_expr_stmt(&arena, lit, &tok);
        assert(stmt != NULL);
        assert(stmt->kind == STMT_EXPR);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_return_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token ret_tok = create_dummy_token(&arena, "return");

    for (int i = 0; i < 50; i++) {
        LiteralValue val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &ret_tok);
        Stmt *ret = ast_create_return_stmt(&arena, ret_tok, lit, &ret_tok);
        assert(ret != NULL);
        assert(ret->kind == STMT_RETURN);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_if_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "if");

    LiteralValue cond_val = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, cond_val, bool_type, false, &tok);

    LiteralValue int_val = {.int_value = 1};
    Expr *lit = ast_create_literal_expr(&arena, int_val, int_type, false, &tok);
    Stmt *then_stmt = ast_create_expr_stmt(&arena, lit, &tok);

    for (int i = 0; i < 50; i++) {
        Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then_stmt, NULL, &tok);
        assert(if_stmt != NULL);
        assert(if_stmt->kind == STMT_IF);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_while_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "while");

    LiteralValue cond_val = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, cond_val, bool_type, false, &tok);

    LiteralValue int_val = {.int_value = 1};
    Expr *lit = ast_create_literal_expr(&arena, int_val, int_type, false, &tok);
    Stmt *body = ast_create_expr_stmt(&arena, lit, &tok);

    for (int i = 0; i < 50; i++) {
        Stmt *while_stmt = ast_create_while_stmt(&arena, cond, body, &tok);
        assert(while_stmt != NULL);
        assert(while_stmt->kind == STMT_WHILE);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_block_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "block");

    for (int i = 0; i < 50; i++) {
        LiteralValue int_val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, int_val, int_type, false, &tok);
        Stmt *stmt = ast_create_expr_stmt(&arena, lit, &tok);
        Stmt **stmts = arena_alloc(&arena, sizeof(Stmt*));
        stmts[0] = stmt;
        Stmt *block = ast_create_block_stmt(&arena, stmts, 1, &tok);
        assert(block != NULL);
        assert(block->kind == STMT_BLOCK);
    }

    cleanup_arena(&arena);
}

static void test_ast_create_for_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "for");

    LiteralValue cond_val = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, cond_val, bool_type, false, &tok);
    LiteralValue int_val = {.int_value = 1};
    Expr *incr = ast_create_literal_expr(&arena, int_val, int_type, false, &tok);
    Stmt *body = ast_create_expr_stmt(&arena, incr, &tok);

    for (int i = 0; i < 50; i++) {
        Stmt *for_stmt = ast_create_for_stmt(&arena, NULL, cond, incr, body, &tok);
        assert(for_stmt != NULL);
        assert(for_stmt->kind == STMT_FOR);
    }

    cleanup_arena(&arena);
}

/* ============================================================================
 * Module Stress Tests
 * ============================================================================ */

static void test_ast_module_many_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");

    for (int i = 0; i < 100; i++) {
        LiteralValue val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        Stmt *stmt = ast_create_expr_stmt(&arena, lit, &tok);
        ast_module_add_statement(&arena, &module, stmt);
    }

    assert(module.num_statements == 100);

    cleanup_arena(&arena);
}

static void test_ast_module_mixed_stmts(void)
{
    Arena arena;
    setup_arena(&arena);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");
    Token ret_tok = create_dummy_token(&arena, "return");

    for (int i = 0; i < 25; i++) {
        // Add var decl
        Stmt *decl = ast_create_var_decl_stmt(&arena, tok, int_type, NULL, &tok);
        ast_module_add_statement(&arena, &module, decl);

        // Add expr stmt
        LiteralValue val = {.int_value = i};
        Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        Stmt *expr = ast_create_expr_stmt(&arena, lit, &tok);
        ast_module_add_statement(&arena, &module, expr);

        // Add return stmt
        Stmt *ret = ast_create_return_stmt(&arena, ret_tok, lit, &tok);
        ast_module_add_statement(&arena, &module, ret);

        // Add another var decl
        Stmt *decl2 = ast_create_var_decl_stmt(&arena, tok, int_type, NULL, &tok);
        ast_module_add_statement(&arena, &module, decl2);
    }

    assert(module.num_statements == 100);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Memory Allocation Stress Tests
 * ============================================================================ */

static void test_ast_many_arenas(void)
{
    for (int i = 0; i < 10; i++) {
        Arena arena;
        setup_arena(&arena);

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Token tok = create_dummy_token(&arena, "x");

        for (int j = 0; j < 100; j++) {
            LiteralValue val = {.int_value = j};
            Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
            assert(lit != NULL);
        }

        cleanup_arena(&arena);
    }
}

static void test_ast_large_expressions(void)
{
    Arena arena;
    arena_init(&arena, 65536);  // Larger arena

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");

    // Build a deeply nested expression
    LiteralValue val = {.int_value = 0};
    Expr *expr = ast_create_literal_expr(&arena, val, int_type, false, &tok);

    for (int i = 0; i < 500; i++) {
        Expr *right = ast_create_literal_expr(&arena, val, int_type, false, &tok);
        expr = ast_create_binary_expr(&arena, expr, TOKEN_PLUS, right, &tok);
    }

    assert(expr != NULL);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Type Comparison Tests
 * ============================================================================ */

static void test_ast_type_compare_primitives(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int2 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double1 = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    assert(ast_types_equal(int1, int2));
    assert(!ast_types_equal(int1, double1));

    cleanup_arena(&arena);
}

static void test_ast_type_compare_arrays(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr1 = ast_create_array_type(&arena, int_type);
    Type *arr2 = ast_create_array_type(&arena, int_type);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *arr3 = ast_create_array_type(&arena, double_type);

    assert(ast_types_equal(arr1, arr2));
    assert(!ast_types_equal(arr1, arr3));

    cleanup_arena(&arena);
}

static void test_ast_type_compare_pointers(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ptr1 = ast_create_pointer_type(&arena, int_type);
    Type *ptr2 = ast_create_pointer_type(&arena, int_type);

    assert(ast_types_equal(ptr1, ptr2));

    cleanup_arena(&arena);
}

static void test_ast_type_compare_function_types(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type **params = arena_alloc(&arena, sizeof(Type*) * 2);
    params[0] = int_type;
    params[1] = int_type;

    Type *func1 = ast_create_function_type(&arena, int_type, params, 2);
    Type *func2 = ast_create_function_type(&arena, int_type, params, 2);

    assert(ast_types_equal(func1, func2));

    cleanup_arena(&arena);
}

/* ============================================================================
 * Expression Kind Tests
 * ============================================================================ */

static void test_ast_expr_kinds(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");
    LiteralValue val = {.int_value = 42};

    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);
    assert(lit->kind == EXPR_LITERAL);

    Expr *var = ast_create_variable_expr(&arena, tok, &tok);
    assert(var->kind == EXPR_VARIABLE);

    Expr *bin = ast_create_binary_expr(&arena, lit, TOKEN_PLUS, lit, &tok);
    assert(bin->kind == EXPR_BINARY);

    Expr *unary = ast_create_unary_expr(&arena, TOKEN_MINUS, lit, &tok);
    assert(unary->kind == EXPR_UNARY);

    cleanup_arena(&arena);
}

/* ============================================================================
 * Statement Kind Tests
 * ============================================================================ */

static void test_ast_stmt_kinds(void)
{
    Arena arena;
    setup_arena(&arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token tok = create_dummy_token(&arena, "x");
    Token ret_tok = create_dummy_token(&arena, "return");
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, tok, int_type, NULL, &tok);
    assert(var_decl->kind == STMT_VAR_DECL);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, lit, &tok);
    assert(expr_stmt->kind == STMT_EXPR);

    Stmt *ret = ast_create_return_stmt(&arena, ret_tok, lit, &tok);
    assert(ret->kind == STMT_RETURN);

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt*));
    stmts[0] = expr_stmt;
    Stmt *block = ast_create_block_stmt(&arena, stmts, 1, &tok);
    assert(block->kind == STMT_BLOCK);

    cleanup_arena(&arena);
}

void test_ast_stress_main(void)
{
    TEST_SECTION("AST Stress Tests");

    // Type creation
    TEST_RUN("ast_create_many_primitives", test_ast_create_many_primitives);
    TEST_RUN("ast_create_all_primitive_types", test_ast_create_all_primitive_types);
    TEST_RUN("ast_create_nested_arrays", test_ast_create_nested_arrays);
    TEST_RUN("ast_create_many_pointers", test_ast_create_many_pointers);
    TEST_RUN("ast_create_array_of_arrays", test_ast_create_array_of_arrays);

    // Expression creation
    TEST_RUN("ast_create_many_literals", test_ast_create_many_literals);
    TEST_RUN("ast_create_many_variables", test_ast_create_many_variables);
    TEST_RUN("ast_create_binary_chain", test_ast_create_binary_chain);
    TEST_RUN("ast_create_unary_chain", test_ast_create_unary_chain);
    TEST_RUN("ast_create_all_binary_ops", test_ast_create_all_binary_ops);
    TEST_RUN("ast_create_call_exprs", test_ast_create_call_exprs);
    TEST_RUN("ast_create_array_access_exprs", test_ast_create_array_access_exprs);
    TEST_RUN("ast_create_member_exprs", test_ast_create_member_exprs);
    TEST_RUN("ast_create_assign_exprs", test_ast_create_assign_exprs);
    TEST_RUN("ast_create_comparison_exprs", test_ast_create_comparison_exprs);

    // Statement creation
    TEST_RUN("ast_create_many_var_decls", test_ast_create_many_var_decls);
    TEST_RUN("ast_create_many_expr_stmts", test_ast_create_many_expr_stmts);
    TEST_RUN("ast_create_return_stmts", test_ast_create_return_stmts);
    TEST_RUN("ast_create_if_stmts", test_ast_create_if_stmts);
    TEST_RUN("ast_create_while_stmts", test_ast_create_while_stmts);
    TEST_RUN("ast_create_block_stmts", test_ast_create_block_stmts);
    TEST_RUN("ast_create_for_stmts", test_ast_create_for_stmts);

    // Module tests
    TEST_RUN("ast_module_many_stmts", test_ast_module_many_stmts);
    TEST_RUN("ast_module_mixed_stmts", test_ast_module_mixed_stmts);

    // Memory allocation
    TEST_RUN("ast_many_arenas", test_ast_many_arenas);
    TEST_RUN("ast_large_expressions", test_ast_large_expressions);

    // Type comparison
    TEST_RUN("ast_type_compare_primitives", test_ast_type_compare_primitives);
    TEST_RUN("ast_type_compare_arrays", test_ast_type_compare_arrays);
    TEST_RUN("ast_type_compare_pointers", test_ast_type_compare_pointers);
    TEST_RUN("ast_type_compare_function_types", test_ast_type_compare_function_types);

    // Kind tests
    TEST_RUN("ast_expr_kinds", test_ast_expr_kinds);
    TEST_RUN("ast_stmt_kinds", test_ast_stmt_kinds);
}
