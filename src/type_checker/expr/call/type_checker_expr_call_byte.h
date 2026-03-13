#ifndef TYPE_CHECKER_EXPR_CALL_BYTE_H
#define TYPE_CHECKER_EXPR_CALL_BYTE_H

#include "ast.h"
#include "symbol_table.h"

/* Type check byte methods
 * Handles: toInt, toChar
 */
Type *type_check_byte_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_BYTE_H */
