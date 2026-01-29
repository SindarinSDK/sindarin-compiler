/**
 * code_gen_expr_call.h - Code generation for call expressions
 *
 * Contains declarations for generating C code from function calls and
 * method calls on arrays, strings, and other object types.
 */

#ifndef CODE_GEN_EXPR_CALL_H
#define CODE_GEN_EXPR_CALL_H

#include "code_gen.h"
#include "ast.h"
#include <stdbool.h>

/* ============================================================================
 * Core Call Expression Code Generation (code_gen_expr_call.c)
 * ============================================================================ */

/**
 * Check if an expression produces a temporary string that needs to be freed.
 * Returns true if the expression creates a new string allocation.
 */
bool expression_produces_temp(Expr *expr);

/**
 * Generate a self reference for struct method calls.
 * If the object is an rvalue (function call / method chain), emits a
 * temporary variable declaration and returns a pointer to it.
 */
char *code_gen_self_ref(CodeGen *gen, Expr *object, const char *struct_c_type, char *self_str);

/**
 * Generate code for call expressions (function calls and method calls).
 * This is the main dispatcher that handles:
 * - Namespace function calls (namespace.function())
 * - Method calls on objects (object.method())
 * - Static method calls (Type.method())
 * - Regular function calls (function())
 */
char *code_gen_call_expression(CodeGen *gen, Expr *expr);

/* ============================================================================
 * Array Method Code Generation (code_gen_expr_call_array.c)
 * ============================================================================ */

/**
 * Generate code for array method calls (push, clear, pop, concat, etc.)
 * Returns generated C code string, or NULL if not an array method.
 */
char *code_gen_array_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                  Expr *object, Type *element_type, int arg_count,
                                  Expr **arguments);

/* ============================================================================
 * String Method Code Generation (code_gen_expr_call_string.c)
 * ============================================================================ */

/**
 * Generate code for string instance method calls.
 * Handles: substring, regionEquals, indexOf, split, trim, toUpper, toLower,
 *          startsWith, endsWith, contains, replace, charAt, toBytes,
 *          splitWhitespace, splitLines, isBlank
 * Returns generated C code string, or NULL if not a string method.
 */
char *code_gen_string_method_call(CodeGen *gen, const char *method_name,
                                   Expr *object, bool object_is_temp,
                                   int arg_count, Expr **arguments);

/**
 * Generate code for string.length property access.
 * Returns generated C code string.
 */
char *code_gen_string_length(CodeGen *gen, Expr *object);

/* ============================================================================
 * Struct Method Interception (code_gen_expr_call.c)
 * ============================================================================ */

/**
 * Check if a struct method should be intercepted.
 * Skips native methods, methods on native structs, and methods with
 * unsupported parameter/return types.
 */
bool should_intercept_method(StructMethod *method, Type *struct_type, Type *return_type);

/**
 * Generate an intercepted struct method call (instance or static).
 * Wraps the method call with interception logic including self boxing for instance methods.
 */
char *code_gen_intercepted_method_call(CodeGen *gen,
                                        const char *struct_name,
                                        StructMethod *method,
                                        Type *struct_type,
                                        int arg_count,
                                        Expr **arguments,
                                        const char *self_ptr_str,
                                        bool is_self_pointer,
                                        Type *return_type);

#endif /* CODE_GEN_EXPR_CALL_H */
