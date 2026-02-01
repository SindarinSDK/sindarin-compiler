#ifndef TYPE_CHECKER_EXPR_MISC_H
#define TYPE_CHECKER_EXPR_MISC_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Miscellaneous Expression Type Checking
 * ============================================================================
 * Type checking for compound assignment and match expressions.
 * ============================================================================ */

/* Compound assignment type checking (x += value, x -= value, etc.)
 * Verifies target is an lvalue and types are compatible.
 * Returns target type or NULL on error.
 */
Type *type_check_compound_assign(Expr *expr, SymbolTable *table);

/* Match expression type checking
 * Verifies all patterns match subject type and arm bodies have consistent types.
 * Returns common arm type if all arms match, void otherwise.
 */
Type *type_check_match(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_MISC_H */
