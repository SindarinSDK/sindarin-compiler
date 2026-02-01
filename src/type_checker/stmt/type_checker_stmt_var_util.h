#ifndef TYPE_CHECKER_STMT_VAR_UTIL_H
#define TYPE_CHECKER_STMT_VAR_UTIL_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Variable Declaration Type Checking Utilities
 * ============================================================================
 * Helper functions for variable declaration type checking.
 * ============================================================================ */

/* Apply array type coercion for variable initializers.
 * Handles int[] to byte[], int[] to int32[], etc.
 * Returns true if coercion was applied, false otherwise. */
bool apply_array_coercion(Stmt *stmt, Type *decl_type, Type **init_type_ptr);

/* Check type compatibility for variable assignment.
 * Handles any boxing, numeric promotions, and nil assignments. */
bool check_var_type_compatibility(Type *decl_type, Type *init_type, Stmt *stmt);

#endif /* TYPE_CHECKER_STMT_VAR_UTIL_H */
