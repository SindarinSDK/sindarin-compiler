// tests/unit/type_checker/type_checker_tests_native_slice.c
// Tests for pointer slicing and array slicing in native functions
// Note: setup_test_token helper is defined in type_checker_tests_native.c

#include "../test_harness.h"

/* Include split modules */
#include "type_checker_tests_native_slice_basic.c"
#include "type_checker_tests_native_slice_as_val.c"
#include "type_checker_tests_native_slice_advanced.c"

/* ============================================================================
 * Main entry point for slice tests
 * ============================================================================ */

void test_type_checker_native_slice_main(void)
{
    TEST_SECTION("Native Slice");

    TEST_RUN("pointer_slice_byte_to_byte_array", test_pointer_slice_byte_to_byte_array);
    TEST_RUN("pointer_slice_int_to_int_array", test_pointer_slice_int_to_int_array);
    TEST_RUN("slice_non_array_non_pointer_fails", test_slice_non_array_non_pointer_fails);
    TEST_RUN("array_slice_still_works", test_array_slice_still_works);
    TEST_RUN("as_val_context_tracking", test_as_val_context_tracking);
    TEST_RUN("pointer_slice_with_as_val_in_regular_fn", test_pointer_slice_with_as_val_in_regular_fn);
    TEST_RUN("pointer_slice_without_as_val_in_regular_fn_fails", test_pointer_slice_without_as_val_in_regular_fn_fails);
    TEST_RUN("as_val_on_array_type_is_noop", test_as_val_on_array_type_is_noop);
    TEST_RUN("get_buffer_slice_as_val_type_inference", test_get_buffer_slice_as_val_type_inference);
    TEST_RUN("slice_invalid_type_error", test_slice_invalid_type_error);
    TEST_RUN("int_pointer_slice_as_val_type_inference", test_int_pointer_slice_as_val_type_inference);
    TEST_RUN("pointer_slice_with_step_fails", test_pointer_slice_with_step_fails);
}
