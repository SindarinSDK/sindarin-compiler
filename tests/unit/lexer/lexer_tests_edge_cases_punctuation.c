// tests/unit/lexer/lexer_tests_edge_cases_punctuation.c
// Punctuation and Comment Tests

/* ============================================================================
 * Punctuation Tests
 * ============================================================================ */

static void test_lex_punc_lparen(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "(");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_rparen(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ")");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_PAREN);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_lbracket(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "[");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACKET);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_rbracket(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "]");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_BRACKET);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_comma(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ",");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COMMA);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_dot(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ".");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOT);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_punc_colon(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ":");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_arrow(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "=>");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ARROW);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_range(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "..");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RANGE);

    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Comment Tests
 * ============================================================================ */

static void test_lex_comment_skip(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "# comment\nx");

    /* Skip newline after comment */
    Token tok = lexer_scan_token(&lexer);
    if (tok.type == TOKEN_NEWLINE) {
        tok = lexer_scan_token(&lexer);
    }
    assert(tok.type == TOKEN_IDENTIFIER);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_comment_at_eof(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x # comment");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}
