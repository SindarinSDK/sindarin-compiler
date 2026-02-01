// tests/unit/parser/parser_tests_basic.c
// Main entry point for basic parser tests - includes split test modules

// Include the split test modules
#include "parser_tests_basic_core.c"
#include "parser_tests_basic_types.c"
#include "parser_tests_basic_native.c"
#include "parser_tests_basic_slice.c"
#include "parser_tests_basic_callback.c"
#include "parser_tests_basic_misc.c"

static void test_parser_basic_main()
{
    // Call all the split test main functions
    test_parser_basic_core_main();
    test_parser_basic_types_main();
    test_parser_basic_native_main();
    test_parser_basic_slice_main();
    test_parser_basic_callback_main();
    test_parser_basic_misc_main();
}
