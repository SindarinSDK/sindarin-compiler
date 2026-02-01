// tests/unit/lexer/lexer_tests_edge_cases_empty.c
// Empty Input Tests

/* ============================================================================
 * Empty Input Tests
 * ============================================================================ */

static void test_lex_empty_string(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_whitespace_only(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "   \t   ");

    Token tok = lexer_scan_token(&lexer);
    /* Should get EOF or newline after whitespace */
    assert(tok.type == TOKEN_EOF || tok.type == TOKEN_NEWLINE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_newlines_only(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\n\n\n");

    /* Skip newlines */
    Token tok;
    do {
        tok = lexer_scan_token(&lexer);
    } while (tok.type == TOKEN_NEWLINE);

    assert(tok.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}
