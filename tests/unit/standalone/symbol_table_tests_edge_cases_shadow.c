// tests/unit/standalone/symbol_table_tests_edge_cases_shadow.c
// Shadowing Edge Cases for symbol table

// =====================================================
// Shadowing Edge Cases
// =====================================================

static void test_edge_multi_level_shadowing(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);
    Type *char_type = create_char_type_edge(&arena);
    Type *double_type = create_double_type_edge(&arena);

    Token name = TOKEN_LITERAL("x");

    // Global x is int
    symbol_table_add_symbol(&table, name, int_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_INT);

    // First nested scope - x is bool
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, name, bool_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_BOOL);

    // Second nested scope - x is char
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, name, char_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_CHAR);

    // Third nested scope - x is double
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, name, double_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_DOUBLE);

    // Pop and verify each level
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_CHAR);

    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_BOOL);

    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_shadowing_in_sibling_scopes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);

    Token name = TOKEN_LITERAL("sibling");

    // Global
    symbol_table_add_symbol(&table, name, int_type);

    // First sibling scope
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, name, bool_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_BOOL);
    symbol_table_pop_scope(&table);

    // Second sibling scope (same level)
    symbol_table_push_scope(&table);
    // Should see global again since sibling's shadow is gone
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_INT);

    // Can shadow again
    symbol_table_add_symbol(&table, name, bool_type);
    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_BOOL);
    symbol_table_pop_scope(&table);

    assert(symbol_table_lookup_symbol(&table, name)->type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_no_shadow_different_names(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("x"), int_type);
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("y"), int_type);

    // Both should be accessible
    Symbol *x = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("x"));
    Symbol *y = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("y"));

    assert(x != NULL && y != NULL);
    assert(x != y);
    assert(x->declaration_scope_depth == 1);
    assert(y->declaration_scope_depth == 2);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
