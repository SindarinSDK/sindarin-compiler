#ifndef PARSER_STMT_DECL_H
#define PARSER_STMT_DECL_H

#include "parser.h"
#include "ast.h"
#include "ast/ast_stmt.h"

/* Declaration parsing functions */
Stmt *parser_var_declaration(Parser *parser);
Stmt *parser_function_declaration(Parser *parser);
Stmt *parser_native_function_declaration(Parser *parser);
Stmt *parser_type_declaration(Parser *parser);
Stmt *parser_struct_declaration(Parser *parser, bool is_native);

/* Helper for native function types in type declarations */
Type *parser_native_function_type(Parser *parser);

#endif /* PARSER_STMT_DECL_H */
