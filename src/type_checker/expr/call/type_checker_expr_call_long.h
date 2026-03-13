#ifndef TYPE_CHECKER_EXPR_CALL_LONG_H
#define TYPE_CHECKER_EXPR_CALL_LONG_H

#include "ast.h"
#include "symbol_table.h"

/* Type check long methods
 * Handles: toInt, toDouble
 */
Type *type_check_long_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_LONG_H */
