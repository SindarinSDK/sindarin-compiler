// tests/token_tests.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../token.h"
#include "../debug.h"
#include "../test_harness.h"

static const char *dummy_source = "example";
static const char *empty_string_content = "";
static const char *single_element_content = "42";
static const char *multi_element_content = "1, 2, 3";
static const char *test_string = "hello";
static const char *filename = "test.sn";

void cleanup_temp_data(char *data)
{
    if (data)
        free(data);
}

static void test_token_init_array_literal(void)
{
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);

    assert(token.type == TOKEN_ARRAY_LITERAL);
    assert(token.start == dummy_source);
    assert(token.length == (int)strlen(dummy_source));
    assert(token.line == 1);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);
    assert(token.literal.string_value == NULL);
}

static void test_token_init_int_literal(void)
{
    Token token;
    token_init(&token, TOKEN_INT_LITERAL, dummy_source, (int)strlen(dummy_source), 5, filename);

    assert(token.type == TOKEN_INT_LITERAL);
    assert(token.start == dummy_source);
    assert(token.length == (int)strlen(dummy_source));
    assert(token.line == 5);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);
}

static void test_token_init_non_literal(void)
{
    Token token;
    const char *plus_str = "+";
    token_init(&token, TOKEN_PLUS, plus_str, 1, 10, filename);

    assert(token.type == TOKEN_PLUS);
    assert(token.start == plus_str);
    assert(token.length == 1);
    assert(token.line == 10);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);
}

static void test_token_init_zero_length(void)
{
    Token token;
    token_init(&token, TOKEN_EOF, NULL, 0, 0, filename);

    assert(token.type == TOKEN_EOF);
    assert(token.start == NULL);
    assert(token.length == 0);
    assert(token.line == 0);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);
}

static void test_token_set_array_literal_null(void)
{
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, NULL);

    assert(token.literal.string_value == NULL);
}

static void test_token_set_array_literal_empty(void)
{
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, empty_string_content);

    assert(token.literal.string_value == empty_string_content);
    assert(strcmp(token.literal.string_value, "") == 0);
}

static void test_token_set_array_literal_single(void)
{
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, single_element_content);

    assert(token.literal.string_value == single_element_content);
    assert(strcmp(token.literal.string_value, "42") == 0);
}

static void test_token_set_array_literal_multi(void)
{
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, multi_element_content);

    assert(token.literal.string_value == multi_element_content);
    assert(strcmp(token.literal.string_value, "1, 2, 3") == 0);
}

static void test_token_set_int_literal(void)
{
    Token token;
    token_init(&token, TOKEN_INT_LITERAL, "42", 2, 1, filename);
    token_set_int_literal(&token, 42);

    assert(token.literal.int_value == 42);
}

static void test_token_set_long_literal(void)
{
    Token token;
    token_init(&token, TOKEN_LONG_LITERAL, "42l", 3, 1, filename);
    token_set_int_literal(&token, 42LL);

    assert(token.literal.int_value == 42LL);
}

static void test_token_set_double_literal(void)
{
    Token token;
    token_init(&token, TOKEN_DOUBLE_LITERAL, "3.14", 4, 1, filename);
    token_set_double_literal(&token, 3.14);

    assert(token.literal.double_value == 3.14);
}

static void test_token_set_char_literal(void)
{
    Token token;
    token_init(&token, TOKEN_CHAR_LITERAL, "'a'", 3, 1, filename);
    token_set_char_literal(&token, 'a');

    assert(token.literal.char_value == 'a');
}

static void test_token_set_string_literal(void)
{
    Token token;
    token_init(&token, TOKEN_STRING_LITERAL, "\"hello\"", 7, 1, filename);
    token_set_string_literal(&token, test_string);

    assert(token.literal.string_value == test_string);
    assert(strcmp(token.literal.string_value, "hello") == 0);
}

static void test_token_set_interpol_string(void)
{
    Token token;
    token_init(&token, TOKEN_INTERPOL_STRING, "\"hello ${var}\"", 13, 1, filename);
    token_set_string_literal(&token, test_string);

    assert(token.literal.string_value == test_string);
}

static void test_token_set_bool_literal_true(void)
{
    Token token;
    token_init(&token, TOKEN_BOOL_LITERAL, "true", 4, 1, filename);
    token_set_bool_literal(&token, 1);

    assert(token.literal.bool_value == 1);
}

static void test_token_set_bool_literal_false(void)
{
    Token token;
    token_init(&token, TOKEN_BOOL_LITERAL, "false", 5, 1, filename);
    token_set_bool_literal(&token, 0);

    assert(token.literal.bool_value == 0);
}

static void test_token_type_to_string_array(void)
{
    const char *result = token_type_to_string(TOKEN_ARRAY_LITERAL);
    assert(result != NULL);
    assert(strcmp(result, "ARRAY_LITERAL") == 0);
}

static void test_token_type_to_string_all_literals(void)
{
    assert(strcmp(token_type_to_string(TOKEN_EOF), "EOF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT_LITERAL), "INT_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LONG_LITERAL), "LONG_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE_LITERAL), "DOUBLE_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR_LITERAL), "CHAR_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STRING_LITERAL), "STRING_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INTERPOL_STRING), "INTERPOL_STRING") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ARRAY_LITERAL), "ARRAY_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL_LITERAL), "BOOL_LITERAL") == 0);
}

static void test_token_type_to_string_keywords(void)
{
    assert(strcmp(token_type_to_string(TOKEN_IDENTIFIER), "IDENTIFIER") == 0);
    assert(strcmp(token_type_to_string(TOKEN_FN), "FN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_VAR), "VAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RETURN), "RETURN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_IF), "IF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ELSE), "ELSE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_FOR), "FOR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_WHILE), "WHILE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_IMPORT), "IMPORT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NIL), "NIL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT), "INT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT32), "INT32") == 0);
    assert(strcmp(token_type_to_string(TOKEN_UINT), "UINT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_UINT32), "UINT32") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LONG), "LONG") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE), "DOUBLE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_FLOAT), "FLOAT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR), "CHAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STR), "STR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL), "BOOL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_VOID), "VOID") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NATIVE), "NATIVE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STRUCT), "STRUCT") == 0);
}

static void test_token_type_to_string_operators(void)
{
    assert(strcmp(token_type_to_string(TOKEN_PLUS), "PLUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MINUS), "MINUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STAR), "STAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_SLASH), "SLASH") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MODULO), "MODULO") == 0);
    assert(strcmp(token_type_to_string(TOKEN_EQUAL), "EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_EQUAL_EQUAL), "EQUAL_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BANG), "BANG") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BANG_EQUAL), "BANG_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LESS), "LESS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LESS_EQUAL), "LESS_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_GREATER), "GREATER") == 0);
    assert(strcmp(token_type_to_string(TOKEN_GREATER_EQUAL), "GREATER_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_AND), "AND") == 0);
    assert(strcmp(token_type_to_string(TOKEN_OR), "OR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_PLUS_PLUS), "PLUS_PLUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MINUS_MINUS), "MINUS_MINUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_PAREN), "LEFT_PAREN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_PAREN), "RIGHT_PAREN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_BRACE), "LEFT_BRACE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_BRACE), "RIGHT_BRACE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_BRACKET), "LEFT_BRACKET") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_BRACKET), "RIGHT_BRACKET") == 0);
    assert(strcmp(token_type_to_string(TOKEN_SEMICOLON), "SEMICOLON") == 0);
    assert(strcmp(token_type_to_string(TOKEN_COLON), "COLON") == 0);
    assert(strcmp(token_type_to_string(TOKEN_COMMA), "COMMA") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOT), "DOT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ARROW), "ARROW") == 0);
}

static void test_token_type_to_string_special(void)
{
    assert(strcmp(token_type_to_string(TOKEN_INDENT), "INDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DEDENT), "DEDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NEWLINE), "NEWLINE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ERROR), "ERROR") == 0);
}

static void test_token_type_to_string_invalid(void)
{
    const char *result = token_type_to_string((SnTokenType)-1);
    assert(strcmp(result, "INVALID") == 0);

    result = token_type_to_string((SnTokenType)999);
    assert(strcmp(result, "INVALID") == 0);

    result = token_type_to_string(TOKEN_ERROR);
    assert(strcmp(result, "ERROR") == 0);
}

static void test_token_print_array_integration(void)
{
    Token token;
    char *lexeme = malloc(10);
    strcpy(lexeme, "{1,2}");
    token_init(&token, TOKEN_ARRAY_LITERAL, lexeme, 5, 42, filename);
    token_set_array_literal(&token, "1,2");

    assert(token.type == TOKEN_ARRAY_LITERAL);
    assert(token.length == 5);
    assert(strcmp(token.literal.string_value, "1,2") == 0);

    free(lexeme);
}

static void test_token_init_invalid_type(void)
{
    Token token;
    token_init(&token, (SnTokenType)999, dummy_source, (int)strlen(dummy_source), 1, filename);

    assert(token.type == (SnTokenType)999);
    const char *type_str = token_type_to_string(token.type);
    assert(strcmp(type_str, "INVALID") == 0);
}

void test_token_main(void)
{
    TEST_SECTION("Token");

    TEST_RUN("token_init_array_literal", test_token_init_array_literal);
    TEST_RUN("token_init_int_literal", test_token_init_int_literal);
    TEST_RUN("token_init_non_literal", test_token_init_non_literal);
    TEST_RUN("token_init_zero_length", test_token_init_zero_length);
    TEST_RUN("token_set_array_literal_null", test_token_set_array_literal_null);
    TEST_RUN("token_set_array_literal_empty", test_token_set_array_literal_empty);
    TEST_RUN("token_set_array_literal_single", test_token_set_array_literal_single);
    TEST_RUN("token_set_array_literal_multi", test_token_set_array_literal_multi);
    TEST_RUN("token_set_int_literal", test_token_set_int_literal);
    TEST_RUN("token_set_long_literal", test_token_set_long_literal);
    TEST_RUN("token_set_double_literal", test_token_set_double_literal);
    TEST_RUN("token_set_char_literal", test_token_set_char_literal);
    TEST_RUN("token_set_string_literal", test_token_set_string_literal);
    TEST_RUN("token_set_interpol_string", test_token_set_interpol_string);
    TEST_RUN("token_set_bool_literal_true", test_token_set_bool_literal_true);
    TEST_RUN("token_set_bool_literal_false", test_token_set_bool_literal_false);
    TEST_RUN("token_type_to_string_array", test_token_type_to_string_array);
    TEST_RUN("token_type_to_string_all_literals", test_token_type_to_string_all_literals);
    TEST_RUN("token_type_to_string_keywords", test_token_type_to_string_keywords);
    TEST_RUN("token_type_to_string_operators", test_token_type_to_string_operators);
    TEST_RUN("token_type_to_string_special", test_token_type_to_string_special);
    TEST_RUN("token_type_to_string_invalid", test_token_type_to_string_invalid);
    TEST_RUN("token_print_array_integration", test_token_print_array_integration);
    TEST_RUN("token_init_invalid_type", test_token_init_invalid_type);
}
