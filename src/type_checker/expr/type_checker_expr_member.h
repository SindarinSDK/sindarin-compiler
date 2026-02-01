#ifndef TYPE_CHECKER_EXPR_MEMBER_H
#define TYPE_CHECKER_EXPR_MEMBER_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Member Expression Type Checking (EXPR_MEMBER)
 * ============================================================================
 * Type checking for member access expressions that may involve method calls,
 * namespace access, or struct field access via the dot operator.
 * ============================================================================ */

/* Member expression type checking (object.member or object.method())
 * Handles struct field access, method resolution, namespace access.
 * Returns the member/method type or NULL on error.
 */
Type *type_check_member(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_MEMBER_H */
