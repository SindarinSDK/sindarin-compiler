// tests/lexer_tests_operator.c
// Operator and punctuation lexer tests

#include "../test_harness.h"

static void test_lexer_operators_single(void)
{
    DEBUG_INFO("Starting test_lexer_operators_single");

    const char *source = "+ - * / %";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_PLUS);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_MINUS);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_STAR);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_SLASH);
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_MODULO);

    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_operators_single");
}

static void test_lexer_operators_compound(void)
{
    DEBUG_INFO("Starting test_lexer_operators_compound");

    const char *source = "== != <= >= ++ -- =>";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_EQUAL_EQUAL);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_BANG_EQUAL);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_LESS_EQUAL);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_GREATER_EQUAL);
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_PLUS_PLUS);
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_MINUS_MINUS);
    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_ARROW); // =>

    Token t8 = lexer_scan_token(&lexer);
    assert(t8.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_operators_compound");
}

static void test_lexer_operators_logical(void)
{
    DEBUG_INFO("Starting test_lexer_operators_logical");

    const char *source = "&& || !";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_AND);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_OR);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_BANG);

    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_operators_logical");
}

static void test_lexer_brackets_parens_braces(void)
{
    DEBUG_INFO("Starting test_lexer_brackets_parens_braces");

    const char *source = "() [] {}";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_LEFT_PAREN);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_RIGHT_PAREN);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_LEFT_BRACKET);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_RIGHT_BRACKET);
    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_LEFT_BRACE);
    Token t6 = lexer_scan_token(&lexer);
    assert(t6.type == TOKEN_RIGHT_BRACE);

    Token t7 = lexer_scan_token(&lexer);
    assert(t7.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_brackets_parens_braces");
}

static void test_lexer_punctuation(void)
{
    DEBUG_INFO("Starting test_lexer_punctuation");

    const char *source = "; : , .";
    Arena arena;
    arena_init(&arena, 1024);
    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Token t1 = lexer_scan_token(&lexer);
    assert(t1.type == TOKEN_SEMICOLON);
    Token t2 = lexer_scan_token(&lexer);
    assert(t2.type == TOKEN_COLON);
    Token t3 = lexer_scan_token(&lexer);
    assert(t3.type == TOKEN_COMMA);
    Token t4 = lexer_scan_token(&lexer);
    assert(t4.type == TOKEN_DOT);

    Token t5 = lexer_scan_token(&lexer);
    assert(t5.type == TOKEN_EOF);

    lexer_cleanup(&lexer);
    arena_free(&arena);

    DEBUG_INFO("Finished test_lexer_punctuation");
}


void test_lexer_operator_main(void)
{
    TEST_SECTION("Lexer Operator Tests");
    TEST_RUN("lexer_operators_single", test_lexer_operators_single);
    TEST_RUN("lexer_operators_compound", test_lexer_operators_compound);
    TEST_RUN("lexer_operators_logical", test_lexer_operators_logical);
    TEST_RUN("lexer_brackets_parens_braces", test_lexer_brackets_parens_braces);
    TEST_RUN("lexer_punctuation", test_lexer_punctuation);
}
