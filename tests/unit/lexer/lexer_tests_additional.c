// tests/unit/lexer/lexer_tests_additional.c
// Additional lexer tests

/* ============================================================================
 * Number Literal Edge Cases
 * ============================================================================ */

static void test_lexer_zero_variations(void)
{
    Arena arena;
    Lexer lexer;

    // Just zero
    init_lexer_test(&arena, &lexer, "0");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 0);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_hex_literals_case(void)
{
    Arena arena;
    Lexer lexer;

    // Uppercase hex
    init_lexer_test(&arena, &lexer, "0xFF");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 255);
    cleanup_lexer_test(&arena, &lexer);

    // Lowercase hex
    init_lexer_test(&arena, &lexer, "0xff");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 255);
    cleanup_lexer_test(&arena, &lexer);

    // Mixed case
    init_lexer_test(&arena, &lexer, "0xAbCd");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 0xABCD);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_binary_literals(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "0b1010");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 10);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "0b0");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 0);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "0b1");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 1);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "0b11111111");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 255);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_double_variations(void)
{
    Arena arena;
    Lexer lexer;

    // Simple decimal
    init_lexer_test(&arena, &lexer, "3.14");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    assert(tok.literal.double_value > 3.13 && tok.literal.double_value < 3.15);
    cleanup_lexer_test(&arena, &lexer);

    // With exponent
    init_lexer_test(&arena, &lexer, "1e10");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "1E10");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Negative exponent
    init_lexer_test(&arena, &lexer, "1e-5");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Positive exponent
    init_lexer_test(&arena, &lexer, "1e+5");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_number_boundaries(void)
{
    Arena arena;
    Lexer lexer;

    // Large int
    init_lexer_test(&arena, &lexer, "9999999999");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Small double
    init_lexer_test(&arena, &lexer, "0.0000001");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * String Literal Edge Cases
 * ============================================================================ */

static void test_lexer_string_escapes(void)
{
    Arena arena;
    Lexer lexer;

    // Newline escape
    init_lexer_test(&arena, &lexer, "\"line1\\nline2\"");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Tab escape
    init_lexer_test(&arena, &lexer, "\"col1\\tcol2\"");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Quote escape
    init_lexer_test(&arena, &lexer, "\"say \\\"hello\\\"\"");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    // Backslash escape
    init_lexer_test(&arena, &lexer, "\"path\\\\to\\\\file\"");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_empty_string(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"\"");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_single_char_string(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"x\"");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_string_with_spaces(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello world test\"");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Character Literal Edge Cases
 * ============================================================================ */

static void test_lexer_char_escapes(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "'\\n'");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "'\\t'");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "'\\''");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "'\\\\'");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_char_simple(void)
{
    Arena arena;
    Lexer lexer;
    const char *chars = "abcxyz0129";

    for (int i = 0; chars[i]; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "'%c'", chars[i]);
        init_lexer_test(&arena, &lexer, buf);
        Token tok = lexer_scan_token(&lexer);
        assert(tok.type == TOKEN_CHAR_LITERAL);
        cleanup_lexer_test(&arena, &lexer);
    }
}

/* ============================================================================
 * Identifier Edge Cases
 * ============================================================================ */

static void test_lexer_identifier_underscore(void)
{
    Arena arena;
    Lexer lexer;

    // Single underscore
    init_lexer_test(&arena, &lexer, "_");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    // Leading underscore
    init_lexer_test(&arena, &lexer, "_name");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    // Double underscore
    init_lexer_test(&arena, &lexer, "__internal");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    // Trailing underscore
    init_lexer_test(&arena, &lexer, "name_");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    // Multiple underscores
    init_lexer_test(&arena, &lexer, "a_b_c_d");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_identifier_with_numbers(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "var1");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "abc123xyz");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "x2y2z2");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_keyword_like_identifiers(void)
{
    Arena arena;
    Lexer lexer;

    // Starts like keyword but isn't
    init_lexer_test(&arena, &lexer, "ifelse");  // Not 'if' or 'else'
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "forwhile");  // Not 'for' or 'while'
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "intx");  // Not 'int'
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Operator Edge Cases
 * ============================================================================ */

static void test_lexer_compound_operators(void)
{
    Arena arena;
    Lexer lexer;

    // Comparison operators
    init_lexer_test(&arena, &lexer, "==");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "!=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BANG_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "<=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LESS_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, ">=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    // Logical operators
    init_lexer_test(&arena, &lexer, "&&");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_AND);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "||");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_OR);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_assignment_operators(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "+=");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "-=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "*=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR_EQUAL);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "/=");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SLASH_EQUAL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_arrow_operators(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "=>");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ARROW);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "->");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_THIN_ARROW);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Comment Edge Cases
 * ============================================================================ */

static void test_lexer_single_line_comment(void)
{
    Arena arena;
    Lexer lexer;

    // Comment at end of input
    init_lexer_test(&arena, &lexer, "x // comment");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EOF);
    cleanup_lexer_test(&arena, &lexer);

    // Just comment
    init_lexer_test(&arena, &lexer, "// only comment");
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EOF);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_multi_line_comment(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "x /* comment */ y");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_nested_comment_content(void)
{
    Arena arena;
    Lexer lexer;

    // Comments can contain special chars
    init_lexer_test(&arena, &lexer, "x /* { } [ ] */ y");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Whitespace Edge Cases
 * ============================================================================ */

static void test_lexer_multiple_spaces(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "a      b");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_tabs_and_newlines(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "a\t\n\t\nb");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_line_tracking(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "a\nb\nc");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.line == 1);
    tok = lexer_scan_token(&lexer);
    assert(tok.line == 2);
    tok = lexer_scan_token(&lexer);
    assert(tok.line == 3);
    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Token Sequence Edge Cases
 * ============================================================================ */

static void test_lexer_no_space_between_tokens(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "a+b");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_parentheses_sequence(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "(((x)))");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_PAREN);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lexer_bracket_types(void)
{
    Arena arena;
    Lexer lexer;

    init_lexer_test(&arena, &lexer, "(){}[]");
    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACE);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_BRACE);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACKET);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_BRACKET);
    cleanup_lexer_test(&arena, &lexer);
}

void test_lexer_additional_main(void)
{
    TEST_SECTION("Lexer Additional");

    // Number literals
    TEST_RUN("lexer_zero_variations", test_lexer_zero_variations);
    TEST_RUN("lexer_hex_literals_case", test_lexer_hex_literals_case);
    TEST_RUN("lexer_binary_literals", test_lexer_binary_literals);
    TEST_RUN("lexer_double_variations", test_lexer_double_variations);
    TEST_RUN("lexer_number_boundaries", test_lexer_number_boundaries);

    // String literals
    TEST_RUN("lexer_string_escapes", test_lexer_string_escapes);
    TEST_RUN("lexer_empty_string", test_lexer_empty_string);
    TEST_RUN("lexer_single_char_string", test_lexer_single_char_string);
    TEST_RUN("lexer_string_with_spaces", test_lexer_string_with_spaces);

    // Character literals
    TEST_RUN("lexer_char_escapes", test_lexer_char_escapes);
    TEST_RUN("lexer_char_simple", test_lexer_char_simple);

    // Identifiers
    TEST_RUN("lexer_identifier_underscore", test_lexer_identifier_underscore);
    TEST_RUN("lexer_identifier_with_numbers", test_lexer_identifier_with_numbers);
    TEST_RUN("lexer_keyword_like_identifiers", test_lexer_keyword_like_identifiers);

    // Operators
    TEST_RUN("lexer_compound_operators", test_lexer_compound_operators);
    TEST_RUN("lexer_assignment_operators", test_lexer_assignment_operators);
    TEST_RUN("lexer_arrow_operators", test_lexer_arrow_operators);

    // Comments
    TEST_RUN("lexer_single_line_comment", test_lexer_single_line_comment);
    TEST_RUN("lexer_multi_line_comment", test_lexer_multi_line_comment);
    TEST_RUN("lexer_nested_comment_content", test_lexer_nested_comment_content);

    // Whitespace
    TEST_RUN("lexer_multiple_spaces", test_lexer_multiple_spaces);
    TEST_RUN("lexer_tabs_and_newlines", test_lexer_tabs_and_newlines);
    TEST_RUN("lexer_line_tracking", test_lexer_line_tracking);

    // Token sequences
    TEST_RUN("lexer_no_space_between_tokens", test_lexer_no_space_between_tokens);
    TEST_RUN("lexer_parentheses_sequence", test_lexer_parentheses_sequence);
    TEST_RUN("lexer_bracket_types", test_lexer_bracket_types);
}
