#ifndef OPTIMIZER_UTIL_H
#define OPTIMIZER_UTIL_H

#include "ast.h"
#include "arena.h"
#include "token.h"
#include "optimizer.h"
#include <stdbool.h>

/* ============================================================================
 * Literal Detection
 * ============================================================================
 * Check if expressions are specific literal values
 */

/* Check if an expression is the literal integer 0 */
bool is_literal_zero(Expr *expr);

/* Check if an expression is the literal integer 1 */
bool is_literal_one(Expr *expr);

/* ============================================================================
 * No-op Detection and Simplification
 * ============================================================================
 * Detect and simplify expressions that can be reduced:
 * - x + 0, 0 + x => x
 * - x - 0 => x
 * - x * 1, 1 * x => x
 * - x / 1 => x
 * - !(!x) => x (double negation)
 * - -(-x) => x (double negation)
 */

/* Check if an expression is a no-op that can be simplified.
   If true, *simplified is set to the simplified expression. */
bool expr_is_noop(Expr *expr, Expr **simplified);

/* ============================================================================
 * Variable Usage Tracking
 * ============================================================================
 * Track which variables are actually read to identify unused declarations
 */

/* Add a variable name to the used variables list */
void add_used_variable(Token **used_vars, int *used_count, int *used_capacity,
                       Arena *arena, Token name);

/* Collect all variable uses from an expression */
void collect_used_variables(Expr *expr, Token **used_vars, int *used_count,
                            int *used_capacity, Arena *arena);

/* Collect all variable uses from a statement */
void collect_used_variables_stmt(Stmt *stmt, Token **used_vars, int *used_count,
                                 int *used_capacity, Arena *arena);

/* Check if a variable name is in the used variables list */
bool is_variable_used(Token *used_vars, int used_count, Token name);

/* ============================================================================
 * Dead Code Removal Helpers
 * ============================================================================
 */

/* Remove unused variable declarations from a list of statements.
   Returns the number of variables removed. */
int remove_unused_variables(Optimizer *opt, Stmt **stmts, int *count);

/* Simplify no-op expressions recursively in an expression.
   Returns the simplified expression. */
Expr *simplify_noop_expr(Optimizer *opt, Expr *expr);

/* Simplify no-op expressions in a statement */
void simplify_noop_stmt(Optimizer *opt, Stmt *stmt);

#endif /* OPTIMIZER_UTIL_H */
