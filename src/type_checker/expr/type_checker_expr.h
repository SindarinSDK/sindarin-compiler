#ifndef TYPE_CHECKER_EXPR_H
#define TYPE_CHECKER_EXPR_H

#include "ast.h"
#include "symbol_table.h"

/* Expression type checking */
Type *type_check_expr(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_H */
