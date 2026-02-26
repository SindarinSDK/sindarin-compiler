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

    /* For void functions with an expression (expression-bodied void functions),
     * we still need to emit the expression for its side effects. */
    if (stmt->value && is_void_return)
    {
        char *value_str = code_gen_expression(gen, stmt->value);
        indented_fprintf(gen, indent, "%s;\n", value_str);
    }

    /* Normal return with value */
    if (stmt->value && !is_void_return)
    {
        /* If returning a lambda expression directly, allocate it in the caller's
         * arena so captured variables survive the function's arena destruction. */
        bool is_lambda_return = (stmt->value->type == EXPR_LAMBDA);
        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = true;
        }

        /* If the function returns a handle type, produce RtHandle value.
         * Native struct returns are also handle-based (RtHandleV2*) for promotion. */
        bool prev_as_handle = gen->expr_as_handle;
        if (gen->current_return_type != NULL && gen->current_arena_var != NULL)
        {
            Type *resolved_ret = resolve_struct_type(gen, gen->current_return_type);
            if (is_handle_type(resolved_ret) ||
                (resolved_ret->kind == TYPE_STRUCT &&
                 resolved_ret->as.struct_type.is_native &&
                 resolved_ret->as.struct_type.c_alias != NULL))
            {
                gen->expr_as_handle = true;
            }
        }

        int saved_temp_count = gen->arena_temp_count;
        char *value_str = code_gen_expression(gen, stmt->value);

        /* Handle temps created during return expression evaluation */
        if (gen->current_arena_var != NULL && gen->arena_temp_count > saved_temp_count)
        {
            bool in_method = (gen->function_arena_var != NULL &&
                              strcmp(gen->function_arena_var, "__caller_arena__") == 0);
            if (in_method)
            {
                /* Struct method: no arena condemn, so free intermediate temps now.
                 * Skip the temp that IS the return value to avoid use-after-free. */
                for (int i = saved_temp_count; i < gen->arena_temp_count; i++)
                {
                    if (strcmp(gen->arena_temps[i], value_str) != 0)
                    {
                        indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", gen->arena_temps[i]);
                    }
                }
                gen->arena_temp_count = saved_temp_count;
            }
            else
            {
                /* Regular function: adopt temps — the return value is promoted to
                 * caller's arena, freeing would cause use-after-free. Arena condemn
                 * handles cleanup of non-return-value temps. */
                code_gen_adopt_arena_temps_from(gen, saved_temp_count);
            }
        }

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
            indented_fprintf(gen, indent, "rt_arena_v2_condemn(%s);\n", gen->arena_stack[i]);
        }
    }

    /* Release all active locks before returning (reverse order for nested locks) */
    for (int i = gen->lock_stack_depth - 1; i >= 0; i--)
    {
        indented_fprintf(gen, indent, "rt_sync_unlock(&%s);\n", gen->lock_stack[i]);
    }

    /* In struct methods, free any tracked arena temps before the goto.
     * Temps created in inner expressions (e.g. toLower in a loop) would normally
     * be freed at statement boundary, but goto skips past that cleanup.
     * IMPORTANT: Do NOT clear arena_temp_count here. This code runs inside a
     * branch (if body). The if-condition/statement flush also emits frees for
     * the fallthrough path. Both paths need frees; only one executes at runtime. */
    if (gen->current_arena_var != NULL && gen->arena_temp_count > 0 &&
        gen->function_arena_var != NULL &&
        strcmp(gen->function_arena_var, "__caller_arena__") == 0)
    {
        for (int i = 0; i < gen->arena_temp_count; i++)
        {
            indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", gen->arena_temps[i]);
        }
    }

    /* Clean up locals in all scopes between current and function scope.
     * When a return (goto) is inside nested blocks (if/while), the normal
     * block-exit cleanup is skipped. Walk inner scopes and emit cleanup
     * so string/struct/array handles are freed before the goto. */
    if (gen->function_scope != NULL)
    {
        Scope *scope = gen->symbol_table->current;
        while (scope != NULL && scope != gen->function_scope)
        {
            code_gen_free_locals(gen, scope, false, indent);
            scope = scope->enclosing;
        }
    }

    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}
