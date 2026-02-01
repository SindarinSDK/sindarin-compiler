// tests/unit/standalone/symbol_table_tests_core_scope.c
// Scope push/pop tests

// Test pushing a single scope
static void test_symbol_table_push_scope_single(void) {
    DEBUG_INFO("Starting test_symbol_table_push_scope_single");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    int initial_count = table.scopes_count;
    symbol_table_push_scope(&table);
    assert(table.scopes_count == initial_count + 1);
    assert(table.current->enclosing == table.global_scope);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);
    assert(table.current->symbols == NULL);

    symbol_table_pop_scope(&table);  // Restore
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_single");
}

// Test pushing multiple scopes and nesting
static void test_symbol_table_push_scope_nested(void) {
    DEBUG_INFO("Starting test_symbol_table_push_scope_nested");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_push_scope(&table);  // Scope 1
    Scope *scope1 = table.current;
    symbol_table_push_scope(&table);  // Scope 2
    Scope *scope2 = table.current;
    assert(scope2->enclosing == scope1);

    symbol_table_pop_scope(&table);  // Back to scope1
    assert(table.current == scope1);
    symbol_table_pop_scope(&table);  // Back to global
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_nested");
}

// Test scope expansion (when capacity exceeded)
static void test_symbol_table_push_scope_expand(void) {
    DEBUG_INFO("Starting test_symbol_table_push_scope_expand");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);  // Larger for multiple reallocs
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Push until expansion (start with capacity 8, push 9th total)
    for (int i = 0; i < 8; i++) {
        symbol_table_push_scope(&table);
    }
    assert(table.scopes_count == 9);
    assert(table.scopes_capacity >= 16);  // Doubled from 8

    // Pop all added scopes (back to global)
    for (int i = 0; i < 8; i++) {
        symbol_table_pop_scope(&table);
    }
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_expand");
}

// Test popping beyond global (should do nothing)
static void test_symbol_table_pop_scope_beyond_global(void) {
    DEBUG_INFO("Starting test_symbol_table_pop_scope_beyond_global");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_pop_scope(&table);  // Should do nothing
    assert(table.current == table.global_scope);

    // Pop after pushing one
    symbol_table_push_scope(&table);
    symbol_table_pop_scope(&table);
    symbol_table_pop_scope(&table);  // Now beyond
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_pop_scope_beyond_global");
}

// Test offset propagation on pop (max of child and parent)
static void test_symbol_table_pop_scope_offset_propagation(void) {
    DEBUG_INFO("Starting test_symbol_table_pop_scope_offset_propagation");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Global offsets at base
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    symbol_table_push_scope(&table);  // Child1, add local to increase
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("var1"), int_type);
    (void)table.current->next_local_offset;  // Increased by 8 (int size aligned)

    symbol_table_push_scope(&table);  // Child2, smaller increase
    symbol_table_add_symbol(&table, TOKEN_LITERAL("var2"), int_type);
    int child2_local = table.current->next_local_offset;  // Increased by another 8

    symbol_table_pop_scope(&table);  // Back to child1, should take max (child2_local > child1_local)
    assert(table.current->next_local_offset == child2_local);

    symbol_table_pop_scope(&table);  // Back to global, take max
    assert(table.global_scope->next_local_offset == child2_local);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_pop_scope_offset_propagation");
}

// Test begin_function_scope (push + reset offsets)
static void test_symbol_table_begin_function_scope(void) {
    DEBUG_INFO("Starting test_symbol_table_begin_function_scope");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add something to global to increase offsets
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("global_var"), int_type);
    int global_offset = table.global_scope->next_local_offset;

    symbol_table_begin_function_scope(&table);  // Pushes and resets
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);
    assert(table.current->enclosing == table.global_scope);

    symbol_table_pop_scope(&table);  // Offsets should propagate max to global
    assert(table.global_scope->next_local_offset == global_offset);  // Unchanged, since func reset to base < global

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_begin_function_scope");
}
