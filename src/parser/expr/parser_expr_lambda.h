#ifndef PARSER_EXPR_LAMBDA_H
#define PARSER_EXPR_LAMBDA_H

#include "parser.h"
#include "ast.h"

/* Parse a lambda expression: fn(params) [modifier]: return_type => body
 * Called from parser_primary when TOKEN_FN is matched.
 * The fn_token is the already-consumed 'fn' keyword token. */
Expr *parse_lambda_expr(Parser *parser, Token *fn_token);

#endif /* PARSER_EXPR_LAMBDA_H */
