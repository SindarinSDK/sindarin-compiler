#ifndef OPTIMIZER_STRING_H
#define OPTIMIZER_STRING_H

#include "ast.h"
#include "optimizer.h"
#include <stdbool.h>

/* ============================================================================
 * String Interpolation Optimization
 * ============================================================================
 * Merge adjacent string literals in interpolated expressions to reduce
 * runtime concatenations and temporary variables.
 *
 * Optimizations performed:
 * - Adjacent string literals in interpolated expressions: "a" "b" -> "ab"
 * - String literal concatenations: "a" + "b" -> "ab"
 * - Nested interpolation optimization
 */

/* Recursively optimize string expressions.
   Merges adjacent string literals and string concatenations.
   Returns the (possibly simplified) expression. */
Expr *optimize_string_expr(Optimizer *opt, Expr *expr);

/* Run string literal merging optimization on an entire module.
   Returns the number of string literals merged. */
int optimizer_merge_string_literals(Optimizer *opt, Module *module);

#endif /* OPTIMIZER_STRING_H */
