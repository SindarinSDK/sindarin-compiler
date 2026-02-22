/**
 * code_gen_expr_call_string.c - Code generation for string method calls
 *
 * Contains implementations for generating C code from method calls on
 * string types. Methods are categorized by return type and argument handling.
 *
 * V2 string functions accept RtHandleV2* for string parameters.
 * V1 string functions still accept const char* (to be updated in Phase 6.2).
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper macro for string methods that return RtHandleV2* with temp object handling.
 * Pins the result handle to produce a char* value.
 * NOTE: Used only for V1 (legacy) method paths. V2 paths handle returns inline. */
#define STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call) \
    do { \
        if (object_is_temp) { \
            if ((gen)->current_arena_var != NULL) { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; RtHandleV2 *_rh = %s; (char *)_rh->ptr; })", \
                    object_str, method_call); \
            } else { \
                return arena_sprintf((gen)->arena, \
                    "({ char *_obj_tmp = %s; RtHandleV2 *_rh = %s; char *_res = (char *)_rh->ptr; rt_free_string(_obj_tmp); _res; })", \
                    object_str, method_call); \
            } \
        } else { \
            return arena_sprintf((gen)->arena, \
                "((char *)(%s)->ptr)", \
                method_call); \
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
                    "((RtHandleV2 *)(%s)->ptr)", \
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
    /* Save handle mode. V1 methods need raw char* operands.
     * V2 methods (in arena mode) need RtHandleV2* operands. */
    bool handle_mode = gen->expr_as_handle;
    bool arena_mode = (gen->current_arena_var != NULL);

    /* Evaluate object in raw mode for V1 methods */
    gen->expr_as_handle = false;
    char *object_str = code_gen_expression(gen, object);

    /* Also get handle version for V2 method paths */
    char *object_h = NULL;
    if (arena_mode) {
        gen->expr_as_handle = true;
        object_h = code_gen_expression(gen, object);

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
    }
    /* V1 methods need raw char* arguments — keep expr_as_handle = false.
     * V2 method handlers set it to true when needed for specific args.
     * Each return path must restore gen->expr_as_handle = handle_mode. */
    gen->expr_as_handle = false;

    /* Check if we're in a struct method (no arena condemn — must track temps) */
    bool in_method = (gen->function_arena_var != NULL &&
                      strcmp(gen->function_arena_var, "__caller_arena__") == 0);

    /* substring(start, end) - returns string */
    if (strcmp(method_name, "substring") == 0 && arg_count == 2) {
        char *start_str = code_gen_expression(gen, arguments[0]);
        char *end_str = code_gen_expression(gen, arguments[1]);
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_substring_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_h, start_str, end_str);
            gen->expr_as_handle = handle_mode;
            /* In struct methods, hoist to temp for tracking */
            if (in_method) {
                char *temp = code_gen_emit_arena_temp(gen, v2_call);
                if (handle_mode) return temp;
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", temp);
            }
            if (handle_mode) {
                return v2_call;
            }
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_substring(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, start_str, end_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* regionEquals(start, end, pattern) - returns bool */
    if (strcmp(method_name, "regionEquals") == 0 && arg_count == 3) {
        if (arena_mode) {
            char *start_str = code_gen_expression(gen, arguments[0]);
            char *end_str = code_gen_expression(gen, arguments[1]);
            gen->expr_as_handle = true;
            char *pattern_h = code_gen_expression(gen, arguments[2]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_region_equals_v2(%s, %s, %s, %s)",
                object_h, start_str, end_str, pattern_h);
        }
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
        if (arena_mode) {
            gen->expr_as_handle = true;
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_indexOf_v2(%s, %s)", object_h, arg_h);
        }
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_indexOf(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long", method_call);
    }

    /* split(delimiter) - returns string array */
    if (strcmp(method_name, "split") == 0 && arg_count == 1) {
        if (arena_mode && handle_mode) {
            /* V2: evaluate delimiter in handle mode, pass handles */
            gen->expr_as_handle = true;
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_split_v2(%s, %s, %s)",
                ARENA_VAR(gen), object_h, arg_h);
        }
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_split(%s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* trim() - returns string */
    if (strcmp(method_name, "trim") == 0 && arg_count == 0) {
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_trim_v2(%s, %s)",
                ARENA_VAR(gen), object_h);
            gen->expr_as_handle = handle_mode;
            /* In struct methods, hoist to temp for tracking */
            if (in_method) {
                char *temp = code_gen_emit_arena_temp(gen, v2_call);
                if (handle_mode) return temp;
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", temp);
            }
            if (handle_mode) {
                return v2_call;
            }
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_trim(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toUpper() - returns string */
    if (strcmp(method_name, "toUpper") == 0 && arg_count == 0) {
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_toUpper_v2(%s, %s)",
                ARENA_VAR(gen), object_h);
            gen->expr_as_handle = handle_mode;
            /* In struct methods, hoist to temp for tracking */
            if (in_method) {
                char *temp = code_gen_emit_arena_temp(gen, v2_call);
                if (handle_mode) return temp;
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", temp);
            }
            if (handle_mode) {
                return v2_call;
            }
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_toUpper(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* toLower() - returns string */
    if (strcmp(method_name, "toLower") == 0 && arg_count == 0) {
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_toLower_v2(%s, %s)",
                ARENA_VAR(gen), object_h);
            gen->expr_as_handle = handle_mode;
            /* In struct methods, hoist to temp for tracking */
            if (in_method) {
                char *temp = code_gen_emit_arena_temp(gen, v2_call);
                if (handle_mode) return temp;
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", temp);
            }
            if (handle_mode) {
                return v2_call;
            }
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_toLower(%s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* startsWith(prefix) - returns bool */
    if (strcmp(method_name, "startsWith") == 0 && arg_count == 1) {
        if (arena_mode) {
            gen->expr_as_handle = true;
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_startsWith_v2(%s, %s)", object_h, arg_h);
        }
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_startsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* endsWith(suffix) - returns bool */
    if (strcmp(method_name, "endsWith") == 0 && arg_count == 1) {
        if (arena_mode) {
            gen->expr_as_handle = true;
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_endsWith_v2(%s, %s)", object_h, arg_h);
        }
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_endsWith(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* contains(search) - returns bool */
    if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        if (arena_mode) {
            gen->expr_as_handle = true;
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_contains_v2(%s, %s)", object_h, arg_h);
        }
        char *arg_str = code_gen_expression(gen, arguments[0]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_contains(%s, %s)",
            object_is_temp ? "_obj_tmp" : object_str, arg_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* replace(old, new) - returns string */
    if (strcmp(method_name, "replace") == 0 && arg_count == 2) {
        if (arena_mode) {
            /* V2: evaluate args in handle mode, pass handles */
            gen->expr_as_handle = true;
            char *old_h = code_gen_expression(gen, arguments[0]);
            char *new_h = code_gen_expression(gen, arguments[1]);
            gen->expr_as_handle = handle_mode;
            char *v2_call = arena_sprintf(gen->arena, "rt_str_replace_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_h, old_h, new_h);
            /* In struct methods, hoist to temp for tracking */
            if (in_method) {
                char *temp = code_gen_emit_arena_temp(gen, v2_call);
                if (handle_mode) return temp;
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", temp);
            }
            if (handle_mode) {
                return v2_call;
            }
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", v2_call);
        }
        char *old_str = code_gen_expression(gen, arguments[0]);
        char *new_str = code_gen_expression(gen, arguments[1]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_replace(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, old_str, new_str);
        STRING_METHOD_RETURNING_STRING(gen, object_is_temp, object_str, method_call);
    }

    /* charAt(index) - returns char */
    if (strcmp(method_name, "charAt") == 0 && arg_count == 1) {
        if (arena_mode) {
            char *index_str = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "(char)rt_str_charAt_v2(%s, %s)", object_h, index_str);
        }
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
            /* Raw byte array to handle - wrap using generic create */
            return arena_sprintf(gen->arena,
                "({ unsigned char *__bytes = %s; "
                "rt_array_create_generic_v2(%s, rt_v2_data_array_length((void *)__bytes), sizeof(unsigned char), __bytes); })",
                _raw_result, ARENA_VAR(gen));
        }
        return _raw_result;
    }

    /* splitWhitespace() - returns string array */
    if (strcmp(method_name, "splitWhitespace") == 0 && arg_count == 0) {
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_split_whitespace_v2(%s, %s)",
                ARENA_VAR(gen), object_h);
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
        if (arena_mode) {
            /* V2: pass handle directly */
            char *v2_call = arena_sprintf(gen->arena, "rt_str_split_lines_v2(%s, %s)",
                ARENA_VAR(gen), object_h);
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
        if (arena_mode) {
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_is_blank_v2(%s)", object_h);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_is_blank(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "int", method_call);
    }

    /* toInt() - returns int (parse string as integer) */
    if (strcmp(method_name, "toInt") == 0 && arg_count == 0) {
        if (arena_mode) {
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_to_int_v2(%s)", object_h);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_int(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long long", method_call);
    }

    /* toLong() - returns long (parse string as long integer) */
    if (strcmp(method_name, "toLong") == 0 && arg_count == 0) {
        if (arena_mode) {
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_to_long_v2(%s)", object_h);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_long(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "long long", method_call);
    }

    /* toDouble() - returns double (parse string as double) */
    if (strcmp(method_name, "toDouble") == 0 && arg_count == 0) {
        if (arena_mode) {
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_to_double_v2(%s)", object_h);
        }
        char *method_call = arena_sprintf(gen->arena, "rt_str_to_double(%s)",
            object_is_temp ? "_obj_tmp" : object_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_VALUE(gen, object_is_temp, object_str, "double", method_call);
    }

    /* split(delimiter, limit) - returns string array with at most 'limit' parts */
    if (strcmp(method_name, "split") == 0 && arg_count == 2) {
        if (arena_mode && handle_mode) {
            /* V2: evaluate delimiter in handle mode, pass handles */
            gen->expr_as_handle = true;
            char *delim_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = false;
            char *limit_str = code_gen_expression(gen, arguments[1]);
            gen->expr_as_handle = handle_mode;
            return arena_sprintf(gen->arena, "rt_str_split_n_v2(%s, %s, %s, %s)",
                ARENA_VAR(gen), object_h, delim_h, limit_str);
        }
        char *delimiter_str = code_gen_expression(gen, arguments[0]);
        char *limit_str = code_gen_expression(gen, arguments[1]);
        char *method_call = arena_sprintf(gen->arena, "rt_str_split_n(%s, %s, %s, %s)",
            ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, delimiter_str, limit_str);
        gen->expr_as_handle = handle_mode;
        STRING_METHOD_RETURNING_ARRAY(gen, object_is_temp, object_str, "char", method_call);
    }

    /* append(str) - appends to mutable string, returns new string pointer */
    if (strcmp(method_name, "append") == 0 && arg_count == 1) {
        Type *arg_type = arguments[0]->expr_type;

        if (arg_type == NULL || arg_type->kind != TYPE_STRING) {
            fprintf(stderr, "Error: append() argument must be a string\n");
            exit(1);
        }

        gen->expr_as_handle = handle_mode;

        /* In handle mode: use rt_str_append_v2 which returns a new handle.
         * rt_str_append_v2(arena, old_handle, suffix_handle) - takes handles directly. */
        if (arena_mode && object->type == EXPR_VARIABLE) {
            /* Get the handle variable name */
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_name = code_gen_expression(gen, object);
            char *arg_h = code_gen_expression(gen, arguments[0]);
            gen->expr_as_handle = prev;
            return arena_sprintf(gen->arena,
                "(%s = rt_str_append_v2(%s, %s, %s))",
                handle_name, ARENA_VAR(gen), object_h, arg_h);
        }

        /* Legacy path: First ensure the string is mutable, then append. */
        char *arg_str = code_gen_expression(gen, arguments[0]);
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
    if (gen->current_arena_var != NULL) {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *obj_h = code_gen_expression(gen, object);
        gen->expr_as_handle = saved;
        return arena_sprintf(gen->arena, "rt_str_length_v2(%s)", obj_h);
    }
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
}
