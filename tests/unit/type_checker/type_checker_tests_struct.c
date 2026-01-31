// tests/type_checker_tests_struct.c
// Main entry point for struct type checker tests
// This file includes all the split struct test files

void test_type_checker_struct_main(void)
{
    test_type_checker_struct_basic_main();
    test_type_checker_struct_circular_main();
    test_type_checker_struct_native_main();
    test_type_checker_struct_layout_main();
    test_type_checker_struct_packed_main();
    test_type_checker_struct_symbol_main();
    test_type_checker_struct_literal_main();
    test_type_checker_struct_defaults_main();
    test_type_checker_struct_required_main();
    test_type_checker_struct_nested_main();
    test_type_checker_struct_member_access_main();
    test_type_checker_struct_field_assign_main();
    test_type_checker_struct_utility_main();
    test_type_checker_struct_misc_main();
}
