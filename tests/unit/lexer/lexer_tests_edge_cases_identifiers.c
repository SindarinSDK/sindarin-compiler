// tests/unit/lexer/lexer_tests_edge_cases_identifiers.c
// Identifier Tests

/* ============================================================================
 * Identifier Tests
 * ============================================================================ */

static void test_lex_identifier_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(strncmp(tok.start, "foo", tok.length) == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_with_underscore(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo_bar");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 7);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_with_numbers(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo123");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 6);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_underscore_prefix(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "_private");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_single_char(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 1);

    cleanup_lexer_test(&arena, &lexer);
}
