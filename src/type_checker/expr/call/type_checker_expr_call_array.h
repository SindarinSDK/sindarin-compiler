#ifndef TYPE_CHECKER_EXPR_CALL_ARRAY_H
#define TYPE_CHECKER_EXPR_CALL_ARRAY_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Array Method Type Checking
 * ============================================================================
 * Type checking for array method access (not calls).
 * Returns the function type for the method, or NULL if not an array method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check array methods
 * Handles: length, push, pop, clear, concat, indexOf, contains, clone,
 *          join, reverse, insert, remove
 * Byte array specific: toString, toStringLatin1, toHex, toBase64
 */
Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_ARRAY_H */
