// tests/type_checker_tests_edge_cases.c
// Edge case tests for type checker

/* Include split modules */
#include "type_checker_tests_edge_equality.c"
#include "type_checker_tests_edge_coercion.c"
#include "type_checker_tests_edge_literal.c"
#include "type_checker_tests_edge_binary.c"
#include "type_checker_tests_edge_unary.c"
#include "type_checker_tests_edge_size.c"

// =====================================================
// Return Type Checking
// =====================================================

static void test_return_type_match(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret = ast_create_return_stmt(&arena, ret_tok, lit, &ret_tok);

    Stmt *body[1] = {ret};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "get_value", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, int_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

static void test_return_void(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret = ast_create_return_stmt(&arena, ret_tok, NULL, &ret_tok);

    Stmt *body[1] = {ret};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "do_nothing", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 1, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

// =====================================================
// Assignment Type Checking
// =====================================================

static void test_assignment_same_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // var x: int = 1
    Token lit1_tok;
    setup_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, lit1, NULL);

    // x = 2
    Token lit2_tok;
    setup_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, int_type, false, &lit2_tok);
    Token eq_tok;
    setup_token(&eq_tok, TOKEN_EQUAL, "=", 2, "test.sn", &arena);
    Expr *assign = ast_create_assign_expr(&arena, var_tok, lit2, &eq_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, &eq_tok);

    Stmt *body[2] = {decl, assign_stmt};
    Token fn_tok;
    setup_token(&fn_tok, TOKEN_IDENTIFIER, "test_fn", 1, "test.sn", &arena);
    Stmt *fn = ast_create_function_stmt(&arena, fn_tok, NULL, 0, void_type, body, 2, &fn_tok);

    ast_module_add_statement(&arena, &module, fn);

    int ok = type_check_module(&module, &table);
    assert(ok == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_edge_cases_main(void)
{
    TEST_SECTION("Type Checker Edge Cases");

    // Type equality
    TEST_RUN("type_equality_same_primitives", test_type_equality_same_primitives);
    TEST_RUN("type_equality_different_primitives", test_type_equality_different_primitives);
    TEST_RUN("type_equality_arrays_same_element", test_type_equality_arrays_same_element);
    TEST_RUN("type_equality_arrays_different_element", test_type_equality_arrays_different_element);
    TEST_RUN("type_equality_nested_arrays", test_type_equality_nested_arrays);
    TEST_RUN("type_equality_function_types", test_type_equality_function_types);
    TEST_RUN("type_equality_function_different_params", test_type_equality_function_different_params);
    TEST_RUN("type_equality_with_null", test_type_equality_with_null);

    // Type coercion
    TEST_RUN("coercion_int_to_double", test_coercion_int_to_double);
    TEST_RUN("coercion_double_to_int_fails", test_coercion_double_to_int_fails);
    TEST_RUN("coercion_byte_to_int", test_coercion_byte_to_int);
    TEST_RUN("coercion_char_to_int", test_coercion_char_to_int);
    TEST_RUN("coercion_same_type", test_coercion_same_type);
    TEST_RUN("coercion_string_to_int_fails", test_coercion_string_to_int_fails);
    TEST_RUN("coercion_bool_to_int_fails", test_coercion_bool_to_int_fails);

    // Literal expressions
    TEST_RUN("literal_int_type", test_literal_int_type);
    TEST_RUN("literal_bool_type", test_literal_bool_type);
    TEST_RUN("literal_string_type", test_literal_string_type);
    TEST_RUN("literal_char_type", test_literal_char_type);

    // Binary expressions
    TEST_RUN("binary_logical_and", test_binary_logical_and);
    TEST_RUN("binary_logical_or", test_binary_logical_or);
    TEST_RUN("binary_comparison_lt", test_binary_comparison_lt);
    TEST_RUN("binary_modulo", test_binary_modulo);

    // Unary expressions
    TEST_RUN("unary_not_bool", test_unary_not_bool);
    TEST_RUN("unary_negate_int", test_unary_negate_int);
    TEST_RUN("unary_negate_double", test_unary_negate_double);

    // Variable declarations
    TEST_RUN("var_decl_no_initializer", test_var_decl_no_initializer);
    TEST_RUN("var_decl_mismatch_type_error", test_var_decl_mismatch_type_error);

    // Type sizes
    TEST_RUN("type_size_int", test_type_size_int);
    TEST_RUN("type_size_bool", test_type_size_bool);
    TEST_RUN("type_size_char", test_type_size_char);
    TEST_RUN("type_size_double", test_type_size_double);
    TEST_RUN("type_size_string", test_type_size_string);
    TEST_RUN("type_size_array", test_type_size_array);
    TEST_RUN("type_size_pointer", test_type_size_pointer);
    TEST_RUN("type_size_void", test_type_size_void);
    TEST_RUN("type_size_byte", test_type_size_byte);
    TEST_RUN("type_size_long", test_type_size_long);

    // Return statements
    TEST_RUN("return_type_match", test_return_type_match);
    TEST_RUN("return_void", test_return_void);

    // Assignments
    TEST_RUN("assignment_same_type", test_assignment_same_type);
}
