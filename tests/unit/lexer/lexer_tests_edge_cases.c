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

/* Include split test modules */
#include "lexer_tests_edge_cases_empty.c"
#include "lexer_tests_edge_cases_numbers.c"
#include "lexer_tests_edge_cases_strings.c"
#include "lexer_tests_edge_cases_identifiers.c"
#include "lexer_tests_edge_cases_keywords.c"
#include "lexer_tests_edge_cases_operators.c"
#include "lexer_tests_edge_cases_punctuation.c"
#include "lexer_tests_edge_cases_sequence.c"

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
