// tests/lexer_tests_edge_cases.c
// Edge case tests for the lexer

#include "../test_harness.h"

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static void init_lexer_test(Arena *arena, Lexer *lexer, const char *source)
{
    arena_init(arena, 4096);
    lexer_init(arena, lexer, source, "test.sn");
}

static void cleanup_lexer_test(Arena *arena, Lexer *lexer)
{
    lexer_cleanup(lexer);
    arena_free(arena);
}

/* ============================================================================
 * Empty Input Tests
 * ============================================================================ */

static void test_lex_empty_string(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_whitespace_only(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "   \t   ");

    Token tok = lexer_scan_token(&lexer);
    /* Should get EOF or newline after whitespace */
    assert(tok.type == TOKEN_EOF || tok.type == TOKEN_NEWLINE);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_newlines_only(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\n\n\n");

    /* Skip newlines */
    Token tok;
    do {
        tok = lexer_scan_token(&lexer);
    } while (tok.type == TOKEN_NEWLINE);

    assert(tok.type == TOKEN_EOF);

    cleanup_lexer_test(&arena, &lexer);
}

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

/* ============================================================================
 * String Literal Tests
 * ============================================================================ */

static void test_lex_string_empty(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "hello") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_with_spaces(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello world\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strcmp(tok.literal.string_value, "hello world") == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_n(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\\nworld\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\n") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_t(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"hello\\tworld\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\t") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_backslash(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"path\\\\file\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\\") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_string_escape_quote(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "\"say \\\"hello\\\"\"");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_STRING_LITERAL);
    assert(strstr(tok.literal.string_value, "\"") != NULL);

    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Char Literal Tests
 * ============================================================================ */

static void test_lex_char_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'a'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == 'a');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_digit(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'5'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '5');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_escape_n(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'\\n'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '\n');

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_char_escape_t(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "'\\t'");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_CHAR_LITERAL);
    assert(tok.literal.char_value == '\t');

    cleanup_lexer_test(&arena, &lexer);
}

/* ============================================================================
 * Identifier Tests
 * ============================================================================ */

static void test_lex_identifier_simple(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(strncmp(tok.start, "foo", tok.length) == 0);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_with_underscore(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo_bar");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 7);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_with_numbers(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "foo123");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 6);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_underscore_prefix(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "_private");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);

    cleanup_lexer_test(&arena, &lexer);
}

static void test_lex_identifier_single_char(void)
{
    Arena arena;
    Lexer lexer;
    init_lexer_test(&arena, &lexer, "x");

    Token tok = lexer_scan_token(&lexer);
    assert(tok.type == TOKEN_IDENTIFIER);
    assert(tok.length == 1);

    cleanup_lexer_test(&arena, &lexer);
}

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

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_lexer_edge_cases_main(void)
{
    TEST_SECTION("Lexer - Empty Input");
    TEST_RUN("empty_string", test_lex_empty_string);
    TEST_RUN("whitespace_only", test_lex_whitespace_only);
    TEST_RUN("newlines_only", test_lex_newlines_only);

    TEST_SECTION("Lexer - Number Literals");
    TEST_RUN("int_zero", test_lex_int_zero);
    TEST_RUN("int_max", test_lex_int_max);
    TEST_RUN("double_zero", test_lex_double_zero);
    TEST_RUN("double_small", test_lex_double_small);
    TEST_RUN("double_large", test_lex_double_large);
    TEST_RUN("multiple_numbers", test_lex_multiple_numbers);

    TEST_SECTION("Lexer - String Literals");
    TEST_RUN("string_empty", test_lex_string_empty);
    TEST_RUN("string_simple", test_lex_string_simple);
    TEST_RUN("string_with_spaces", test_lex_string_with_spaces);
    TEST_RUN("string_escape_n", test_lex_string_escape_n);
    TEST_RUN("string_escape_t", test_lex_string_escape_t);
    TEST_RUN("string_escape_backslash", test_lex_string_escape_backslash);
    TEST_RUN("string_escape_quote", test_lex_string_escape_quote);

    TEST_SECTION("Lexer - Char Literals");
    TEST_RUN("char_simple", test_lex_char_simple);
    TEST_RUN("char_digit", test_lex_char_digit);
    TEST_RUN("char_escape_n", test_lex_char_escape_n);
    TEST_RUN("char_escape_t", test_lex_char_escape_t);

    TEST_SECTION("Lexer - Identifiers");
    TEST_RUN("identifier_simple", test_lex_identifier_simple);
    TEST_RUN("identifier_with_underscore", test_lex_identifier_with_underscore);
    TEST_RUN("identifier_with_numbers", test_lex_identifier_with_numbers);
    TEST_RUN("identifier_underscore_prefix", test_lex_identifier_underscore_prefix);
    TEST_RUN("identifier_single_char", test_lex_identifier_single_char);

    TEST_SECTION("Lexer - Keywords");
    TEST_RUN("keyword_var", test_lex_keyword_var);
    TEST_RUN("keyword_fn", test_lex_keyword_fn);
    TEST_RUN("keyword_if", test_lex_keyword_if);
    TEST_RUN("keyword_else", test_lex_keyword_else);
    TEST_RUN("keyword_while", test_lex_keyword_while);
    TEST_RUN("keyword_for", test_lex_keyword_for);
    TEST_RUN("keyword_return", test_lex_keyword_return);
    TEST_RUN("keyword_break", test_lex_keyword_break);
    TEST_RUN("keyword_continue", test_lex_keyword_continue);
    TEST_RUN("keyword_true", test_lex_keyword_true);
    TEST_RUN("keyword_false", test_lex_keyword_false);
    TEST_RUN("keyword_nil", test_lex_keyword_nil);
    TEST_RUN("keyword_match", test_lex_keyword_match);
    TEST_RUN("keyword_native", test_lex_keyword_native);
    TEST_RUN("keyword_import", test_lex_keyword_import);

    TEST_SECTION("Lexer - Operators");
    TEST_RUN("op_plus", test_lex_op_plus);
    TEST_RUN("op_minus", test_lex_op_minus);
    TEST_RUN("op_star", test_lex_op_star);
    TEST_RUN("op_slash", test_lex_op_slash);
    TEST_RUN("op_percent", test_lex_op_percent);
    TEST_RUN("op_eq", test_lex_op_eq);
    TEST_RUN("op_eq_eq", test_lex_op_eq_eq);
    TEST_RUN("op_not_eq", test_lex_op_not_eq);
    TEST_RUN("op_less", test_lex_op_less);
    TEST_RUN("op_less_eq", test_lex_op_less_eq);
    TEST_RUN("op_greater", test_lex_op_greater);
    TEST_RUN("op_greater_eq", test_lex_op_greater_eq);
    TEST_RUN("op_plus_eq", test_lex_op_plus_eq);
    TEST_RUN("op_minus_eq", test_lex_op_minus_eq);
    TEST_RUN("op_star_eq", test_lex_op_star_eq);
    TEST_RUN("op_slash_eq", test_lex_op_slash_eq);

    TEST_SECTION("Lexer - Punctuation");
    TEST_RUN("punc_lparen", test_lex_punc_lparen);
    TEST_RUN("punc_rparen", test_lex_punc_rparen);
    TEST_RUN("punc_lbracket", test_lex_punc_lbracket);
    TEST_RUN("punc_rbracket", test_lex_punc_rbracket);
    TEST_RUN("punc_comma", test_lex_punc_comma);
    TEST_RUN("punc_dot", test_lex_punc_dot);
    TEST_RUN("punc_colon", test_lex_punc_colon);
    TEST_RUN("arrow", test_lex_arrow);
    TEST_RUN("range", test_lex_range);

    TEST_SECTION("Lexer - Comments");
    TEST_RUN("comment_skip", test_lex_comment_skip);
    TEST_RUN("comment_at_eof", test_lex_comment_at_eof);

    TEST_SECTION("Lexer - Token Sequences");
    TEST_RUN("var_decl_sequence", test_lex_var_decl_sequence);
    TEST_RUN("function_header_sequence", test_lex_function_header_sequence);
    TEST_RUN("expression_sequence", test_lex_expression_sequence);

    TEST_SECTION("Lexer - Type Keywords");
    TEST_RUN("type_int", test_lex_type_int);
    TEST_RUN("type_long", test_lex_type_long);
    TEST_RUN("type_double", test_lex_type_double);
    TEST_RUN("type_str", test_lex_type_str);
    TEST_RUN("type_char", test_lex_type_char);
    TEST_RUN("type_bool", test_lex_type_bool);
    TEST_RUN("type_void", test_lex_type_void);
    TEST_RUN("type_byte", test_lex_type_byte);
}
