#include "standalone/arena_tests.c"
#include "standalone/arena_tests_edge_cases.c"
#include "standalone/compiler_driver_tests.c"
#include "standalone/diagnostic_tests.c"
#include "ast/ast_tests.c"
#include "backend/gcc_backend_tests.c"
#include "lexer/lexer_tests.c"
#include "optimizer/optimizer_tests.c"
#include "parser/parser_tests.c"
#include "standalone/symbol_table_tests_core.c"
#include "standalone/symbol_table_tests_thread.c"
#include "standalone/symbol_table_tests_namespace.c"
#include "standalone/symbol_table_tests_edge_cases.c"
#include "standalone/symbol_table_tests_stress.c"
#include "standalone/token_tests.c"
#include "standalone/token_tests_extended.c"
#include "type_checker/type_checker_tests.c"
#include "package/package_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_ERROR);
    init_debug(DEBUG_LEVEL_ERROR);

    TEST_INIT();

    // *** Arena ***

    test_arena_main();
    test_arena_edge_cases_main();

    // *** Compiler Driver ***

    test_compiler_driver_main();

    // *** Diagnostic System ***

    test_diagnostic_main();


    // *** AST ***
    
    test_ast_main();

    // *** GCC Backend ***

    test_gcc_backend_main();


    // *** Lexer ***

    test_lexer_main();

    // *** Parser ***

    test_parser_main();

    // *** Symbol Table ***

    test_symbol_table_core_main();
    test_symbol_table_thread_main();
    test_symbol_table_namespace_main();
    test_symbol_table_edge_cases_main();
    test_symbol_table_stress_main();
    
    // *** Token ***

    test_token_main();
    test_token_extended_main();

    // *** Type Checker ***

    test_type_checker_main();

    // *** Optimizer ***

    run_optimizer_tests();

    // *** Package Manager ***

    test_package_main();

    // *** Complete ***

    TEST_SUMMARY();

    if (TEST_GET_FAILED() > 0) {
        return 1;
    }
    printf("All tests passed!\n");

    return 0;
}