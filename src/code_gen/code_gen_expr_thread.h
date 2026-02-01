/**
 * code_gen_expr_thread.h - Code generation for thread expressions
 *
 * Contains declarations for generating C code from thread spawn (&fn())
 * and thread sync (var!) expressions.
 *
 * This module is split into:
 * - code_gen_expr_thread_util.h/c - Utility functions (get_rt_result_type)
 * - code_gen_expr_thread_sync.h/c - Thread sync expression generation
 * - code_gen_expr_thread.h/c - Thread spawn expression generation (this file)
 */

#ifndef CODE_GEN_EXPR_THREAD_H
#define CODE_GEN_EXPR_THREAD_H

#include "code_gen.h"
#include "ast.h"

/* Include sub-modules for convenience */
#include "code_gen/code_gen_expr_thread_util.h"
#include "code_gen/code_gen_expr_thread_sync.h"

/**
 * Generate code for thread spawn expressions (&fn()).
 * Creates wrapper function and argument struct for thread creation.
 * Handles shared, private, and default arena modes.
 */
char *code_gen_thread_spawn_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_THREAD_H */
