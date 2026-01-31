#ifndef TYPE_CHECKER_EXPR_CALL_CHAR_H
#define TYPE_CHECKER_EXPR_CALL_CHAR_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Char Method Type Checking
 * ============================================================================
 * Type checking for char method access (not calls).
 * Returns the function type for the method, or NULL if not a char method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check char methods
 * Handles: toString, toUpper, toLower, toInt, isDigit, isAlpha, isWhitespace
 */
Type *type_check_char_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_CHAR_H */
