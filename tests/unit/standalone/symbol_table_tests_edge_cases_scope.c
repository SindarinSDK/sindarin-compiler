// tests/unit/standalone/symbol_table_tests_edge_cases_scope.c
// Scope Nesting Edge Cases for symbol table

// =====================================================
// Scope Nesting Edge Cases
// =====================================================

static void test_edge_deeply_nested_scopes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    char name_buf[32];

    // Create 20 nested scopes with a symbol in each
    for (int i = 0; i < 20; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "level_%d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);
        symbol_table_push_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 21);

    // Verify all symbols are accessible from innermost scope
    for (int i = 0; i < 20; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "level_%d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
        assert(sym->declaration_scope_depth == i + 1);
    }

    // Pop all scopes
    for (int i = 0; i < 20; i++) {
        symbol_table_pop_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_multiple_function_scopes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Simulate 3 function definitions one after another
    for (int fn = 0; fn < 3; fn++) {
        symbol_table_begin_function_scope(&table);

        // Each function has params
        char name_buf[32];
        int len = snprintf(name_buf, sizeof(name_buf), "param%d", fn);
        symbol_table_add_symbol_with_kind(&table, TOKEN_PTR(name_buf, len), int_type, SYMBOL_PARAM);

        // Each function has locals
        len = snprintf(name_buf, sizeof(name_buf), "local%d", fn);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);

        symbol_table_pop_scope(&table);
    }

    // Should be back at global scope
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_function_scope_with_nested_blocks(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Function scope
    symbol_table_begin_function_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("func_local"), int_type);

    // if block
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("if_local"), int_type);

    // nested for block
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("for_local"), int_type);

    // All should be accessible
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("func_local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("for_local")) != NULL);

    // Pop for block
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("for_local")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) != NULL);

    // Pop if block
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("func_local")) != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
