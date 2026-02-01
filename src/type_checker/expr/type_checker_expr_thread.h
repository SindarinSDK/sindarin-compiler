#ifndef TYPE_CHECKER_EXPR_THREAD_H
#define TYPE_CHECKER_EXPR_THREAD_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Thread Expression Type Checking
 * ============================================================================
 * Type checking for thread spawn and thread synchronization expressions.
 * ============================================================================ */

/* Thread spawn expression type checking (thread fn())
 * Verifies the expression is a valid function call.
 * Returns ThreadResult<T> where T is the function's return type.
 */
Type *type_check_thread_spawn(Expr *expr, SymbolTable *table);

/* Thread sync expression type checking (result! or [r1, r2]!)
 * Verifies the operand is a ThreadResult type.
 * Returns the inner type T from ThreadResult<T>.
 */
Type *type_check_thread_sync(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_THREAD_H */
