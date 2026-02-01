#ifndef PARSER_EXPR_MATCH_H
#define PARSER_EXPR_MATCH_H

#include "parser.h"
#include "ast.h"

/* Parse a match expression.
 * Called from parser_primary when TOKEN_MATCH is matched.
 * The match_token is the already-consumed 'match' keyword token. */
Expr *parse_match_expr(Parser *parser, Token *match_token);

#endif /* PARSER_EXPR_MATCH_H */
