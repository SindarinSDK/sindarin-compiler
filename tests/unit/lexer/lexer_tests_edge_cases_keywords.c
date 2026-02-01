// tests/unit/lexer/lexer_tests_edge_cases_keywords.c
// Keyword Tests

/* ============================================================================
 * Keyword Tests
 * ============================================================================ */

static void test_lex_keyword_var(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_fn(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "fn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FN);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_if(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "if");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_else(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "else");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ELSE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_while(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "while");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_WHILE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_for(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "for");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FOR);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_return(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "return");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RETURN);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_break(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "break");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BREAK);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_continue(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "continue");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CONTINUE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_true(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "true");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BOOL_LITERAL);
    assert(tok.literal.bool_value == 1);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_false(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "false");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BOOL_LITERAL);
    assert(tok.literal.bool_value == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_nil(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "nil");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NIL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_match(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "match");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MATCH);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_native(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "native");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_NATIVE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_keyword_import(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "import");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IMPORT);

    cleanup_lexer_test(&arena, &lexer);
}
