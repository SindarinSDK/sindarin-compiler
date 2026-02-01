// tests/unit/standalone/symbol_table_tests_edge_cases_types.c
// Type Size and Alignment Edge Cases for symbol table

// =====================================================
// Type Size and Alignment Edge Cases
// =====================================================

static void test_edge_all_primitive_types_size(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);

    // Add one of each primitive type
    Type *types[] = {
        create_int_type_edge(&arena),
        create_bool_type_edge(&arena),
        create_char_type_edge(&arena),
        create_double_type_edge(&arena),
        create_string_type_edge(&arena),
        create_byte_type_edge(&arena),
        create_long_type_edge(&arena)
    };
    const char *names[] = {"i", "b", "c", "d", "s", "by", "l"};

    for (int i = 0; i < 7; i++) {
        Token name = TOKEN_PTR(names[i], strlen(names[i]));
        symbol_table_add_symbol(&table, name, types[i]);
        Symbol *sym = symbol_table_lookup_symbol(&table, name);
        assert(sym != NULL);
    }

    // Check offsets are properly aligned
    int expected_offset = LOCAL_BASE_OFFSET + 7 * 8;  // All types align to 8
    assert(table.current->next_local_offset == expected_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_void_type_symbol(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *void_type = create_void_type_edge(&arena);
    Token name = TOKEN_LITERAL("void_sym");
    symbol_table_add_symbol(&table, name, void_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_VOID);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_array_of_arrays_type(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *arr_arr_type = ast_create_array_type(&arena, arr_type);

    Token name = TOKEN_LITERAL("nested_arr");
    symbol_table_add_symbol(&table, name, arr_arr_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_ARRAY);
    assert(sym->type->data.array.element_type->kind == TYPE_ARRAY);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_function_type_with_many_params(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *param_types[10];
    for (int i = 0; i < 10; i++) {
        param_types[i] = int_type;
    }

    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 10);

    Token name = TOKEN_LITERAL("many_params_fn");
    symbol_table_add_symbol(&table, name, func_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_FUNCTION);
    assert(sym->type->data.function.param_count == 10);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_pointer_type(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *ptr_type = ast_create_pointer_type(&arena, int_type);

    Token name = TOKEN_LITERAL("ptr_var");
    symbol_table_add_symbol(&table, name, ptr_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_POINTER);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}
