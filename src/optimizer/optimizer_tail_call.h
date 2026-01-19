#ifndef OPTIMIZER_TAIL_CALL_H
#define OPTIMIZER_TAIL_CALL_H

#include "ast.h"
#include "optimizer.h"
#include <stdbool.h>

/* ============================================================================
 * Tail Call Optimization
 * ============================================================================
 * Detect and mark tail-recursive calls for optimization.
 *
 * A tail call is when a function's last action before returning is to call
 * another function and return its result directly. For self-recursive calls,
 * this can be converted to a loop, eliminating stack frame overhead.
 *
 * Example of tail recursion:
 *   fn loop(n: int): int =>
 *       if n <= 0 => return 0
 *       return loop(n - 1)   // <-- tail call, last action is the call itself
 *
 * Example of NON-tail recursion:
 *   fn factorial(n: int): int =>
 *       if n <= 1 => return 1
 *       return n * factorial(n - 1)  // NOT a tail call, multiplication after call
 */

/* Check if a return statement contains a tail-recursive call to the given function */
bool is_tail_recursive_return(Stmt *stmt, Token func_name);

/* Check if a function has any tail-recursive patterns */
bool function_has_tail_recursion(FunctionStmt *fn);

/* Mark tail calls in a statement, returns count of calls marked */
int mark_tail_calls_in_stmt(Stmt *stmt, Token func_name);

/* Mark all tail calls in a function, returns count of calls marked */
int optimizer_mark_tail_calls(Optimizer *opt, FunctionStmt *fn);

/* Run tail call optimization on an entire module */
void optimizer_tail_call_optimization(Optimizer *opt, Module *module);

#endif /* OPTIMIZER_TAIL_CALL_H */
