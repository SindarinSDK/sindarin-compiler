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
