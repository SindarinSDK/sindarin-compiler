// tests/unit/standalone/symbol_table_tests_core_init.c
// Initialization and basic cleanup tests

// Test initialization with NULL
static void test_symbol_table_init_null_arena(void) {
    DEBUG_INFO("Starting test_symbol_table_init_null_arena");

    SymbolTable table = {0};  // Zero-initialize to check it stays zeroed
    symbol_table_init(NULL, &table);
    // Should not crash, and table remains zeroed since arena is NULL
    assert(table.scopes == NULL);
    assert(table.scopes_count == 0);
    assert(table.current == NULL);

    DEBUG_INFO("Finished test_symbol_table_init_null_arena");
}

// Test initialization and basic global scope
static void test_symbol_table_init_basic(void) {
    DEBUG_INFO("Starting test_symbol_table_init_basic");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(table.arena == &arena);
    assert(table.scopes != NULL);
    assert(table.scopes_count == 1);
    assert(table.current != NULL);
    assert(table.global_scope == table.current);
    assert(table.current->symbols == NULL);
    assert(table.current->enclosing == NULL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_init_basic");
}

// Test cleanup on empty table
static void test_symbol_table_cleanup_empty(void) {
    DEBUG_INFO("Starting test_symbol_table_cleanup_empty");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_cleanup(&table);  // Should handle empty scopes
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_cleanup_empty");
}
