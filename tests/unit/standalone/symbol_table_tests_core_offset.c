// tests/unit/standalone/symbol_table_tests_core_offset.c
// Offset and alignment tests

// Test get_symbol_offset basic and not found
static void test_symbol_table_get_symbol_offset(void) {
    DEBUG_INFO("Starting test_symbol_table_get_symbol_offset");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("offset_var");
    symbol_table_add_symbol(&table, name, int_type);

    int offset = symbol_table_get_symbol_offset(&table, name);
    assert(offset == -LOCAL_BASE_OFFSET);

    // Not found
    Token bad_name = TOKEN_LITERAL("bad_offset");
    offset = symbol_table_get_symbol_offset(&table, bad_name);
    assert(offset == -1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_get_symbol_offset");
}

// Test offset alignment for different type sizes (char=1, int=8, etc.)
static void test_symbol_table_offsets_alignment(void) {
    DEBUG_INFO("Starting test_symbol_table_offsets_alignment");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);  // For params/locals

    // Char (1 byte, align to 8 -> 8)
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Token char_name = TOKEN_LITERAL("ch");
    symbol_table_add_symbol_with_kind(&table, char_name, char_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 8);

    // Int (8 bytes, align 8)
    Type *int_type = create_int_type(&arena);
    Token int_name = TOKEN_LITERAL("i");
    symbol_table_add_symbol_with_kind(&table, int_name, int_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 16);

    // Bool (1 byte, align 8)
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token bool_name = TOKEN_LITERAL("b");
    symbol_table_add_symbol_with_kind(&table, bool_name, bool_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 24);

    // Double (8)
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token double_name = TOKEN_LITERAL("d");
    symbol_table_add_symbol_with_kind(&table, double_name, double_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 32);

    // String (8)
    Type *str_type = create_string_type(&arena);
    Token str_name = TOKEN_LITERAL("s");
    symbol_table_add_symbol_with_kind(&table, str_name, str_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 40);

    // Check actual offsets negative
    Symbol *sym = symbol_table_lookup_symbol(&table, char_name);
    assert(sym->offset == -LOCAL_BASE_OFFSET);
    sym = symbol_table_lookup_symbol(&table, int_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 8));
    sym = symbol_table_lookup_symbol(&table, bool_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 16));
    sym = symbol_table_lookup_symbol(&table, double_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 24));
    sym = symbol_table_lookup_symbol(&table, str_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 32));

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_offsets_alignment");
}

// Test print function (basic output verification via printf, but since DEBUG, assume it works)
static void test_symbol_table_print(void) {
    DEBUG_INFO("Starting test_symbol_table_print");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("print_var"), int_type);

    // Print should output without crash
    symbol_table_print(&table, "test_print");

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_print");
}
