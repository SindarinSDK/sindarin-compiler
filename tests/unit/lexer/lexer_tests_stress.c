// tests/unit/lexer/lexer_tests_stress.c
// Lexer stress tests - large inputs and various edge cases

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
 * Main Test Entry Point
 * ============================================================================ */

void test_lexer_stress_main(void)
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
