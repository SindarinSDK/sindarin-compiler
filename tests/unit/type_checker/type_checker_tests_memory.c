/**
 * type_checker_tests_memory.c - Type checker memory management tests entry point
 *
 * Includes all memory-related type checker test modules.
 */

/* Include split modules directly (unity build style for smaller translation units) */
#include "type_checker_tests_memory_qualifiers_var.c"
#include "type_checker_tests_memory_qualifiers_param.c"
#include "type_checker_tests_memory_context.c"
#include "type_checker_tests_memory_escape_assign.c"
#include "type_checker_tests_memory_escape_return.c"
#include "type_checker_tests_memory_escape_control.c"

void test_type_checker_memory_main()
{
    TEST_SECTION("Type Checker Memory");

    test_type_checker_memory_qualifiers_var_main();
    test_type_checker_memory_qualifiers_param_main();
    test_type_checker_memory_context_main();
    test_type_checker_memory_escape_assign_main();
    test_type_checker_memory_escape_return_main();
    test_type_checker_memory_escape_control_main();
}
