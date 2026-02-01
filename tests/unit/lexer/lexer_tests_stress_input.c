// tests/unit/lexer/lexer_tests_stress_input.c
// Lexer stress tests - large inputs and token sequences

#include "../test_harness.h"

/* ============================================================================
 * Large Input Tests
 * ============================================================================ */

static void test_lexer_many_tokens(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "a b c d e f g h i j k l m n o p q r s t u v w x y z";
    lexer_init(&arena, &lexer, source, "test.sn");

    int count = 0;
    Token tok;
    while ((tok = lexer_scan_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_IDENTIFIER) count++;
    }
    assert(count == 26);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_many_numbers(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20";
    lexer_init(&arena, &lexer, source, "test.sn");

    int count = 0;
    Token tok;
    while ((tok = lexer_scan_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_INT_LITERAL) count++;
    }
    assert(count == 20);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_many_operators(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "+ - * / % == != < > <= >= = ( ) [ ] { } , : .";
    lexer_init(&arena, &lexer, source, "test.sn");

    int count = 0;
    Token tok;
    while ((tok = lexer_scan_token(&lexer)).type != TOKEN_EOF) {
        count++;
    }
    assert(count > 15);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_long_identifier(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "abcdefghijklmnopqrstuvwxyz_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 64);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_long_string(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "\"This is a very long string literal that contains many characters and words for testing purposes\"";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_large_number(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "9223372036854775807";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Token Sequence Tests
 * ============================================================================ */

static void test_lexer_function_declaration(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "fn test(x: int): int";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
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

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_var_declaration(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "var x: int = 42";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
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

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_if_statement(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "if x > 0 => print(x)";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IF);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_GREATER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FAT_ARROW);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_for_loop(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "for i in 0..10 => print(i)";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FOR);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IN);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_while_loop(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "while x > 0 => x = x - 1";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_WHILE);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_match_expression(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "match x => 0 => zero, else => other";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_MATCH);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_FAT_ARROW);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_array_literal(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "[1, 2, 3, 4, 5]";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACKET);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_INT_LITERAL);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COMMA);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

static void test_lexer_struct_literal(void)
{
    Arena arena;
    arena_init(&arena, 8192);
    Lexer lexer;
    const char *source = "Point { x: 10, y: 20 }";
    lexer_init(&arena, &lexer, source, "test.sn");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_LEFT_BRACE);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_COLON);

    lexer_cleanup(&lexer);
    arena_free(&arena);
}

/* ============================================================================
 * Test Entry Point
 * ============================================================================ */

void test_lexer_stress_input_main(void)
{
    TEST_SECTION("Lexer Stress - Large Inputs");
    TEST_RUN("lexer_many_tokens", test_lexer_many_tokens);
    TEST_RUN("lexer_many_numbers", test_lexer_many_numbers);
    TEST_RUN("lexer_many_operators", test_lexer_many_operators);
    TEST_RUN("lexer_long_identifier", test_lexer_long_identifier);
    TEST_RUN("lexer_long_string", test_lexer_long_string);
    TEST_RUN("lexer_large_number", test_lexer_large_number);

    TEST_SECTION("Lexer Stress - Token Sequences");
    TEST_RUN("lexer_function_declaration", test_lexer_function_declaration);
    TEST_RUN("lexer_var_declaration", test_lexer_var_declaration);
    TEST_RUN("lexer_if_statement", test_lexer_if_statement);
    TEST_RUN("lexer_for_loop", test_lexer_for_loop);
    TEST_RUN("lexer_while_loop", test_lexer_while_loop);
    TEST_RUN("lexer_match_expression", test_lexer_match_expression);
    TEST_RUN("lexer_array_literal", test_lexer_array_literal);
    TEST_RUN("lexer_struct_literal", test_lexer_struct_literal);
}
