#ifndef TYPE_CHECKER_EXPR_CALL_BOOL_H
#define TYPE_CHECKER_EXPR_CALL_BOOL_H

#include "ast.h"
#include "symbol_table.h"

/* Type check bool methods
 * Handles: toInt
 */
Type *type_check_bool_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_BOOL_H */
