// tests/unit/standalone/symbol_table_tests_core_lookup.c
// Lookup tests

// Test lookup current scope basic
static void test_symbol_table_lookup_current_basic(void) {
    DEBUG_INFO("Starting test_symbol_table_lookup_current_basic");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("local_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(strcmp(sym->name.start, "local_var") == 0);

    // Non-existent
    Token bad_name = TOKEN_LITERAL("bad_var");
    sym = symbol_table_lookup_symbol_current(&table, bad_name);
    assert(sym == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_current_basic");
}

// Test lookup all scopes (enclosing)
static void test_symbol_table_lookup_enclosing(void) {
    DEBUG_INFO("Starting test_symbol_table_lookup_enclosing");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Global
    Type *int_type = create_int_type(&arena);
    Token global_name = TOKEN_LITERAL("global_var");
    symbol_table_add_symbol(&table, global_name, int_type);

    symbol_table_push_scope(&table);  // Inner
    Token inner_name = TOKEN_LITERAL("inner_var");
    symbol_table_add_symbol(&table, inner_name, int_type);

    // Lookup inner (current)
    Symbol *sym = symbol_table_lookup_symbol(&table, inner_name);
    assert(sym != NULL);
    assert(sym->name.length == 9);  // "inner_var"

    // Lookup global (enclosing)
    sym = symbol_table_lookup_symbol(&table, global_name);
    assert(sym != NULL);
    assert(strcmp(sym->name.start, "global_var") == 0);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_enclosing");
}

// Test shadowing (inner hides outer)
static void test_symbol_table_lookup_shadowing(void) {
    DEBUG_INFO("Starting test_symbol_table_lookup_shadowing");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("shadow_var");

    // Global
    symbol_table_add_symbol(&table, name, int_type);
    Symbol *global_sym = symbol_table_lookup_symbol(&table, name);
    assert(global_sym != NULL);
    assert(global_sym->offset == -LOCAL_BASE_OFFSET);  // Local in global scope?

    symbol_table_push_scope(&table);  // Inner, add same name
    symbol_table_add_symbol(&table, name, create_string_type(&arena));  // Different type

    // Lookup should find inner
    Symbol *inner_sym = symbol_table_lookup_symbol(&table, name);
    assert(inner_sym != NULL);
    assert(inner_sym->type->kind == TYPE_STRING);
    assert(inner_sym != global_sym);  // Different symbol

    symbol_table_pop_scope(&table);
    // Now back, should find global
    Symbol *back_sym = symbol_table_lookup_symbol(&table, name);
    assert(back_sym == global_sym);
    assert(back_sym->type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_shadowing");
}

// Test token equality variations (pointer match, content match, different lengths)
static void test_symbol_table_lookup_token_variations(void) {
    DEBUG_INFO("Starting test_symbol_table_lookup_token_variations");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    const char *name_str = "test123";
    Token orig_name = TOKEN_PTR(name_str, 7);
    symbol_table_add_symbol(&table, orig_name, int_type);

    // Same pointer
    Token same_ptr = TOKEN_PTR(name_str, 7);  // Same start addr
    Symbol *sym1 = symbol_table_lookup_symbol(&table, same_ptr);
    assert(sym1 != NULL);

    // Different pointer, same content (e.g., duplicated string)
    char dup_str[] = "test123";
    Token diff_ptr = TOKEN_PTR(dup_str, 7);
    Symbol *sym2 = symbol_table_lookup_symbol(&table, diff_ptr);
    assert(sym2 != NULL);
    assert(sym2 == sym1);  // Same symbol

    // Mismatch length
    Token short_name = TOKEN_PTR(name_str, 6);  // "test12"
    Symbol *sym3 = symbol_table_lookup_symbol(&table, short_name);
    assert(sym3 == NULL);

    // Mismatch content
    char diff_str[] = "test124";
    Token diff_content = TOKEN_PTR(diff_str, 7);
    Symbol *sym4 = symbol_table_lookup_symbol(&table, diff_content);
    assert(sym4 == NULL);

    // Case sensitive? Assuming yes, as memcmp
    char upper_str[] = "TEST123";
    Token upper = TOKEN_PTR(upper_str, 7);
    Symbol *sym5 = symbol_table_lookup_symbol(&table, upper);
    assert(sym5 == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_token_variations");
}

// Test lookup with NULL table or current
static void test_symbol_table_lookup_nulls(void) {
    DEBUG_INFO("Starting test_symbol_table_lookup_nulls");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token name = TOKEN_LITERAL("any_var");
    Symbol *sym = symbol_table_lookup_symbol(NULL, name);
    assert(sym == NULL);

    sym = symbol_table_lookup_symbol(&table, name);  // Not added yet
    assert(sym == NULL);

    // Set current to NULL manually (edge)
    table.current = NULL;
    sym = symbol_table_lookup_symbol(&table, name);
    assert(sym == NULL);

    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_nulls");
}
