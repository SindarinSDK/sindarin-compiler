#ifndef TYPE_CHECKER_EXPR_ACCESS_H
#define TYPE_CHECKER_EXPR_ACCESS_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Member Access Expression Type Checking (EXPR_MEMBER_ACCESS/ASSIGN)
 * ============================================================================
 * Type checking for direct struct field access (x.field) and assignment
 * (x.field = value). Different from EXPR_MEMBER which handles method calls.
 * ============================================================================ */

/* Member access type checking (object.field_name)
 * Verifies object is a struct and field exists.
 * Performs escape analysis for scope tracking.
 * Returns field type or NULL on error.
 */
Type *type_check_member_access(Expr *expr, SymbolTable *table);

/* Member assignment type checking (object.field_name = value)
 * Verifies object is a struct, field exists, and types match.
 * Performs escape analysis for scope tracking.
 * Returns field type or NULL on error.
 */
Type *type_check_member_assign(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_ACCESS_H */
