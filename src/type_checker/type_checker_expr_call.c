/* ============================================================================
 * type_checker_expr_call.c - Call Expression Type Checking Facade
 * ============================================================================
 * This file serves as a facade for backwards compatibility.
 * The actual implementations are in type_checker_expr_call_core.c.
 *
 * All call expression type checking functionality is provided by the core
 * module and the specialized type-checking modules:
 * - type_checker_expr_call_core.c: Main dispatchers and helpers
 * - type_checker_expr_call_array.c: Array method type checking
 * - type_checker_expr_call_string.c: String method type checking
 * ============================================================================ */

/* This file intentionally left mostly empty.
 * All implementations are in type_checker_expr_call_core.c
 * which is included via the headers. */
