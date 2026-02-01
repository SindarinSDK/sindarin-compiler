#ifndef CODE_GEN_EXPR_STRING_H
#define CODE_GEN_EXPR_STRING_H

#include "code_gen.h"
#include "ast.h"

/* ============================================================================
 * String Interpolation Code Generation
 * ============================================================================
 * Generates C code for string interpolation expressions like $"Hello {name}"
 * Handles format specifiers, type conversions, and optimizations.
 * ============================================================================ */

/* Generate code for an interpolated string expression.
 * Handles:
 * - Empty interpolation → empty string literal
 * - Single string literal/variable optimizations
 * - Format specifiers (e.g., {value:05d})
 * - Type conversions to string
 * - Concatenation chains for multiple parts
 */
char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr);

#endif /* CODE_GEN_EXPR_STRING_H */
