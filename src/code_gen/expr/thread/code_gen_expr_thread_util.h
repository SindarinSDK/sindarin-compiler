/**
 * code_gen_expr_thread_util.h - Thread code generation utility functions
 *
 * Contains helper functions for thread spawn/sync code generation.
 */

#ifndef CODE_GEN_EXPR_THREAD_UTIL_H
#define CODE_GEN_EXPR_THREAD_UTIL_H

#include "ast.h"

/**
 * Get the RtResultType enum constant string for a given type.
 * Used for thread result type information.
 *
 * @param type The type to convert
 * @return String constant like "RT_TYPE_INT", "RT_TYPE_STRING", etc.
 */
const char *get_rt_result_type(Type *type);

#endif /* CODE_GEN_EXPR_THREAD_UTIL_H */
