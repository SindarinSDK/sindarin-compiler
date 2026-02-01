#ifndef PARSER_EXPR_INTERPOL_H
#define PARSER_EXPR_INTERPOL_H

#include "parser.h"
#include "ast.h"

/* Parse an interpolated string expression.
 * Called from parser_primary when TOKEN_INTERPOL_STRING is matched.
 * The interpol_token is the already-consumed interpolated string token. */
Expr *parse_interpol_string(Parser *parser, Token *interpol_token);

#endif /* PARSER_EXPR_INTERPOL_H */
