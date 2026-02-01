// tests/unit/runtime/runtime_array_tests.c
// Tests for runtime array operations - main entry point

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* Include split modules */
#include "runtime_array_tests_basic.c"
#include "runtime_array_tests_ops.c"
#include "runtime_array_tests_query.c"

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_array_main(void)
{
    TEST_SECTION("Runtime Array");

    /* Clear */
    TEST_RUN("rt_array_clear", test_rt_array_clear);

    /* Push */
    TEST_RUN("rt_array_push_long", test_rt_array_push_long);
    TEST_RUN("rt_array_push_double", test_rt_array_push_double);
    TEST_RUN("rt_array_push_char", test_rt_array_push_char);
    TEST_RUN("rt_array_push_string", test_rt_array_push_string);
    TEST_RUN("rt_array_push_byte", test_rt_array_push_byte);

    /* Pop */
    TEST_RUN("rt_array_pop_long", test_rt_array_pop_long);
    TEST_RUN("rt_array_pop_string", test_rt_array_pop_string);

    /* Concat */
    TEST_RUN("rt_array_concat_long", test_rt_array_concat_long);
    TEST_RUN("rt_array_concat_string", test_rt_array_concat_string);

    /* Slice */
    TEST_RUN("rt_array_slice_long", test_rt_array_slice_long);
    TEST_RUN("rt_array_slice_string", test_rt_array_slice_string);

    /* Reverse */
    TEST_RUN("rt_array_rev_long", test_rt_array_rev_long);
    TEST_RUN("rt_array_rev_string", test_rt_array_rev_string);

    /* Remove */
    TEST_RUN("rt_array_rem_long", test_rt_array_rem_long);

    /* Insert */
    TEST_RUN("rt_array_ins_long", test_rt_array_ins_long);

    /* IndexOf */
    TEST_RUN("rt_array_indexOf_long", test_rt_array_indexOf_long);
    TEST_RUN("rt_array_indexOf_string", test_rt_array_indexOf_string);

    /* Contains */
    TEST_RUN("rt_array_contains_long", test_rt_array_contains_long);
    TEST_RUN("rt_array_contains_string", test_rt_array_contains_string);

    /* Clone */
    TEST_RUN("rt_array_clone_long", test_rt_array_clone_long);
    TEST_RUN("rt_array_clone_string", test_rt_array_clone_string);

    /* Join */
    TEST_RUN("rt_array_join_long", test_rt_array_join_long);
    TEST_RUN("rt_array_join_string", test_rt_array_join_string);

    /* Equality */
    TEST_RUN("rt_array_eq_long", test_rt_array_eq_long);
    TEST_RUN("rt_array_eq_string", test_rt_array_eq_string);

    /* Range */
    TEST_RUN("rt_array_range", test_rt_array_range);

    /* Create */
    TEST_RUN("rt_array_create_long", test_rt_array_create_long);
    TEST_RUN("rt_array_create_string", test_rt_array_create_string);

    /* Push Copy */
    TEST_RUN("rt_array_push_copy_long", test_rt_array_push_copy_long);
}
