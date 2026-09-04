// tests/unit/lexer/lexer_tests_edge_cases_strings.c
// String and Char Literal Tests

/* ============================================================================
 * String Literal Tests
 * ============================================================================ */

static void test_lex_string_empty(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "hello") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_with_spaces(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello world\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "hello world") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_n(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\\nworld\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\n") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_t(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\\tworld\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\t") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_backslash(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"path\\\\file\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\\") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_quote(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"say \\\"hello\\\"\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\"") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_brace_open(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"{\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "{") == 0);

    Token eof = lexer_scan_token(&lexer);
    assert(eof.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_brace_close(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"}\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "}") == 0);

    Token eof = lexer_scan_token(&lexer);
    assert(eof.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_brace_contents(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"{a}\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "{a}") == 0);

    Token eof = lexer_scan_token(&lexer);
    assert(eof.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_brace_open_close_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"{\" \"}\"");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(strcmp(t1.literal.string_value, "{") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_STRING_LITERAL);
    assert(strcmp(t2.literal.string_value, "}") == 0);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_brace_with_ident(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"{\" x \"}\"");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_STRING_LITERAL);
    assert(strcmp(t1.literal.string_value, "{") == 0);

    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_IDENTIFIER);

    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_STRING_LITERAL);
    assert(strcmp(t3.literal.string_value, "}") == 0);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escaped_quote_solo(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"\\\"\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "\"") == 0);

    Token eof = lexer_scan_token(&lexer);
    assert(eof.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void assert_lexed_string_bytes(const char *source, const unsigned char *expected,
                                      size_t expected_length)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, source);

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strlen(tok.literal.string_value) == expected_length);
    assert(memcmp(tok.literal.string_value, expected, expected_length) == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void assert_invalid_utf8_string(const char *source)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, source);

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ERROR);
    assert(strcmp(tok.start, "Invalid UTF-8 sequence in string literal") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_utf8_boundaries(void)
{
    const char *source =
        "\"ASCII:"
        "\\xC2\\x80\\xDF\\xBF"
        "\\xE0\\xA0\\x80\\xED\\x9F\\xBF\\xEE\\x80\\x80\\xEF\\xBF\\xBF"
        "\\xF0\\x90\\x80\\x80\\xF4\\x8F\\xBF\\xBF\"";
    const unsigned char expected[] = {
        'A', 'S', 'C', 'I', 'I', ':',
        0xc2, 0x80, 0xdf, 0xbf,
        0xe0, 0xa0, 0x80, 0xed, 0x9f, 0xbf,
        0xee, 0x80, 0x80, 0xef, 0xbf, 0xbf,
        0xf0, 0x90, 0x80, 0x80, 0xf4, 0x8f, 0xbf, 0xbf
    };
    assert_lexed_string_bytes(source, expected, sizeof(expected));
}

static void test_lex_string_utf8_raw_and_mixed(void)
{
    const char raw_source[] = {'"', (char)0xc2, (char)0x80, (char)0xf4,
        (char)0x8f, (char)0xbf, (char)0xbf, '"', '\0'};
    const unsigned char raw_expected[] = {0xc2, 0x80, 0xf4, 0x8f, 0xbf, 0xbf};
    assert_lexed_string_bytes(raw_source, raw_expected, sizeof(raw_expected));

    const char mixed_source[] = {'"', (char)0xc3, (char)0xa9, '-', '\\', 'x',
        'E', '4', '\\', 'x', 'B', '8', '\\', 'x', '9', '6', '"', '\0'};
    const unsigned char mixed_expected[] = {0xc3, 0xa9, '-', 0xe4, 0xb8, 0x96};
    assert_lexed_string_bytes(mixed_source, mixed_expected, sizeof(mixed_expected));
}

static void test_lex_string_control_and_hex_case(void)
{
    const unsigned char expected[] = {'X', 0x1f, 'A', 'b', '0', '9', 'Y',
        '\n', '\t', '\r', '"', '\\'};
    assert_lexed_string_bytes("\"X\\x1FAb09Y\\n\\t\\r\\\"\\\\\"", expected,
        sizeof(expected));

    const unsigned char lower_expected[] = {'x', 0x1f, 'a', 'y'};
    assert_lexed_string_bytes("\"x\\x1fay\"", lower_expected, sizeof(lower_expected));
}

static void test_lex_string_invalid_utf8_classes(void)
{
    const char *invalid_sources[] = {
        "\"\\x80\"", "\"\\xAF\"", "\"\\xaf\"", "\"\\xBF\"",
        "\"\\xC0\\x80\"", "\"\\xC1\\xBF\"",
        "\"\\xC2\"", "\"\\xE0\\xA0\"", "\"\\xF0\\x90\\x80\"",
        "\"\\xC2A\"", "\"\\xE1\\x80A\"",
        "\"\\xE0\\x80\\x80\"", "\"\\xED\\xA0\\x80\"",
        "\"\\xF0\\x80\\x80\\x80\"", "\"\\xF4\\x90\\x80\\x80\"",
        "\"\\xF5\\x80\\x80\\x80\"", "\"\\xF6\"", "\"\\xF7\"",
        "\"\\xF8\"", "\"\\xF9\"", "\"\\xFA\"", "\"\\xFB\"",
        "\"\\xFC\"", "\"\\xFD\"", "\"\\xFE\"", "\"\\xFF\""
    };
    size_t count = sizeof(invalid_sources) / sizeof(invalid_sources[0]);
    for (size_t i = 0; i < count; i++) assert_invalid_utf8_string(invalid_sources[i]);
}

static void test_lex_string_invalid_utf8_forms(void)
{
    const char raw_source[] = {'"', (char)0xaf, '"', '\0'};
    assert_invalid_utf8_string(raw_source);
    assert_invalid_utf8_string("$\"\\xAF\"");

    const char pipe_source[] = {'|', '\n', ' ', ' ', (char)0xaf, '\n', '\0'};
    assert_invalid_utf8_string(pipe_source);
    const char interpolated_pipe_source[] = {'$', '|', '\n', ' ', ' ', (char)0xaf,
        '\n', '\0'};
    assert_invalid_utf8_string(interpolated_pipe_source);
}

static void test_lex_string_utf8_valid_forms(void)
{
    Arena arena;
    Lexer lexer;
    const unsigned char expected[] = {0xc2, 0x80};
    init_lexer_test(&arena, &lexer, "$\"\\xC2\\x80\"");
    Token interpolated = lexer_scan_token(&lexer);
    assert(interpolated.type == TOKEN_INTERPOL_STRING);
    assert(strlen(interpolated.literal.string_value) == sizeof(expected));
    assert(memcmp(interpolated.literal.string_value, expected, sizeof(expected)) == 0);
    cleanup_lexer_test(&arena, &lexer);

    const char pipe_source[] = {'|', '\n', ' ', ' ', (char)0xc2, (char)0x80,
        '\n', '\0'};
    const unsigned char pipe_expected[] = {0xc2, 0x80, '\n'};
    assert_lexed_string_bytes(pipe_source, pipe_expected, sizeof(pipe_expected));

    const char interpolated_pipe_source[] = {'$', '|', '\n', ' ', ' ', (char)0xc2,
        (char)0x80, '\n', '\0'};
    init_lexer_test(&arena, &lexer, interpolated_pipe_source);
    Token interpolated_pipe = lexer_scan_token(&lexer);
    assert(interpolated_pipe.type == TOKEN_INTERPOL_STRING);
    assert(strlen(interpolated_pipe.literal.string_value) == sizeof(pipe_expected));
    assert(memcmp(interpolated_pipe.literal.string_value, pipe_expected,
        sizeof(pipe_expected)) == 0);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_utf8_diagnostic_precedence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"\\xG0\"");
    Token malformed = lexer_scan_token(&lexer);
    assert(malformed.type == TOKEN_ERROR);
    assert(strcmp(malformed.start, "Invalid hex digit in escape") == 0);
    cleanup_lexer_test(&arena, &lexer);

    init_lexer_test(&arena, &lexer, "\"\\xAF");
    Token unterminated = lexer_scan_token(&lexer);
    assert(unterminated.type == TOKEN_ERROR);
    assert(strstr(unterminated.start, "Unterminated string") != NULL);
    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_embedded_nul_existing_limitation(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"A\\0B\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "A") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Char Literal Tests
 * ============================================================================ */

static void test_lex_char_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'a'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == 'a');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_digit(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'5'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '5');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_escape_n(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'\\n'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '\n');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_escape_t(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'\\t'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '\t');

    cleanup_lexer_test(&arena, &lexer);
}
