// tests/unit/lexer/lexer_tests_edge_cases_operators.c
// Operator Tests

/* ============================================================================
 * Operator Tests
 * ============================================================================ */

static void test_lex_op_plus(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "+");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_minus(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "-");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_star(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "*");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_slash(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "/");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SLASH);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_percent(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "%");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PERCENT);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_eq_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "==");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_not_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "!=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BANG_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_less(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "<");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LESS);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_less_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "<=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LESS_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_greater(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ">");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_greater_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, ">=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_plus_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "+=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_minus_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "-=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_star_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "*=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_op_slash_eq(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "/=");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SLASH_EQUAL);

    cleanup_lexer_test(&arena, &lexer);
}
