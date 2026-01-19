#ifndef AST_PRINT_H
#define AST_PRINT_H

#include "ast.h"
#include "arena.h"

/* Debug printing functions */
void ast_print_stmt(Arena *arena, Stmt *stmt, int indent_level);
void ast_print_expr(Arena *arena, Expr *expr, int indent_level);

#endif /* AST_PRINT_H */
