#ifndef TYPE_CHECKER_EXPR_CALL_DOUBLE_H
#define TYPE_CHECKER_EXPR_CALL_DOUBLE_H

#include "ast.h"
#include "symbol_table.h"

/* Type check double methods
 * Handles: toInt, toLong
 */
Type *type_check_double_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_DOUBLE_H */
