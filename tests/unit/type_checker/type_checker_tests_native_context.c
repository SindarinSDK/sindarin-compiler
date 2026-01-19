// tests/unit/type_checker/type_checker_tests_native_context.c
// Tests for native function context tracking

#include "../test_harness.h"
#include "../type_checker/type_checker_util.h"
#include "../type_checker.h"
#include <assert.h>
#include <stdio.h>

/* Test that native_context_is_active returns false by default */
static void test_native_context_default_inactive(void)
{
    /* Ensure we're starting fresh - exit any leftover context */
    while (native_context_is_active())
    {
        native_context_exit();
    }
    assert(native_context_is_active() == false);
}

/* Test that native_context_enter activates the context */
static void test_native_context_enter(void)
{
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }
    assert(native_context_is_active() == false);

    native_context_enter();
    assert(native_context_is_active() == true);

    /* Cleanup */
    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test that native_context_exit deactivates the context */
static void test_native_context_exit(void)
{
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    native_context_enter();
    assert(native_context_is_active() == true);

    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test nested native contexts (native function calling another native function) */
static void test_native_context_nesting(void)
{
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    /* Enter outer native function */
    native_context_enter();
    assert(native_context_is_active() == true);

    /* Enter inner native function (nested) */
    native_context_enter();
    assert(native_context_is_active() == true);

    /* Exit inner native function */
    native_context_exit();
    assert(native_context_is_active() == true);  /* Still in outer */

    /* Exit outer native function */
    native_context_exit();
    assert(native_context_is_active() == false);  /* Now inactive */
}

/* Test that excessive exits don't go negative */
static void test_native_context_excessive_exit(void)
{
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    /* Try to exit when not active - should be safe */
    native_context_exit();
    native_context_exit();
    native_context_exit();
    assert(native_context_is_active() == false);

    /* Should still work after excessive exits */
    native_context_enter();
    assert(native_context_is_active() == true);
    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test multiple enter/exit cycles */
static void test_native_context_multiple_cycles(void)
{
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    for (int i = 0; i < 5; i++)
    {
        assert(native_context_is_active() == false);
        native_context_enter();
        assert(native_context_is_active() == true);
        native_context_exit();
        assert(native_context_is_active() == false);
    }
}

/* ============================================================================
 * Main entry point for native context tests
 * ============================================================================ */

void test_type_checker_native_context_main(void)
{
    TEST_SECTION("Native Context");

    TEST_RUN("native_context_default_inactive", test_native_context_default_inactive);
    TEST_RUN("native_context_enter", test_native_context_enter);
    TEST_RUN("native_context_exit", test_native_context_exit);
    TEST_RUN("native_context_nesting", test_native_context_nesting);
    TEST_RUN("native_context_excessive_exit", test_native_context_excessive_exit);
    TEST_RUN("native_context_multiple_cycles", test_native_context_multiple_cycles);
}
