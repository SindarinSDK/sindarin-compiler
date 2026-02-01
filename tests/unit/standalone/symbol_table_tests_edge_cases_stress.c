// tests/unit/standalone/symbol_table_tests_edge_cases_stress.c
// Thread State and Stress/Boundary Tests for symbol table

// =====================================================
// Thread State Edge Cases
// =====================================================

static void test_edge_thread_state_multiple_symbols(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("a"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("b"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("c"), int_type);

    Symbol *a = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("a"));
    Symbol *b = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("b"));
    Symbol *c = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("c"));

    // Each symbol has independent thread state
    symbol_table_mark_pending(a);
    assert(symbol_table_is_pending(a) == true);
    assert(symbol_table_is_pending(b) == false);
    assert(symbol_table_is_pending(c) == false);

    symbol_table_mark_pending(b);
    symbol_table_mark_synchronized(a);
    assert(symbol_table_is_synchronized(a) == true);
    assert(symbol_table_is_pending(b) == true);
    assert(symbol_table_is_pending(c) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Stress and Boundary Tests
// =====================================================

static void test_edge_many_symbols_same_scope(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 16);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    char name_buf[32];

    // Add 200 symbols to same scope
    for (int i = 0; i < 200; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "sym_%03d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);
    }

    // Verify all are accessible
    for (int i = 0; i < 200; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "sym_%03d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
    }

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_scope_capacity_expansion(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Push enough scopes to trigger capacity expansion (initial is 8)
    for (int i = 0; i < 20; i++) {
        symbol_table_push_scope(&table);
    }

    assert(table.scopes_count == 21);
    assert(table.scopes_capacity >= 32);  // Should have expanded

    for (int i = 0; i < 20; i++) {
        symbol_table_pop_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_symbol_type_update(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);

    Token name = TOKEN_LITERAL("updateable");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym->type->kind == TYPE_INT);

    // Adding same symbol again updates type
    symbol_table_add_symbol(&table, name, bool_type);
    assert(sym->type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_type_equals_cloned(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token name = TOKEN_LITERAL("cloned_type");
    symbol_table_add_symbol(&table, name, arr_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);

    // Type should be cloned but equal
    assert(sym->type != arr_type);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_zero_length_token(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token empty = TOKEN_PTR("", 0);

    // Zero length token should not crash
    symbol_table_add_symbol(&table, empty, int_type);

    // Lookup should work
    Symbol *sym = symbol_table_lookup_symbol(&table, empty);
    assert(sym != NULL);
    assert(sym->name.length == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_scope_depth_consistency(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Create complex scope pattern
    symbol_table_push_scope(&table);      // depth 2
    symbol_table_push_scope(&table);      // depth 3
    symbol_table_pop_scope(&table);       // depth 2
    symbol_table_push_scope(&table);      // depth 3
    symbol_table_push_scope(&table);      // depth 4

    symbol_table_add_symbol(&table, TOKEN_LITERAL("deep_var"), int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("deep_var"));

    assert(symbol_table_get_scope_depth(&table) == 4);
    assert(sym->declaration_scope_depth == 4);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_current_lookup_vs_full_lookup(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("global"), int_type);
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("local"), int_type);

    // Current scope lookup should find only local
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("local")) != NULL);
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("global")) == NULL);

    // Full lookup should find both
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global")) != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_function_scope_offset_reset(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Add vars at global
    symbol_table_add_symbol(&table, TOKEN_LITERAL("g1"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("g2"), int_type);
    int global_offset = table.global_scope->next_local_offset;

    // Enter function - offsets should reset
    symbol_table_begin_function_scope(&table);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);

    // Add function vars
    symbol_table_add_symbol(&table, TOKEN_LITERAL("f1"), int_type);

    // Exit function
    symbol_table_pop_scope(&table);

    // Global offset should remain unchanged or take max
    assert(table.global_scope->next_local_offset >= global_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_print_empty_scope(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Print should not crash on empty scope
    symbol_table_print(&table, "empty_test");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_print_with_namespaces(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns"));
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("y"), int_type);

    // Print should handle both regular and namespace symbols
    symbol_table_print(&table, "namespace_test");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
