#ifndef PARSER_STMT_H
#define PARSER_STMT_H

#include "parser.h"
#include "ast.h"
#include "ast/ast_stmt.h"
#include "parser/parser_stmt_decl.h"
#include "parser/parser_stmt_control.h"

/* Block/indentation helpers */
int is_at_function_boundary(Parser *parser);
Stmt *parser_indented_block(Parser *parser);

/* Statement parsing functions */
Stmt *parser_statement(Parser *parser);
Stmt *parser_declaration(Parser *parser);
Stmt *parser_block_statement(Parser *parser);
Stmt *parser_expression_statement(Parser *parser);
Stmt *parser_import_statement(Parser *parser);
Stmt *parser_pragma_statement(Parser *parser, PragmaType pragma_type);

#endif /* PARSER_STMT_H */
