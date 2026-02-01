// tests/parser_tests_struct.c
// Struct declaration parser tests

/* Include split modules */
#include "parser_tests_struct_decl.c"
#include "parser_tests_struct_defaults.c"
#include "parser_tests_struct_nested.c"
#include "parser_tests_struct_literal.c"

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_parser_struct_main()
{
    TEST_SECTION("Parser Struct Declaration Tests");
    TEST_RUN("struct_decl_empty_parsing", test_struct_decl_empty_parsing);
    TEST_RUN("native_struct_decl_empty_parsing", test_native_struct_decl_empty_parsing);
    TEST_RUN("struct_decl_name_only", test_struct_decl_name_only);
    TEST_RUN("multiple_struct_decls", test_multiple_struct_decls);
    TEST_RUN("struct_with_two_fields", test_struct_with_two_fields);
    TEST_RUN("struct_with_three_fields", test_struct_with_three_fields);
    TEST_RUN("struct_with_various_types", test_struct_with_various_types);
    TEST_RUN("native_struct_with_fields", test_native_struct_with_fields);
    TEST_RUN("struct_field_with_default_int", test_struct_field_with_default_int);
    TEST_RUN("struct_field_with_default_bool", test_struct_field_with_default_bool);
    TEST_RUN("struct_field_with_default_string", test_struct_field_with_default_string);
    TEST_RUN("struct_mixed_defaults_and_no_defaults", test_struct_mixed_defaults_and_no_defaults);
    TEST_RUN("struct_field_with_default_double", test_struct_field_with_default_double);
    TEST_RUN("struct_with_nested_struct_field", test_struct_with_nested_struct_field);
    TEST_RUN("struct_with_mixed_primitive_and_struct_fields", test_struct_with_mixed_primitive_and_struct_fields);

    /* Error handling tests */
    TEST_RUN("struct_error_duplicate_field_names", test_struct_error_duplicate_field_names);
    TEST_RUN("struct_error_pointer_in_non_native", test_struct_error_pointer_in_non_native);
    TEST_RUN("native_struct_allows_pointer_fields", test_native_struct_allows_pointer_fields);
    TEST_RUN("struct_error_missing_arrow", test_struct_error_missing_arrow);
    TEST_RUN("struct_error_missing_field_type", test_struct_error_missing_field_type);

    /* Struct literal tests */
    TEST_SECTION("Parser Struct Literal Tests");
    TEST_RUN("struct_literal_empty", test_struct_literal_empty);
    TEST_RUN("struct_literal_with_fields", test_struct_literal_with_fields);
    TEST_RUN("struct_literal_partial_init", test_struct_literal_partial_init);

    /* Field access tests */
    TEST_SECTION("Parser Field Access Tests");
    TEST_RUN("field_access_simple", test_field_access_simple);
    TEST_RUN("field_access_nested", test_field_access_nested);
    TEST_RUN("field_assignment", test_field_assignment);
    TEST_RUN("field_assignment_nested", test_field_assignment_nested);

    /* Error handling tests for struct literals */
    TEST_RUN("struct_literal_missing_colon", test_struct_literal_missing_colon);
    TEST_RUN("struct_literal_invalid_field_name", test_struct_literal_invalid_field_name);
}
