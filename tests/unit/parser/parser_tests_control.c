// tests/parser_tests_control.c
// Control flow parser tests (while, for, interpolated_string, literals, recursive)

/* Include split modules */
#include "parser_tests_control_loops.c"
#include "parser_tests_control_escape.c"
#include "parser_tests_control_nested.c"

void test_parser_control_main()
{
    TEST_SECTION("Parser Control Flow Tests");

    TEST_RUN("while_loop_parsing", test_while_loop_parsing);
    TEST_RUN("for_loop_parsing", test_for_loop_parsing);
    TEST_RUN("interpolated_string_parsing", test_interpolated_string_parsing);
    TEST_RUN("interpolated_string_with_postfix_decrement_parsing", test_interpolated_string_with_postfix_decrement_parsing);
    TEST_RUN("interpolated_string_with_postfix_increment_parsing", test_interpolated_string_with_postfix_increment_parsing);
    TEST_RUN("interpolated_string_with_unary_negate_parsing", test_interpolated_string_with_unary_negate_parsing);
    TEST_RUN("literal_types_parsing", test_literal_types_parsing);
    TEST_RUN("recursive_function_parsing", test_recursive_function_parsing);
    TEST_RUN("interpolated_string_with_string_literal_in_braces", test_interpolated_string_with_string_literal_in_braces);
    TEST_RUN("interpolated_string_with_func_call_string_arg", test_interpolated_string_with_func_call_string_arg);
    TEST_RUN("interpolated_string_with_newline_escape_in_braces", test_interpolated_string_with_newline_escape_in_braces);
    TEST_RUN("interpolated_string_with_tab_escape_in_braces", test_interpolated_string_with_tab_escape_in_braces);
    TEST_RUN("interpolated_string_with_backslash_escape_in_braces", test_interpolated_string_with_backslash_escape_in_braces);
    TEST_RUN("interpolated_string_with_carriage_return_escape_in_braces", test_interpolated_string_with_carriage_return_escape_in_braces);
    TEST_RUN("interpolated_string_with_empty_string_in_braces", test_interpolated_string_with_empty_string_in_braces);
    TEST_RUN("interpolated_string_with_nested_parens", test_interpolated_string_with_nested_parens);
    TEST_RUN("interpolated_string_with_multiple_string_literals", test_interpolated_string_with_multiple_string_literals);
    TEST_RUN("interpolated_string_with_string_method_on_literal", test_interpolated_string_with_string_method_on_literal);
    TEST_RUN("interpolated_string_with_complex_escape_sequence", test_interpolated_string_with_complex_escape_sequence);
    TEST_RUN("interpolated_string_with_braces_escape", test_interpolated_string_with_braces_escape);
    TEST_RUN("nested_interpolated_string_basic", test_nested_interpolated_string_basic);
    TEST_RUN("nested_interpolated_string_with_expression", test_nested_interpolated_string_with_expression);
    TEST_RUN("nested_interpolated_string_double_nested", test_nested_interpolated_string_double_nested);
    TEST_RUN("nested_interpolated_string_with_func_call", test_nested_interpolated_string_with_func_call);
    TEST_RUN("nested_interpolated_string_adjacent", test_nested_interpolated_string_adjacent);
    TEST_RUN("interpolated_string_with_format_specifier", test_interpolated_string_with_format_specifier);
    TEST_RUN("interpolated_string_with_format_specifier_float", test_interpolated_string_with_format_specifier_float);
    TEST_RUN("interpolated_string_with_format_specifier_and_expr", test_interpolated_string_with_format_specifier_and_expr);
    TEST_RUN("format_specifier_not_detected_in_nested", test_format_specifier_not_detected_in_nested);
}
