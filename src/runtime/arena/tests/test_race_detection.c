#include "test_race_detection.h"

/* ============================================================================
 * Race Detection Stress Tests - Main Runner
 * ============================================================================
 * These tests are designed to expose race conditions by:
 * - Using higher thread counts than normal tests
 * - Running operations for longer durations
 * - Mixing operations that stress synchronization boundaries
 * - Using barriers to maximize timing sensitivity
 *
 * Test modules:
 * - test_race_detection_scaling.c       - Thread scaling tests
 * - test_race_detection_mixed_lifecycle.c - Mixed ops and lifecycle tests
 * - test_race_detection_compaction.c    - Compaction and promotion tests
 * - test_race_detection_table_stability.c - Table growth and stability tests
 * - test_race_detection_pins.c          - Pin-related tests
 * - test_race_detection_hierarchy.c     - Deep hierarchy stress tests
 * - test_race_detection_destroy_reset.c - Destroy/promote and reset tests
 * - test_race_detection_recycling_clone.c - Recycling and clone tests
 * ============================================================================ */

void test_race_detection_run(void)
{
    /* Thread scaling tests */
    test_race_scaling_run();

    /* Mixed operations and lifecycle tests */
    test_race_mixed_lifecycle_run();

    /* Compaction and promotion tests */
    test_race_compaction_run();

    /* Table growth and stability tests */
    test_race_table_stability_run();

    /* Pin-related tests */
    test_race_pins_run();

    /* Hierarchy tests */
    test_race_hierarchy_run();

    /* Destroy and reset tests */
    test_race_destroy_reset_run();

    /* Recycling and clone tests */
    test_race_recycling_clone_run();
}
