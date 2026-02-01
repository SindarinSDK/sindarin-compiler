// ast_tests_edge_cases.c
// Additional edge case tests for AST creation functions

/* Include split modules */
#include "ast_tests_edge_types.c"
#include "ast_tests_edge_literal.c"
#include "ast_tests_edge_binary.c"
#include "ast_tests_edge_other.c"

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_ast_edge_cases_main(void)
{
    TEST_SECTION("AST - Primitive Types");
    TEST_RUN("primitive_type_int", test_ast_primitive_type_int);
    TEST_RUN("primitive_type_long", test_ast_primitive_type_long);
    TEST_RUN("primitive_type_double", test_ast_primitive_type_double);
    TEST_RUN("primitive_type_bool", test_ast_primitive_type_bool);
    TEST_RUN("primitive_type_char", test_ast_primitive_type_char);
    TEST_RUN("primitive_type_byte", test_ast_primitive_type_byte);
    TEST_RUN("primitive_type_string", test_ast_primitive_type_string);
    TEST_RUN("primitive_type_void", test_ast_primitive_type_void);

    TEST_SECTION("AST - Composite Types");
    TEST_RUN("array_type_int", test_ast_array_type_int);
    TEST_RUN("array_type_string", test_ast_array_type_string);
    TEST_RUN("pointer_type_int", test_ast_pointer_type_int);
    TEST_RUN("pointer_type_char", test_ast_pointer_type_char);
    TEST_RUN("opaque_type", test_ast_opaque_type);
    TEST_RUN("nested_array_type", test_ast_nested_array_type);
    TEST_RUN("pointer_to_pointer", test_ast_pointer_to_pointer);

    TEST_SECTION("AST - Literal Expressions");
    TEST_RUN("literal_int_zero", test_ast_literal_int_zero);
    TEST_RUN("literal_int_negative", test_ast_literal_int_negative);
    TEST_RUN("literal_double_zero", test_ast_literal_double_zero);
    TEST_RUN("literal_double_pi", test_ast_literal_double_pi);
    TEST_RUN("literal_bool_true", test_ast_literal_bool_true);
    TEST_RUN("literal_bool_false", test_ast_literal_bool_false);
    TEST_RUN("literal_string_empty", test_ast_literal_string_empty);
    TEST_RUN("literal_string_hello", test_ast_literal_string_hello);
    TEST_RUN("literal_char_a", test_ast_literal_char_a);
    TEST_RUN("literal_char_newline", test_ast_literal_char_newline);
    TEST_RUN("literal_byte_zero", test_ast_literal_byte_zero);
    TEST_RUN("literal_byte_max", test_ast_literal_byte_max);

    TEST_SECTION("AST - Binary Expressions");
    TEST_RUN("binary_add", test_ast_binary_add);
    TEST_RUN("binary_sub", test_ast_binary_sub);
    TEST_RUN("binary_mul", test_ast_binary_mul);
    TEST_RUN("binary_div", test_ast_binary_div);
    TEST_RUN("binary_mod", test_ast_binary_mod);
    TEST_RUN("binary_eq", test_ast_binary_eq);
    TEST_RUN("binary_neq", test_ast_binary_neq);
    TEST_RUN("binary_less", test_ast_binary_less);
    TEST_RUN("binary_less_eq", test_ast_binary_less_eq);
    TEST_RUN("binary_greater", test_ast_binary_greater);
    TEST_RUN("binary_greater_eq", test_ast_binary_greater_eq);

    TEST_SECTION("AST - Unary Expressions");
    TEST_RUN("unary_negate", test_ast_unary_negate);
    TEST_RUN("unary_not", test_ast_unary_not);

    TEST_SECTION("AST - Variable Expressions");
    TEST_RUN("variable_simple", test_ast_variable_simple);
    TEST_RUN("variable_underscore", test_ast_variable_underscore);
    TEST_RUN("variable_long_name", test_ast_variable_long_name);

    TEST_SECTION("AST - Statements");
    TEST_RUN("expr_stmt", test_ast_expr_stmt);
    TEST_RUN("return_stmt_with_value", test_ast_return_stmt_with_value);
    TEST_RUN("return_stmt_void", test_ast_return_stmt_void);
    TEST_RUN("block_stmt_empty", test_ast_block_stmt_empty);
    TEST_RUN("block_stmt_with_stmts", test_ast_block_stmt_with_stmts);
    TEST_RUN("var_decl_no_init", test_ast_var_decl_no_init);
    TEST_RUN("var_decl_with_init", test_ast_var_decl_with_init);

    TEST_SECTION("AST - Module");
    TEST_RUN("module_init", test_ast_module_init);
    TEST_RUN("module_add_single_stmt", test_ast_module_add_single_stmt);
    TEST_RUN("module_add_multiple_stmts", test_ast_module_add_multiple_stmts);
}
