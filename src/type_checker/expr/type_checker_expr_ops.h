#ifndef TYPE_CHECKER_EXPR_OPS_H
#define TYPE_CHECKER_EXPR_OPS_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Operator Expression Type Checking
 * ============================================================================
 * Type checking for binary operators, unary operators, and string
 * interpolation expressions.
 * ============================================================================ */

/* Binary operator type checking (+, -, *, /, %, ==, !=, <, >, etc.)
 * Handles numeric operations, string concatenation, comparisons.
 * Returns result type or NULL on error.
 */
Type *type_check_binary(Expr *expr, SymbolTable *table);

/* Unary operator type checking (!, -, ~, etc.)
 * Handles logical negation, numeric negation, bitwise not.
 * Returns result type or NULL on error.
 */
Type *type_check_unary(Expr *expr, SymbolTable *table);

/* String interpolation type checking ($"Hello {name}")
 * Verifies all interpolated expressions are printable.
 * Returns str type.
 */
Type *type_check_interpolated(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_OPS_H */
