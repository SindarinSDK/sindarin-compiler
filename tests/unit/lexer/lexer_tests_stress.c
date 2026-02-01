// tests/unit/lexer/lexer_tests_stress.c
// Lexer stress tests - main entry point
// Split into modular files for maintainability

#include "../test_harness.h"

#include "lexer_tests_stress_input.c"
#include "lexer_tests_stress_literals.c"
#include "lexer_tests_stress_ops.c"

void test_lexer_stress_main(void)
{
    test_lexer_stress_input_main();
    test_lexer_stress_literals_main();
    test_lexer_stress_ops_main();
}
