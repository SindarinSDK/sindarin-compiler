// tests/unit/standalone/symbol_table_tests_core.c
// Core symbol table tests: initialization, scope management, symbol operations, lookup

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

// Helper to create a simple int type
static Type *create_int_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

// Helper to create a simple string type (pointer-sized)
static Type *create_string_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_STRING);
}

// Include split test modules
#include "symbol_table_tests_core_init.c"
#include "symbol_table_tests_core_scope.c"
#include "symbol_table_tests_core_add.c"
#include "symbol_table_tests_core_lookup.c"
#include "symbol_table_tests_core_offset.c"
#include "symbol_table_tests_core_depth.c"

void test_symbol_table_core_main(void)
{
    TEST_SECTION("Symbol Table Core");

    TEST_RUN("symbol_table_init_null_arena", test_symbol_table_init_null_arena);
    TEST_RUN("symbol_table_init_basic", test_symbol_table_init_basic);
    TEST_RUN("symbol_table_cleanup_empty", test_symbol_table_cleanup_empty);
    TEST_RUN("symbol_table_push_scope_single", test_symbol_table_push_scope_single);
    TEST_RUN("symbol_table_push_scope_nested", test_symbol_table_push_scope_nested);
    TEST_RUN("symbol_table_push_scope_expand", test_symbol_table_push_scope_expand);
    TEST_RUN("symbol_table_pop_scope_beyond_global", test_symbol_table_pop_scope_beyond_global);
    TEST_RUN("symbol_table_pop_scope_offset_propagation", test_symbol_table_pop_scope_offset_propagation);
    TEST_RUN("symbol_table_begin_function_scope", test_symbol_table_begin_function_scope);
    TEST_RUN("symbol_table_add_symbol_local_basic", test_symbol_table_add_symbol_local_basic);
    TEST_RUN("symbol_table_add_symbol_param", test_symbol_table_add_symbol_param);
    TEST_RUN("symbol_table_add_symbol_global", test_symbol_table_add_symbol_global);
    TEST_RUN("symbol_table_add_symbol_no_scope", test_symbol_table_add_symbol_no_scope);
    TEST_RUN("symbol_table_lookup_current_basic", test_symbol_table_lookup_current_basic);
    TEST_RUN("symbol_table_lookup_enclosing", test_symbol_table_lookup_enclosing);
    TEST_RUN("symbol_table_lookup_shadowing", test_symbol_table_lookup_shadowing);
    TEST_RUN("symbol_table_lookup_token_variations", test_symbol_table_lookup_token_variations);
    TEST_RUN("symbol_table_lookup_nulls", test_symbol_table_lookup_nulls);
    TEST_RUN("symbol_table_get_symbol_offset", test_symbol_table_get_symbol_offset);
    TEST_RUN("symbol_table_offsets_alignment", test_symbol_table_offsets_alignment);
    TEST_RUN("symbol_table_add_symbol_type_clone", test_symbol_table_add_symbol_type_clone);
    TEST_RUN("symbol_table_add_symbol_arena_exhaust", test_symbol_table_add_symbol_arena_exhaust);
    TEST_RUN("symbol_table_add_many_symbols", test_symbol_table_add_many_symbols);
    TEST_RUN("symbol_table_add_symbol_token_dup", test_symbol_table_add_symbol_token_dup);
    TEST_RUN("symbol_table_add_complex_types", test_symbol_table_add_complex_types);
    TEST_RUN("symbol_table_print", test_symbol_table_print);
    TEST_RUN("symbol_table_scope_depth_init", test_symbol_table_scope_depth_init);
    TEST_RUN("symbol_table_scope_depth_push", test_symbol_table_scope_depth_push);
    TEST_RUN("symbol_table_scope_depth_function", test_symbol_table_scope_depth_function);
    TEST_RUN("symbol_table_scope_depth_bounds", test_symbol_table_scope_depth_bounds);
    TEST_RUN("symbol_table_scope_depth_null", test_symbol_table_scope_depth_null);
    TEST_RUN("symbol_table_scope_depth_deep", test_symbol_table_scope_depth_deep);
    TEST_RUN("symbol_declaration_scope_depth_basic", test_symbol_declaration_scope_depth_basic);
    TEST_RUN("symbol_declaration_scope_depth_lookup_persistence", test_symbol_declaration_scope_depth_lookup_persistence);
    TEST_RUN("symbol_declaration_scope_depth_comparison", test_symbol_declaration_scope_depth_comparison);
    TEST_RUN("symbol_declaration_scope_depth_function_scope", test_symbol_declaration_scope_depth_function_scope);
    TEST_RUN("symbol_declaration_scope_depth_deep_nesting", test_symbol_declaration_scope_depth_deep_nesting);
}
