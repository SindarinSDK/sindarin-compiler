#ifndef TYPE_CHECKER_EXPR_CAST_H
#define TYPE_CHECKER_EXPR_CAST_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Cast/Conversion Expression Type Checking
 * ============================================================================
 * Type checking for type conversion and introspection expressions:
 * addressOf(), valueOf(), copyOf(), typeof, is, and as Type.
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

/* typeof expression type checking (typeof(value) or typeof(Type))
 * Returns int (type tag value).
 */
Type *type_check_typeof(Expr *expr, SymbolTable *table);

/* 'is' expression type checking (any_val is Type)
 * Checks if an any value is of a specific type.
 * Returns bool type.
 */
Type *type_check_is(Expr *expr, SymbolTable *table);

/* 'as Type' expression type checking (any_val as int)
 * Casts any values to concrete types, or numeric conversions.
 * Returns target type or NULL on error.
 */
Type *type_check_as_type(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CAST_H */
