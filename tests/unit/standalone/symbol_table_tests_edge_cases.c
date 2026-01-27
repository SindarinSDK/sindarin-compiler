// tests/unit/standalone/symbol_table_tests_edge_cases.c
// Edge case tests for symbol table

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../symbol_table.h"
#include "../ast.h"
#include "../test_harness.h"

// Helper macros and constants
#define TEST_ARENA_SIZE 4096
#define TOKEN_LITERAL(str) ((Token){ .start = str, .length = sizeof(str) - 1, .line = 1, .type = TOKEN_IDENTIFIER })
#define TOKEN_PTR(str, len) ((Token){ .start = (const char*)str, .length = len, .line = 1, .type = TOKEN_IDENTIFIER })

// Helper to create types
static Type *create_int_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

static Type *create_void_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_VOID);
}

static Type *create_bool_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_BOOL);
}

static Type *create_char_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_CHAR);
}

static Type *create_double_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_DOUBLE);
}

static Type *create_string_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_STRING);
}

static Type *create_byte_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_BYTE);
}

static Type *create_long_type_edge(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_LONG);
}

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

// =====================================================
// Scope Nesting Edge Cases
// =====================================================

static void test_edge_deeply_nested_scopes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    char name_buf[32];

    // Create 20 nested scopes with a symbol in each
    for (int i = 0; i < 20; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "level_%d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);
        symbol_table_push_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 21);

    // Verify all symbols are accessible from innermost scope
    for (int i = 0; i < 20; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "level_%d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
        assert(sym->declaration_scope_depth == i + 1);
    }

    // Pop all scopes
    for (int i = 0; i < 20; i++) {
        symbol_table_pop_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_multiple_function_scopes(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Simulate 3 function definitions one after another
    for (int fn = 0; fn < 3; fn++) {
        symbol_table_begin_function_scope(&table);

        // Each function has params
        char name_buf[32];
        int len = snprintf(name_buf, sizeof(name_buf), "param%d", fn);
        symbol_table_add_symbol_with_kind(&table, TOKEN_PTR(name_buf, len), int_type, SYMBOL_PARAM);

        // Each function has locals
        len = snprintf(name_buf, sizeof(name_buf), "local%d", fn);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);

        symbol_table_pop_scope(&table);
    }

    // Should be back at global scope
    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_function_scope_with_nested_blocks(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Function scope
    symbol_table_begin_function_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("func_local"), int_type);

    // if block
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("if_local"), int_type);

    // nested for block
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("for_local"), int_type);

    // All should be accessible
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("func_local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("for_local")) != NULL);

    // Pop for block
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("for_local")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) != NULL);

    // Pop if block
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("if_local")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("func_local")) != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

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

// =====================================================
// Lookup Edge Cases
// =====================================================

static void test_edge_lookup_case_sensitive(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("myVar"), int_type);

    // Different cases should not match
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("myVar")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("MYVAR")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("myvar")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("MyVar")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_partial_match_fails(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("fullname"), int_type);

    // Partial matches should fail
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("full")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("fullnam")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("fullnamee")) == NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("name")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_empty_table(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Lookup in empty table should return NULL
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("anything")) == NULL);
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("anything")) == NULL);
    assert(symbol_table_get_symbol_offset(&table, TOKEN_LITERAL("anything")) == -1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_lookup_after_removal(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Add in nested scope
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("scoped_var"), int_type);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("scoped_var")) != NULL);

    // Pop scope - symbol should no longer be accessible
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("scoped_var")) == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Namespace Edge Cases
// =====================================================

static void test_edge_multiple_namespaces(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Create multiple namespaces
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns1"));
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns2"));
    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns3"));

    // Add same symbol name to each namespace
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns1"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns2"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns3"), TOKEN_LITERAL("x"), int_type);

    // Each namespace should have its own x
    Symbol *x1 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns1"), TOKEN_LITERAL("x"));
    Symbol *x2 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns2"), TOKEN_LITERAL("x"));
    Symbol *x3 = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("ns3"), TOKEN_LITERAL("x"));

    assert(x1 != NULL && x2 != NULL && x3 != NULL);
    assert(x1 != x2 && x2 != x3 && x1 != x3);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_namespace_and_regular_same_name(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);

    // Create namespace
    symbol_table_add_namespace(&table, TOKEN_LITERAL("myns"));
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("myns"), TOKEN_LITERAL("x"), int_type);

    // Add regular symbol with same name as namespace symbol
    symbol_table_add_symbol(&table, TOKEN_LITERAL("x"), bool_type);

    // Regular lookup should find regular symbol
    Symbol *regular_x = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("x"));
    assert(regular_x != NULL);
    assert(regular_x->type->kind == TYPE_BOOL);

    // Namespace lookup should find namespace symbol
    Symbol *ns_x = symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("myns"), TOKEN_LITERAL("x"));
    assert(ns_x != NULL);
    assert(ns_x->type->kind == TYPE_INT);

    assert(regular_x != ns_x);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_empty_namespace(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_add_namespace(&table, TOKEN_LITERAL("emptyns"));

    // Lookup in empty namespace should return NULL
    assert(symbol_table_lookup_in_namespace(&table, TOKEN_LITERAL("emptyns"), TOKEN_LITERAL("anything")) == NULL);

    // But namespace should exist
    assert(symbol_table_is_namespace(&table, TOKEN_LITERAL("emptyns")) == true);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Thread State Edge Cases
// =====================================================

static void test_edge_thread_state_multiple_symbols(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("a"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("b"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("c"), int_type);

    Symbol *a = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("a"));
    Symbol *b = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("b"));
    Symbol *c = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("c"));

    // Each symbol has independent thread state
    symbol_table_mark_pending(a);
    assert(symbol_table_is_pending(a) == true);
    assert(symbol_table_is_pending(b) == false);
    assert(symbol_table_is_pending(c) == false);

    symbol_table_mark_pending(b);
    symbol_table_mark_synchronized(a);
    assert(symbol_table_is_synchronized(a) == true);
    assert(symbol_table_is_pending(b) == true);
    assert(symbol_table_is_pending(c) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Stress and Boundary Tests
// =====================================================

static void test_edge_many_symbols_same_scope(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 16);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    char name_buf[32];

    // Add 200 symbols to same scope
    for (int i = 0; i < 200; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "sym_%03d", i);
        symbol_table_add_symbol(&table, TOKEN_PTR(name_buf, len), int_type);
    }

    // Verify all are accessible
    for (int i = 0; i < 200; i++) {
        int len = snprintf(name_buf, sizeof(name_buf), "sym_%03d", i);
        Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_PTR(name_buf, len));
        assert(sym != NULL);
    }

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_scope_capacity_expansion(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Push enough scopes to trigger capacity expansion (initial is 8)
    for (int i = 0; i < 20; i++) {
        symbol_table_push_scope(&table);
    }

    assert(table.scopes_count == 21);
    assert(table.scopes_capacity >= 32);  // Should have expanded

    for (int i = 0; i < 20; i++) {
        symbol_table_pop_scope(&table);
    }

    assert(symbol_table_get_scope_depth(&table) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_symbol_type_update(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *bool_type = create_bool_type_edge(&arena);

    Token name = TOKEN_LITERAL("updateable");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym->type->kind == TYPE_INT);

    // Adding same symbol again updates type
    symbol_table_add_symbol(&table, name, bool_type);
    assert(sym->type->kind == TYPE_BOOL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_type_equals_cloned(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token name = TOKEN_LITERAL("cloned_type");
    symbol_table_add_symbol(&table, name, arr_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);

    // Type should be cloned but equal
    assert(sym->type != arr_type);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_zero_length_token(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);
    Token empty = TOKEN_PTR("", 0);

    // Zero length token should not crash
    symbol_table_add_symbol(&table, empty, int_type);

    // Lookup should work
    Symbol *sym = symbol_table_lookup_symbol(&table, empty);
    assert(sym != NULL);
    assert(sym->name.length == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_scope_depth_consistency(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Create complex scope pattern
    symbol_table_push_scope(&table);      // depth 2
    symbol_table_push_scope(&table);      // depth 3
    symbol_table_pop_scope(&table);       // depth 2
    symbol_table_push_scope(&table);      // depth 3
    symbol_table_push_scope(&table);      // depth 4

    symbol_table_add_symbol(&table, TOKEN_LITERAL("deep_var"), int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("deep_var"));

    assert(symbol_table_get_scope_depth(&table) == 4);
    assert(sym->declaration_scope_depth == 4);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_current_lookup_vs_full_lookup(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_symbol(&table, TOKEN_LITERAL("global"), int_type);
    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("local"), int_type);

    // Current scope lookup should find only local
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("local")) != NULL);
    assert(symbol_table_lookup_symbol_current(&table, TOKEN_LITERAL("global")) == NULL);

    // Full lookup should find both
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local")) != NULL);
    assert(symbol_table_lookup_symbol(&table, TOKEN_LITERAL("global")) != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_function_scope_offset_reset(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    // Add vars at global
    symbol_table_add_symbol(&table, TOKEN_LITERAL("g1"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("g2"), int_type);
    int global_offset = table.global_scope->next_local_offset;

    // Enter function - offsets should reset
    symbol_table_begin_function_scope(&table);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);

    // Add function vars
    symbol_table_add_symbol(&table, TOKEN_LITERAL("f1"), int_type);

    // Exit function
    symbol_table_pop_scope(&table);

    // Global offset should remain unchanged or take max
    assert(table.global_scope->next_local_offset >= global_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_print_empty_scope(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Print should not crash on empty scope
    symbol_table_print(&table, "empty_test");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_edge_print_with_namespaces(void) {
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type_edge(&arena);

    symbol_table_add_namespace(&table, TOKEN_LITERAL("ns"));
    symbol_table_add_symbol_to_namespace(&table, TOKEN_LITERAL("ns"), TOKEN_LITERAL("x"), int_type);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("y"), int_type);

    // Print should handle both regular and namespace symbols
    symbol_table_print(&table, "namespace_test");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_symbol_table_edge_cases_main(void)
{
    TEST_SECTION("Symbol Table Edge Cases");

    // Token name edge cases
    TEST_RUN("edge_single_char_name", test_edge_single_char_name);
    TEST_RUN("edge_long_name", test_edge_long_name);
    TEST_RUN("edge_underscore_only_name", test_edge_underscore_only_name);
    TEST_RUN("edge_double_underscore_name", test_edge_double_underscore_name);
    TEST_RUN("edge_numeric_suffix_name", test_edge_numeric_suffix_name);
    TEST_RUN("edge_similar_prefixes", test_edge_similar_prefixes);

    // Type size and alignment
    TEST_RUN("edge_all_primitive_types_size", test_edge_all_primitive_types_size);
    TEST_RUN("edge_void_type_symbol", test_edge_void_type_symbol);
    TEST_RUN("edge_array_of_arrays_type", test_edge_array_of_arrays_type);
    TEST_RUN("edge_function_type_with_many_params", test_edge_function_type_with_many_params);
    TEST_RUN("edge_pointer_type", test_edge_pointer_type);

    // Scope nesting
    TEST_RUN("edge_deeply_nested_scopes", test_edge_deeply_nested_scopes);
    TEST_RUN("edge_multiple_function_scopes", test_edge_multiple_function_scopes);
    TEST_RUN("edge_function_scope_with_nested_blocks", test_edge_function_scope_with_nested_blocks);

    // Shadowing
    TEST_RUN("edge_multi_level_shadowing", test_edge_multi_level_shadowing);
    TEST_RUN("edge_shadowing_in_sibling_scopes", test_edge_shadowing_in_sibling_scopes);
    TEST_RUN("edge_no_shadow_different_names", test_edge_no_shadow_different_names);

    // Symbol kinds
    TEST_RUN("edge_all_symbol_kinds", test_edge_all_symbol_kinds);
    TEST_RUN("edge_mixed_params_and_locals", test_edge_mixed_params_and_locals);

    // Offset calculation
    TEST_RUN("edge_offset_after_many_variables", test_edge_offset_after_many_variables);
    TEST_RUN("edge_offset_propagation_complex", test_edge_offset_propagation_complex);
    TEST_RUN("edge_param_offset_sequence", test_edge_param_offset_sequence);

    // Lookup
    TEST_RUN("edge_lookup_case_sensitive", test_edge_lookup_case_sensitive);
    TEST_RUN("edge_lookup_partial_match_fails", test_edge_lookup_partial_match_fails);
    TEST_RUN("edge_lookup_empty_table", test_edge_lookup_empty_table);
    TEST_RUN("edge_lookup_after_removal", test_edge_lookup_after_removal);

    // Namespaces
    TEST_RUN("edge_multiple_namespaces", test_edge_multiple_namespaces);
    TEST_RUN("edge_namespace_and_regular_same_name", test_edge_namespace_and_regular_same_name);
    TEST_RUN("edge_empty_namespace", test_edge_empty_namespace);

    // Thread state
    TEST_RUN("edge_thread_state_multiple_symbols", test_edge_thread_state_multiple_symbols);

    // Stress and boundary
    TEST_RUN("edge_many_symbols_same_scope", test_edge_many_symbols_same_scope);
    TEST_RUN("edge_scope_capacity_expansion", test_edge_scope_capacity_expansion);
    TEST_RUN("edge_symbol_type_update", test_edge_symbol_type_update);
    TEST_RUN("edge_type_equals_cloned", test_edge_type_equals_cloned);
    TEST_RUN("edge_zero_length_token", test_edge_zero_length_token);
    TEST_RUN("edge_scope_depth_consistency", test_edge_scope_depth_consistency);
    TEST_RUN("edge_current_lookup_vs_full_lookup", test_edge_current_lookup_vs_full_lookup);
    TEST_RUN("edge_function_scope_offset_reset", test_edge_function_scope_offset_reset);
    TEST_RUN("edge_print_empty_scope", test_edge_print_empty_scope);
    TEST_RUN("edge_print_with_namespaces", test_edge_print_with_namespaces);
}
