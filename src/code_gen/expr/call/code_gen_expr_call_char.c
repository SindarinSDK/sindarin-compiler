/**
 * code_gen_expr_call_char.c - Code generation for char method calls
 *
 * Handles char type methods: toString, toUpper, toLower, toInt, isDigit,
 * isAlpha, isWhitespace, isAlnum.
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Generate code for char method calls.
 * Returns generated C code string, or NULL if not a char method.
 */
char *code_gen_char_method_call(CodeGen *gen, const char *method_name,
                                 Expr *object, int arg_count)
{
    char *object_str = code_gen_expression(gen, object);

    /* char.toString() -> str (single character string)
     * Returns RtHandleV2* in handle mode, char* otherwise */
    if (strcmp(method_name, "toString") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_to_string_char_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            if (gen->expr_as_handle) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        return arena_sprintf(gen->arena, "rt_char_toString(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    /* char.toUpper() -> char */
    if (strcmp(method_name, "toUpper") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_toUpper(%s)", object_str);
    }

    /* char.toLower() -> char */
    if (strcmp(method_name, "toLower") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_toLower(%s)", object_str);
    }

    /* char.toInt() -> int (ASCII value) */
    if (strcmp(method_name, "toInt") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "((int)%s)", object_str);
    }

    /* char.isDigit() -> bool */
    if (strcmp(method_name, "isDigit") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_isDigit(%s)", object_str);
    }

    /* char.isAlpha() -> bool */
    if (strcmp(method_name, "isAlpha") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_isAlpha(%s)", object_str);
    }

    /* char.isWhitespace() -> bool */
    if (strcmp(method_name, "isWhitespace") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_isWhitespace(%s)", object_str);
    }

    /* char.isAlnum() -> bool */
    if (strcmp(method_name, "isAlnum") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_char_isAlnum(%s)", object_str);
    }

    return NULL;
}
