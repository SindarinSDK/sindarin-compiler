#ifndef TYPE_CHECKER_EXPR_STRUCT_H
#define TYPE_CHECKER_EXPR_STRUCT_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Struct Expression Type Checking
 * ============================================================================
 * Type checking for struct literal initialization and sizeof expressions.
 * ============================================================================ */

/* Struct literal type checking (StructName { field1: value1, ... })
 * Verifies all required fields are provided and types match.
 * Applies default values for optional fields.
 * Returns struct type or NULL on error.
 */
Type *type_check_struct_literal(Expr *expr, SymbolTable *table);

/* Sizeof expression type checking (sizeof(Type) or sizeof(expr))
 * Returns int type (size in bytes).
 */
Type *type_check_sizeof(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_STRUCT_H */
