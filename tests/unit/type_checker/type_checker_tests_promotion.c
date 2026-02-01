// tests/type_checker_tests_promotion.c
// Type checker tests for numeric type promotion (int -> double)

/* Include split modules */
#include "type_checker_tests_promotion_arithmetic.c"
#include "type_checker_tests_promotion_comparison.c"
#include "type_checker_tests_promotion_interop.c"
#include "type_checker_tests_promotion_mixed.c"

void test_type_checker_promotion_main()
{
    TEST_SECTION("Type Checker Promotion");

    TEST_RUN("int_double_addition", test_type_check_int_double_addition);
    TEST_RUN("int_double_subtraction", test_type_check_int_double_subtraction);
    TEST_RUN("int_int_no_promotion", test_type_check_int_int_no_promotion);
    TEST_RUN("int_double_comparison", test_type_check_int_double_comparison);
    TEST_RUN("double_int_equality", test_type_check_double_int_equality);
    TEST_RUN("int_double_greater", test_type_check_int_double_greater);
    TEST_RUN("int32_var_decl", test_type_check_int32_var_decl);
    TEST_RUN("uint_var_decl", test_type_check_uint_var_decl);
    TEST_RUN("uint32_var_decl", test_type_check_uint32_var_decl);
    TEST_RUN("float_var_decl", test_type_check_float_var_decl);
    TEST_RUN("int32_addition", test_type_check_int32_addition);
    TEST_RUN("float_double_promotion", test_type_check_float_double_promotion);
    TEST_RUN("interop_type_mismatch", test_type_check_interop_type_mismatch);
    TEST_RUN("pointer_nil_assignment", test_type_check_pointer_nil_assignment);
    TEST_RUN("double_pointer", test_type_check_double_pointer);
    TEST_RUN("pointer_type_equality", test_type_check_pointer_type_equality);
}
