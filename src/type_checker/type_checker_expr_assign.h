#ifndef TYPE_CHECKER_EXPR_ASSIGN_H
#define TYPE_CHECKER_EXPR_ASSIGN_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Assignment Expression Type Checking
 * ============================================================================
 * Type checking for variable assignment, index assignment, and increment/
 * decrement operations. Includes escape analysis helpers.
 * ============================================================================ */

/* Helper to get the scope depth from an expression.
 * For EXPR_VARIABLE: returns the symbol's declaration_scope_depth
 * For EXPR_MEMBER_ACCESS: returns the already-computed scope_depth
 * Returns -1 if unable to determine scope depth.
 */
int get_expr_scope_depth(Expr *expr, SymbolTable *table);

/* Helper to mark ALL member access nodes in a chain as escaped.
 * For outer.a.b, marks both outer.a and outer.a.b as escaped.
 */
void mark_member_access_chain_escaped(Expr *expr);

/* Helper to get the BASE (root variable) scope depth from a member access chain.
 * For outer.a.b, returns the scope depth of 'outer' (the root variable).
 */
int get_base_scope_depth(Expr *expr, SymbolTable *table);

/* Variable assignment type checking (x = value)
 * Verifies assignment compatibility and performs escape analysis.
 * Returns assigned type or NULL on error.
 */
Type *type_check_assign(Expr *expr, SymbolTable *table);

/* Index assignment type checking (arr[i] = value)
 * Verifies array/map element assignment compatibility.
 * Returns assigned type or NULL on error.
 */
Type *type_check_index_assign(Expr *expr, SymbolTable *table);

/* Increment/decrement type checking (x++, x--, ++x, --x)
 * Verifies operand is numeric.
 * Returns the numeric type.
 */
Type *type_check_increment_decrement(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_ASSIGN_H */
