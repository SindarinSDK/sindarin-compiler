// tests/lexer_tests.c
// Lexer tests - main entry point

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../lexer.h"
#include "../token.h"

#include "lexer_tests_array.c"
#include "lexer_tests_literal.c"
#include "lexer_tests_operator.c"
#include "lexer_tests_indent.c"
#include "lexer_tests_memory.c"

void test_lexer_main(void)
{
    test_lexer_array_main();
    test_lexer_literal_main();
    test_lexer_operator_main();
    test_lexer_indent_main();
    test_lexer_memory_main();
}
