#ifndef TYPE_CHECKER_EXPR_CALL_H
#define TYPE_CHECKER_EXPR_CALL_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, method calls, static method calls,
 * and built-in functions. This module handles all call-related type checking
 * extracted from type_checker_expr.c.
 *
 * The implementation is split across multiple modules:
 * - type_checker_expr_call_core.c: Main dispatchers and helpers
 * - type_checker_expr_call_array.c: Array method type checking
 * - type_checker_expr_call_string.c: String method type checking
 * ============================================================================ */

/* ============================================================================
 * Core Functions (type_checker_expr_call_core.c)
 * ============================================================================ */

/* Main call expression type checker
 * Dispatches to appropriate handler based on call type:
 * - Built-in functions (len)
 * - User-defined function calls
 * - Method calls on objects
 * - Static method calls (e.g., TextFile.open)
 */
Type *type_check_call_expression(Expr *expr, SymbolTable *table);

/* Static method type checking for built-in types
 * Handles: Stdin, Stdout, Stderr, Process, Environment
 */
Type *type_check_static_method_call(Expr *expr, SymbolTable *table);

/* Helper to check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name);

/* Helper to compare a token's text against a string */
bool token_equals(Token tok, const char *str);

/* ============================================================================
 * Array Methods (type_checker_expr_call_array.c)
 * ============================================================================ */

/* Type check array instance methods:
 * push, pop, reverse, remove, insert, slice, contains, indexOf, lastIndexOf,
 * first, last, isEmpty, clear, copy, join, sort, filter, map, reduce, forEach,
 * any, all, count, find, findIndex, take, drop, unique, flatten, zip, sum,
 * average, min, max
 */
Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * String Methods (type_checker_expr_call_string.c)
 * ============================================================================ */

/* Type check string instance methods:
 * length, isEmpty, trim, trimStart, trimEnd, upper, lower, reverse, split,
 * replace, replaceAll, startsWith, endsWith, contains, indexOf, lastIndexOf,
 * substring, charAt, repeat, padStart, padEnd, lines, toInt, toLong, toDouble,
 * toBytes, toHex, toBase64, fromHex, fromBase64
 */
Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* ============================================================================
 * Process Methods
 * ============================================================================ */

/* Type check Process instance methods:
 * exitCode, stdout, stderr
 */
Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_H */
