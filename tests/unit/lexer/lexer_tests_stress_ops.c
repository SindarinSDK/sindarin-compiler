// tests/unit/lexer/lexer_tests_stress_ops.c
// Lexer stress tests - operators and keywords

#include "../test_harness.h"

/* ============================================================================
 * Operator Combinations
 * ============================================================================ */

static void test_lexer_comparison_operators(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "== != < > <= >=";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BANG_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LESS);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LESS_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER_EQUAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_arithmetic_operators(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "+ - * / %";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SLASH);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MODULO);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_stress_assign_ops(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "= += -= *= /= %=";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STAR_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_SLASH_EQUAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MOD_EQUAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_increment_decrement(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "++ --";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_PLUS_PLUS);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MINUS_MINUS);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_stress_range_op(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "0..10";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOT_DOT);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Keyword Tests
 * ============================================================================ */

static void test_lexer_all_type_keywords(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "int long double str bool char byte void";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LONG);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_DOUBLE);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STR);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BOOL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BYTE);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_VOID);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_control_keywords(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "if else while for return break continue";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_ELSE);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_WHILE);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FOR);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_RETURN);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_BREAK);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CONTINUE);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_lexer_stress_ops_main(void)
{
    TEST_SECTION("Lexer Stress - Operators");
    TEST_RUN("lexer_comparison_operators", test_lexer_comparison_operators);
    TEST_RUN("lexer_arithmetic_operators", test_lexer_arithmetic_operators);
    TEST_RUN("lexer_assignment_operators", test_lexer_stress_assign_ops);
    TEST_RUN("lexer_increment_decrement", test_lexer_increment_decrement);
    TEST_RUN("lexer_range_operator", test_lexer_stress_range_op);

    TEST_SECTION("Lexer Stress - Keywords");
    TEST_RUN("lexer_all_type_keywords", test_lexer_all_type_keywords);
    TEST_RUN("lexer_control_keywords", test_lexer_control_keywords);
}
