#ifndef PARSER_EXPR_H
#define PARSER_EXPR_H

#include "parser.h"
#include "ast.h"

/* Expression parsing functions */
Expr *parser_expression(Parser *parser);
Expr *parser_assignment(Parser *parser);
Expr *parser_logical_or(Parser *parser);
Expr *parser_logical_and(Parser *parser);
Expr *parser_equality(Parser *parser);
Expr *parser_comparison(Parser *parser);
Expr *parser_range(Parser *parser);
Expr *parser_term(Parser *parser);
Expr *parser_factor(Parser *parser);
Expr *parser_unary(Parser *parser);
Expr *parser_postfix(Parser *parser);
Expr *parser_primary(Parser *parser);
Expr *parser_call(Parser *parser, Expr *callee);
Expr *parser_array_access(Parser *parser, Expr *array);
Expr *parser_multi_line_expression(Parser *parser);

#endif /* PARSER_EXPR_H */
