#ifndef TYPE_CHECKER_EXPR_CALL_STRING_H
#define TYPE_CHECKER_EXPR_CALL_STRING_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * String Method Type Checking
 * ============================================================================
 * Type checking for string method access (not calls).
 * Returns the function type for the method, or NULL if not a string method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check string methods
 * Handles: length, substring, regionEquals, indexOf, split, trim, toUpper,
 *          toLower, startsWith, endsWith, contains, replace, charAt, toBytes,
 *          splitWhitespace, splitLines, isBlank, append
 */
Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_STRING_H */
