/**
 * code_gen_expr_call_string.c - Code generation for string method calls
 *
 * All methods use V2 handle-based string functions (RtHandleV2*).
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Dispatch string instance method calls */
char *code_gen_string_method_call(CodeGen *gen, const char *method_name,
                                   Expr *object,
                                   int arg_count, Expr **arguments)
{
    /* Evaluate object in handle mode */
    char *object_h = code_gen_expression(gen, object);

    /* If the receiver is a call expression that creates a new string handle
     * (e.g., chained call like text.substring(0, idx).trim()), hoist the
     * receiver result to a tracked temp so the intermediate handle can be
     * freed. Without this, the intermediate is inlined and never freed,
     * leaking in long-lived arenas (e.g., server handler loops). */
    if (object->type == EXPR_CALL &&
        object->expr_type != NULL && object->expr_type->kind == TYPE_STRING)
    {
        object_h = code_gen_emit_arena_temp(gen, object_h);
    }

    /* Check if we're in a struct method (no arena condemn — must track temps) */
    bool in_method = (gen->function_arena_var != NULL &&
                      strcmp(gen->function_arena_var, "__caller_arena__") == 0);

    /* substring(start, end) - returns string */
    if (strcmp(method_name, "substring") == 0 && arg_count == 2) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        char *v2_call = arena_sprintf(gen->arena, "rt_str_substring_v2(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_h, start_str, end_str);
        if (in_method) {
            return code_gen_emit_arena_temp(gen, v2_call);
        }
        return v2_call;
    }

    /* regionEquals(start, end, pattern) - returns bool */
    if (strcmp(method_name, "regionEquals") == 0 && arg_count == 3) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        char *pattern_h = code_gen_expression(gen, arguments[2]);
        return arena_sprintf(gen->arena, "rt_str_region_equals_v2(%s, %s, %s, %s)",
            object_h, start_str, end_str, pattern_h);
    }

    /* indexOf(search) - returns long */
    if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        char *arg_h = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_str_indexOf_v2(%s, %s)", object_h, arg_h);
    }

    /* split(delimiter) - returns string array */
    if (strcmp(method_name, "split") == 0 && arg_count == 1) {
        char *arg_h = code_gen_expression(gen, arguments[0]);
        char *v2_call = arena_sprintf(gen->arena, "rt_str_split_v2(%s, %s, %s)",
            ARENA_VAR(gen), object_h, arg_h);
        return v2_call;
    }

    /* trim() - returns string */
    if (strcmp(method_name, "trim") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_str_trim_v2(%s, %s)",
            ARENA_VAR(gen), object_h);
        if (in_method) {
            return code_gen_emit_arena_temp(gen, v2_call);
        }
        return v2_call;
    }

    /* toUpper() - returns string */
    if (strcmp(method_name, "toUpper") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_str_toUpper_v2(%s, %s)",
            ARENA_VAR(gen), object_h);
        if (in_method) {
            return code_gen_emit_arena_temp(gen, v2_call);
        }
        return v2_call;
    }

    /* toLower() - returns string */
    if (strcmp(method_name, "toLower") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_str_toLower_v2(%s, %s)",
            ARENA_VAR(gen), object_h);
        if (in_method) {
            return code_gen_emit_arena_temp(gen, v2_call);
        }
        return v2_call;
    }

    /* startsWith(prefix) - returns bool */
    if (strcmp(method_name, "startsWith") == 0 && arg_count == 1) {
        char *arg_h = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_str_startsWith_v2(%s, %s)", object_h, arg_h);
    }

    /* endsWith(suffix) - returns bool */
    if (strcmp(method_name, "endsWith") == 0 && arg_count == 1) {
        char *arg_h = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_str_endsWith_v2(%s, %s)", object_h, arg_h);
    }

    /* contains(search) - returns bool */
    if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        char *arg_h = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_str_contains_v2(%s, %s)", object_h, arg_h);
    }

    /* replace(old, new) - returns string */
    if (strcmp(method_name, "replace") == 0 && arg_count == 2) {
        char *old_h = code_gen_expression(gen, arguments[0]);
        char *new_h = code_gen_expression(gen, arguments[1]);
        char *v2_call = arena_sprintf(gen->arena, "rt_str_replace_v2(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_h, old_h, new_h);
        if (in_method) {
            return code_gen_emit_arena_temp(gen, v2_call);
        }
        return v2_call;
    }

    /* charAt(index) - returns char */
    if (strcmp(method_name, "charAt") == 0 && arg_count == 1) {
        char *index_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "(char)rt_str_charAt_v2(%s, %s)", object_h, index_str);
    }

    /* toBytes() - returns byte array (UTF-8 encoding) */
    if (strcmp(method_name, "toBytes") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_string_to_bytes(%s, %s)",
            ARENA_VAR(gen), object_h);
        return v2_call;
    }

    /* splitWhitespace() - returns string array */
    if (strcmp(method_name, "splitWhitespace") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_str_split_whitespace_v2(%s, %s)",
            ARENA_VAR(gen), object_h);
        return v2_call;
    }

    /* splitLines() - returns string array */
    if (strcmp(method_name, "splitLines") == 0 && arg_count == 0) {
        char *v2_call = arena_sprintf(gen->arena, "rt_str_split_lines_v2(%s, %s)",
            ARENA_VAR(gen), object_h);
        return v2_call;
    }

    /* isBlank() - returns bool */
    if (strcmp(method_name, "isBlank") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_str_is_blank_v2(%s)", object_h);
    }

    /* toInt() - returns int (parse string as integer) */
    if (strcmp(method_name, "toInt") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_str_to_int_v2(%s)", object_h);
    }

    /* toLong() - returns long (parse string as long integer) */
    if (strcmp(method_name, "toLong") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_str_to_long_v2(%s)", object_h);
    }

    /* toDouble() - returns double (parse string as double) */
    if (strcmp(method_name, "toDouble") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_str_to_double_v2(%s)", object_h);
    }

    /* split(delimiter, limit) - returns string array with at most 'limit' parts */
    if (strcmp(method_name, "split") == 0 && arg_count == 2) {
        char *delim_h = code_gen_expression(gen, arguments[0]);
        char *limit_str = code_gen_expression(gen, arguments[1]);
        char *v2_call = arena_sprintf(gen->arena, "rt_str_split_n_v2(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_h, delim_h, limit_str);
        return v2_call;
    }

    /* append(str) - appends to string, returns new string handle */
    if (strcmp(method_name, "append") == 0 && arg_count == 1) {
        Type *arg_type = arguments[0]->expr_type;
        if (arg_type == NULL || arg_type->kind != TYPE_STRING) {
            fprintf(stderr, "Error: append() argument must be a string\n");
            exit(1);
        }
        char *arg_h = code_gen_expression(gen, arguments[0]);
        if (object->type == EXPR_VARIABLE) {
            char *handle_name = code_gen_expression(gen, object);
            return arena_sprintf(gen->arena,
                "(%s = rt_str_append_v2(%s, %s, %s))",
                handle_name, ARENA_VAR(gen), object_h, arg_h);
        }
        return arena_sprintf(gen->arena, "rt_str_append_v2(%s, %s, %s)",
            ARENA_VAR(gen), object_h, arg_h);
    }

    /* Method not handled here */
    return NULL;
}

/* Generate code for string.length property */
char *code_gen_string_length(CodeGen *gen, Expr *object)
{
    char *obj_h = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_str_length_v2(%s)", obj_h);
}
