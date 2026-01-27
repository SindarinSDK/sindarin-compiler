// tests/unit/standalone/symbol_table_tests_stress.c
// Symbol table stress tests - multiple operations and edge cases

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../symbol_table.h"
#include "../ast.h"
#include "../test_harness.h"

#define ST_STRESS_ARENA_SIZE 16384
#define TOKEN_LITERAL(str) ((Token){ .start = str, .length = sizeof(str) - 1, .line = 1, .type = TOKEN_IDENTIFIER })
#define TOKEN_PTR(str, len) ((Token){ .start = (const char*)str, .length = len, .line = 1, .type = TOKEN_IDENTIFIER })

static Type *make_int_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

static Type *make_str_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_STRING);
}

static Type *make_bool_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_BOOL);
}

/* ============================================================================
 * Many Symbols Tests
 * ============================================================================ */

static void test_symbol_table_stress_many_symbols(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    char names[50][16];
    for (int i = 0; i < 50; i++) {
        snprintf(names[i], 16, "var_%d", i);
        Token tok = TOKEN_PTR(names[i], strlen(names[i]));
        Type *type = make_int_type(&arena);
        symbol_table_add_symbol(&table, tok, type);
    }

    // Lookup all symbols
    for (int i = 0; i < 50; i++) {
        Token tok = TOKEN_PTR(names[i], strlen(names[i]));
        Symbol *sym = symbol_table_lookup_symbol(&table, tok);
        assert(sym != NULL);
    }

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_deep_nesting(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Push 20 scopes
    for (int i = 0; i < 20; i++) {
        symbol_table_push_scope(&table);
    }

    assert(table.scopes_count == 21);  // global + 20

    // Pop all scopes
    for (int i = 0; i < 20; i++) {
        symbol_table_pop_scope(&table);
    }

    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_scope_shadowing(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add x in global scope
    Token x = TOKEN_LITERAL("x");
    Type *int_type = make_int_type(&arena);
    symbol_table_add_symbol(&table, x, int_type);

    // Push a new scope and add another x
    symbol_table_push_scope(&table);
    Type *str_type = make_str_type(&arena);
    symbol_table_add_symbol(&table, x, str_type);

    // Lookup should find inner x (string type)
    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_STRING);

    // Pop scope, lookup should find outer x (int type)
    symbol_table_pop_scope(&table);
    sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);
    assert(sym->type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_multiple_types(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token a = TOKEN_LITERAL("a");
    Token b = TOKEN_LITERAL("b");
    Token c = TOKEN_LITERAL("c");
    Token d = TOKEN_LITERAL("d");

    symbol_table_add_symbol(&table, a, make_int_type(&arena));
    symbol_table_add_symbol(&table, b, make_str_type(&arena));
    symbol_table_add_symbol(&table, c, make_bool_type(&arena));
    Type *arr_type = ast_create_array_type(&arena, make_int_type(&arena));
    symbol_table_add_symbol(&table, d, arr_type);

    Symbol *sym_a = symbol_table_lookup_symbol(&table, a);
    Symbol *sym_b = symbol_table_lookup_symbol(&table, b);
    Symbol *sym_c = symbol_table_lookup_symbol(&table, c);
    Symbol *sym_d = symbol_table_lookup_symbol(&table, d);

    assert(sym_a != NULL && sym_a->type->kind == TYPE_INT);
    assert(sym_b != NULL && sym_b->type->kind == TYPE_STRING);
    assert(sym_c != NULL && sym_c->type->kind == TYPE_BOOL);
    assert(sym_d != NULL && sym_d->type->kind == TYPE_ARRAY);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Lookup Tests
 * ============================================================================ */

static void test_symbol_table_stress_lookup_not_found(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token x = TOKEN_LITERAL("x");
    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_lookup_current_only(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add x in global scope
    Token x = TOKEN_LITERAL("x");
    symbol_table_add_symbol(&table, x, make_int_type(&arena));

    // Push scope - should still find x
    symbol_table_push_scope(&table);
    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);

    // Lookup in current scope only - should NOT find x
    sym = symbol_table_lookup_symbol_current(&table, x);
    assert(sym == NULL);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_lookup_enclosing(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add x in global
    Token x = TOKEN_LITERAL("x");
    symbol_table_add_symbol(&table, x, make_int_type(&arena));

    // Push two scopes
    symbol_table_push_scope(&table);
    symbol_table_push_scope(&table);

    // Should still find x from global
    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);

    symbol_table_pop_scope(&table);
    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Symbol Kind Tests
 * ============================================================================ */

static void test_symbol_table_stress_global_symbol(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token global = TOKEN_LITERAL("global_var");
    Type *int_type = make_int_type(&arena);
    symbol_table_add_symbol_with_kind(&table, global, int_type, SYMBOL_GLOBAL);

    Symbol *sym = symbol_table_lookup_symbol(&table, global);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_GLOBAL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_local_symbol(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_push_scope(&table);
    Token local = TOKEN_LITERAL("local_var");
    Type *int_type = make_int_type(&arena);
    symbol_table_add_symbol_with_kind(&table, local, int_type, SYMBOL_LOCAL);

    Symbol *sym = symbol_table_lookup_symbol(&table, local);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_LOCAL);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_param_symbol(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_begin_function_scope(&table);
    Token param = TOKEN_LITERAL("param1");
    symbol_table_add_symbol_with_kind(&table, param, make_int_type(&arena), SYMBOL_PARAM);

    Symbol *sym = symbol_table_lookup_symbol(&table, param);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_PARAM);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_type_alias(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_name = TOKEN_LITERAL("MyType");
    symbol_table_add_type(&table, type_name, make_int_type(&arena));

    Symbol *sym = symbol_table_lookup_type(&table, type_name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_TYPE);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Memory Qualifier Tests
 * ============================================================================ */

static void test_symbol_table_stress_mem_as_val(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token x = TOKEN_LITERAL("x");
    symbol_table_add_symbol_full(&table, x, make_int_type(&arena), SYMBOL_LOCAL, MEM_AS_VAL);

    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);
    assert(sym->mem_qual == MEM_AS_VAL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_mem_as_ref(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token x = TOKEN_LITERAL("x");
    symbol_table_add_symbol_full(&table, x, make_int_type(&arena), SYMBOL_LOCAL, MEM_AS_REF);

    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);
    assert(sym->mem_qual == MEM_AS_REF);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_mem_default(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token x = TOKEN_LITERAL("x");
    symbol_table_add_symbol(&table, x, make_int_type(&arena));

    Symbol *sym = symbol_table_lookup_symbol(&table, x);
    assert(sym != NULL);
    assert(sym->mem_qual == MEM_DEFAULT);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_symbol_table_stress_rapid_push_pop(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    for (int i = 0; i < 100; i++) {
        symbol_table_push_scope(&table);
        Token x = TOKEN_LITERAL("x");
        symbol_table_add_symbol(&table, x, make_int_type(&arena));
        symbol_table_pop_scope(&table);
    }

    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_many_arenas(void)
{
    for (int i = 0; i < 10; i++) {
        Arena arena;
        arena_init(&arena, ST_STRESS_ARENA_SIZE);
        SymbolTable table;
        symbol_table_init(&arena, &table);

        Token x = TOKEN_LITERAL("x");
        symbol_table_add_symbol(&table, x, make_int_type(&arena));

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }
}

static void test_symbol_table_stress_mixed_operations(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Mix of operations
    Token a = TOKEN_LITERAL("a");
    Token b = TOKEN_LITERAL("b");
    Token c = TOKEN_LITERAL("c");

    symbol_table_add_symbol(&table, a, make_int_type(&arena));

    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, b, make_str_type(&arena));

    symbol_table_push_scope(&table);
    symbol_table_add_symbol(&table, c, make_bool_type(&arena));

    // All should be visible
    assert(symbol_table_lookup_symbol(&table, a) != NULL);
    assert(symbol_table_lookup_symbol(&table, b) != NULL);
    assert(symbol_table_lookup_symbol(&table, c) != NULL);

    // Pop back
    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, c) == NULL);

    symbol_table_pop_scope(&table);
    assert(symbol_table_lookup_symbol(&table, b) == NULL);
    assert(symbol_table_lookup_symbol(&table, a) != NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_interleaved_kinds(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add globals, then push scope and add locals/params
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("g1"), make_int_type(&arena), SYMBOL_GLOBAL);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("g2"), make_str_type(&arena), SYMBOL_GLOBAL);

    symbol_table_begin_function_scope(&table);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("p1"), make_int_type(&arena), SYMBOL_PARAM);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("p2"), make_bool_type(&arena), SYMBOL_PARAM);
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("l1"), make_int_type(&arena), SYMBOL_LOCAL);

    Symbol *g1 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("g1"));
    Symbol *g2 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("g2"));
    Symbol *p1 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("p1"));
    Symbol *p2 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("p2"));
    Symbol *l1 = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("l1"));

    assert(g1 != NULL && g1->kind == SYMBOL_GLOBAL);
    assert(g2 != NULL && g2->kind == SYMBOL_GLOBAL);
    assert(p1 != NULL && p1->kind == SYMBOL_PARAM);
    assert(p2 != NULL && p2->kind == SYMBOL_PARAM);
    assert(l1 != NULL && l1->kind == SYMBOL_LOCAL);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_loop_context(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(!symbol_table_in_loop(&table));

    symbol_table_enter_loop(&table);
    assert(symbol_table_in_loop(&table));

    symbol_table_enter_loop(&table);  // nested loop
    assert(symbol_table_in_loop(&table));

    symbol_table_exit_loop(&table);
    assert(symbol_table_in_loop(&table));  // still in outer loop

    symbol_table_exit_loop(&table);
    assert(!symbol_table_in_loop(&table));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_symbol_table_stress_function_scope(void)
{
    Arena arena;
    arena_init(&arena, ST_STRESS_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    int initial_scope_depth = table.scope_depth;

    symbol_table_begin_function_scope(&table);
    assert(table.scope_depth == initial_scope_depth + 1);

    // Add params and locals
    symbol_table_add_symbol_with_kind(&table, TOKEN_LITERAL("arg"), make_int_type(&arena), SYMBOL_PARAM);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("local"), make_int_type(&arena));

    Symbol *arg = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("arg"));
    Symbol *local = symbol_table_lookup_symbol(&table, TOKEN_LITERAL("local"));
    assert(arg != NULL);
    assert(local != NULL);

    symbol_table_pop_scope(&table);
    assert(table.scope_depth == initial_scope_depth);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main Test Entry Point
 * ============================================================================ */

void test_symbol_table_stress_main(void)
{
    TEST_SECTION("Symbol Table Stress - Many Symbols");
    TEST_RUN("stress_many_symbols", test_symbol_table_stress_many_symbols);
    TEST_RUN("stress_deep_nesting", test_symbol_table_stress_deep_nesting);
    TEST_RUN("stress_scope_shadowing", test_symbol_table_stress_scope_shadowing);
    TEST_RUN("stress_multiple_types", test_symbol_table_stress_multiple_types);

    TEST_SECTION("Symbol Table Stress - Lookups");
    TEST_RUN("stress_lookup_not_found", test_symbol_table_stress_lookup_not_found);
    TEST_RUN("stress_lookup_current_only", test_symbol_table_stress_lookup_current_only);
    TEST_RUN("stress_lookup_enclosing", test_symbol_table_stress_lookup_enclosing);

    TEST_SECTION("Symbol Table Stress - Symbol Kinds");
    TEST_RUN("stress_global_symbol", test_symbol_table_stress_global_symbol);
    TEST_RUN("stress_local_symbol", test_symbol_table_stress_local_symbol);
    TEST_RUN("stress_param_symbol", test_symbol_table_stress_param_symbol);
    TEST_RUN("stress_type_alias", test_symbol_table_stress_type_alias);

    TEST_SECTION("Symbol Table Stress - Memory Qualifiers");
    TEST_RUN("stress_mem_as_val", test_symbol_table_stress_mem_as_val);
    TEST_RUN("stress_mem_as_ref", test_symbol_table_stress_mem_as_ref);
    TEST_RUN("stress_mem_default", test_symbol_table_stress_mem_default);

    TEST_SECTION("Symbol Table Stress - Operations");
    TEST_RUN("stress_rapid_push_pop", test_symbol_table_stress_rapid_push_pop);
    TEST_RUN("stress_many_arenas", test_symbol_table_stress_many_arenas);
    TEST_RUN("stress_mixed_operations", test_symbol_table_stress_mixed_operations);
    TEST_RUN("stress_interleaved_kinds", test_symbol_table_stress_interleaved_kinds);
    TEST_RUN("stress_loop_context", test_symbol_table_stress_loop_context);
    TEST_RUN("stress_function_scope", test_symbol_table_stress_function_scope);
}
