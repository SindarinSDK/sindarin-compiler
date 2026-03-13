#ifndef TYPE_CHECKER_EXPR_CALL_UINT_H
#define TYPE_CHECKER_EXPR_CALL_UINT_H

#include "ast.h"
#include "symbol_table.h"

/* Type check uint methods
 * Handles: toInt, toLong, toDouble
 */
Type *type_check_uint_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_UINT_H */
