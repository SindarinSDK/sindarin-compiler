#ifndef TYPE_CHECKER_EXPR_CALL_INT_H
#define TYPE_CHECKER_EXPR_CALL_INT_H

#include "ast.h"
#include "symbol_table.h"

/* Type check int methods
 * Handles: toDouble, toLong, toUint, toByte, toChar
 */
Type *type_check_int_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_INT_H */
