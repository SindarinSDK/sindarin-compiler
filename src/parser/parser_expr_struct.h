#ifndef PARSER_EXPR_STRUCT_H
#define PARSER_EXPR_STRUCT_H

#include "parser.h"
#include "ast.h"

/* Parse a struct literal expression: TypeName { field: value, ... }
 * Called from parser_primary when identifier is a struct type followed by '{'.
 * The var_token is the type name token. */
Expr *parse_struct_literal(Parser *parser, Token *var_token);

/* Parse a static method call: TypeName.method(args...)
 * Called from parser_primary when identifier is followed by '.' and is a static type.
 * The var_token is the type name token. */
Expr *parse_static_call(Parser *parser, Token *var_token);

#endif /* PARSER_EXPR_STRUCT_H */
