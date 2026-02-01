// tests/unit/standalone/symbol_table_tests_edge_cases_kinds.c
// Symbol Kind and Offset Calculation Edge Cases for symbol table

// =====================================================
// Symbol Kind Edge Cases
// =====================================================

static void test_edge_all_symbol_kinds(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Test each kind
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("global_sym"), int_type, SYMBOL_GLOBAL);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("local_sym"), int_type, SYMBOL_LOCAL);

    symbol_table_begin_function_scope(&table);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("param_sym"), int_type, SYMBOL_PARAM);

    Symbol *global = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global_sym"));
    Symbol *local = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local_sym"));
    Symbol *param = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("param_sym"));

    assert(global->kind == SYMBOL_GLOBAL);
    assert(local->kind == SYMBOL_LOCAL);
    assert(param->kind == SYMBOL_PARAM);

    // Global symbols have offset 0
    assert(global->offset == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_mixed_params_and_locals(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_begin_function_scope(&table);

    // Alternate params and locals
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("p1"), int_type, SYMBOL_PARAM);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("l1"), int_type);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("p2"), int_type, SYMBOL_PARAM);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("l2"), int_type);

    Symbol *p1 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("p1"));
    Symbol *l1 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("l1"));
    Symbol *p2 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("p2"));
    Symbol *l2 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("l2"));

    assert(p1->kind == SYMBOL_PARAM);
    assert(l1->kind == SYMBOL_LOCAL);
    assert(p2->kind == SYMBOL_PARAM);
    assert(l2->kind == SYMBOL_LOCAL);

    // Params and locals should have separate offset sequences
    assert(p1->offset != l1->offset);
    assert(p2->offset != l2->offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Offset Calculation Edge Cases
// =====================================================

static void test_edge_offset_after_many_variables(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 8);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);

    Type *int_type = create_int_type_edge(&arena);
    char name_buf[32];

    // Add 50 local variables
    for (int i = 0; i < 50; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "var_%d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);
    }

    // Offset should be base + 50 * 8
    int expected = LOCAL_BASE_OFFSET + 50 * 8;
    assert(table.current->next_local_offset == expected);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_offset_propagation_complex(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);

    Type *int_type = create_int_type_edge(&arena);

    // Add some vars at function level
    symbol_table_add_symbol(&table, TOKEN_LITERAL("a"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("b"), int_type);

    // Nested block adds more
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("c"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("d"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("e"), int_type);

    int inner_offset = table.current->next_local_offset;

    // Pop should propagate max offset
    symbol_table_pop_scope(&table);
    assert(table.current->next_local_offset == inner_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_param_offset_sequence(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);

    Type *int_type = create_int_type_edge(&arena);

    // Add 5 params
    for (int i = 0; i < 5; i++) {
        char name_buf[16];
        int len = snprintf(name_buf, sizeof(name_buf), "p%d", i);
        symbol_table_add_symbol_with_kind(&table, TOKEN_PTR(name_buf, len), int_type, SYMBOL_PARAM);
    }

    // Each param should have a different offset
    int offsets[5];
    for (int i = 0; i < 5; i++) {
        char name_buf[16];
        int len = snprintf(name_buf, sizeof(name_buf), "p%d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        offsets[i] = sym->offset;
    }

    // Verify all offsets are unique and negative
    for (int i = 0; i < 5; i++) {
        assert(offsets[i] < 0);
        for (int j = i + 1; j < 5; j++) {
            assert(offsets[i] != offsets[j]);
        }
    }

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
