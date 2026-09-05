#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* Dead Code Elimination Pass
 * ==========================
 * This pass removes:
 * 1. Unreachable code after return/break/continue statements
 * 2. Unused variable declarations (variables declared but never read)
 * 3. No-op operations (like 0 + x, x * 1, etc.)
 *
 * Tail Call Optimization Pass
 * ===========================
 * This pass detects tail-recursive calls and marks them for loop conversion
 * in code generation. A tail call is when a function's last action is to
 * call itself and return the result directly.
 */

/* Initialize optimizer with arena for allocations */
typedef struct {
    Arena *arena;
    int statements_removed;
    int variables_removed;
    int noops_removed;
    int tail_calls_optimized;
    int string_literals_merged;
} Optimizer;

void optimizer_init(Optimizer *opt, Arena *arena);

/* Run dead code elimination on a module */
void optimizer_dead_code_elimination(Optimizer *opt, Module *module);

/* Run dead code elimination on a function */
void optimizer_eliminate_dead_code_function(Optimizer *opt, FunctionStmt *fn);

/* Check if a statement is a control flow terminator (return, break, continue) */
bool stmt_is_terminator(Stmt *stmt);

/* Check if an expression is a no-op (evaluates to the same as one of its operands) */
bool expr_is_noop(Expr *expr, Expr **simplified);

/* Remove unreachable statements from a block (after return/break/continue) */
int remove_unreachable_statements(Optimizer *opt, Stmt ***stmts, int *count);

/* Collect all variable names that are used (read) in an expression */
void collect_used_variables(Expr *expr, Token **used_vars, int *used_count, int *used_capacity, Arena *arena);

/* Collect all variable names that are used (read) in a statement */
void collect_used_variables_stmt(Stmt *stmt, Token **used_vars, int *used_count, int *used_capacity, Arena *arena);

/* Check if a variable name is in the used list */
bool is_variable_used(Token *used_vars, int used_count, Token name);

/* Get optimizer statistics */
void optimizer_get_stats(Optimizer *opt, int *stmts_removed, int *vars_removed, int *noops_removed);

/* ============================================================================
 * Tail Call Optimization
 * ============================================================================
 */

/* Check if a return statement contains a tail-recursive call to the given function */
bool is_tail_recursive_return(Stmt *stmt, Token func_name);

/* Check if a function has tail-recursive patterns that can be optimized */
bool function_has_tail_recursion(FunctionStmt *fn);

/* Mark tail-recursive calls in a function for optimization.
   Returns the number of tail calls marked. */
int optimizer_mark_tail_calls(Optimizer *opt, FunctionStmt *fn);

/* Run tail call optimization on an entire module */
void optimizer_tail_call_optimization(Optimizer *opt, Module *module);

/* ============================================================================
 * String Interpolation Optimization
 * ============================================================================
 */

/* Merge adjacent string literals in interpolated expressions.
   For example: $"Hello " + "World" -> $"Hello World"
   Returns the number of literals merged. */
int optimizer_merge_string_literals(Optimizer *opt, Module *module);

/* Merge adjacent string literals in an expression.
   Returns the optimized expression (may be the same or a new one). */
Expr *optimize_string_expr(Optimizer *opt, Expr *expr);

/* Include sub-module headers */
#include "optimizer/optimizer_util.h"
#include "optimizer/optimizer_string.h"
#include "optimizer/optimizer_tail_call.h"

#endif /* OPTIMIZER_H */
