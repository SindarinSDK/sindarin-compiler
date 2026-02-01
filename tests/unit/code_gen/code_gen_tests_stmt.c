// tests/code_gen_tests_stmt.c
// Statement code generation tests - main entry point
// Split into modular files for maintainability

#include "code_gen_tests_stmt_func.c"
#include "code_gen_tests_stmt_control.c"
#include "code_gen_tests_stmt_misc.c"

void test_code_gen_stmt_main(void)
{
    test_code_gen_stmt_func_main();
    test_code_gen_stmt_control_main();
    test_code_gen_stmt_misc_main();
}
