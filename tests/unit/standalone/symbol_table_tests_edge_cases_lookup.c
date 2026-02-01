// tests/unit/standalone/symbol_table_tests_edge_cases_lookup.c
// Lookup Edge Cases for symbol table

// =====================================================
// Lookup Edge Cases
// =====================================================

static void test_edge_lookup_case_sensitive(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("myVar"), int_type);

    // Different cases should not match
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("myVar")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("MYVAR")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("myvar")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("MyVar")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_partial_match_fails(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("fullname"), int_type);

    // Partial matches should fail
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("full")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("fullnam")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("fullnamee")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("name")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_empty_table(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Lookup in empty table should return NULL
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("anything")) == NULL);
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("anything")) == NULL);
    assert(symbol_table_get_symbol_offset(&table, TOKEN_LITERAL("anything")) == -1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_after_removal(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Add in nested scope
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("scoped_var"), int_type);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("scoped_var")) != NULL);

    // Pop scope - symbol should no longer be accessible
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("scoped_var")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
