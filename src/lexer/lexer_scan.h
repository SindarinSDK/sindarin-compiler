#ifndef LEXER_SCAN_H
#define LEXER_SCAN_H

#include "lexer.h"

/* Keyword recognition */
SnTokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, SnTokenType type);
SnTokenType lexer_identifier_type(Lexer *lexer);

/* Token scanners */
Token lexer_scan_identifier(Lexer *lexer);
Token lexer_scan_number(Lexer *lexer);
Token lexer_scan_string(Lexer *lexer);
Token lexer_scan_char(Lexer *lexer);

#endif /* LEXER_SCAN_H */
