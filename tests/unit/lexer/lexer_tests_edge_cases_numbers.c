// tests/unit/lexer/lexer_tests_edge_cases_numbers.c
// Number Literal Tests

/* ============================================================================
 * Number Literal Tests
 * ============================================================================ */

static void test_lex_int_zero(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "0");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    assert(tok.literal.int_value == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_int_max(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "9223372036854775807");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_double_zero(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "0.0");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);
    assert(tok.literal.double_value == 0.0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_double_small(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "0.001");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_double_large(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "123456.789");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_multiple_numbers(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "1 2 3 4 5");

    for (int i = 1; i <= 5; i++) {
        Token tok = lexer_scan_token(&lexer);
        assert(tok.type == TOKEN_INT_LITERAL);
        assert(tok.literal.int_value == i);
    }

    cleanup_lexer_test(&arena, &lexer);
}
