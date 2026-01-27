// tests/unit/standalone/token_tests_extended.c
// Extended token tests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../token.h"
#include "../test_harness.h"

/* ============================================================================
 * Token Type to String Tests
 * ============================================================================ */

static void test_token_type_string_literals(void)
{
    const char *name = token_type_to_string(TOKEN_INT_LITERAL);
    assert(name != NULL);
    assert(strlen(name) > 0);
}

static void test_token_type_string_keywords(void)
{
    assert(token_type_to_string(TOKEN_IF) != NULL);
    assert(token_type_to_string(TOKEN_ELSE) != NULL);
    assert(token_type_to_string(TOKEN_FOR) != NULL);
    assert(token_type_to_string(TOKEN_WHILE) != NULL);
    assert(token_type_to_string(TOKEN_RETURN) != NULL);
}

static void test_token_type_string_operators(void)
{
    assert(token_type_to_string(TOKEN_PLUS) != NULL);
    assert(token_type_to_string(TOKEN_MINUS) != NULL);
    assert(token_type_to_string(TOKEN_STAR) != NULL);
    assert(token_type_to_string(TOKEN_SLASH) != NULL);
}

static void test_token_type_string_comparison(void)
{
    assert(token_type_to_string(TOKEN_EQUAL_EQUAL) != NULL);
    assert(token_type_to_string(TOKEN_BANG_EQUAL) != NULL);
    assert(token_type_to_string(TOKEN_LESS) != NULL);
    assert(token_type_to_string(TOKEN_GREATER) != NULL);
    assert(token_type_to_string(TOKEN_LESS_EQUAL) != NULL);
    assert(token_type_to_string(TOKEN_GREATER_EQUAL) != NULL);
}

static void test_token_type_string_punctuation(void)
{
    assert(token_type_to_string(TOKEN_LEFT_PAREN) != NULL);
    assert(token_type_to_string(TOKEN_RIGHT_PAREN) != NULL);
    assert(token_type_to_string(TOKEN_LEFT_BRACE) != NULL);
    assert(token_type_to_string(TOKEN_RIGHT_BRACE) != NULL);
    assert(token_type_to_string(TOKEN_LEFT_BRACKET) != NULL);
    assert(token_type_to_string(TOKEN_RIGHT_BRACKET) != NULL);
}

static void test_token_type_string_special(void)
{
    assert(token_type_to_string(TOKEN_EOF) != NULL);
    assert(token_type_to_string(TOKEN_ERROR) != NULL);
    assert(token_type_to_string(TOKEN_IDENTIFIER) != NULL);
}

static void test_token_type_string_types(void)
{
    assert(token_type_to_string(TOKEN_INT) != NULL);
    assert(token_type_to_string(TOKEN_LONG) != NULL);
    assert(token_type_to_string(TOKEN_DOUBLE) != NULL);
    assert(token_type_to_string(TOKEN_STR) != NULL);
    assert(token_type_to_string(TOKEN_BOOL) != NULL);
    assert(token_type_to_string(TOKEN_VOID) != NULL);
}

/* ============================================================================
 * Token Initialization Tests
 * ============================================================================ */

static void test_token_init_basic(void)
{
    Token tok;
    token_init(&tok, TOKEN_INT_LITERAL, "42", 2, 1, "test.sn");
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.start != NULL);
    assert(tok.length == 2);
    assert(tok.line == 1);
}

static void test_token_init_identifier(void)
{
    Token tok;
    token_init(&tok, TOKEN_IDENTIFIER, "myVar", 5, 10, "test.sn");
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 5);
    assert(tok.line == 10);
}

static void test_token_init_string_literal(void)
{
    Token tok;
    token_init(&tok, TOKEN_STRING_LITERAL, "\"hello\"", 7, 1, "test.sn");
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(tok.length == 7);
}

static void test_token_init_operator(void)
{
    Token tok;
    token_init(&tok, TOKEN_PLUS, "+", 1, 5, "test.sn");
    assert(tok.type == TOKEN_PLUS);
    assert(tok.length == 1);
}

static void test_token_init_long_lexeme(void)
{
    Token tok;
    const char *long_name = "veryLongVariableNameForTesting";
    token_init(&tok, TOKEN_IDENTIFIER, long_name, (int)strlen(long_name), 1, "test.sn");
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 30);
}

/* ============================================================================
 * Token Literal Value Tests
 * ============================================================================ */

static void test_token_int_literal_value(void)
{
    Token tok;
    token_init(&tok, TOKEN_INT_LITERAL, "42", 2, 1, "test.sn");
    token_set_int_literal(&tok, 42);
    assert(tok.literal.int_value == 42);
}

static void test_token_int_literal_zero(void)
{
    Token tok;
    token_init(&tok, TOKEN_INT_LITERAL, "0", 1, 1, "test.sn");
    token_set_int_literal(&tok, 0);
    assert(tok.literal.int_value == 0);
}

static void test_token_int_literal_negative(void)
{
    Token tok;
    token_init(&tok, TOKEN_INT_LITERAL, "-100", 4, 1, "test.sn");
    token_set_int_literal(&tok, -100);
    assert(tok.literal.int_value == -100);
}

static void test_token_int_literal_max(void)
{
    Token tok;
    token_init(&tok, TOKEN_INT_LITERAL, "2147483647", 10, 1, "test.sn");
    token_set_int_literal(&tok, 2147483647);
    assert(tok.literal.int_value == 2147483647);
}

static void test_token_int_literal_large(void)
{
    Token tok;
    token_init(&tok, TOKEN_LONG_LITERAL, "9999999999", 10, 1, "test.sn");
    token_set_int_literal(&tok, 9999999999LL);
    assert(tok.literal.int_value == 9999999999LL);
}

static void test_token_double_literal_value(void)
{
    Token tok;
    token_init(&tok, TOKEN_DOUBLE_LITERAL, "3.14", 4, 1, "test.sn");
    token_set_double_literal(&tok, 3.14159);
    assert(tok.literal.double_value > 3.14 && tok.literal.double_value < 3.15);
}

static void test_token_double_literal_zero(void)
{
    Token tok;
    token_init(&tok, TOKEN_DOUBLE_LITERAL, "0.0", 3, 1, "test.sn");
    token_set_double_literal(&tok, 0.0);
    assert(tok.literal.double_value == 0.0);
}

static void test_token_double_literal_negative(void)
{
    Token tok;
    token_init(&tok, TOKEN_DOUBLE_LITERAL, "-1.5", 4, 1, "test.sn");
    token_set_double_literal(&tok, -1.5);
    assert(tok.literal.double_value == -1.5);
}

static void test_token_double_literal_small(void)
{
    Token tok;
    token_init(&tok, TOKEN_DOUBLE_LITERAL, "0.001", 5, 1, "test.sn");
    token_set_double_literal(&tok, 0.001);
    assert(tok.literal.double_value > 0.0009 && tok.literal.double_value < 0.0011);
}

static void test_token_double_literal_large(void)
{
    Token tok;
    token_init(&tok, TOKEN_DOUBLE_LITERAL, "1e10", 4, 1, "test.sn");
    token_set_double_literal(&tok, 1e10);
    assert(tok.literal.double_value > 9e9 && tok.literal.double_value < 1.1e10);
}

static void test_token_string_literal_value(void)
{
    Token tok;
    token_init(&tok, TOKEN_STRING_LITERAL, "\"hello\"", 7, 1, "test.sn");
    token_set_string_literal(&tok, "hello");
    assert(strcmp(tok.literal.string_value, "hello") == 0);
}

static void test_token_string_literal_empty(void)
{
    Token tok;
    token_init(&tok, TOKEN_STRING_LITERAL, "\"\"", 2, 1, "test.sn");
    token_set_string_literal(&tok, "");
    assert(strcmp(tok.literal.string_value, "") == 0);
}

static void test_token_string_literal_with_space(void)
{
    Token tok;
    token_init(&tok, TOKEN_STRING_LITERAL, "\"hello world\"", 13, 1, "test.sn");
    token_set_string_literal(&tok, "hello world");
    assert(strcmp(tok.literal.string_value, "hello world") == 0);
}

static void test_token_char_literal_value(void)
{
    Token tok;
    token_init(&tok, TOKEN_CHAR_LITERAL, "'A'", 3, 1, "test.sn");
    token_set_char_literal(&tok, 'A');
    assert(tok.literal.char_value == 'A');
}

static void test_token_char_literal_digit(void)
{
    Token tok;
    token_init(&tok, TOKEN_CHAR_LITERAL, "'9'", 3, 1, "test.sn");
    token_set_char_literal(&tok, '9');
    assert(tok.literal.char_value == '9');
}

static void test_token_char_literal_newline(void)
{
    Token tok;
    token_init(&tok, TOKEN_CHAR_LITERAL, "'\\n'", 4, 1, "test.sn");
    token_set_char_literal(&tok, '\n');
    assert(tok.literal.char_value == '\n');
}

static void test_token_char_literal_tab(void)
{
    Token tok;
    token_init(&tok, TOKEN_CHAR_LITERAL, "'\\t'", 4, 1, "test.sn");
    token_set_char_literal(&tok, '\t');
    assert(tok.literal.char_value == '\t');
}

static void test_token_bool_literal_true(void)
{
    Token tok;
    token_init(&tok, TOKEN_BOOL_LITERAL, "true", 4, 1, "test.sn");
    token_set_bool_literal(&tok, 1);
    assert(tok.literal.bool_value == 1);
}

static void test_token_bool_literal_false(void)
{
    Token tok;
    token_init(&tok, TOKEN_BOOL_LITERAL, "false", 5, 1, "test.sn");
    token_set_bool_literal(&tok, 0);
    assert(tok.literal.bool_value == 0);
}

/* ============================================================================
 * Token Type Tests
 * ============================================================================ */

static void test_token_type_arithmetic_ops(void)
{
    assert(TOKEN_PLUS != TOKEN_MINUS);
    assert(TOKEN_STAR != TOKEN_SLASH);
    assert(TOKEN_MODULO != TOKEN_PLUS);
}

static void test_token_type_comparison_ops(void)
{
    assert(TOKEN_EQUAL_EQUAL != TOKEN_BANG_EQUAL);
    assert(TOKEN_LESS != TOKEN_GREATER);
    assert(TOKEN_LESS_EQUAL != TOKEN_GREATER_EQUAL);
}

static void test_token_type_logical_ops(void)
{
    assert(TOKEN_AND != TOKEN_OR);
    assert(TOKEN_BANG != TOKEN_AND);
}

static void test_token_type_assignment_ops(void)
{
    assert(TOKEN_EQUAL != TOKEN_EQUAL_EQUAL);
    assert(TOKEN_PLUS_EQUAL != TOKEN_MINUS_EQUAL);
    assert(TOKEN_STAR_EQUAL != TOKEN_SLASH_EQUAL);
}

static void test_token_type_bitwise_ops(void)
{
    assert(TOKEN_AMPERSAND != TOKEN_PIPE);
    assert(TOKEN_CARET != TOKEN_TILDE);
    assert(TOKEN_LSHIFT != TOKEN_RSHIFT);
}

static void test_token_type_brackets(void)
{
    assert(TOKEN_LEFT_PAREN != TOKEN_RIGHT_PAREN);
    assert(TOKEN_LEFT_BRACE != TOKEN_RIGHT_BRACE);
    assert(TOKEN_LEFT_BRACKET != TOKEN_RIGHT_BRACKET);
}

static void test_token_type_type_keywords(void)
{
    assert(TOKEN_INT != TOKEN_LONG);
    assert(TOKEN_DOUBLE != TOKEN_FLOAT);
    assert(TOKEN_STR != TOKEN_CHAR);
    assert(TOKEN_BOOL != TOKEN_BYTE);
    assert(TOKEN_VOID != TOKEN_ANY);
}

static void test_token_type_control_flow(void)
{
    assert(TOKEN_IF != TOKEN_ELSE);
    assert(TOKEN_FOR != TOKEN_WHILE);
    assert(TOKEN_BREAK != TOKEN_CONTINUE);
    assert(TOKEN_RETURN != TOKEN_MATCH);
}

static void test_token_type_declarations(void)
{
    assert(TOKEN_FN != TOKEN_VAR);
    assert(TOKEN_STRUCT != TOKEN_KEYWORD_TYPE);
    assert(TOKEN_NATIVE != TOKEN_STATIC);
}

static void test_token_type_memory_keywords(void)
{
    assert(TOKEN_SHARED != TOKEN_PRIVATE);
    assert(TOKEN_AS != TOKEN_VAL);
    assert(TOKEN_VAL != TOKEN_REF);
}

/* ============================================================================
 * Token Location Tests
 * ============================================================================ */

static void test_token_location_line_one(void)
{
    Token tok;
    token_init(&tok, TOKEN_IDENTIFIER, "x", 1, 1, "test.sn");
    assert(tok.line == 1);
}

static void test_token_location_line_large(void)
{
    Token tok;
    token_init(&tok, TOKEN_IDENTIFIER, "x", 1, 1000, "test.sn");
    assert(tok.line == 1000);
}

static void test_token_location_filename(void)
{
    Token tok;
    token_init(&tok, TOKEN_IDENTIFIER, "x", 1, 1, "myfile.sn");
    assert(strcmp(tok.filename, "myfile.sn") == 0);
}

static void test_token_location_different_files(void)
{
    Token tok1, tok2;
    token_init(&tok1, TOKEN_IDENTIFIER, "x", 1, 1, "file1.sn");
    token_init(&tok2, TOKEN_IDENTIFIER, "y", 1, 1, "file2.sn");
    assert(strcmp(tok1.filename, tok2.filename) != 0);
}

/* ============================================================================
 * Token Copy Tests
 * ============================================================================ */

static void test_token_copy_basic(void)
{
    Token tok1;
    token_init(&tok1, TOKEN_IDENTIFIER, "foo", 3, 1, "test.sn");

    Token tok2 = tok1;
    assert(tok2.type == tok1.type);
    assert(tok2.start == tok1.start);
    assert(tok2.length == tok1.length);
    assert(tok2.line == tok1.line);
}

static void test_token_copy_with_literal(void)
{
    Token tok1;
    token_init(&tok1, TOKEN_INT_LITERAL, "42", 2, 1, "test.sn");
    token_set_int_literal(&tok1, 42);

    Token tok2 = tok1;
    assert(tok2.literal.int_value == tok1.literal.int_value);
}

/* ============================================================================
 * Token Array Tests
 * ============================================================================ */

static void test_token_array_store(void)
{
    Token tokens[5];
    token_init(&tokens[0], TOKEN_VAR, "var", 3, 1, "test.sn");
    token_init(&tokens[1], TOKEN_IDENTIFIER, "x", 1, 1, "test.sn");
    token_init(&tokens[2], TOKEN_COLON, ":", 1, 1, "test.sn");
    token_init(&tokens[3], TOKEN_INT, "int", 3, 1, "test.sn");
    token_init(&tokens[4], TOKEN_EOF, "", 0, 1, "test.sn");

    assert(tokens[0].type == TOKEN_VAR);
    assert(tokens[1].type == TOKEN_IDENTIFIER);
    assert(tokens[2].type == TOKEN_COLON);
    assert(tokens[3].type == TOKEN_INT);
    assert(tokens[4].type == TOKEN_EOF);
}

static void test_token_array_iterate(void)
{
    Token tokens[3];
    token_init(&tokens[0], TOKEN_INT_LITERAL, "1", 1, 1, "test.sn");
    token_set_int_literal(&tokens[0], 1);
    token_init(&tokens[1], TOKEN_INT_LITERAL, "2", 1, 1, "test.sn");
    token_set_int_literal(&tokens[1], 2);
    token_init(&tokens[2], TOKEN_INT_LITERAL, "3", 1, 1, "test.sn");
    token_set_int_literal(&tokens[2], 3);

    int64_t sum = 0;
    for (int i = 0; i < 3; i++) {
        sum += tokens[i].literal.int_value;
    }
    assert(sum == 6);
}

/* ============================================================================
 * Token Edge Cases
 * ============================================================================ */

static void test_token_empty_start(void)
{
    Token tok;
    token_init(&tok, TOKEN_EOF, "", 0, 1, "test.sn");
    assert(tok.length == 0);
}

static void test_token_long_literal_type(void)
{
    Token tok;
    token_init(&tok, TOKEN_LONG_LITERAL, "123L", 4, 1, "test.sn");
    assert(tok.type == TOKEN_LONG_LITERAL);
}

static void test_token_byte_literal_type(void)
{
    Token tok;
    token_init(&tok, TOKEN_BYTE_LITERAL, "0xFF", 4, 1, "test.sn");
    assert(tok.type == TOKEN_BYTE_LITERAL);
}

static void test_token_interpol_string_type(void)
{
    Token tok;
    token_init(&tok, TOKEN_INTERPOL_STRING, "$\"hello\"", 8, 1, "test.sn");
    assert(tok.type == TOKEN_INTERPOL_STRING);
}

static void test_token_pragma_types(void)
{
    assert(TOKEN_PRAGMA != TOKEN_PRAGMA_INCLUDE);
    assert(TOKEN_PRAGMA_LINK != TOKEN_PRAGMA_SOURCE);
    assert(TOKEN_PRAGMA_PACK != TOKEN_PRAGMA_ALIAS);
}

static void test_token_error_type(void)
{
    Token tok;
    token_init(&tok, TOKEN_ERROR, "unexpected", 10, 1, "test.sn");
    assert(tok.type == TOKEN_ERROR);
}

static void test_token_comment_type(void)
{
    Token tok;
    token_init(&tok, TOKEN_COMMENT, "// comment", 10, 1, "test.sn");
    assert(tok.type == TOKEN_COMMENT);
}

void test_token_extended_main(void)
{
    TEST_SECTION("Token Extended Tests");

    // Token type to string
    TEST_RUN("token_type_string_literals", test_token_type_string_literals);
    TEST_RUN("token_type_string_keywords", test_token_type_string_keywords);
    TEST_RUN("token_type_string_operators", test_token_type_string_operators);
    TEST_RUN("token_type_string_comparison", test_token_type_string_comparison);
    TEST_RUN("token_type_string_punctuation", test_token_type_string_punctuation);
    TEST_RUN("token_type_string_special", test_token_type_string_special);
    TEST_RUN("token_type_string_types", test_token_type_string_types);

    // Token initialization
    TEST_RUN("token_init_basic", test_token_init_basic);
    TEST_RUN("token_init_identifier", test_token_init_identifier);
    TEST_RUN("token_init_string_literal", test_token_init_string_literal);
    TEST_RUN("token_init_operator", test_token_init_operator);
    TEST_RUN("token_init_long_lexeme", test_token_init_long_lexeme);

    // Token literal values
    TEST_RUN("token_int_literal_value", test_token_int_literal_value);
    TEST_RUN("token_int_literal_zero", test_token_int_literal_zero);
    TEST_RUN("token_int_literal_negative", test_token_int_literal_negative);
    TEST_RUN("token_int_literal_max", test_token_int_literal_max);
    TEST_RUN("token_int_literal_large", test_token_int_literal_large);
    TEST_RUN("token_double_literal_value", test_token_double_literal_value);
    TEST_RUN("token_double_literal_zero", test_token_double_literal_zero);
    TEST_RUN("token_double_literal_negative", test_token_double_literal_negative);
    TEST_RUN("token_double_literal_small", test_token_double_literal_small);
    TEST_RUN("token_double_literal_large", test_token_double_literal_large);
    TEST_RUN("token_string_literal_value", test_token_string_literal_value);
    TEST_RUN("token_string_literal_empty", test_token_string_literal_empty);
    TEST_RUN("token_string_literal_with_space", test_token_string_literal_with_space);
    TEST_RUN("token_char_literal_value", test_token_char_literal_value);
    TEST_RUN("token_char_literal_digit", test_token_char_literal_digit);
    TEST_RUN("token_char_literal_newline", test_token_char_literal_newline);
    TEST_RUN("token_char_literal_tab", test_token_char_literal_tab);
    TEST_RUN("token_bool_literal_true", test_token_bool_literal_true);
    TEST_RUN("token_bool_literal_false", test_token_bool_literal_false);

    // Token types
    TEST_RUN("token_type_arithmetic_ops", test_token_type_arithmetic_ops);
    TEST_RUN("token_type_comparison_ops", test_token_type_comparison_ops);
    TEST_RUN("token_type_logical_ops", test_token_type_logical_ops);
    TEST_RUN("token_type_assignment_ops", test_token_type_assignment_ops);
    TEST_RUN("token_type_bitwise_ops", test_token_type_bitwise_ops);
    TEST_RUN("token_type_brackets", test_token_type_brackets);
    TEST_RUN("token_type_type_keywords", test_token_type_type_keywords);
    TEST_RUN("token_type_control_flow", test_token_type_control_flow);
    TEST_RUN("token_type_declarations", test_token_type_declarations);
    TEST_RUN("token_type_memory_keywords", test_token_type_memory_keywords);

    // Token location
    TEST_RUN("token_location_line_one", test_token_location_line_one);
    TEST_RUN("token_location_line_large", test_token_location_line_large);
    TEST_RUN("token_location_filename", test_token_location_filename);
    TEST_RUN("token_location_different_files", test_token_location_different_files);

    // Token copy
    TEST_RUN("token_copy_basic", test_token_copy_basic);
    TEST_RUN("token_copy_with_literal", test_token_copy_with_literal);

    // Token array
    TEST_RUN("token_array_store", test_token_array_store);
    TEST_RUN("token_array_iterate", test_token_array_iterate);

    // Token edge cases
    TEST_RUN("token_empty_start", test_token_empty_start);
    TEST_RUN("token_long_literal_type", test_token_long_literal_type);
    TEST_RUN("token_byte_literal_type", test_token_byte_literal_type);
    TEST_RUN("token_interpol_string_type", test_token_interpol_string_type);
    TEST_RUN("token_pragma_types", test_token_pragma_types);
    TEST_RUN("token_error_type", test_token_error_type);
    TEST_RUN("token_comment_type", test_token_comment_type);
}
