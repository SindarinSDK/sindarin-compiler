// tests/lexer_tests_literal.c
// Literal-related lexer tests (keywords, numbers, strings, chars)

#include "../test_harness.h"

static void test_lexer_empty_source(void)
{
    DEBUG_INFO("Starting test_lexer_empty_source");

    const char *source = "";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_empty_source");
}

static void test_lexer_only_whitespace(void)
{
    DEBUG_INFO("Starting test_lexer_only_whitespace");

    const char *source = "   \t  \n";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NEWLINE);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_only_whitespace");
}

static void test_lexer_single_identifier(void)
{
    DEBUG_INFO("Starting test_lexer_single_identifier");

    const char *source = "var";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_VAR);
    assert(t1.length == 3);
    assert(strcmp(t1.start, "var") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_single_identifier");
}

static void test_lexer_keywords(void)
{
    DEBUG_INFO("Starting test_lexer_keywords");

    const char *source = "fn if else for while return var int bool str char double long void nil import byte";
    Arena arena;
    arena_init(&arena, 1024 * 2);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_FN);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IF);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_ELSE);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_FOR);
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_WHILE);
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RETURN);
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_VAR);
    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_INT);
    Token t9 = lexer_scan_token(&lexer);
    assert(t9.type == TOKEN_BOOL);
    Token t10 = lexer_scan_token(&lexer);
    assert(t10.type == TOKEN_STR);
    Token t11 = lexer_scan_token(&lexer);
    assert(t11.type == TOKEN_CHAR);
    Token t12 = lexer_scan_token(&lexer);
    assert(t12.type == TOKEN_DOUBLE);
    Token t13 = lexer_scan_token(&lexer);
    assert(t13.type == TOKEN_LONG);
    Token t14 = lexer_scan_token(&lexer);
    assert(t14.type == TOKEN_VOID);
    Token t15 = lexer_scan_token(&lexer);
    assert(t15.type == TOKEN_NIL);
    Token t16 = lexer_scan_token(&lexer);
    assert(t16.type == TOKEN_IMPORT);
    Token t17 = lexer_scan_token(&lexer);
    assert(t17.type == TOKEN_BYTE);

    Token t18 = lexer_scan_token(&lexer);
    assert(t18.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_keywords");
}

static void test_lexer_interop_type_keywords(void)
{
    DEBUG_INFO("Starting test_lexer_interop_type_keywords");

    const char *source = "int32 uint uint32 float";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INT32);
    assert(t1.length == 5);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_UINT);
    assert(t2.length == 4);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_UINT32);
    assert(t3.length == 6);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_FLOAT);
    assert(t4.length == 5);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_interop_type_keywords");
}

static void test_lexer_opaque_type_keywords(void)
{
    DEBUG_INFO("Starting test_lexer_opaque_type_keywords");

    const char *source = "type opaque";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_KEYWORD_TYPE);
    assert(t1.length == 4);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_OPAQUE);
    assert(t2.length == 6);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_opaque_type_keywords");
}

static void test_lexer_bool_literals(void)
{
    DEBUG_INFO("Starting test_lexer_bool_literals");

    const char *source = "true false";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_BOOL_LITERAL);
    assert(t1.literal.bool_value == 1);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_BOOL_LITERAL);
    assert(t2.literal.bool_value == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_bool_literals");
}

static void test_lexer_int_literal(void)
{
    DEBUG_INFO("Starting test_lexer_int_literal");

    const char *source = "42";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INT_LITERAL);
    assert(t1.literal.int_value == 42);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_int_literal");
}

static void test_lexer_long_literal(void)
{
    DEBUG_INFO("Starting test_lexer_long_literal");

    const char *source = "42l";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LONG_LITERAL);
    assert(t1.literal.int_value == 42);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_long_literal");
}

static void test_lexer_double_literal_decimal(void)
{
    DEBUG_INFO("Starting test_lexer_double_literal_decimal");

    const char *source = "3.14";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_DOUBLE_LITERAL);
    assert(t1.literal.double_value == 3.14);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_double_literal_decimal");
}

static void test_lexer_double_literal_with_d(void)
{
    DEBUG_INFO("Starting test_lexer_double_literal_with_d");

    const char *source = "3.14d";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_DOUBLE_LITERAL);
    assert(t1.literal.double_value == 3.14);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_double_literal_with_d");
}

static void test_lexer_string_literal(void)
{
    DEBUG_INFO("Starting test_lexer_string_literal");

    const char *source = "\"hello\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_string_literal");
}

static void test_lexer_string_with_escapes(void)
{
    DEBUG_INFO("Starting test_lexer_string_with_escapes");

    const char *source = "\"hello\\n\\t\\\"world\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello\n\t\"world") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_string_with_escapes");
}

static void test_lexer_unterminated_string(void)
{
    DEBUG_INFO("Starting test_lexer_unterminated_string");

    const char *source = "\"unterminated";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ERROR);
    assert(strstr(t1.start, "Unterminated string") != NULL);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unterminated_string");
}

static void test_lexer_interpolated_string(void)
{
    DEBUG_INFO("Starting test_lexer_interpolated_string");

    const char *source = "$\"hello\"";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_INTERPOL_STRING);
    assert(t1.literal.string_value != NULL);
    assert(strcmp(t1.literal.string_value, "hello") == 0); // Assuming escapes handled similarly

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_interpolated_string");
}

static void test_lexer_char_literal(void)
{
    DEBUG_INFO("Starting test_lexer_char_literal");

    const char *source = "'a'";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_CHAR_LITERAL);
    assert(t1.literal.char_value == 'a');

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_char_literal");
}

static void test_lexer_char_escape(void)
{
    DEBUG_INFO("Starting test_lexer_char_escape");

    const char *source = "'\\n'";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_CHAR_LITERAL);
    assert(t1.literal.char_value == '\n');

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_char_escape");
}

static void test_lexer_unterminated_char(void)
{
    DEBUG_INFO("Starting test_lexer_unterminated_char");

    const char *source = "'unterminated";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ERROR);
    assert(strstr(t1.start, "Unterminated character literal") != NULL);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_unterminated_char");
}

static void test_lexer_native_keyword(void)
{
    DEBUG_INFO("Starting test_lexer_native_keyword");

    const char *source = "native fn nil";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NATIVE);
    assert(t1.length == 6);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_FN);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_NIL);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_native_keyword");
}

static void test_lexer_pragma_include(void)
{
    DEBUG_INFO("Starting test_lexer_pragma_include");

    const char *source = "#pragma include <stdio.h>\n";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_PRAGMA_INCLUDE);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_pragma_include");
}

static void test_lexer_pragma_link(void)
{
    DEBUG_INFO("Starting test_lexer_pragma_link");

    const char *source = "#pragma link m\n";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_PRAGMA_LINK);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_pragma_link");
}

static void test_lexer_val_ref_keywords(void)
{
    DEBUG_INFO("Starting test_lexer_val_ref_keywords");

    const char *source = "as val ref";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_AS);
    assert(t1.length == 2);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_VAL);
    assert(t2.length == 3);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_REF);
    assert(t3.length == 3);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_val_ref_keywords");
}

static void test_lexer_ampersand_operator(void)
{
    DEBUG_INFO("Starting test_lexer_ampersand_operator");

    const char *source = "&x";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_AMPERSAND);
    assert(t1.length == 1);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);
    assert(t2.length == 1);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_ampersand_operator");
}

static void test_lexer_pointer_type_syntax(void)
{
    DEBUG_INFO("Starting test_lexer_pointer_type_syntax");

    const char *source = "*int";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STAR);
    assert(t1.length == 1);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_INT);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_pointer_type_syntax");
}

static void test_lexer_spread_operator(void)
{
    DEBUG_INFO("Starting test_lexer_spread_operator");

    const char *source = "...";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_SPREAD);
    assert(t1.length == 3);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_spread_operator");
}

static void test_lexer_environment_keyword(void)
{
    DEBUG_INFO("Starting test_lexer_environment_keyword");

    const char *source = "Environment";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ENV);
    assert(t1.length == 11);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_environment_keyword");
}

static void test_lexer_environment_in_context(void)
{
    DEBUG_INFO("Starting test_lexer_environment_in_context");

    const char *source = "Environment.get";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_ENV);
    assert(t1.length == 11);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_DOT);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);
    assert(t3.length == 3);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_environment_in_context");
}

static void test_lexer_struct_keyword(void)
{
    DEBUG_INFO("Starting test_lexer_struct_keyword");

    const char *source = "struct";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRUCT);
    assert(t1.length == 6);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_struct_keyword");
}

static void test_lexer_struct_in_context(void)
{
    DEBUG_INFO("Starting test_lexer_struct_in_context");

    const char *source = "struct Point";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRUCT);
    assert(t1.length == 6);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);
    assert(t2.length == 5);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_struct_in_context");
}

static void test_lexer_native_struct_sequence(void)
{
    DEBUG_INFO("Starting test_lexer_native_struct_sequence");

    const char *source = "native struct ZStream";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_NATIVE);
    assert(t1.length == 6);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_STRUCT);
    assert(t2.length == 6);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);
    assert(t3.length == 7);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_native_struct_sequence");
}

static void test_lexer_str_vs_struct_disambiguation(void)
{
    DEBUG_INFO("Starting test_lexer_str_vs_struct_disambiguation");

    // Test that 'str' and 'struct' are correctly distinguished
    const char *source = "str struct string";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STR);
    assert(t1.length == 3);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_STRUCT);
    assert(t2.length == 6);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_IDENTIFIER);  // 'string' is an identifier, not a keyword
    assert(t3.length == 6);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_str_vs_struct_disambiguation");
}


void test_lexer_literal_main(void)
{
    TEST_SECTION("Lexer Literal Tests");
    TEST_RUN("lexer_empty_source", test_lexer_empty_source);
    TEST_RUN("lexer_only_whitespace", test_lexer_only_whitespace);
    TEST_RUN("lexer_single_identifier", test_lexer_single_identifier);
    TEST_RUN("lexer_keywords", test_lexer_keywords);
    TEST_RUN("lexer_interop_type_keywords", test_lexer_interop_type_keywords);
    TEST_RUN("lexer_opaque_type_keywords", test_lexer_opaque_type_keywords);
    TEST_RUN("lexer_native_keyword", test_lexer_native_keyword);
    TEST_RUN("lexer_bool_literals", test_lexer_bool_literals);
    TEST_RUN("lexer_int_literal", test_lexer_int_literal);
    TEST_RUN("lexer_long_literal", test_lexer_long_literal);
    TEST_RUN("lexer_double_literal_decimal", test_lexer_double_literal_decimal);
    TEST_RUN("lexer_double_literal_with_d", test_lexer_double_literal_with_d);
    TEST_RUN("lexer_string_literal", test_lexer_string_literal);
    TEST_RUN("lexer_string_with_escapes", test_lexer_string_with_escapes);
    TEST_RUN("lexer_unterminated_string", test_lexer_unterminated_string);
    TEST_RUN("lexer_interpolated_string", test_lexer_interpolated_string);
    TEST_RUN("lexer_char_literal", test_lexer_char_literal);
    TEST_RUN("lexer_char_escape", test_lexer_char_escape);
    TEST_RUN("lexer_unterminated_char", test_lexer_unterminated_char);
    // Pragma tests
    TEST_RUN("lexer_pragma_include", test_lexer_pragma_include);
    TEST_RUN("lexer_pragma_link", test_lexer_pragma_link);
    // Interop keyword tests
    TEST_RUN("lexer_val_ref_keywords", test_lexer_val_ref_keywords);
    TEST_RUN("lexer_ampersand_operator", test_lexer_ampersand_operator);
    TEST_RUN("lexer_pointer_type_syntax", test_lexer_pointer_type_syntax);
    TEST_RUN("lexer_spread_operator", test_lexer_spread_operator);
    // Environment keyword tests
    TEST_RUN("lexer_environment_keyword", test_lexer_environment_keyword);
    TEST_RUN("lexer_environment_in_context", test_lexer_environment_in_context);
    // Struct keyword tests
    TEST_RUN("lexer_struct_keyword", test_lexer_struct_keyword);
    TEST_RUN("lexer_struct_in_context", test_lexer_struct_in_context);
    TEST_RUN("lexer_native_struct_sequence", test_lexer_native_struct_sequence);
    TEST_RUN("lexer_str_vs_struct_disambiguation", test_lexer_str_vs_struct_disambiguation);
}
