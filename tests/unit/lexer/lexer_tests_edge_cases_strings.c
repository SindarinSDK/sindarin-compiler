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
