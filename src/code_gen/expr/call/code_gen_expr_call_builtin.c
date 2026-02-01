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
        switch (elem_type->kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            print_func = "rt_print_array_long";
            break;
        case TYPE_DOUBLE:
            print_func = "rt_print_array_double";
            break;
        case TYPE_CHAR:
            print_func = "rt_print_array_char";
            break;
        case TYPE_BOOL:
            print_func = "rt_print_array_bool";
            break;
        case TYPE_BYTE:
            print_func = "rt_print_array_byte";
            break;
        case TYPE_STRING:
            if (gen->current_arena_var) {
                /* For print(str_array), we need the raw handle array (RtHandle*),
                 * not the char** that rt_managed_pin_string_array returns. */
                bool prev = gen->expr_as_handle;
                gen->expr_as_handle = true;
                char *handle_expr = code_gen_expression(gen, call->arguments[0]);
                gen->expr_as_handle = prev;
                return arena_sprintf(gen->arena, "rt_print_array_string_h(%s, (RtHandle *)rt_managed_pin_array(%s, %s))",
                                     ARENA_VAR(gen), ARENA_VAR(gen), handle_expr);
            }
            print_func = "rt_print_array_string";
            break;
        default:
            fprintf(stderr, "Error: unsupported array element type for print\n");
            exit(1);
        }
        break;
    }
    default:
        fprintf(stderr, "Error: unsupported type for print\n");
        exit(1);
    }
    return arena_sprintf(gen->arena, "%s(%s)", print_func, arg_strs[0]);
}

/**
 * Generate code for the len() builtin.
 * Returns strlen for strings, rt_array_length for arrays.
 */
static char *code_gen_builtin_len(CodeGen *gen, CallExpr *call, char **arg_strs)
{
    if (call->arg_count != 1)
        return NULL;

    Type *arg_type = call->arguments[0]->expr_type;
    if (arg_type && arg_type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "(long)strlen(%s)", arg_strs[0]);
    }
    return arena_sprintf(gen->arena, "rt_array_length(%s)", arg_strs[0]);
}

/**
 * Generate code for the readLine() builtin.
 */
static char *code_gen_builtin_readline(CodeGen *gen)
{
    if (gen->expr_as_handle && gen->current_arena_var != NULL)
    {
        return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, rt_read_line(%s))",
                             ARENA_VAR(gen), ARENA_VAR(gen));
    }
    return arena_sprintf(gen->arena, "rt_read_line(%s)", ARENA_VAR(gen));
}

/**
 * Generate code for the println() builtin.
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
            ? get_rt_to_string_func_for_type_h(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
        return arena_sprintf(gen->arena, "rt_println(%s(%s, %s))",
                             to_str_func, ARENA_VAR(gen), arg_strs[0]);
    }
}

/**
 * Generate code for the printErr() builtin.
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
            ? get_rt_to_string_func_for_type_h(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
        return arena_sprintf(gen->arena, "rt_print_err(%s(%s, %s))",
                             to_str_func, ARENA_VAR(gen), arg_strs[0]);
    }
}

/**
 * Generate code for the printErrLn() builtin.
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
            ? get_rt_to_string_func_for_type_h(arg_type)
            : get_rt_to_string_func_for_type(arg_type);
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
