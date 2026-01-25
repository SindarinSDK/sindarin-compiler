#include <stdio.h>
#include "test_framework.h"

/* Shared test counters */
int tests_passed = 0;
int tests_failed = 0;
double tests_total_ms = 0.0;

/* Test module runners */
extern void test_alloc_run(void);
extern void test_pin_run(void);
extern void test_reassignment_run(void);
extern void test_gc_run(void);
extern void test_concurrency_run(void);
extern void test_hierarchy_run(void);
extern void test_api_run(void);
extern void test_stress_run(void);

int main(void)
{
    printf("\n" TEST_COLOR_BOLD "Managed Arena" TEST_COLOR_RESET "\n");
    printf("------------------------------------------------------------\n");

    test_alloc_run();
    test_pin_run();
    test_reassignment_run();
    test_gc_run();
    test_concurrency_run();
    test_hierarchy_run();
    test_api_run();
    test_stress_run();

    printf("\n------------------------------------------------------------\n");
    if (tests_total_ms >= 1000.0)
        printf("Results: " TEST_COLOR_GREEN "%d passed" TEST_COLOR_RESET ", "
               TEST_COLOR_RED "%d failed" TEST_COLOR_RESET "  (total: %.2fs)\n",
               tests_passed, tests_failed, tests_total_ms / 1000.0);
    else
        printf("Results: " TEST_COLOR_GREEN "%d passed" TEST_COLOR_RESET ", "
               TEST_COLOR_RED "%d failed" TEST_COLOR_RESET "  (total: %.2fms)\n",
               tests_passed, tests_failed, tests_total_ms);

    return tests_failed > 0 ? 1 : 0;
}
