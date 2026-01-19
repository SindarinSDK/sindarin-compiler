// tests/type_checker_tests.c
// Type checker tests - main entry point

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../arena.h"
#include "../debug.h"
#include "../ast.h"
#include "../ast/ast_expr.h"
#include "../token.h"
#include "../type_checker.h"
#include "../type_checker/type_checker_util.h"
#include "../symbol_table.h"
#include "../symbol_table/symbol_table_thread.h"
#include "../test_harness.h"

void setup_token(Token *tok, SnTokenType type, const char *lexeme, int line, const char *filename, Arena *arena) {
    tok->type = type;
    tok->line = line;
    size_t lex_len = strlen(lexeme);
    char *allocated_lexeme = (char *)arena_alloc(arena, lex_len + 1);
    memcpy(allocated_lexeme, lexeme, lex_len + 1);
    tok->start = allocated_lexeme;
    tok->length = (int)lex_len;
    tok->filename = filename;
}

static void setup_literal_token(Token *token, SnTokenType type, const char *lexeme_str, int line, const char *filename, Arena *arena)
{
    setup_token(token, type, lexeme_str, line, filename, arena);
}

#include "type_checker_tests_array.c"
#include "type_checker_tests_member.c"
#include "type_checker_tests_func.c"
#include "type_checker_tests_memory.c"
#include "type_checker_tests_promotion.c"
#include "type_checker_tests_errors.c"
#include "type_checker_tests_thread.c"
#include "type_checker_tests_namespace.c"
#include "type_checker_tests_native_context.c"
#include "type_checker_tests_native.c"
#include "type_checker_tests_native_types.c"
#include "type_checker_tests_native_callback.c"
#include "type_checker_tests_native_slice.c"
#include "type_checker_tests_native_pointer.c"
#include "type_checker_tests_struct.c"

void test_type_checker_main()
{
    test_type_checker_array_main();
    test_type_checker_member_main();
    test_type_checker_func_main();
    test_type_checker_memory_main();
    test_type_checker_promotion_main();
    test_type_checker_errors_main();
    test_type_checker_thread_main();
    test_type_checker_namespace_main();
    test_type_checker_native_context_main();
    test_type_checker_native_main();
    test_type_checker_native_types_main();
    test_type_checker_native_callback_main();
    test_type_checker_native_slice_main();
    test_type_checker_native_pointer_main();
    test_type_checker_struct_main();
}
