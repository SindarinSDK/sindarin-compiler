// tests/lexer_tests_array.c
// Array-related lexer tests

#include "../test_harness.h"

static void test_lexer_array_empty(void)
{
    DEBUG_INFO("Starting test_lexer_array_empty");

    const char *source = "{}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);
    assert(t1.length == 1);
    assert(strcmp(t1.start, "{") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_RIGHT_BRACE);
    assert(t2.length == 1);
    assert(strcmp(t2.start, "}") == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_empty");
}

static void test_lexer_array_single_element(void)
{
    DEBUG_INFO("Starting test_lexer_array_single_element");

    const char *source = "{1}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.length == 1);
    assert(strcmp(t2.start, "1") == 0);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_RIGHT_BRACE);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_single_element");
}

static void test_lexer_array_multi_element(void)
{
    DEBUG_INFO("Starting test_lexer_array_multi_element");

    const char *source = "{1, 2, 3}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_COMMA);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_INT_LITERAL);
    assert(t4.literal.int_value == 2);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_COMMA);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_INT_LITERAL);
    assert(t6.literal.int_value == 3);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_RIGHT_BRACE);

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_multi_element");
}

static void test_lexer_inline_array_expression(void)
{
    DEBUG_INFO("Starting test_lexer_inline_array_expression");

    const char *source = "arr.concat({1, 2})";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // concat

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_LEFT_PAREN);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACE);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_INT_LITERAL);
    assert(t6.literal.int_value == 1);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_COMMA);

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT_LITERAL);
    assert(t8.literal.int_value == 2);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_RIGHT_BRACE);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_RIGHT_PAREN);

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_inline_array_expression");
}

static void test_lexer_array_assignment(void)
{
    DEBUG_INFO("Starting test_lexer_array_assignment");

    const char *source = "var arr: int[] = {1, 2}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // var (identifier)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAR);

    // arr (identifier)
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);

    // : (colon)
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_COLON);

    // int (identifier/keyword)
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_INT);

    // [ (left bracket)
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACKET); // Assuming [] for type, but lexer needs [ ] too? Wait, add if missing.

    // ] (right bracket)
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACKET);

    // = (equal)
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EQUAL);

    // {1, 2}
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_LEFT_BRACE);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_INT_LITERAL);
    assert(t9.literal.int_value == 1);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_COMMA);

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_INT_LITERAL);
    assert(t11.literal.int_value == 2);

    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_RIGHT_BRACE);

    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_assignment");
}

static void test_lexer_array_method_calls(void)
{
    DEBUG_INFO("Starting test_lexer_array_method_calls");

    const char *source = "arr.push(1); arr.length; arr.pop()";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // arr.push(1);
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // push

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_LEFT_PAREN);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_INT_LITERAL);
    assert(t5.literal.int_value == 1);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_PAREN);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_SEMICOLON);

    // arr.length
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_IDENTIFIER); // arr

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_DOT);

    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_IDENTIFIER); // length

    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_SEMICOLON);

    // arr.pop()
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_IDENTIFIER); // arr

    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_DOT);

    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_IDENTIFIER); // pop

    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_LEFT_PAREN);

    Token t16 = lexer_scan_token(&lexer);
    assert(t16.type == TOKEN_RIGHT_PAREN);

    Token t17 = lexer_scan_token(&lexer);
    assert(t17.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_method_calls");
}

static void test_lexer_unmatched_brace_error(void)
{
    DEBUG_INFO("Starting test_lexer_unmatched_brace_error");

    const char *source = "{1";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_BRACE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT_LITERAL);
    assert(t2.literal.int_value == 1);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF); // Changed: EOF is correct; parser handles mismatch

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unmatched_brace_error");
}

static void test_lexer_array_with_indentation(void)
{
    DEBUG_INFO("Starting test_lexer_array_with_indentation");

    const char *source = "  var arr = {\n    1,\n    2\n  }";
    Arena arena;
    arena_init(&arena, 1024 * 2); // Larger for multi-line
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // t1: INDENT (2 spaces for block)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INDENT);

    // Line 1: var arr = {
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_VAR);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // arr

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EQUAL);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACE);

    // t6: NEWLINE (end line 1)
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_NEWLINE);

    // t7: INDENT (4 spaces for array elements block)
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_INDENT);

    // Line 2: 1,
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT_LITERAL);
    assert(t8.literal.int_value == 1);

    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_COMMA);

    // t10: NEWLINE (end line 2)
    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_NEWLINE);

    // Line 3: 2 (same indent level 4, no extra INDENT)
    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_INT_LITERAL); // Changed: Directly the number "2"
    assert(t11.literal.int_value == 2);

    // t12: NEWLINE (end line 3)
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_NEWLINE);

    // t13: DEDENT (back to 2 spaces for line 4)
    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_DEDENT);

    // No second DEDENT yet (line 4 indent=2 == previous 2)

    // Line 4: }
    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_RIGHT_BRACE);

    // t15: EOF (end, with possible final DEDENT if needed, but since base, direct EOF)
    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_with_indentation");
}

static void test_lexer_array_at_line_start(void)
{
    DEBUG_INFO("Starting test_lexer_array_at_line_start");

    const char *source = "\n{1, 2}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    // Newline (empty line, no indent)
    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NEWLINE);

    // {1, 2} at line start, no indent
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACE);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_INT_LITERAL);
    assert(t3.literal.int_value == 1);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_COMMA);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_INT_LITERAL);
    assert(t5.literal.int_value == 2);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACE);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_at_line_start");
}

// Range operator tests for slicing

static void test_lexer_range_operator(void)
{
    DEBUG_INFO("Starting test_lexer_range_operator");

    const char *source = "..";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_RANGE);
    assert(t1.length == 2);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_range_operator");
}

static void test_lexer_array_slice_full(void)
{
    DEBUG_INFO("Starting test_lexer_array_slice_full");

    const char *source = "arr[1..3]";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACKET);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_INT_LITERAL);
    assert(t3.literal.int_value == 1);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RANGE);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_INT_LITERAL);
    assert(t5.literal.int_value == 3);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACKET);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_slice_full");
}

static void test_lexer_array_slice_from_start(void)
{
    DEBUG_INFO("Starting test_lexer_array_slice_from_start");

    const char *source = "arr[..3]";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACKET);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_RANGE);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_INT_LITERAL);
    assert(t4.literal.int_value == 3);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_RIGHT_BRACKET);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_slice_from_start");
}

static void test_lexer_array_slice_to_end(void)
{
    DEBUG_INFO("Starting test_lexer_array_slice_to_end");

    const char *source = "arr[2..]";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACKET);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_INT_LITERAL);
    assert(t3.literal.int_value == 2);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RANGE);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_RIGHT_BRACKET);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_slice_to_end");
}

static void test_lexer_array_slice_full_copy(void)
{
    DEBUG_INFO("Starting test_lexer_array_slice_full_copy");

    const char *source = "arr[..]";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // arr

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_LEFT_BRACKET);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_RANGE);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RIGHT_BRACKET);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_array_slice_full_copy");
}

static void test_lexer_dot_vs_range(void)
{
    DEBUG_INFO("Starting test_lexer_dot_vs_range");

    const char *source = "a.b..c";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_IDENTIFIER); // a

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER); // b

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RANGE);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_IDENTIFIER); // c

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_dot_vs_range");
}

void test_lexer_array_main(void)
{
    TEST_SECTION("Lexer Array Tests");
    TEST_RUN("lexer_array_empty", test_lexer_array_empty);
    TEST_RUN("lexer_array_single_element", test_lexer_array_single_element);
    TEST_RUN("lexer_array_multi_element", test_lexer_array_multi_element);
    TEST_RUN("lexer_inline_array_expression", test_lexer_inline_array_expression);
    TEST_RUN("lexer_array_assignment", test_lexer_array_assignment);
    TEST_RUN("lexer_array_method_calls", test_lexer_array_method_calls);
    TEST_RUN("lexer_unmatched_brace_error", test_lexer_unmatched_brace_error);
    TEST_RUN("lexer_array_with_indentation", test_lexer_array_with_indentation);
    TEST_RUN("lexer_array_at_line_start", test_lexer_array_at_line_start);
    // Range operator tests for slicing
    TEST_RUN("lexer_range_operator", test_lexer_range_operator);
    TEST_RUN("lexer_array_slice_full", test_lexer_array_slice_full);
    TEST_RUN("lexer_array_slice_from_start", test_lexer_array_slice_from_start);
    TEST_RUN("lexer_array_slice_to_end", test_lexer_array_slice_to_end);
    TEST_RUN("lexer_array_slice_full_copy", test_lexer_array_slice_full_copy);
    TEST_RUN("lexer_dot_vs_range", test_lexer_dot_vs_range);
}
