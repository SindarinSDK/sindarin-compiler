// tests/token_tests.c

#include <assert.h>
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
    DEBUG_INFO("Starting test_token_init_array_literal");
    
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);

    assert(token.type == TOKEN_ARRAY_LITERAL);
    assert(token.start == dummy_source);
    assert(token.length == (int)strlen(dummy_source));
    assert(token.line == 1);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);       // Zero-initialized
    assert(token.literal.string_value == NULL); // Pointer zero

    token_print(&token); // Visual: Should print lexeme without value yet

    DEBUG_INFO("Finished test_token_init_array_literal");
}

static void test_token_init_int_literal(void)
{
    DEBUG_INFO("Starting test_token_init_int_literal");
    
    Token token;
    token_init(&token, TOKEN_INT_LITERAL, dummy_source, (int)strlen(dummy_source), 5, filename);

    assert(token.type == TOKEN_INT_LITERAL);
    assert(token.start == dummy_source);
    assert(token.length == (int)strlen(dummy_source));
    assert(token.line == 5);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);

    token_print(&token);

    DEBUG_INFO("Finished test_token_init_int_literal");
}

static void test_token_init_non_literal(void)
{
    DEBUG_INFO("Starting test_token_init_non_literal");
    
    Token token;
    const char *plus_str = "+";
    token_init(&token, TOKEN_PLUS, plus_str, 1, 10, filename);

    assert(token.type == TOKEN_PLUS);
    assert(token.start == plus_str);
    assert(token.length == 1);
    assert(token.line == 10);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);

    token_print(&token); // Should not print value

    DEBUG_INFO("Finished test_token_init_non_literal");
}

static void test_token_init_zero_length(void)
{
    DEBUG_INFO("Starting test_token_init_zero_length");
    
    Token token;
    token_init(&token, TOKEN_EOF, NULL, 0, 0, filename);

    assert(token.type == TOKEN_EOF);
    assert(token.start == NULL);
    assert(token.length == 0);
    assert(token.line == 0);
    assert(token.filename == filename);
    assert(token.literal.int_value == 0);

    token_print(&token); // Edge case: empty lexeme

    DEBUG_INFO("Finished test_token_init_zero_length");
}

static void test_token_set_array_literal_null(void)
{
    DEBUG_INFO("Starting test_token_set_array_literal_null");
    
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, NULL);

    assert(token.literal.string_value == NULL);

    token_print(&token); // Visual: value: {}

    DEBUG_INFO("Finished test_token_set_array_literal_null");
}

static void test_token_set_array_literal_empty(void)
{
    DEBUG_INFO("Starting test_token_set_array_literal_empty");
    
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, empty_string_content);

    assert(token.literal.string_value == empty_string_content);
    assert(strcmp(token.literal.string_value, "") == 0);

    token_print(&token); // Visual: value: {}

    DEBUG_INFO("Finished test_token_set_array_literal_empty");
}

static void test_token_set_array_literal_single(void)
{
    DEBUG_INFO("Starting test_token_set_array_literal_single");
    
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, single_element_content);

    assert(token.literal.string_value == single_element_content);
    assert(strcmp(token.literal.string_value, "42") == 0);

    token_print(&token); // Visual: value: {42}

    DEBUG_INFO("Finished test_token_set_array_literal_single");
}

static void test_token_set_array_literal_multi(void)
{
    DEBUG_INFO("Starting test_token_set_array_literal_multi");
    
    Token token;
    token_init(&token, TOKEN_ARRAY_LITERAL, dummy_source, (int)strlen(dummy_source), 1, filename);
    token_set_array_literal(&token, multi_element_content);

    assert(token.literal.string_value == multi_element_content);
    assert(strcmp(token.literal.string_value, "1, 2, 3") == 0);

    token_print(&token); // Visual: value: {1, 2, 3}

    DEBUG_INFO("Finished test_token_set_array_literal_multi");
}

static void test_token_set_int_literal(void)
{
    DEBUG_INFO("Starting test_token_set_int_literal");
    
    Token token;
    token_init(&token, TOKEN_INT_LITERAL, "42", 2, 1, filename);
    token_set_int_literal(&token, 42);

    assert(token.literal.int_value == 42);

    token_print(&token); // Visual: value: 42

    DEBUG_INFO("Finished test_token_set_int_literal");
}

static void test_token_set_long_literal(void)
{
    DEBUG_INFO("Starting test_token_set_long_literal");
    
    Token token;
    token_init(&token, TOKEN_LONG_LITERAL, "42l", 3, 1, filename);
    token_set_int_literal(&token, 42LL); // Note: Uses same setter as int

    assert(token.literal.int_value == 42LL);

    token_print(&token); // Visual: value: 42l (print formats as %ldl)

    DEBUG_INFO("Finished test_token_set_long_literal");
}

static void test_token_set_double_literal(void)
{
    DEBUG_INFO("Starting test_token_set_double_literal");
    
    Token token;
    token_init(&token, TOKEN_DOUBLE_LITERAL, "3.14", 4, 1, filename);
    token_set_double_literal(&token, 3.14);

    assert(token.literal.double_value == 3.14);

    token_print(&token); // Visual: value: 3.140000d

    DEBUG_INFO("Finished test_token_set_double_literal");
}

static void test_token_set_char_literal(void)
{
    DEBUG_INFO("Starting test_token_set_char_literal");
    
    Token token;
    token_init(&token, TOKEN_CHAR_LITERAL, "'a'", 3, 1, filename);
    token_set_char_literal(&token, 'a');

    assert(token.literal.char_value == 'a');

    token_print(&token); // Visual: value: 'a'

    DEBUG_INFO("Finished test_token_set_char_literal");
}

static void test_token_set_string_literal(void)
{
    DEBUG_INFO("Starting test_token_set_string_literal");
    
    Token token;
    token_init(&token, TOKEN_STRING_LITERAL, "\"hello\"", 7, 1, filename);
    token_set_string_literal(&token, test_string);

    assert(token.literal.string_value == test_string);
    assert(strcmp(token.literal.string_value, "hello") == 0);

    token_print(&token); // Visual: value: "hello"

    DEBUG_INFO("Finished test_token_set_string_literal");
}

static void test_token_set_interpol_string(void)
{
    DEBUG_INFO("Starting test_token_set_interpol_string");
    
    Token token;
    token_init(&token, TOKEN_INTERPOL_STRING, "\"hello ${var}\"", 13, 1, filename);
    token_set_string_literal(&token, test_string);

    assert(token.literal.string_value == test_string);

    token_print(&token); // Visual: value: "hello" (uses string print)

    DEBUG_INFO("Finished test_token_set_interpol_string");
}

static void test_token_set_bool_literal_true(void)
{
    DEBUG_INFO("Starting test_token_set_bool_literal_true");
    
    Token token;
    token_init(&token, TOKEN_BOOL_LITERAL, "true", 4, 1, filename);
    token_set_bool_literal(&token, 1);

    assert(token.literal.bool_value == 1);

    token_print(&token); // Visual: value: true

    DEBUG_INFO("Finished test_token_set_bool_literal_true");
}

static void test_token_set_bool_literal_false(void)
{
    DEBUG_INFO("Starting test_token_set_bool_literal_false");
    
    Token token;
    token_init(&token, TOKEN_BOOL_LITERAL, "false", 5, 1, filename);
    token_set_bool_literal(&token, 0);

    assert(token.literal.bool_value == 0);

    token_print(&token); // Visual: value: false

    DEBUG_INFO("Finished test_token_set_bool_literal_false");
}

static void test_token_type_to_string_array(void)
{
    DEBUG_INFO("Starting test_token_type_to_string_array");
    
    const char *result = token_type_to_string(TOKEN_ARRAY_LITERAL);
    assert(result != NULL);
    assert(strcmp(result, "ARRAY_LITERAL") == 0);

    DEBUG_INFO("Finished test_token_type_to_string_array");
}

static void test_token_type_to_string_all_literals(void)
{
    DEBUG_INFO("Starting test_token_type_to_string_all_literals");
    
    assert(strcmp(token_type_to_string(TOKEN_EOF), "EOF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT_LITERAL), "INT_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LONG_LITERAL), "LONG_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE_LITERAL), "DOUBLE_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR_LITERAL), "CHAR_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STRING_LITERAL), "STRING_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INTERPOL_STRING), "INTERPOL_STRING") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ARRAY_LITERAL), "ARRAY_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL_LITERAL), "BOOL_LITERAL") == 0);

    DEBUG_INFO("Finished test_token_type_to_string_all_literals");
}

static void test_token_type_to_string_keywords(void)
{
    DEBUG_INFO("Starting test_token_type_to_string_keywords");
    
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

    DEBUG_INFO("Finished test_token_type_to_string_keywords");
}

static void test_token_type_to_string_operators(void)
{
    DEBUG_INFO("Starting test_token_type_to_string_operators");
    
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

    DEBUG_INFO("Finished test_token_type_to_string_operators");
}

static void test_token_type_to_string_special(void)
{
    DEBUG_INFO("Starting test_token_type_to_string_special");
    
    assert(strcmp(token_type_to_string(TOKEN_INDENT), "INDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DEDENT), "DEDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NEWLINE), "NEWLINE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ERROR), "ERROR") == 0);

    DEBUG_INFO("Finished test_token_type_to_string_special");
}

static void test_token_type_to_string_invalid(void)
{
    const char *result = token_type_to_string((SnTokenType)-1);
    assert(strcmp(result, "INVALID") == 0);

    result = token_type_to_string((SnTokenType)999);
    assert(strcmp(result, "INVALID") == 0); // Out of range

    result = token_type_to_string(TOKEN_ERROR);
    assert(strcmp(result, "ERROR") == 0); // Valid but error

    DEBUG_INFO("Finished test_token_type_to_string_invalid");
}

static void test_token_print_array_integration(void)
{
    DEBUG_INFO("Starting test_token_print_array_integration");
    
    Token token;
    char *lexeme = malloc(10);
    strcpy(lexeme, "{1,2}");
    token_init(&token, TOKEN_ARRAY_LITERAL, lexeme, 5, 42, filename);
    token_set_array_literal(&token, "1,2");

    assert(token.type == TOKEN_ARRAY_LITERAL);
    assert(token.length == 5);
    assert(strcmp(token.literal.string_value, "1,2") == 0);

    token_print(&token); // Visual: Token { type: ARRAY_LITERAL, lexeme: '{1,2}', line: 42, value: {1,2} }

    free(lexeme);

    DEBUG_INFO("Finished test_token_print_array_integration");
}

static void test_token_print_int_literal(void)
{
    DEBUG_INFO("Starting test_token_print_int_literal");
    
    Token token;
    token_init(&token, TOKEN_INT_LITERAL, "42", 2, 1, filename);
    token_set_int_literal(&token, 42);

    token_print(&token); // Visual: value: 42

    DEBUG_INFO("Finished test_token_print_int_literal");
}

static void test_token_print_long_literal(void)
{
    DEBUG_INFO("Starting test_token_print_long_literal");
    
    Token token;
    token_init(&token, TOKEN_LONG_LITERAL, "42l", 3, 1, filename);
    token_set_int_literal(&token, 42LL);

    token_print(&token); // Visual: value: 42l

    DEBUG_INFO("Finished test_token_print_long_literal");
}

static void test_token_print_double_literal(void)
{
    DEBUG_INFO("Starting test_token_print_double_literal");
    
    Token token;
    token_init(&token, TOKEN_DOUBLE_LITERAL, "3.14", 4, 1, filename);
    token_set_double_literal(&token, 3.14);

    token_print(&token); // Visual: value: 3.140000d

    DEBUG_INFO("Finished test_token_print_double_literal");
}

static void test_token_print_char_literal(void)
{
    DEBUG_INFO("Starting test_token_print_char_literal");
    
    Token token;
    token_init(&token, TOKEN_CHAR_LITERAL, "'a'", 3, 1, filename);
    token_set_char_literal(&token, 'a');

    token_print(&token); // Visual: value: 'a'

    DEBUG_INFO("Finished test_token_print_char_literal");
}

static void test_token_print_string_literal(void)
{
    DEBUG_INFO("Starting test_token_print_string_literal");
    
    Token token;
    token_init(&token, TOKEN_STRING_LITERAL, "\"hello\"", 7, 1, filename);
    token_set_string_literal(&token, test_string);

    token_print(&token); // Visual: value: "hello"

    DEBUG_INFO("Finished test_token_print_string_literal");
}

static void test_token_print_interpol_string(void)
{
    DEBUG_INFO("Starting test_token_print_interpol_string");
    
    Token token;
    token_init(&token, TOKEN_INTERPOL_STRING, "\"hello ${var}\"", 13, 1, filename);
    token_set_string_literal(&token, test_string);

    token_print(&token); // Visual: value: "hello"

    DEBUG_INFO("Finished test_token_print_interpol_string");
}

static void test_token_print_bool_literal(void)
{
    DEBUG_INFO("Starting test_token_print_bool_literal");
    
    Token token_true;
    token_init(&token_true, TOKEN_BOOL_LITERAL, "true", 4, 1, filename);
    token_set_bool_literal(&token_true, 1);
    token_print(&token_true); // Visual: value: true

    Token token_false;
    token_init(&token_false, TOKEN_BOOL_LITERAL, "false", 5, 1, filename);
    token_set_bool_literal(&token_false, 0);
    token_print(&token_false); // Visual: value: false

    DEBUG_INFO("Finished test_token_print_bool_literal");
}

static void test_token_print_non_literal(void)
{
    DEBUG_INFO("Starting test_token_print_non_literal");
    
    Token token;
    token_init(&token, TOKEN_PLUS, "+", 1, 1, filename);

    token_print(&token); // No value printed

    DEBUG_INFO("Finished test_token_print_non_literal");
}

static void test_token_print_empty_lexeme(void)
{
    DEBUG_INFO("Starting test_token_print_empty_lexeme");
    
    Token token;
    token_init(&token, TOKEN_NEWLINE, "", 0, 1, filename);

    token_print(&token); // lexeme: ''

    DEBUG_INFO("Finished test_token_print_empty_lexeme");
}

static void test_token_init_invalid_type(void)
{
    DEBUG_INFO("Starting test_token_init_invalid_type");
    
    Token token;
    token_init(&token, (SnTokenType)999, dummy_source, (int)strlen(dummy_source), 1, filename);

    assert(token.type == (SnTokenType)999);
    const char *type_str = token_type_to_string(token.type);
    assert(strcmp(type_str, "INVALID") == 0);

    token_print(&token); // Visual: type: INVALID

    DEBUG_INFO("Finished test_token_init_invalid_type");
}

static void test_token_print_invalid_type(void)
{
    DEBUG_INFO("Starting test_token_print_invalid_type");
    
    Token token;
    token_init(&token, (SnTokenType)999, dummy_source, (int)strlen(dummy_source), 1, filename);

    token_print(&token); // type: INVALID, no value

    DEBUG_INFO("Finished test_token_print_invalid_type");
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
    TEST_RUN("token_print_int_literal", test_token_print_int_literal);
    TEST_RUN("token_print_long_literal", test_token_print_long_literal);
    TEST_RUN("token_print_double_literal", test_token_print_double_literal);
    TEST_RUN("token_print_char_literal", test_token_print_char_literal);
    TEST_RUN("token_print_string_literal", test_token_print_string_literal);
    TEST_RUN("token_print_interpol_string", test_token_print_interpol_string);
    TEST_RUN("token_print_bool_literal", test_token_print_bool_literal);
    TEST_RUN("token_print_non_literal", test_token_print_non_literal);
    TEST_RUN("token_print_empty_lexeme", test_token_print_empty_lexeme);
    TEST_RUN("token_init_invalid_type", test_token_init_invalid_type);
    TEST_RUN("token_print_invalid_type", test_token_print_invalid_type);
}
