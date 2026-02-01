/**
 * code_gen_tests_optimization.c - Code generation optimization tests entry point
 *
 * Includes all optimization test modules.
 */

/* Include split modules directly (unity build style for smaller translation units) */
#include "code_gen_tests_optimization_helpers.c"
#include "code_gen_tests_optimization_fold_overflow.c"
#include "code_gen_tests_optimization_fold_edge.c"
#include "code_gen_tests_optimization_native.c"
#include "code_gen_tests_optimization_arena.c"
#include "code_gen_tests_optimization_tailcall.c"
#include "code_gen_tests_optimization_loop.c"
#include "code_gen_tests_optimization_nonneg.c"

void test_code_gen_optimization_main(void)
{
    TEST_SECTION("Code Gen Optimization Tests");

    test_code_gen_optimization_fold_overflow_main();
    test_code_gen_optimization_fold_edge_main();
    test_code_gen_optimization_native_main();
    test_code_gen_optimization_arena_main();
    test_code_gen_optimization_tailcall_main();
    test_code_gen_optimization_loop_main();
    test_code_gen_optimization_nonneg_main();
}
