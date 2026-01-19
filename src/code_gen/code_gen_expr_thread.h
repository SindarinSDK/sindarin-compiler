/**
 * code_gen_expr_thread.h - Code generation for thread expressions
 *
 * Contains declarations for generating C code from thread spawn (&fn())
 * and thread sync (var!) expressions.
 */

#ifndef CODE_GEN_EXPR_THREAD_H
#define CODE_GEN_EXPR_THREAD_H

#include "code_gen.h"
#include "ast.h"

/**
 * Generate RtResultType constant from Type.
 * Used by thread sync to determine result type for proper casting.
 */
const char *get_rt_result_type(Type *type);

/**
 * Generate code for thread spawn expressions (&fn()).
 * Creates wrapper function and argument struct for thread creation.
 * Handles shared, private, and default arena modes.
 */
char *code_gen_thread_spawn_expression(CodeGen *gen, Expr *expr);

/**
 * Generate code for thread sync expressions (var! or [r1,r2]!).
 * Handles single variable sync and sync list expressions.
 * Propagates panics from threads to caller.
 */
char *code_gen_thread_sync_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_THREAD_H */
