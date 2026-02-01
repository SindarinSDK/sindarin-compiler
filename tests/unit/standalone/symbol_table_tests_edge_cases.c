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

// Include split test modules
#include "symbol_table_tests_edge_cases_names.c"
#include "symbol_table_tests_edge_cases_types.c"
#include "symbol_table_tests_edge_cases_scope.c"
#include "symbol_table_tests_edge_cases_shadow.c"
#include "symbol_table_tests_edge_cases_kinds.c"
#include "symbol_table_tests_edge_cases_lookup.c"
#include "symbol_table_tests_edge_cases_namespace.c"
#include "symbol_table_tests_edge_cases_stress.c"

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
