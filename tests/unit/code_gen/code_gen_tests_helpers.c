// tests/code_gen_tests_helpers.c
// Tests for code generation helper/utility functions - main entry point
// Split into modular files for maintainability

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"

#include "code_gen_tests_helpers_types.c"
#include "code_gen_tests_helpers_values.c"
#include "code_gen_tests_helpers_funcs.c"

void test_code_gen_helpers_main(void)
{
    test_code_gen_helpers_types_main();
    test_code_gen_helpers_values_main();
    test_code_gen_helpers_funcs_main();
}
