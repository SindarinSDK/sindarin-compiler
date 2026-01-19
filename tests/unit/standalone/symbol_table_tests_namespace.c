// tests/unit/standalone/symbol_table_tests_namespace.c
// Namespace tests for symbol table

#include <assert.h>
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

// Helper to create a simple int type
static Type *create_int_type_ns(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

// Helper to create a simple string type (pointer-sized)
static Type *create_string_type_ns(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_STRING);
}

// =====================================================
// Namespace Tests
// =====================================================

// Test symbol_table_add_namespace creates namespace correctly
static void test_namespace_add_namespace(void) {
    DEBUG_INFO("Starting test_namespace_add_namespace");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token ns_name = TOKEN_LITERAL("myns");
    symbol_table_add_namespace(&table, ns_name);

    // Check namespace was added to global scope
    Symbol *ns_sym = table.global_scope->symbols;
    assert(ns_sym != NULL);
    assert(ns_sym->is_namespace == true);
    assert(ns_sym->kind == SYMBOL_NAMESPACE);
    assert(ns_sym->namespace_symbols == NULL);  // Initially empty
    assert(memcmp(ns_sym->name.start, "myns", 4) == 0);
    assert(ns_sym->name.length == 4);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_add_namespace");
}

// Test symbol_table_add_symbol_to_namespace adds symbols to namespace
static void test_namespace_add_symbol_to_namespace(void) {
    DEBUG_INFO("Starting test_namespace_add_symbol_to_namespace");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token ns_name = TOKEN_LITERAL("mathns");
    symbol_table_add_namespace(&table, ns_name);

    // Add symbols to namespace
    Type *int_type = create_int_type_ns(&arena);
    Token sym_name1 = TOKEN_LITERAL("PI");
    Token sym_name2 = TOKEN_LITERAL("E");

    symbol_table_add_symbol_to_namespace(&table, ns_name, sym_name1, int_type);
    symbol_table_add_symbol_to_namespace(&table, ns_name, sym_name2, int_type);

    // Verify symbols were added to namespace
    Symbol *ns_sym = table.global_scope->symbols;
    assert(ns_sym != NULL);
    assert(ns_sym->is_namespace == true);
    assert(ns_sym->namespace_symbols != NULL);

    // Symbols are in linked list (added at head, so E first, then PI)
    Symbol *first_sym = ns_sym->namespace_symbols;
    assert(first_sym != NULL);
    assert(memcmp(first_sym->name.start, "E", 1) == 0);
    assert(first_sym->kind == SYMBOL_GLOBAL);

    Symbol *second_sym = first_sym->next;
    assert(second_sym != NULL);
    assert(memcmp(second_sym->name.start, "PI", 2) == 0);
    assert(second_sym->kind == SYMBOL_GLOBAL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_add_symbol_to_namespace");
}

// Test symbol_table_lookup_in_namespace finds namespaced symbols
static void test_namespace_lookup_in_namespace(void) {
    DEBUG_INFO("Starting test_namespace_lookup_in_namespace");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token ns_name = TOKEN_LITERAL("utils");
    symbol_table_add_namespace(&table, ns_name);

    Type *int_type = create_int_type_ns(&arena);
    Type *str_type = create_string_type_ns(&arena);
    Token sym_name1 = TOKEN_LITERAL("helper");
    Token sym_name2 = TOKEN_LITERAL("format");

    symbol_table_add_symbol_to_namespace(&table, ns_name, sym_name1, int_type);
    symbol_table_add_symbol_to_namespace(&table, ns_name, sym_name2, str_type);

    // Lookup existing symbols
    Symbol *found1 = symbol_table_lookup_in_namespace(&table, ns_name, sym_name1);
    assert(found1 != NULL);
    assert(found1->type->kind == TYPE_INT);
    assert(memcmp(found1->name.start, "helper", 6) == 0);

    Symbol *found2 = symbol_table_lookup_in_namespace(&table, ns_name, sym_name2);
    assert(found2 != NULL);
    assert(found2->type->kind == TYPE_STRING);

    // Lookup non-existent symbol
    Token bad_sym = TOKEN_LITERAL("nonexistent");
    Symbol *not_found = symbol_table_lookup_in_namespace(&table, ns_name, bad_sym);
    assert(not_found == NULL);

    // Lookup in non-existent namespace
    Token bad_ns = TOKEN_LITERAL("badns");
    Symbol *ns_not_found = symbol_table_lookup_in_namespace(&table, bad_ns, sym_name1);
    assert(ns_not_found == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_lookup_in_namespace");
}

// Test symbol_table_is_namespace correctly identifies namespaces
static void test_namespace_is_namespace(void) {
    DEBUG_INFO("Starting test_namespace_is_namespace");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add a namespace
    Token ns_name = TOKEN_LITERAL("myns");
    symbol_table_add_namespace(&table, ns_name);

    // Add a regular symbol to global scope
    Type *int_type = create_int_type_ns(&arena);
    Token regular_sym = TOKEN_LITERAL("regular");
    symbol_table_add_symbol_with_kind(&table, regular_sym, int_type, SYMBOL_GLOBAL);

    // Check is_namespace returns true for namespace
    assert(symbol_table_is_namespace(&table, ns_name) == true);

    // Check is_namespace returns false for regular symbol
    assert(symbol_table_is_namespace(&table, regular_sym) == false);

    // Check is_namespace returns false for non-existent symbol
    Token nonexistent = TOKEN_LITERAL("nonexistent");
    assert(symbol_table_is_namespace(&table, nonexistent) == false);

    // Check with NULL table
    assert(symbol_table_is_namespace(NULL, ns_name) == false);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_is_namespace");
}

// Test error handling: adding symbol to non-existent namespace
static void test_namespace_add_symbol_to_nonexistent(void) {
    DEBUG_INFO("Starting test_namespace_add_symbol_to_nonexistent");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Try to add symbol to non-existent namespace
    Token bad_ns = TOKEN_LITERAL("badns");
    Token sym_name = TOKEN_LITERAL("sym");
    Type *int_type = create_int_type_ns(&arena);

    // Should not crash, just log error
    symbol_table_add_symbol_to_namespace(&table, bad_ns, sym_name, int_type);

    // Verify nothing was added to global scope (only the initial empty global scope)
    // The only symbol should be NULL since we didn't add any regular symbols
    // and the namespace didn't exist
    assert(table.global_scope->symbols == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_add_symbol_to_nonexistent");
}

// Test error handling: duplicate namespace names
static void test_namespace_duplicate_names(void) {
    DEBUG_INFO("Starting test_namespace_duplicate_names");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token ns_name = TOKEN_LITERAL("dupns");

    // Add namespace first time
    symbol_table_add_namespace(&table, ns_name);
    Symbol *first_ns = table.global_scope->symbols;
    assert(first_ns != NULL);
    assert(first_ns->is_namespace == true);

    // Try to add same namespace again (should log error and not add)
    symbol_table_add_namespace(&table, ns_name);

    // Should still only have one namespace symbol
    assert(table.global_scope->symbols == first_ns);
    assert(first_ns->next == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_duplicate_names");
}

// Test that regular lookup still works for direct (non-namespaced) symbols
static void test_namespace_regular_lookup_unaffected(void) {
    DEBUG_INFO("Starting test_namespace_regular_lookup_unaffected");
    
    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add a namespace
    Token ns_name = TOKEN_LITERAL("myns");
    symbol_table_add_namespace(&table, ns_name);

    // Add symbol to namespace
    Type *int_type = create_int_type_ns(&arena);
    Token ns_sym = TOKEN_LITERAL("ns_var");
    symbol_table_add_symbol_to_namespace(&table, ns_name, ns_sym, int_type);

    // Add regular symbol to current scope (global)
    Token regular_sym = TOKEN_LITERAL("regular_var");
    symbol_table_add_symbol(&table, regular_sym, int_type);

    // Regular lookup should find regular_var
    Symbol *found_regular = symbol_table_lookup_symbol(&table, regular_sym);
    assert(found_regular != NULL);
    assert(memcmp(found_regular->name.start, "regular_var", 11) == 0);

    // Regular lookup should NOT find ns_var (it's in namespace, not current scope)
    Symbol *not_found = symbol_table_lookup_symbol(&table, ns_sym);
    assert(not_found == NULL);

    // But namespace lookup should find ns_var
    Symbol *found_ns = symbol_table_lookup_in_namespace(&table, ns_name, ns_sym);
    assert(found_ns != NULL);
    assert(memcmp(found_ns->name.start, "ns_var", 6) == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_namespace_regular_lookup_unaffected");
}

void test_symbol_table_namespace_main(void)
{
    TEST_SECTION("Symbol Table Namespace");

    TEST_RUN("namespace_add_namespace", test_namespace_add_namespace);
    TEST_RUN("namespace_add_symbol_to_namespace", test_namespace_add_symbol_to_namespace);
    TEST_RUN("namespace_lookup_in_namespace", test_namespace_lookup_in_namespace);
    TEST_RUN("namespace_is_namespace", test_namespace_is_namespace);
    TEST_RUN("namespace_add_symbol_to_nonexistent", test_namespace_add_symbol_to_nonexistent);
    TEST_RUN("namespace_duplicate_names", test_namespace_duplicate_names);
    TEST_RUN("namespace_regular_lookup_unaffected", test_namespace_regular_lookup_unaffected);
}
