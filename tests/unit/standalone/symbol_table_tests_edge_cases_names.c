// tests/unit/standalone/symbol_table_tests_edge_cases_names.c
// Token Name Edge Cases for symbol table

// =====================================================
// Token Name Edge Cases
// =====================================================

static void test_edge_single_char_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token name = TOKEN_PTR("x", 1);
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->name.length == 1);
    assert(sym->name.start[0] == 'x');

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_long_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Create a 256 character name
    char long_name[257];
    memset(long_name, 'a', 256);
    long_name[256] = '\0';

    Type *int_type = create_int_type_edge(&arena);
    Token name = TOKEN_PTR(long_name, 256);
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->name.length == 256);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_underscore_only_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token name = TOKEN_LITERAL("_");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_double_underscore_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token name = TOKEN_LITERAL("__reserved");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_numeric_suffix_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token name1 = TOKEN_LITERAL("var1");
    Token name2 = TOKEN_LITERAL("var12");
    Token name3 = TOKEN_LITERAL("var123");

    symbol_table_add_symbol(&table, name1, int_type);
    symbol_table_add_symbol(&table, name2, int_type);
    symbol_table_add_symbol(&table, name3, int_type);

    assert(symbol_table_lookup_symbol(&table, name1) != NULL);
    assert(symbol_table_lookup_symbol(&table, name2) != NULL);
    assert(symbol_table_lookup_symbol(&table, name3) != NULL);

    // Make sure they're distinct
    assert(symbol_table_lookup_symbol(&table, name1) != symbol_table_lookup_symbol(&table, name2));
    assert(symbol_table_lookup_symbol(&table, name2) != symbol_table_lookup_symbol(&table, name3));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_similar_prefixes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token name1 = TOKEN_LITERAL("foo");
    Token name2 = TOKEN_LITERAL("foobar");
    Token name3 = TOKEN_LITERAL("foobarbaz");

    symbol_table_add_symbol(&table, name1, int_type);
    symbol_table_add_symbol(&table, name2, int_type);
    symbol_table_add_symbol(&table, name3, int_type);

    Symbol *sym1 = symbol_table_lookup_symbol(&table, name1);
    Symbol *sym2 = symbol_table_lookup_symbol(&table, name2);
    Symbol *sym3 = symbol_table_lookup_symbol(&table, name3);

    assert(sym1 != sym2 && sym2 != sym3 && sym1 != sym3);
    assert(sym1->name.length == 3);
    assert(sym2->name.length == 6);
    assert(sym3->name.length == 9);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
