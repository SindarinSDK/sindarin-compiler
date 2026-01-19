/**
 * code_gen_expr_call_string.c - Code generation for string method calls
 *
 * Contains implementations for generating C code from method calls on
 * string types. Methods are categorized by return type and argument handling.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper macro for string methods that return strings with temp object handling */
#define STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call) \
    do { \
        if (object_is_temp) { \
            if ((gen)->current_arena_var != NULL) { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; char *_res = %s; _res; })", \
                    object_str, method_call); \
            } else { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; char *_res = %s; rt_free_string(_obj_tmp); _res; })", \
                    object_str, method_call); \
            } \
        } else { \
            return arena_sprintf((gen)->arena, "%s", method_call); \
        } \
    } while(0)

/* Helper for methods returning non-string (int/bool/char) with temp object handling */
#define STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, result_type, method_call) \
    do { \
        if (object_is_temp) { \
            if ((gen)->current_arena_var != NULL) { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; " result_type " _res = %s; _res; })", \
                    object_str, method_call); \
            } \
            return arena_sprintf((gen)->arena, \
                "({ char *_obj_tmp = %s; " result_type " _res = %s; rt_free_string(_obj_tmp); _res; })", \
                object_str, method_call); \
        } \
        return arena_sprintf((gen)->arena, "%s", method_call); \
    } while(0)

/* Helper for methods returning arrays with temp object handling */
#define STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, elem_type, method_call) \
    do { \
        if (object_is_temp) { \
            if ((gen)->current_arena_var != NULL) { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; " elem_type " *_res = %s; _res; })", \
                    object_str, method_call); \
            } \
            return arena_sprintf((gen)->arena, \
                "({ char *_obj_tmp = %s; " elem_type " *_res = %s; rt_free_string(_obj_tmp); _res; })", \
                object_str, method_call); \
        } \
        return arena_sprintf((gen)->arena, "%s", method_call); \
    } while(0)

/* Dispatch string instance method calls */
char *code_gen_string_method_call(CodeGen *gen, const char *method_name,
                                   Expr *object, bool object_is_temp,
                                   int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* substring(start, end) - returns string */
    if (strcmp(method_name, "substring") == 0 && arg_count == 2) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_substring(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, start_str, end_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* regionEquals(start, end, pattern) - returns bool */
    if (strcmp(method_name, "regionEquals") == 0 && arg_count == 3) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        char *pattern_str = code_gen_expression(gen, arguments[2]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_region_equals(%s, %s, %s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, start_str, end_str, pattern_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* indexOf(search) - returns long */
    if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_indexOf(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long", method_call);
    }

    /* split(delimiter) - returns string array */
    if (strcmp(method_name, "split") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_split(%s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, arg_str);
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* trim() - returns string */
    if (strcmp(method_name, "trim") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_trim(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toUpper() - returns string */
    if (strcmp(method_name, "toUpper") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_toUpper(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toLower() - returns string */
    if (strcmp(method_name, "toLower") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_toLower(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* startsWith(prefix) - returns bool */
    if (strcmp(method_name, "startsWith") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_startsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* endsWith(suffix) - returns bool */
    if (strcmp(method_name, "endsWith") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_endsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* contains(search) - returns bool */
    if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_contains(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* replace(old, new) - returns string */
    if (strcmp(method_name, "replace") == 0 && arg_count == 2) {
        char *old_str = code_gen_expression(gen, arguments[0]);
        char *new_str = code_gen_expression(gen, arguments[1]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_replace(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, old_str, new_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* charAt(index) - returns char */
    if (strcmp(method_name, "charAt") == 0 && arg_count == 1) {
        char *index_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "(char)rt_str_charAt(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, index_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "char", method_call);
    }

    /* toBytes() - returns byte array (UTF-8 encoding) */
    if (strcmp(method_name, "toBytes") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_string_to_bytes(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "unsigned char", method_call);
    }

    /* splitWhitespace() - returns string array */
    if (strcmp(method_name, "splitWhitespace") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_whitespace(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* splitLines() - returns string array */
    if (strcmp(method_name, "splitLines") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_lines(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* isBlank() - returns bool */
    if (strcmp(method_name, "isBlank") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_is_blank(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* Method not handled here */
    return NULL;
}

/* Generate code for string.length property */
char *code_gen_string_length(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
}
