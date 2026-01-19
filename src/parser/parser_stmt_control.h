#ifndef PARSER_STMT_CONTROL_H
#define PARSER_STMT_CONTROL_H

#include "parser.h"
#include "ast.h"
#include "ast/ast_stmt.h"

/* Control flow parsing functions */
Stmt *parser_return_statement(Parser *parser);
Stmt *parser_if_statement(Parser *parser);
Stmt *parser_while_statement(Parser *parser, bool is_shared);
Stmt *parser_for_statement(Parser *parser, bool is_shared);

#endif /* PARSER_STMT_CONTROL_H */
