/**
 * code_gen_expr_thread_sync.h - Thread sync expression code generation
 *
 * Contains code generation for var! sync expressions.
 */

#ifndef CODE_GEN_EXPR_THREAD_SYNC_H
#define CODE_GEN_EXPR_THREAD_SYNC_H

#include "code_gen.h"
#include "ast.h"

/**
 * Generate code for a thread sync expression (var!).
 *
 * Handles both single variable sync (r!) and sync lists ([r1, r2, r3]!).
 *
 * @param gen The code generator context
 * @param expr The thread sync expression
 * @return Generated C code string
 */
char *code_gen_thread_sync_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_THREAD_SYNC_H */
