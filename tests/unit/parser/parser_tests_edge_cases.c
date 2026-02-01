// tests/parser_tests_edge_cases.c
// Edge case tests for the parser

/* Include split modules */
#include "parser_tests_edge_precedence.c"
#include "parser_tests_edge_stmts.c"
#include "parser_tests_edge_funcs.c"
#include "parser_tests_edge_types.c"

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_parser_edge_cases_main(void)
{
    TEST_SECTION("Parser - Expression Precedence");
    TEST_RUN("precedence_mul_over_add", test_parse_precedence_mul_over_add);
    TEST_RUN("precedence_parens_override", test_parse_precedence_parens_override);
    TEST_RUN("precedence_comparison_lower", test_parse_precedence_comparison_lower);
    TEST_RUN("precedence_and_lower_than_comparison", test_parse_precedence_and_lower_than_comparison);
    TEST_RUN("precedence_or_lower_than_and", test_parse_precedence_or_lower_than_and);
    TEST_RUN("unary_precedence", test_parse_unary_precedence);
    TEST_RUN("double_negation", test_parse_double_negation);
    TEST_RUN("not_and_comparison", test_parse_not_and_comparison);

    TEST_SECTION("Parser - Literals");
    TEST_RUN("int_literal_zero", test_parse_int_literal_zero);
    TEST_RUN("int_literal_negative", test_parse_int_literal_negative);
    TEST_RUN("double_literal", test_parse_double_literal);
    TEST_RUN("bool_true", test_parse_bool_true);
    TEST_RUN("bool_false", test_parse_bool_false);
    TEST_RUN("char_literal", test_parse_char_literal);
    TEST_RUN("string_empty", test_parse_string_empty);
    TEST_RUN("string_with_escapes", test_parse_string_with_escapes);

    TEST_SECTION("Parser - Statements");
    TEST_RUN("multiple_var_decls", test_parse_multiple_var_decls);
    TEST_RUN("nested_if", test_parse_nested_if);
    TEST_RUN("if_elif_else", test_parse_if_elif_else);
    TEST_RUN("while_loop", test_parse_while_loop);
    TEST_RUN("for_range", test_parse_for_range);
    TEST_RUN("return_value", test_parse_return_value);
    TEST_RUN("return_void", test_parse_return_void);
    TEST_RUN("break_statement", test_parse_break_statement);
    TEST_RUN("continue_statement", test_parse_continue_statement);

    TEST_SECTION("Parser - Function Declarations");
    TEST_RUN("function_one_param", test_parse_function_one_param);
    TEST_RUN("function_multiple_params", test_parse_function_multiple_params);
    TEST_RUN("function_void_return", test_parse_function_void_return);
    TEST_RUN("function_string_return", test_parse_function_string_return);
    TEST_RUN("function_bool_return", test_parse_function_bool_return);
    TEST_RUN("function_multiple_statements", test_parse_function_multiple_statements);

    TEST_SECTION("Parser - Call Expressions");
    TEST_RUN("call_no_args", test_parse_call_no_args);
    TEST_RUN("call_one_arg", test_parse_call_one_arg);
    TEST_RUN("call_multiple_args", test_parse_call_multiple_args);
    TEST_RUN("call_expression_args", test_parse_call_expression_args);
    TEST_RUN("nested_calls", test_parse_nested_calls);

    TEST_SECTION("Parser - Types");
    TEST_RUN("array_type", test_parse_array_type);
    TEST_RUN("pointer_type", test_parse_pointer_type);
    TEST_RUN("nullable_type", test_parse_nullable_type);
    TEST_RUN("long_type", test_parse_long_type);
    TEST_RUN("byte_type", test_parse_byte_type);

    TEST_SECTION("Parser - Complex Expressions");
    TEST_RUN("chained_method_calls", test_parse_chained_method_calls);
    TEST_RUN("array_indexing", test_parse_array_indexing);
    TEST_RUN("compound_assignment", test_parse_compound_assignment);
}
