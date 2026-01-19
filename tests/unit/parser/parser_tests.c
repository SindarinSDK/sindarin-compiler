// tests/parser_tests.c
// Parser tests - main entry point

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../arena.h"
#include "../lexer.h"
#include "../parser.h"
#include "../ast.h"
#include "../debug.h"
#include "../symbol_table.h"
#include "../test_harness.h"

static void setup_parser(Arena *arena, Lexer *lexer, Parser *parser, SymbolTable *symbol_table, const char *source)
{
    arena_init(arena, 4096);
    lexer_init(arena, lexer, source, "test.sn");
    symbol_table_init(arena, symbol_table);
    parser_init(arena, parser, lexer, symbol_table);
}

static void cleanup_parser(Arena *arena, Lexer *lexer, Parser *parser, SymbolTable *symbol_table)
{
    parser_cleanup(parser);
    lexer_cleanup(lexer);
    symbol_table_cleanup(symbol_table);
    arena_free(arena);
}

#include "parser_tests_basic.c"
#include "parser_tests_control.c"
#include "parser_tests_program.c"
#include "parser_tests_array.c"
#include "parser_tests_memory.c"
#include "parser_tests_lambda.c"
#include "parser_tests_static.c"
#include "parser_tests_namespace.c"
#include "parser_tests_struct.c"

void test_parser_main()
{
    test_parser_basic_main();
    test_parser_control_main();
    test_parser_program_main();
    test_parser_array_main();
    test_parser_memory_main();
    test_parser_lambda_main();
    test_parser_static_main();
    test_parser_namespace_main();
    test_parser_struct_main();
}
