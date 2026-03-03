/**
 * code_gen_expr_call_closure.c - Code generation for closure calls
 *
 * Handles invocation of closure variables (function-type variables that
 * hold lambda expressions or function references).
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
 * Check if a call expression is a closure call (as opposed to a direct function call).
 * A closure call is identified by:
 * - Callee has function type
 * - Callee is not a named function (it's a variable holding a closure)
 * - Callee is not a native callback
 */
bool is_closure_call_expr(CodeGen *gen, CallExpr *call)
{
    Type *callee_type = call->callee->expr_type;
    if (callee_type == NULL || callee_type->kind != TYPE_FUNCTION)
        return false;

    /* Native callbacks are called directly as function pointers, not closures */
    if (callee_type->as.function.is_native)
        return false;

    if (call->callee->type == EXPR_VARIABLE)
    {
        char *name = get_var_name(gen->arena, call->callee->as.variable.name);
        /* Skip builtins */
        if (strcmp(name, "print") == 0 || strcmp(name, "len") == 0 ||
            strcmp(name, "readLine") == 0 || strcmp(name, "println") == 0 ||
            strcmp(name, "printErr") == 0 || strcmp(name, "printErrLn") == 0 ||
            strcmp(name, "exit") == 0 || strcmp(name, "assert") == 0)
        {
            return false;
        }

        /* Check if this is a named function or a closure variable.
         * Only treat as closure if we find a symbol that ISN'T a function. */
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
        if (sym != NULL && !sym->is_function)
        {
            return true;
        }
        return false;
    }
    /* Array access where element is a function type (e.g., callbacks[0]()) */
    else if (call->callee->type == EXPR_ARRAY_ACCESS)
    {
        return true;
    }
    /* Member access where struct field is a function type (e.g., handler.callback()) */
    else if (call->callee->type == EXPR_MEMBER)
    {
        return true;
    }

    return false;
}

/**
 * Generate code for a closure call.
 * Closures are called by casting closure->fn to the appropriate function pointer type
 * and passing the closure itself as the first argument.
 */
char *code_gen_closure_call(CodeGen *gen, Expr *expr, CallExpr *call)
{
    (void)expr; /* used for interface consistency */
    Type *callee_type = call->callee->expr_type;
    char *closure_str = code_gen_expression(gen, call->callee);

    /* Build function pointer cast */
    const char *ret_c_type = get_c_type(gen->arena, callee_type->as.function.return_type);
    /* First param is closure (void *), second is caller arena (RtArenaV2 *) */
    char *param_types_str = arena_strdup(gen->arena, "void *, RtArenaV2 *");
    for (int i = 0; i < callee_type->as.function.param_count; i++)
    {
        const char *param_c_type = get_c_type(gen->arena, callee_type->as.function.param_types[i]);
        param_types_str = arena_sprintf(gen->arena, "%s, %s", param_types_str, param_c_type);
    }

    /* Generate arguments: closure, caller_arena, then actual args */
    const char *arena_arg = gen->current_arena_var != NULL ? gen->current_arena_var : "NULL";
    char *args_str = arena_sprintf(gen->arena, "%s, %s", closure_str, arena_arg);
    for (int i = 0; i < call->arg_count; i++)
    {
        char *arg_str = code_gen_expression(gen, call->arguments[i]);
        args_str = arena_sprintf(gen->arena, "%s, %s", args_str, arg_str);
    }

    /* Generate the call: ((<ret> (*)(<params>))closure->fn)(closure, arena, args...) */
    return arena_sprintf(gen->arena, "((%s (*)(%s))((__Closure__ *)%s->ptr)->fn)(%s)",
                         ret_c_type, param_types_str, closure_str, args_str);
}
