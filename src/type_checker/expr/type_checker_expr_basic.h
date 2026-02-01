#ifndef TYPE_CHECKER_EXPR_BASIC_H
#define TYPE_CHECKER_EXPR_BASIC_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Basic Expression Type Checking
 * ============================================================================
 * Type checking for literal values and variable references.
 * ============================================================================ */

/* Literal expression type checking (42, "hello", true, etc.)
 * Returns the type corresponding to the literal kind.
 */
Type *type_check_literal(Expr *expr, SymbolTable *table);

/* Variable expression type checking (identifier lookup)
 * Looks up the symbol and returns its declared type.
 * Returns NULL on undefined variable error.
 */
Type *type_check_variable(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_BASIC_H */
