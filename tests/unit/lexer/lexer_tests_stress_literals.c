// tests/unit/lexer/lexer_tests_stress_literals.c
// Lexer stress tests - numeric, string, character literals and comments

#include "../test_harness.h"

/* ============================================================================
 * Numeric Literal Variations
 * ============================================================================ */

static void test_lexer_zero(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "0";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_negative_number(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "-42";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_decimal_number(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "3.14159";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_scientific_notation(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "1e10";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE_LITERAL || tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * String Literal Variations
 * ============================================================================ */

static void test_lexer_stress_empty_str(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "\"\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_escaped_newline_string(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "\"line1\\nline2\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_escaped_tab_string(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "\"col1\\tcol2\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_escaped_quote_string(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "\"say \\\"hello\\\"\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_stress_interp_string(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "$\"Hello {name}\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INTERP_STRING_START);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Character Literal Variations
 * ============================================================================ */

static void test_lexer_char_letter(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "'A'";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_char_digit(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "'0'";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_char_newline(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "'\\n'";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_char_tab(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "'\\t'";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Comment Tests
 * ============================================================================ */

static void test_lexer_line_comment(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "// this is a comment\n42";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    // After skipping comment and newline, should get 42
    while (tok.type == TOKEN_NEWLINE) {
        tok = lexer_scan_token(&lexer);
    }
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_multiple_comments(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "// comment 1\n// comment 2\n42";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok;
    do {
        tok = lexer_scan_token(&lexer);
    } while (tok.type == TOKEN_NEWLINE);
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_lexer_stress_literals_main(void)
{
    TEST_SECTION("Lexer Stress - Numeric Literals");
    TEST_RUN("lexer_zero", test_lexer_zero);
    TEST_RUN("lexer_negative_number", test_lexer_negative_number);
    TEST_RUN("lexer_decimal_number", test_lexer_decimal_number);
    TEST_RUN("lexer_scientific_notation", test_lexer_scientific_notation);

    TEST_SECTION("Lexer Stress - String Literals");
    TEST_RUN("lexer_empty_string", test_lexer_stress_empty_str);
    TEST_RUN("lexer_escaped_newline_string", test_lexer_escaped_newline_string);
    TEST_RUN("lexer_escaped_tab_string", test_lexer_escaped_tab_string);
    TEST_RUN("lexer_escaped_quote_string", test_lexer_escaped_quote_string);
    TEST_RUN("lexer_interpolated_string", test_lexer_stress_interp_string);

    TEST_SECTION("Lexer Stress - Character Literals");
    TEST_RUN("lexer_char_letter", test_lexer_char_letter);
    TEST_RUN("lexer_char_digit", test_lexer_char_digit);
    TEST_RUN("lexer_char_newline", test_lexer_char_newline);
    TEST_RUN("lexer_char_tab", test_lexer_char_tab);

    TEST_SECTION("Lexer Stress - Comments");
    TEST_RUN("lexer_line_comment", test_lexer_line_comment);
    TEST_RUN("lexer_multiple_comments", test_lexer_multiple_comments);
}
