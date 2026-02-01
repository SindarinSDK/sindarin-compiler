// tests/unit/standalone/symbol_table_tests_core_depth.c
// Scope depth and declaration scope depth tests

// Test scope_depth basic initialization
static void test_symbol_table_scope_depth_init(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_init");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // After init, we have the global scope, so depth should be 1
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_init");
}

// Test scope_depth increments on push_scope
static void test_symbol_table_scope_depth_push(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_push");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    int initial_depth = symbol_table_get_scope_depth(&table);
    assert(initial_depth == 1);  // Global scope

    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 2);

    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 3);

    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 4);

    // Pop back and verify decrement
    symbol_table_pop_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 3);

    symbol_table_pop_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 2);

    symbol_table_pop_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_push");
}

// Test scope_depth with function scope
static void test_symbol_table_scope_depth_function(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_function");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(symbol_table_get_scope_depth(&table) == 1);  // Global

    symbol_table_begin_function_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 2);  // Function scope

    // Nested block inside function
    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 3);

    symbol_table_pop_scope(&table);  // Exit block
    assert(symbol_table_get_scope_depth(&table) == 2);

    symbol_table_pop_scope(&table);  // Exit function
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_function");
}

// Test scope_depth doesn't go below 1 when popping beyond global
static void test_symbol_table_scope_depth_bounds(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_bounds");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(symbol_table_get_scope_depth(&table) == 1);

    // Try to pop global scope - should do nothing
    symbol_table_pop_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 1);  // Still 1

    // Multiple pops on global should stay at 1
    symbol_table_pop_scope(&table);
    symbol_table_pop_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_bounds");
}

// Test scope_depth with NULL table
static void test_symbol_table_scope_depth_null(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_null");

    // NULL table should return 0
    assert(symbol_table_get_scope_depth(NULL) == 0);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_null");
}

// Test scope_depth with deeply nested scopes
static void test_symbol_table_scope_depth_deep(void) {
    DEBUG_INFO("Starting test_symbol_table_scope_depth_deep");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(symbol_table_get_scope_depth(&table) == 1);

    // Push 10 nested scopes
    for (int i = 0; i < 10; i++) {
        symbol_table_push_scope(&table);
        assert(symbol_table_get_scope_depth(&table) == i + 2);
    }

    assert(symbol_table_get_scope_depth(&table) == 11);

    // Pop all 10 nested scopes
    for (int i = 10; i > 0; i--) {
        symbol_table_pop_scope(&table);
        assert(symbol_table_get_scope_depth(&table) == i);
    }

    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_scope_depth_deep");
}

// Test declaration_scope_depth is populated correctly
static void test_symbol_declaration_scope_depth_basic(void) {
    DEBUG_INFO("Starting test_symbol_declaration_scope_depth_basic");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);

    // Add symbol at global scope (depth 1)
    symbol_table_add_symbol(&table, TOKEN_LITERAL("global_var"), int_type);
    Symbol *global_sym = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global_var"));
    assert(global_sym != NULL);
    assert(global_sym->declaration_scope_depth == 1);

    // Push new scope (depth 2)
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("local_var"), int_type);
    Symbol *local_sym = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local_var"));
    assert(local_sym != NULL);
    assert(local_sym->declaration_scope_depth == 2);

    // The global symbol still has its original declaration depth
    Symbol *global_lookup = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global_var"));
    assert(global_lookup != NULL);
    assert(global_lookup->declaration_scope_depth == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_declaration_scope_depth_basic");
}

// Test declaration_scope_depth persists through symbol table lookups
static void test_symbol_declaration_scope_depth_lookup_persistence(void) {
    DEBUG_INFO("Starting test_symbol_declaration_scope_depth_lookup_persistence");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);

    // Add symbol at depth 1
    symbol_table_add_symbol(&table, TOKEN_LITERAL("x"), int_type);

    // Push scopes and verify lookup still returns original declaration depth
    symbol_table_push_scope(&table);
    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 3);

    Symbol *x = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("x"));
    assert(x != NULL);
    assert(x->declaration_scope_depth == 1);  // Still reports where it was declared

    // Pop scopes and verify again
    symbol_table_pop_scope(&table);
    x = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("x"));
    assert(x != NULL);
    assert(x->declaration_scope_depth == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_declaration_scope_depth_lookup_persistence");
}

// Test comparing declaration depth with current scope depth
static void test_symbol_declaration_scope_depth_comparison(void) {
    DEBUG_INFO("Starting test_symbol_declaration_scope_depth_comparison");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);

    // Add symbol at global scope
    symbol_table_add_symbol(&table, TOKEN_LITERAL("outer"), int_type);

    // Enter nested scope and add another symbol
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("inner"), int_type);

    // Enter another scope
    symbol_table_push_scope(&table);
    int current_depth = symbol_table_get_scope_depth(&table);
    assert(current_depth == 3);

    // Check outer variable is from an outer scope
    Symbol *outer = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("outer"));
    assert(outer != NULL);
    assert(outer->declaration_scope_depth < current_depth);
    assert(outer->declaration_scope_depth == 1);

    // Check inner variable is also from an outer scope (but closer)
    Symbol *inner = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("inner"));
    assert(inner != NULL);
    assert(inner->declaration_scope_depth < current_depth);
    assert(inner->declaration_scope_depth == 2);

    // Add a symbol at the current scope
    symbol_table_add_symbol(&table, TOKEN_LITERAL("local"), int_type);
    Symbol *local = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local"));
    assert(local != NULL);
    assert(local->declaration_scope_depth == current_depth);
    assert(local->declaration_scope_depth == 3);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_declaration_scope_depth_comparison");
}

// Test declaration_scope_depth with function scopes
static void test_symbol_declaration_scope_depth_function_scope(void) {
    DEBUG_INFO("Starting test_symbol_declaration_scope_depth_function_scope");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);

    // Global variable at depth 1
    symbol_table_add_symbol(&table, TOKEN_LITERAL("global"), int_type);
    assert(symbol_table_get_scope_depth(&table) == 1);

    // Enter function scope
    symbol_table_begin_function_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 2);

    // Function parameter at depth 2
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("param"), int_type, SYMBOL_PARAM);
    Symbol *param = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("param"));
    assert(param != NULL);
    assert(param->declaration_scope_depth == 2);

    // Enter block in function
    symbol_table_push_scope(&table);
    assert(symbol_table_get_scope_depth(&table) == 3);

    // Local at depth 3
    symbol_table_add_symbol(&table, TOKEN_LITERAL("block_local"), int_type);
    Symbol *block_local = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("block_local"));
    assert(block_local != NULL);
    assert(block_local->declaration_scope_depth == 3);

    // Verify all lookups return correct declaration depths
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global"))->declaration_scope_depth == 1);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("param"))->declaration_scope_depth == 2);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("block_local"))->declaration_scope_depth == 3);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_declaration_scope_depth_function_scope");
}

// Test declaration_scope_depth with deeply nested scopes
static void test_symbol_declaration_scope_depth_deep_nesting(void) {
    DEBUG_INFO("Starting test_symbol_declaration_scope_depth_deep_nesting");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    char name_buf[32];

    // Add symbols at each depth level
    for (int i = 0; i < 5; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "var_%d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);

        // Verify the symbol has the correct declaration depth
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
        assert(sym->declaration_scope_depth == i + 1);

        symbol_table_push_scope(&table);
    }

    // Verify all symbols can still be found with correct declaration depths
    for (int i = 0; i < 5; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "var_%d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
        assert(sym->declaration_scope_depth == i + 1);
    }

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_declaration_scope_depth_deep_nesting");
}
