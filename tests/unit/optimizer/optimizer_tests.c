#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../test_harness.h"
#include "../optimizer.h"
#include "../arena.h"
#include "../ast.h"
#include "../debug.h"

/* Note: setup_basic_token is defined in code_gen_tests_util.c */

/* Include split modules */
#include "optimizer_tests_helpers.c"
#include "optimizer_tests_terminator.c"
#include "optimizer_tests_passes.c"
#include "optimizer_tests_tail_call.c"
#include "optimizer_tests_string.c"
#include "optimizer_tests_edge_cases.c"
#include "optimizer_tests_stress.c"

/* ============================================================================
 * Run all tests
 * ============================================================================ */

void run_optimizer_tests(void)
{
    run_optimizer_edge_cases_tests();
    test_optimizer_stress_main();
    TEST_SECTION("Optimizer Tests");

    /* Terminator detection tests */
    TEST_RUN("stmt_is_terminator_return", test_stmt_is_terminator_return);
    TEST_RUN("stmt_is_terminator_break_continue", test_stmt_is_terminator_break_continue);
    TEST_RUN("stmt_is_terminator_non_terminator", test_stmt_is_terminator_non_terminator);

    /* No-op detection tests */
    TEST_RUN("expr_is_noop_add_zero", test_expr_is_noop_add_zero);
    TEST_RUN("expr_is_noop_sub_zero", test_expr_is_noop_sub_zero);
    TEST_RUN("expr_is_noop_mul_one", test_expr_is_noop_mul_one);
    TEST_RUN("expr_is_noop_div_one", test_expr_is_noop_div_one);
    TEST_RUN("expr_is_noop_double_negation", test_expr_is_noop_double_negation);
    TEST_RUN("expr_is_noop_not_noop", test_expr_is_noop_not_noop);

    /* Unreachable code removal tests */
    TEST_RUN("remove_unreachable_after_return", test_remove_unreachable_after_return);
    TEST_RUN("remove_unreachable_after_break", test_remove_unreachable_after_break);
    TEST_RUN("no_unreachable_statements", test_no_unreachable_statements);

    /* Variable usage tracking tests */
    TEST_RUN("collect_used_variables", test_collect_used_variables);
    TEST_RUN("is_variable_used", test_is_variable_used);

    /* Full optimization pass tests */
    TEST_RUN("optimizer_dead_code_elimination_function", test_optimizer_dead_code_elimination_function);
    TEST_RUN("optimizer_noop_simplification", test_optimizer_noop_simplification);

    /* Tail call optimization tests */
    TEST_RUN("tail_call_detection_simple", test_tail_call_detection_simple);
    TEST_RUN("tail_call_detection_not_tail", test_tail_call_detection_not_tail);
    TEST_RUN("function_has_tail_recursion", test_function_has_tail_recursion);
    TEST_RUN("tail_call_marking", test_tail_call_marking);

    /* String literal merging tests */
    TEST_RUN("string_literal_merge_adjacent", test_string_literal_merge_adjacent);
    TEST_RUN("string_literal_merge_with_variable", test_string_literal_merge_with_variable);
    TEST_RUN("string_literal_concat_fold", test_string_literal_concat_fold);
    TEST_RUN("string_no_merge_different_types", test_string_no_merge_different_types);
}
