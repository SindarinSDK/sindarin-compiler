/**
 * type_checker_tests_native_pointer.c - Native pointer tests entry point
 *
 * Tests for native function pointer variable handling, as val, and as ref.
 * Note: setup_test_token helper is defined in type_checker_tests_native.c
 */

/* Suppress warning for operator name arrays used for readability but not in assertions */
#pragma clang diagnostic ignored "-Wunused-variable"

#include "../test_harness.h"

/* Include split modules directly (unity build style for smaller translation units) */
#include "type_checker_tests_native_pointer_var.c"
#include "type_checker_tests_native_pointer_compare.c"
#include "type_checker_tests_native_pointer_inline.c"
#include "type_checker_tests_native_pointer_asval_misc.c"
#include "type_checker_tests_native_pointer_return.c"
#include "type_checker_tests_native_pointer_asref.c"
#include "type_checker_tests_native_pointer_struct.c"

void test_type_checker_native_pointer_main(void)
{
    TEST_SECTION("Native Pointer");

    test_type_checker_native_pointer_var_main();
    test_type_checker_native_pointer_compare_main();
    test_type_checker_native_pointer_inline_main();
    test_type_checker_native_pointer_asval_misc_main();
    test_type_checker_native_pointer_return_main();
    test_type_checker_native_pointer_asref_main();
    test_type_checker_native_pointer_struct_main();
}
