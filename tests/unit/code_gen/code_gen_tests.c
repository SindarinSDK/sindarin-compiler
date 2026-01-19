// tests/code_gen_tests.c
// Code generation tests - main entry point
// This file includes all code gen test modules

#include "../test_harness.h"

#include "code_gen_tests_util.c"
#include "code_gen_tests_expr.c"
#include "code_gen_tests_stmt.c"
#include "code_gen_tests_array.c"
#include "code_gen_tests_memory.c"
#include "code_gen_tests_constfold.c"
#include "code_gen_tests_optimization.c"

void test_code_gen_main()
{
    test_code_gen_util_main();
    test_code_gen_expr_main();
    test_code_gen_stmt_main();
    test_code_gen_array_main();
    test_code_gen_memory_main();
    test_code_gen_constfold_main();
    test_code_gen_optimization_main();
}
