// tests/type_checker_tests_thread.c
// Tests for thread spawn and sync type checking
// Main entry point that calls sub-test files

#include "type_checker_tests_thread_spawn.c"
#include "type_checker_tests_thread_sync.c"
#include "type_checker_tests_thread_access.c"

void test_type_checker_thread_main(void)
{
    test_type_checker_thread_spawn_main();
    test_type_checker_thread_sync_main();
    test_type_checker_thread_access_main();
}
