#ifndef TYPE_CHECKER_EXPR_ARRAY_H
#define TYPE_CHECKER_EXPR_ARRAY_H

#include "ast.h"
#include "symbol_table.h"
#include "type_checker/type_checker_util.h"

/* ============================================================================
 * Array Expression Type Checking
 * ============================================================================
 * Type checking for array literals, array access, array slicing, range
 * expressions, spread operators, and sized array allocations.
 * This module handles all array-related type checking extracted from
 * type_checker_expr.c.
 * ============================================================================ */

/* Array literal type checking
 * Verifies all elements have the same type.
 * Returns array type with element type, or NULL on error.
 */
Type *type_check_array(Expr *expr, SymbolTable *table);

/* Array access type checking (arr[index])
 * Verifies the operand is an array and the index is numeric.
 * Returns the element type of the array, or NULL on error.
 */
Type *type_check_array_access(Expr *expr, SymbolTable *table);

/* Array slice type checking (arr[start..end])
 * Verifies the operand is an array and indices are numeric.
 * Returns the same array type (slice preserves array type).
 */
Type *type_check_array_slice(Expr *expr, SymbolTable *table);

/* Range expression type checking (start..end)
 * Verifies both start and end are numeric types.
 * Returns int[] array type.
 */
Type *type_check_range(Expr *expr, SymbolTable *table);

/* Spread operator type checking (...arr)
 * Verifies the operand is an array.
 * Returns the element type of the array (for use in array literals).
 */
Type *type_check_spread(Expr *expr, SymbolTable *table);

/* Sized array allocation type checking (int[10] or int[n] = 0)
 * Verifies size is an integer type and default value matches element type.
 * Returns array type with the specified element type.
 */
Type *type_check_sized_array_alloc(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_ARRAY_H */
