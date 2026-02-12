/**
 * code_gen_expr_call_builtin.c - Code generation for builtin function calls
 *
 * Handles builtin functions: print, println, printErr, printErrLn, len,
 * readLine, exit, assert
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Generate code for the print() builtin.
 * Maps to appropriate rt_print_* based on argument type.
 */
static char *code_gen_builtin_print(CodeGen *gen, Expr *expr, CallExpr *call, char **arg_strs)
{
    (void)expr; /* used for interface consistency */
    if (call->arg_count != 1)
    {
        fprintf(stderr, "Error: print expects exactly one argument\n");
        exit(1);
    }
    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type == NULL)
    {
        fprintf(stderr, "Error: print argument has no type\n");
        exit(1);
    }

    const char *print_func = NULL;
    switch (arg_type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        print_func = "rt_print_long";
        break;
    case TYPE_DOUBLE:
        print_func = "rt_print_double";
        break;
    case TYPE_CHAR:
        print_func = "rt_print_char";
        break;
    case TYPE_BOOL:
        print_func = "rt_print_bool";
        break;
    case TYPE_BYTE:
        print_func = "rt_print_byte";
        break;
    case TYPE_STRING:
        print_func = "rt_print_string";
        break;
    case TYPE_ARRAY:
    {
        Type *elem_type = arg_type->as.array.element_type;
        /* Use V2 print functions that take handles */
        const char *v2_func = NULL;
        switch (elem_type->kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            v2_func = "rt_print_array_long_v2";
            break;
        case TYPE_INT32:
            v2_func = "rt_print_array_int32_v2";
            break;
        case TYPE_UINT:
            v2_func = "rt_print_array_uint_v2";
            break;
        case TYPE_UINT32:
            v2_func = "rt_print_array_uint32_v2";
            break;
        case TYPE_FLOAT:
            v2_func = "rt_print_array_float_v2";
            break;
        case TYPE_DOUBLE:
            v2_func = "rt_print_array_double_v2";
            break;
        case TYPE_CHAR:
            v2_func = "rt_print_array_char_v2";
            break;
        case TYPE_BOOL:
            v2_func = "rt_print_array_bool_v2";
            break;
        case TYPE_BYTE:
            v2_func = "rt_print_array_byte_v2";
            break;
        case TYPE_STRING:
            v2_func = "rt_print_array_string_v2";
            break;
        default:
            fprintf(stderr, "Error: unsupported array element type for print\n");
            exit(1);
        }
        bool prev = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_expr = code_gen_expression(gen, call->arguments[0]);
        gen->expr_as_handle = prev;
        return arena_sprintf(gen->arena, "%s(%s)", v2_func, handle_expr);
    }
    default:
        fprintf(stderr, "Error: unsupported type for print\n");
        exit(1);
    }
    return arena_sprintf(gen->arena, "%s(%s)", print_func, arg_strs[0]);
}

/**
 * Generate code for the len() builtin.
 * Returns strlen for strings, rt_array_length_v2 for arrays.
 */
static char *code_gen_builtin_len(CodeGen *gen, CallExpr *call, char **arg_strs)
{
    (void)arg_strs;  /* We generate the array expression in handle mode ourselves */
    if (call->arg_count != 1)
        return NULL;

    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type && arg_type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "(long)strlen(%s)", arg_strs[0]);
    }
    /* For arrays, generate the expression in handle mode for V2 */
    bool saved = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, call->arguments[0]);
    gen->expr_as_handle = saved;
    return arena_sprintf(gen->arena, "(long long)rt_array_length_v2(%s)", handle_str);
}

/**
 * Generate code for the readLine() builtin.
 * rt_read_line now returns RtHandleV2*.
 */
static char *code_gen_builtin_readline(CodeGen *gen)
{
    if (gen->expr_as_handle && gen->current_arena_var != NULL)
    {
        /* Handle mode: return the RtHandleV2* directly */
        return arena_sprintf(gen->arena, "rt_read_line(%s)", ARENA_VAR(gen));
    }
    /* Non-handle mode: pin to get char* */
    return arena_sprintf(gen->arena,
        "({ RtHandleV2 *__h = rt_read_line(%s); rt_handle_v2_pin(__h); (char *)__h->ptr; })",
        ARENA_VAR(gen));
}

/**
 * Generate code for the println() builtin.
 * V2 toString functions return RtHandleV2* - need to pin before passing to rt_println.
 */
static char *code_gen_builtin_println(CodeGen *gen, CallExpr *call, char **arg_strs)
{
    if (call->arg_count != 1)
        return NULL;

    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "rt_println(%s)", arg_strs[0]);
    }
    else
    {
        const char *to_str_func = gen->current_arena_var
            ? get_rt_to_string_func_for_type_v2(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
        if (gen->current_arena_var)
        {
            /* V2 toString returns RtHandleV2* - pin to get char* for rt_println */
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_println((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        /* Non-arena path: rt_any_to_string returns RtHandleV2*, others return char* */
        if (arg_type->kind == TYPE_ANY)
        {
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_println((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        return arena_sprintf(gen->arena, "rt_println(%s(%s, %s))",
                             to_str_func, ARENA_VAR(gen), arg_strs[0]);
    }
}

/**
 * Generate code for the printErr() builtin.
 * V2 toString functions return RtHandleV2* - need to pin before passing to rt_print_err.
 */
static char *code_gen_builtin_printerr(CodeGen *gen, CallExpr *call, char **arg_strs)
{
    if (call->arg_count != 1)
        return NULL;

    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "rt_print_err(%s)", arg_strs[0]);
    }
    else
    {
        const char *to_str_func = gen->current_arena_var
            ? get_rt_to_string_func_for_type_v2(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
        if (gen->current_arena_var)
        {
            /* V2 toString returns RtHandleV2* - pin to get char* for rt_print_err */
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_print_err((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        /* Non-arena path: rt_any_to_string returns RtHandleV2*, others return char* */
        if (arg_type->kind == TYPE_ANY)
        {
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_print_err((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        return arena_sprintf(gen->arena, "rt_print_err(%s(%s, %s))",
                             to_str_func, ARENA_VAR(gen), arg_strs[0]);
    }
}

/**
 * Generate code for the printErrLn() builtin.
 * V2 toString functions return RtHandleV2* - need to pin before passing to rt_print_err_ln.
 */
static char *code_gen_builtin_printerrln(CodeGen *gen, CallExpr *call, char **arg_strs)
{
    if (call->arg_count != 1)
        return NULL;

    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "rt_print_err_ln(%s)", arg_strs[0]);
    }
    else
    {
        const char *to_str_func = gen->current_arena_var
            ? get_rt_to_string_func_for_type_v2(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
        if (gen->current_arena_var)
        {
            /* V2 toString returns RtHandleV2* - pin to get char* for rt_print_err_ln */
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_print_err_ln((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        /* Non-arena path: rt_any_to_string returns RtHandleV2*, others return char* */
        if (arg_type->kind == TYPE_ANY)
        {
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__h = %s(%s, %s); rt_handle_v2_pin(__h); rt_print_err_ln((char *)__h->ptr); })",
                to_str_func, ARENA_VAR(gen), arg_strs[0]);
        }
        return arena_sprintf(gen->arena, "rt_print_err_ln(%s(%s, %s))",
                             to_str_func, ARENA_VAR(gen), arg_strs[0]);
    }
}

/**
 * Try to generate code for a builtin function call.
 * Returns the generated code if the callee is a builtin, NULL otherwise.
 * Also updates callee_str_out to the appropriate runtime function if needed.
 */
char *code_gen_try_builtin_call(CodeGen *gen, Expr *expr, CallExpr *call,
                                 char **arg_strs, char **callee_str_out)
{
    (void)callee_str_out; /* reserved for future use */
    if (call->callee->type != EXPR_VARIABLE)
        return NULL;

    char *callee_name = get_var_name(gen->arena, call->callee->as.variable.name);

    if (strcmp(callee_name, "print") == 0)
    {
        return code_gen_builtin_print(gen, expr, call, arg_strs);
    }
    else if (strcmp(callee_name, "len") == 0 && call->arg_count == 1)
    {
        return code_gen_builtin_len(gen, call, arg_strs);
    }
    else if (strcmp(callee_name, "readLine") == 0 && call->arg_count == 0)
    {
        return code_gen_builtin_readline(gen);
    }
    else if (strcmp(callee_name, "println") == 0 && call->arg_count == 1)
    {
        return code_gen_builtin_println(gen, call, arg_strs);
    }
    else if (strcmp(callee_name, "printErr") == 0 && call->arg_count == 1)
    {
        return code_gen_builtin_printerr(gen, call, arg_strs);
    }
    else if (strcmp(callee_name, "printErrLn") == 0 && call->arg_count == 1)
    {
        return code_gen_builtin_printerrln(gen, call, arg_strs);
    }
    else if (strcmp(callee_name, "exit") == 0 && call->arg_count == 1)
    {
        return arena_sprintf(gen->arena, "rt_exit(%s)", arg_strs[0]);
    }
    else if (strcmp(callee_name, "assert") == 0 && call->arg_count == 2)
    {
        return arena_sprintf(gen->arena, "rt_assert(%s, %s)", arg_strs[0], arg_strs[1]);
    }

    return NULL;
}
