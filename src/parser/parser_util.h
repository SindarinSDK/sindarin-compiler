#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "parser.h"
#include "ast.h"

/* Low-level parser utilities */
int parser_is_at_end(Parser *parser);
void skip_newlines(Parser *parser);
int skip_newlines_and_check_end(Parser *parser);

/* Error handling */
void parser_error(Parser *parser, const char *message);
void parser_error_at_current(Parser *parser, const char *message);
void parser_error_at(Parser *parser, Token *token, const char *message);

/* Token operations */
void parser_advance(Parser *parser);
void parser_consume(Parser *parser, SnTokenType type, const char *message);
int parser_check(Parser *parser, SnTokenType type);
int parser_match(Parser *parser, SnTokenType type);

/* Error recovery */
void synchronize(Parser *parser);

/* Type parsing */
Type *parser_type(Parser *parser);
ParsedType parser_type_with_size(Parser *parser);

/* Static type name checking - returns 1 if identifier could be a static type name */
int parser_is_static_type_name(const char *name, int length);

/* Method name checking - returns 1 if current token can be a method name
 * (identifier or type keyword like int, long, double, bool, byte) */
int parser_check_method_name(Parser *parser);

#endif /* PARSER_UTIL_H */
