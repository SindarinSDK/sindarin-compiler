#ifndef TYPE_CHECKER_EXPR_CALL_CORE_H
#define TYPE_CHECKER_EXPR_CALL_CORE_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Core Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, method calls, static method calls,
 * and built-in functions. This module handles all core call-related type
 * checking logic and delegates to specialized modules for type-specific methods.
 * ============================================================================ */

/* Main call expression type checker
 * Dispatches to appropriate handler based on call type:
 * - Built-in functions (len)
 * - User-defined function calls
 * - Method calls on objects
 * - Static method calls (e.g., Stdin.readLine)
 */
Type *type_check_call_expression(Expr *expr, SymbolTable *table);

/* Helper to check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name);

/* Helper to compare a token's text against a string */
bool token_equals(Token tok, const char *str);

/* Static method type checking for built-in types
 * Handles: Stdin, Stdout, Stderr, Interceptor, and user-defined struct static methods
 */
Type *type_check_static_method_call(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_CORE_H */
