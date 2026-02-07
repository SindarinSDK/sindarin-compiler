/**
 * code_gen_expr_call_string.c - Code generation for string method calls
 *
 * Contains implementations for generating C code from method calls on
 * string types. Methods are categorized by return type and argument handling.
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
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

/* Helper for methods returning arrays with temp object handling.
 * Always uses handle-based representation when arena is available to ensure
 * consistent element types (RtHandle) that can be properly pinned during array indexing. */
#define STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, elem_type, method_call) \
    do { \
        char *_raw_result; \
        if (object_is_temp) { \
            if ((gen)->current_arena_var != NULL) { \
                _raw_result = arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; " elem_type " *_res = %s; _res; })", \
                    object_str, method_call); \
            } else { \
                _raw_result = arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; " elem_type " *_res = %s; rt_free_string(_obj_tmp); _res; })", \
                    object_str, method_call); \
            } \
        } else { \
            _raw_result = arena_sprintf((gen)->arena, "%s", method_call); \
        } \
        if ((gen)->current_arena_var != NULL) { \
            /* Always create handle-based array so elements are RtHandleV2* */ \
            char *_handle_result = arena_sprintf((gen)->arena, \
                "rt_array_from_raw_strings_v2(%s, %s)", \
                ARENA_VAR(gen), _raw_result); \
            if ((gen)->expr_as_handle) { \
                return _handle_result; \
            } else { \
                /* Pin the handle-based array - elements are still RtHandleV2*, */ \
                /* which is correct for array indexing to pin to char* */ \
                return arena_sprintf((gen)->arena, \
                    "((RtHandleV2 *)rt_handle_v2_pin(%s))", \
                    _handle_result); \
            } \
        } \
        return _raw_result; \
    } while(0)

/* Dispatch string instance method calls */
char *code_gen_string_method_call(CodeGen *gen, const char *method_name,
                                   Expr *object, bool object_is_temp,
                                   int arg_count, Expr **arguments)
{
    /* Save handle mode - object and method args must be evaluated as raw pointers
     * (runtime string functions take char*), but string-returning methods should
     * return RtHandle when caller expects it */
    bool handle_mode = gen->expr_as_handle;
    gen->expr_as_handle = false;
    char *object_str = code_gen_expression(gen, object);

    /* substring(start, end) - returns string */
    if (strcmp(method_name, "substring") == 0 && arg_count == 2) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_substring_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_str, start_str, end_str);
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))", v2_call);
        }
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
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* indexOf(search) - returns long */
    if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_indexOf(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long", method_call);
    }

    /* split(delimiter) - returns string array */
    if (strcmp(method_name, "split") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        if (handle_mode && gen->current_arena_var != NULL) {
            return arena_sprintf(gen->arena, "rt_str_split_v2(%s, %s, %s)",
                ARENA_VAR(gen), object_str, arg_str);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_split(%s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* trim() - returns string */
    if (strcmp(method_name, "trim") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_trim_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_trim(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toUpper() - returns string */
    if (strcmp(method_name, "toUpper") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_toUpper_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_toUpper(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toLower() - returns string */
    if (strcmp(method_name, "toLower") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_toLower_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_toLower(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* startsWith(prefix) - returns bool */
    if (strcmp(method_name, "startsWith") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_startsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* endsWith(suffix) - returns bool */
    if (strcmp(method_name, "endsWith") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_endsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* contains(search) - returns bool */
    if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_contains(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* replace(old, new) - returns string */
    if (strcmp(method_name, "replace") == 0 && arg_count == 2) {
        char *old_str = code_gen_expression(gen, arguments[0]);
        char *new_str = code_gen_expression(gen, arguments[1]);
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - always use V2 function */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_replace_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_str, old_str, new_str);
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - pin the result */
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_replace(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, old_str, new_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* charAt(index) - returns char */
    if (strcmp(method_name, "charAt") == 0 && arg_count == 1) {
        char *index_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "(char)rt_str_charAt(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, index_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "char", method_call);
    }

    /* toBytes() - returns byte array (UTF-8 encoding) */
    if (strcmp(method_name, "toBytes") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_string_to_bytes(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        char *_raw_result;
        if (object_is_temp) {
            if (gen->current_arena_var != NULL) {
                _raw_result = arena_sprintf(gen->arena,
                    "({ char *_obj_tmp = %s; unsigned char *_res = %s; _res; })",
                    object_str, method_call);
            } else {
                _raw_result = arena_sprintf(gen->arena,
                    "({ char *_obj_tmp = %s; unsigned char *_res = %s; rt_free_string(_obj_tmp); _res; })",
                    object_str, method_call);
            }
        } else {
            _raw_result = arena_sprintf(gen->arena, "%s", method_call);
        }
        if (gen->expr_as_handle && gen->current_arena_var != NULL) {
            return arena_sprintf(gen->arena, "rt_array_clone_byte_v2(%s, %s)",
                                 ARENA_VAR(gen), _raw_result);
        }
        return _raw_result;
    }

    /* splitWhitespace() - returns string array */
    if (strcmp(method_name, "splitWhitespace") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - use V2 function that returns RtHandleV2* directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_split_whitespace_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            gen->expr_as_handle = handle_mode;
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - get data pointer to element array */
            return arena_sprintf(gen->arena, "((RtHandleV2 **)rt_array_data_v2(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_whitespace(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* splitLines() - returns string array */
    if (strcmp(method_name, "splitLines") == 0 && arg_count == 0) {
        if (gen->current_arena_var != NULL) {
            /* V2 arena mode - use V2 function that returns RtHandleV2* directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_split_lines_v2(%s, %s)",
                ARENA_VAR(gen), object_str);
            gen->expr_as_handle = handle_mode;
            if (handle_mode) {
                return v2_call;
            }
            /* Want raw pointer - get data pointer to element array */
            return arena_sprintf(gen->arena, "((RtHandleV2 **)rt_array_data_v2(%s))", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_lines(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* isBlank() - returns bool */
    if (strcmp(method_name, "isBlank") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_is_blank(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* toInt() - returns int (parse string as integer) */
    if (strcmp(method_name, "toInt") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_int(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long long", method_call);
    }

    /* toLong() - returns long (parse string as long integer) */
    if (strcmp(method_name, "toLong") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_long(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long long", method_call);
    }

    /* toDouble() - returns double (parse string as double) */
    if (strcmp(method_name, "toDouble") == 0 && arg_count == 0) {
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_double(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "double", method_call);
    }

    /* split(delimiter, limit) - returns string array with at most 'limit' parts */
    if (strcmp(method_name, "split") == 0 && arg_count == 2) {
        char *delimiter_str = code_gen_expression(gen, arguments[0]);
        char *limit_str = code_gen_expression(gen, arguments[1]);
        if (handle_mode && gen->current_arena_var != NULL) {
            /* Use handle-returning variant directly */
            if (object_is_temp) {
                gen->expr_as_handle = handle_mode;
                return arena_sprintf(gen->arena,
                    "({ char *_obj_tmp = %s; RtHandleV2 *_res = rt_str_split_n_v2(%s, _obj_tmp, %s, %s); _res; })",
                    object_str, ARENA_VAR(gen), delimiter_str, limit_str);
            }
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_split_n_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_str, delimiter_str, limit_str);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_n(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, delimiter_str, limit_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* append(str) - appends to mutable string, returns new string pointer */
    if (strcmp(method_name, "append") == 0 && arg_count == 1) {
        char *arg_str = code_gen_expression(gen, arguments[0]);
        Type *arg_type = arguments[0]->expr_type;

        if (arg_type == NULL || arg_type->kind != TYPE_STRING) {
            fprintf(stderr, "Error: append() argument must be a string\n");
            exit(1);
        }

        gen->expr_as_handle = handle_mode;

        /* In handle mode: use rt_str_append_v2 which returns a new handle.
         * rt_str_append_v2(arena, old_str, suffix) - takes pinned old string and suffix. */
        if (gen->current_arena_var != NULL && object->type == EXPR_VARIABLE) {
            /* Get the handle variable name */
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_name = code_gen_expression(gen, object);
            gen->expr_as_handle = prev;
            return arena_sprintf(gen->arena,
                "(%s = rt_str_append_v2(%s, %s, %s))",
                handle_name, ARENA_VAR(gen), object_str, arg_str);
        }

        /* Legacy path: First ensure the string is mutable, then append. */
        if (object->type == EXPR_VARIABLE) {
            return arena_sprintf(gen->arena,
                "(%s = rt_string_append(rt_string_ensure_mutable_inline(__local_arena__, %s), %s))",
                object_str, object_str, arg_str);
        }
        return arena_sprintf(gen->arena,
            "rt_string_append(rt_string_ensure_mutable_inline(__local_arena__, %s), %s)",
            object_str, arg_str);
    }

    /* Method not handled here - restore handle mode */
    gen->expr_as_handle = handle_mode;
    return NULL;
}

/* Generate code for string.length property */
char *code_gen_string_length(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
}
