#ifndef LEXER_UTIL_H
#define LEXER_UTIL_H

#include "lexer.h"

/* Initialization and cleanup */
void lexer_init(Arena *arena, Lexer *lexer, const char *source, const char *filename);
void lexer_cleanup(Lexer *lexer);
void lexer_report_indentation_error(Lexer *lexer, int expected, int actual);

/* Character navigation */
int lexer_is_at_end(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);
char lexer_peek_next(Lexer *lexer);
int lexer_match(Lexer *lexer, char expected);

/* Token creation */
Token lexer_make_token(Lexer *lexer, SnTokenType type);
Token lexer_error_token(Lexer *lexer, const char *message);

/* Whitespace handling */
void lexer_skip_whitespace(Lexer *lexer);

#endif /* LEXER_UTIL_H */
