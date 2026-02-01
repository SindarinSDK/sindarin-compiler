#ifndef TYPE_CHECKER_EXPR_CAST_H
#define TYPE_CHECKER_EXPR_CAST_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Cast/Conversion Expression Type Checking
 * ============================================================================
 * Type checking for type conversion and introspection expressions:
 * as val, as ref, typeof, is, and as Type.
 * ============================================================================ */

/* 'as val' expression type checking (ptr as val)
 * Dereferences pointers, converts *char to str, deep-copies structs.
 * Returns the dereferenced/converted type or NULL on error.
 */
Type *type_check_as_val(Expr *expr, SymbolTable *table);

/* 'as ref' expression type checking (value as ref)
 * Gets a pointer to a value (only in native function context).
 * Returns pointer type or NULL on error.
 */
Type *type_check_as_ref(Expr *expr, SymbolTable *table);

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
