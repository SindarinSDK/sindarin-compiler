// tests/unit/standalone/symbol_table_tests_edge_cases_namespace.c
// Namespace Edge Cases for symbol table

// =====================================================
// Namespace Edge Cases
// =====================================================

static void test_edge_multiple_namespaces(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Create multiple namespaces
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns1"));
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns2"));
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns3"));

    // Add same symbol name to each namespace
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns1"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns2"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns3"), TOKEN_LITERAL("x"), int_type);

    // Each namespace should have its own x
    Symbol *x1 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns1"), TOKEN_LITERAL("x"));
    Symbol *x2 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns2"), TOKEN_LITERAL("x"));
    Symbol *x3 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns3"), TOKEN_LITERAL("x"));

    assert(x1 != NULL && x2 != NULL && x3 != NULL);
    assert(x1 != x2 && x2 != x3 && x1 != x3);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_namespace_and_regular_same_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);

    // Create namespace
    symbol_table_add_namespace(&table, TOKEN_LITERAL("myns"));
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("myns"), TOKEN_LITERAL("x"), int_type);

    // Add regular symbol with same name as namespace symbol
    symbol_table_add_symbol(&table, TOKEN_LITERAL("x"), bool_type);

    // Regular lookup should find regular symbol
    Symbol *regular_x = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("x"));
    assert(regular_x != NULL);
    assert(regular_x->type->kind == TYPE_BOOL);

    // Namespace lookup should find namespace symbol
    Symbol *ns_x = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("myns"), TOKEN_LITERAL("x"));
    assert(ns_x != NULL);
    assert(ns_x->type->kind == TYPE_INT);

    assert(regular_x != ns_x);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_empty_namespace(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_add_namespace(&table, TOKEN_LITERAL("emptyns"));

    // Lookup in empty namespace should return NULL
    assert(symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("emptyns"), TOKEN_LITERAL("anything")) == NULL);

    // But namespace should exist
    assert(symbol_table_is_namespace(&table, TOKEN_LITERAL("emptyns")) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
