/*
 * code_gen_stmt_return.c - Return statement code generation
 *
 * Handles code generation for return statements including tail call
 * optimization and value boxing.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");

    /* Check if returning from a void function/lambda */
    int is_void_return = (gen->current_return_type && gen->current_return_type->kind == TYPE_VOID);

    /* Check if this return contains a tail call that should be optimized */
    if (gen->in_tail_call_function && stmt->value &&
        stmt->value->type == EXPR_CALL && stmt->value->as.call.is_tail_call)
    {
        CallExpr *call = &stmt->value->as.call;
        FunctionStmt *fn = gen->tail_call_fn;

        /* Generate parameter assignments */
        if (fn->param_count > 1)
        {
            /* First, generate temp variables for all new argument values */
            for (int i = 0; i < call->arg_count; i++)
            {
                const char *param_type_c = get_c_type(gen->arena, fn->params[i].type);
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                indented_fprintf(gen, indent, "%s __tail_arg_%d__ = %s;\n",
                                 param_type_c, i, arg_str);
            }
            /* Then, assign temps to actual parameters */
            for (int i = 0; i < call->arg_count; i++)
            {
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, fn->params[i].name));
                indented_fprintf(gen, indent, "%s = __tail_arg_%d__;\n",
                                 param_name, i);
            }
        }
        else if (fn->param_count == 1)
        {
            /* Single parameter - direct assignment is safe */
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, fn->params[0].name));
            char *arg_str = code_gen_expression(gen, call->arguments[0]);
            indented_fprintf(gen, indent, "%s = %s;\n", param_name, arg_str);
        }
        /* Continue the tail call loop */
        indented_fprintf(gen, indent, "continue;\n");
        return;
    }

    /* Normal return */
    if (stmt->value && !is_void_return)
    {
        /* If returning a lambda expression directly, allocate it in the caller's
         * arena so captured variables survive the function's arena destruction. */
        bool is_lambda_return = (stmt->value->type == EXPR_LAMBDA);
        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = true;
        }

        /* If the function returns a handle type, produce RtHandle value */
        bool prev_as_handle = gen->expr_as_handle;
        if (gen->current_return_type != NULL && is_handle_type(gen->current_return_type) &&
            gen->current_arena_var != NULL)
        {
            gen->expr_as_handle = true;
        }

        char *value_str = code_gen_expression(gen, stmt->value);

        gen->expr_as_handle = prev_as_handle;

        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = false;
        }

        /* Handle returning 'self' pointer as struct value (builder/fluent pattern) */
        if (stmt->value->type == EXPR_VARIABLE &&
            gen->current_return_type != NULL &&
            gen->current_return_type->kind == TYPE_STRUCT)
        {
            Token var_name = stmt->value->as.variable.name;
            if (var_name.length == 4 && strncmp(var_name.start, "self", 4) == 0)
            {
                value_str = arena_sprintf(gen->arena, "(*%s)", value_str);
            }
        }

        /* Handle boxing when function returns 'any' but expression is concrete */
        if (gen->current_return_type != NULL && gen->current_return_type->kind == TYPE_ANY &&
            stmt->value->expr_type != NULL && stmt->value->expr_type->kind != TYPE_ANY)
        {
            value_str = code_gen_box_value(gen, value_str, stmt->value->expr_type);
        }

        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }

    /* Clean up all active private block arenas before returning */
    for (int i = gen->arena_stack_depth - 1; i >= 0; i--)
    {
        if (gen->arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_managed_arena_destroy_child(%s);\n", gen->arena_stack[i]);
        }
    }

    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}
