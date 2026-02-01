// tests/type_checker_tests_array.c
// Array declaration, literal, access, and assignment type checker tests

/* Include split modules */
#include "type_checker_tests_array_decl.c"
#include "type_checker_tests_array_literal.c"
#include "type_checker_tests_array_access.c"
#include "type_checker_tests_array_assign.c"
#include "type_checker_tests_array_slice.c"
#include "type_checker_tests_array_sized.c"

void test_type_checker_array_main()
{
    TEST_SECTION("Type Checker Arrays");

    TEST_RUN("array_decl_no_init", test_type_check_array_decl_no_init);
    TEST_RUN("array_decl_with_init_matching", test_type_check_array_decl_with_init_matching);
    TEST_RUN("array_decl_with_init_mismatch", test_type_check_array_decl_with_init_mismatch);
    TEST_RUN("array_literal_empty", test_type_check_array_literal_empty);
    TEST_RUN("array_literal_heterogeneous", test_type_check_array_literal_heterogeneous);
    TEST_RUN("array_access_valid", test_type_check_array_access_valid);
    TEST_RUN("array_access_non_array", test_type_check_array_access_non_array);
    TEST_RUN("array_access_invalid_index", test_type_check_array_access_invalid_index);
    TEST_RUN("array_assignment_matching", test_type_check_array_assignment_matching);
    TEST_RUN("array_assignment_mismatch", test_type_check_array_assignment_mismatch);
    TEST_RUN("nested_array", test_type_check_nested_array);
    // Slice tests
    TEST_RUN("array_slice_full", test_type_check_array_slice_full);
    TEST_RUN("array_slice_from_start", test_type_check_array_slice_from_start);
    TEST_RUN("array_slice_to_end", test_type_check_array_slice_to_end);
    TEST_RUN("array_slice_non_array", test_type_check_array_slice_non_array);
    // Sized array allocation tests
    TEST_RUN("sized_array_alloc_basic", test_type_check_sized_array_alloc_basic);
    TEST_RUN("sized_array_alloc_with_default", test_type_check_sized_array_alloc_with_default);
    TEST_RUN("sized_array_alloc_mismatch_default", test_type_check_sized_array_alloc_mismatch_default);
    TEST_RUN("sized_array_alloc_runtime_size", test_type_check_sized_array_alloc_runtime_size);
    TEST_RUN("sized_array_alloc_invalid_size", test_type_check_sized_array_alloc_invalid_size);
    TEST_RUN("sized_array_alloc_long_size", test_type_check_sized_array_alloc_long_size);
}
