#ifndef TYPE_CHECKER_EXPR_CAST_H
#define TYPE_CHECKER_EXPR_CAST_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Cast/Conversion Expression Type Checking
 * ============================================================================
 * Type checking for type conversion and introspection expressions:
 * addressOf(), valueOf(), copyOf().
 * ============================================================================ */

/* addressOf(expr) - get pointer to value (native context only).
 * Returns pointer type or NULL on error.
 */
Type *type_check_address_of(Expr *expr, SymbolTable *table);

/* valueOf(expr) - dereference pointer or convert *char to str (native context only).
 * Returns the dereferenced/converted type or NULL on error.
 */
Type *type_check_value_of(Expr *expr, SymbolTable *table);

/* copyOf(expr) - deep copy a struct value.
 * Returns the struct type or NULL on error.
 */
Type *type_check_copy_of(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CAST_H */
