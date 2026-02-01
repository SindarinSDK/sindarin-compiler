// tests/unit/lexer/lexer_tests_edge_cases_sequence.c
// Complex Token Sequence and Type Keyword Tests

/* ============================================================================
 * Complex Token Sequence Tests
 * ============================================================================ */

static void test_lex_var_decl_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "var x: int = 42");

    Token tok;
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VAR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_function_header_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "fn foo(x: int): int");

    Token tok;
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RIGHT_PAREN);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_expression_sequence(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "1 + 2 * 3");

    Token tok;
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR);
    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Type Keyword Tests
 * ============================================================================ */

static void test_lex_type_int(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "int");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_long(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "long");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LONG);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_double(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "double");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_str(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "str");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STR);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_char(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "char");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_bool(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "bool");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BOOL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_void(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "void");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VOID);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_type_byte(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "byte");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BYTE);

    cleanup_lexer_test(&arena, &lexer);
}
