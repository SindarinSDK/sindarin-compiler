#include <stdio.h>

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
    printf("=== Managed Arena Tests ===\n\n");

    test_alloc_run();
    test_pin_run();
    test_reassignment_run();
    test_gc_run();
    test_concurrency_run();
    test_hierarchy_run();
    test_api_run();
    test_stress_run();

    printf("\n=== Results: %d passed, %d failed (%.2fms total) ===\n",
           tests_passed, tests_failed, tests_total_ms);
    return tests_failed > 0 ? 1 : 0;
}
