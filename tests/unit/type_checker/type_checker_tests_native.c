// tests/type_checker_tests_native.c
// Tests for native function context tracking and pointer variable restrictions
// This file contains the shared helper function used by all native test files.

#include "../test_harness.h"
#include "../type_checker/type_checker_util.h"
#include "../type_checker.h"
#include "../ast.h"
#include "../ast/ast_expr.h"
#include "../ast/ast_stmt.h"
#include "../symbol_table.h"
#include "../arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Helper to set up a token for testing */
static void setup_test_token(Token *tok, SnTokenType type, const char *lexeme, int line, const char *filename, Arena *arena)
{
    tok->type = type;
    tok->line = line;
    size_t lex_len = strlen(lexeme);
    char *allocated_lexeme = (char *)arena_alloc(arena, lex_len + 1);
    memcpy(allocated_lexeme, lexeme, lex_len + 1);
    tok->start = allocated_lexeme;
    tok->length = (int)lex_len;
    tok->filename = filename;
}

void test_type_checker_native_main(void)
{
    // All tests have been moved to separate files:
    // - type_checker_tests_native_types.c (opaque and interop types)
    // - type_checker_tests_native_callback.c (callbacks and lambdas)
    // - type_checker_tests_native_slice.c (pointer slicing)
    // - type_checker_tests_native_pointer.c (pointer variables)
    // This file now only provides the shared setup_test_token helper.
}
