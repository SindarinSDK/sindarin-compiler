#include "standalone/arena_tests.c"
#include "ast/ast_tests.c"
#include "code_gen/code_gen_tests.c"
#include "lexer/lexer_tests.c"
#include "optimizer/optimizer_tests.c"
#include "parser/parser_tests.c"
#include "runtime/runtime_arena_tests.c"
#include "runtime/runtime_arithmetic_tests.c"
#include "runtime/runtime_string_tests.c"
#include "runtime/runtime_array_tests.c"
#include "runtime/runtime_thread_tests.c"
#include "standalone/symbol_table_tests_core.c"
#include "standalone/symbol_table_tests_thread.c"
#include "standalone/symbol_table_tests_namespace.c"
#include "standalone/token_tests.c"
#include "type_checker/type_checker_tests.c"

int main()
{

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_ERROR);
    init_debug(DEBUG_LEVEL_ERROR);

    // *** Arena ***

    test_arena_main();

    // *** Runtime Arena ***

    test_rt_arena_main();

    // *** Runtime Arithmetic ***

    test_rt_arithmetic_main();

    // *** Runtime String ***

    test_rt_string_main();

    // *** Runtime Array ***

    test_rt_array_main();

    // *** Runtime Thread ***

    test_rt_thread_main();

    // *** AST ***
    
    test_ast_main();

    // *** Code Gen ***

    test_code_gen_main();

    // *** Lexer ***

    test_lexer_main();

    // *** Parser ***

    test_parser_main();

    // *** Symbol Table ***

    test_symbol_table_core_main();
    test_symbol_table_thread_main();
    test_symbol_table_namespace_main();
    
    // *** Token ***

    test_token_main();

    // *** Type Checker ***

    test_type_checker_main();

    // *** Optimizer ***

    run_optimizer_tests();

    // *** Complete ***

    printf("All tests passed!\n");

    return 0;
}