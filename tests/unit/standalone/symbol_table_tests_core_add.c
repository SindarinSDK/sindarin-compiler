// tests/unit/standalone/symbol_table_tests_core_add.c
// Add symbol tests

// Test adding local symbol basic
static void test_symbol_table_add_symbol_local_basic(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_local_basic");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("test_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_LOCAL);
    assert(sym->type->kind == TYPE_INT);
    assert(sym->offset == -LOCAL_BASE_OFFSET);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 8);  // int size 8, aligned

    // Check duplicated name (should update type, but since same, ok; code updates type)
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    symbol_table_add_symbol(&table, name, double_type);
    assert(sym->type->kind == TYPE_DOUBLE);  // Updated

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_local_basic");
}

static void test_symbol_table_add_symbol_param(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_param");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);  // Fix: Initialize the symbol table with the arena
    symbol_table_begin_function_scope(&table);  // Function scope

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("param1");
    symbol_table_add_symbol_with_kind(&table, name, int_type, SYMBOL_PARAM);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_PARAM);
    assert(sym->offset == -PARAM_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET + 8);

    // Add another param, offset accumulates negatively
    Token name2 = TOKEN_LITERAL("param2");
    symbol_table_add_symbol_with_kind(&table, name2, int_type, SYMBOL_PARAM);
    Symbol *sym2 = symbol_table_lookup_symbol_current(&table, name2);
    assert(sym2->offset == -(PARAM_BASE_OFFSET + 8));

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_param");
}

// Test adding global (kind default, offset 0)
static void test_symbol_table_add_symbol_global(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_global");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("global_sym");
    symbol_table_add_symbol_with_kind(&table, name, int_type, SYMBOL_GLOBAL);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_GLOBAL);
    assert(sym->offset == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_global");
}

// Test adding in no current scope (should error, do nothing)
static void test_symbol_table_add_symbol_no_scope(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_no_scope");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table = {0};  // Uninitialized, current NULL

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("no_scope_var");
    symbol_table_add_symbol(&table, name, int_type);  // Should do nothing

    assert(table.current == NULL);  // Unchanged

    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_no_scope");
}

// Test type cloning on add (since add clones type)
static void test_symbol_table_add_symbol_type_clone(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_type_clone");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *orig_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("clone_var");
    symbol_table_add_symbol(&table, name, orig_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym->type != orig_type);  // Cloned
    assert(ast_type_equals(sym->type, orig_type));  // But equal

    // Modify orig (if possible), but since primitive, just check equality

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_type_clone");
}

// Test arena exhaustion (simulate by small arena, but hard; test alloc failures)
static void test_symbol_table_add_symbol_arena_exhaust(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_arena_exhaust");

    // Note: Hard to simulate exact OOM in unit test without mocking arena.
    // Assume code handles NULL from arena_alloc gracefully (DEBUG_ERROR, return early)

    Arena small_arena;
    arena_init(&small_arena, 64);  // Very small
    SymbolTable table;
    symbol_table_init(&small_arena, &table);

    // This may fail, but test that it doesn't crash
    Type *int_type = create_int_type(&small_arena);  // May fail if arena too small
    if (int_type == NULL) {
        printf("Arena too small, skipping add test.\n");
    } else {
        Token name = TOKEN_LITERAL("oom_var");
        symbol_table_add_symbol(&table, name, int_type);
        // If added, ok; if not, current->symbols may be NULL
    }

    symbol_table_cleanup(&table);
    arena_free(&small_arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_arena_exhaust");
}

// Test adding many symbols (stress offsets)
static void test_symbol_table_add_many_symbols(void) {
    DEBUG_INFO("Starting test_symbol_table_add_many_symbols");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);  // Larger for many
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    int expected_offset = LOCAL_BASE_OFFSET;
    for (int i = 0; i < 100; i++) {  // 100 locals, each 8 bytes
        char name_buf[16];
        snprintf(name_buf, sizeof(name_buf), "var_%d", i);
        Token name = TOKEN_PTR(name_buf, strlen(name_buf));
        symbol_table_add_symbol(&table, name, int_type);

        Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
        assert(sym != NULL);
        assert(sym->offset == -expected_offset);
        expected_offset += 8;
    }
    assert(table.current->next_local_offset == expected_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_many_symbols");
}

// Test token duplication on add (arena_strndup)
static void test_symbol_table_add_symbol_token_dup(void) {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_token_dup");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    const char *orig_str = "dup_test";
    Token orig_token = TOKEN_PTR(orig_str, 8);
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, orig_token, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, orig_token);
    assert(sym != NULL);
    assert(sym->name.start != orig_str);  // Duplicated
    assert(memcmp(sym->name.start, orig_str, 8) == 0);
    assert(sym->name.length == 8);
    assert(sym->name.line == 1);
    assert(sym->name.type == TOKEN_IDENTIFIER);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_token_dup");
}

// Test complex types (array, function) cloning and size
static void test_symbol_table_add_complex_types(void) {
    DEBUG_INFO("Starting test_symbol_table_add_complex_types");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Array type
    Type *int_type = create_int_type(&arena);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Token arr_name = TOKEN_LITERAL("arr_sym");
    symbol_table_add_symbol(&table, arr_name, arr_type);
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_name);
    assert(arr_sym->type->kind == TYPE_ARRAY);
    assert(get_type_size(arr_sym->type) == 8);  // Default size 8 for pointer?

    // Function type
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *param_types[] = {int_type};
    Type *func_type = ast_create_function_type(&arena, void_type, param_types, 1);
    Token func_name = TOKEN_LITERAL("func_sym");
    symbol_table_add_symbol(&table, func_name, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_name);
    assert(func_sym->type->kind == TYPE_FUNCTION);
    assert(get_type_size(func_sym->type) == 8);  // Default

    // Check equality after clone
    assert(ast_type_equals(arr_sym->type, arr_type));
    assert(ast_type_equals(func_sym->type, func_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_complex_types");
}
