/**
 * code_gen_expr_call_namespace.c - Code generation for namespace function calls
 *
 * Handles:
 * - Direct namespace function calls (namespace.function())
 * - Nested namespace function calls (parent.nested.function())
 * - Namespace struct static method calls (namespace.Struct.method())
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Generate code for a direct namespace function call (namespace.function()).
 * Returns the generated C code, or NULL if not a namespace call.
 */
char *code_gen_namespace_function_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                        const char *member_name_str, CallExpr *call)
{
    Token ns_name = member->object->as.variable.name;
    Symbol *func_sym = symbol_table_lookup_in_namespace(gen->symbol_table, ns_name, member->member_name);
    bool callee_has_body = (func_sym != NULL &&
                            func_sym->type != NULL &&
                            func_sym->type->kind == TYPE_FUNCTION &&
                            func_sym->type->as.function.has_body);

    /* Determine which namespace prefix to use for the function call. */
    const char *effective_ns_prefix = NULL;
    char ns_buf[256];
    int ns_len = ns_name.length < 255 ? ns_name.length : 255;
    memcpy(ns_buf, ns_name.start, ns_len);
    ns_buf[ns_len] = '\0';
    effective_ns_prefix = arena_strdup(gen->arena, ns_buf);

    /* Always use prefixed names for namespace function calls. */
    bool use_prefixed_name = true;

    /* Use c_alias for native functions, or appropriate name for Sindarin functions. */
    const char *func_name_to_use;
    if (func_sym != NULL && func_sym->is_native)
    {
        func_name_to_use = (func_sym->c_alias != NULL) ? func_sym->c_alias : member_name_str;
    }
    else if (use_prefixed_name)
    {
        char prefixed_name[512];
        snprintf(prefixed_name, sizeof(prefixed_name), "%s__%s",
                 effective_ns_prefix, member_name_str);
        func_name_to_use = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        func_name_to_use = sn_mangle_name(gen->arena, member_name_str);
    }

    /* Generate arguments */
    bool ns_outer_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = (callee_has_body && gen->current_arena_var != NULL);
    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    for (int i = 0; i < call->arg_count; i++)
    {
        /* For native functions receiving str[] args: evaluate in handle mode
         * and convert RtHandle[] to char** using rt_managed_pin_string_array */
        if (!callee_has_body && gen->current_arena_var != NULL &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
            call->arguments[i]->expr_type->as.array.element_type != NULL &&
            call->arguments[i]->expr_type->as.array.element_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            arg_strs[i] = arena_sprintf(gen->arena, "rt_managed_pin_string_array(%s, %s)",
                                         ARENA_VAR(gen), handle_expr);
        }
        else
        {
            arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
        }
    }
    gen->expr_as_handle = ns_outer_as_handle;

    /* Build args list - prepend arena if function has body (Sindarin function) */
    char *args_list;
    if (callee_has_body && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (callee_has_body)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        if (args_list[0] == '\0')
        {
            args_list = arg_strs[i];
        }
        else
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_strs[i]);
        }
    }

    /* Emit function call using the resolved function name */
    char *ns_call_expr = arena_sprintf(gen->arena, "%s(%s)", func_name_to_use, args_list);

    /* If the function returns a handle type and we need raw pointer, pin it */
    if (!gen->expr_as_handle && callee_has_body &&
        gen->current_arena_var != NULL &&
        expr->expr_type != NULL && is_handle_type(expr->expr_type))
    {
        if (expr->expr_type->kind == TYPE_STRING)
        {
            return arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, %s)",
                                 ARENA_VAR(gen), ns_call_expr);
        }
        else if (expr->expr_type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
            return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                 elem_c, ARENA_VAR(gen), ns_call_expr);
        }
    }
    return ns_call_expr;
}

/**
 * Generate code for a nested namespace function call (parent.nested.function()).
 * Returns the generated C code, or NULL if not a nested namespace call.
 */
char *code_gen_nested_namespace_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                      const char *member_name_str, CallExpr *call)
{
    Symbol *nested_ns = member->object->as.member.resolved_namespace;

    /* Search for the function in the nested namespace */
    Symbol *func_sym = NULL;
    for (Symbol *sym = nested_ns->namespace_symbols; sym != NULL; sym = sym->next)
    {
        if (sym->name.length == member->member_name.length &&
            memcmp(sym->name.start, member->member_name.start, member->member_name.length) == 0)
        {
            func_sym = sym;
            break;
        }
    }

    bool callee_has_body = (func_sym != NULL &&
                            func_sym->type != NULL &&
                            func_sym->type->kind == TYPE_FUNCTION &&
                            func_sym->type->as.function.has_body);

    /* Determine which namespace prefix to use. */
    const char *effective_ns_prefix = NULL;
    if (nested_ns != NULL)
    {
        char ns_buf[256];
        int ns_len = nested_ns->name.length < 255 ? nested_ns->name.length : 255;
        memcpy(ns_buf, nested_ns->name.start, ns_len);
        ns_buf[ns_len] = '\0';
        effective_ns_prefix = arena_strdup(gen->arena, ns_buf);
    }

    /* Always use prefixed names for nested namespace function calls. */
    bool use_prefixed_name = true;

    const char *func_name_to_use;
    if (func_sym != NULL && func_sym->is_native)
    {
        func_name_to_use = (func_sym->c_alias != NULL) ? func_sym->c_alias : member_name_str;
    }
    else if (use_prefixed_name && effective_ns_prefix != NULL)
    {
        char prefixed_name[512];
        snprintf(prefixed_name, sizeof(prefixed_name), "%s__%s",
                 effective_ns_prefix, member_name_str);
        func_name_to_use = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        func_name_to_use = sn_mangle_name(gen->arena, member_name_str);
    }

    /* Generate arguments - same logic as regular namespace calls */
    bool ns_outer_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = (callee_has_body && gen->current_arena_var != NULL);
    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    for (int i = 0; i < call->arg_count; i++)
    {
        if (!callee_has_body && gen->current_arena_var != NULL &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
            call->arguments[i]->expr_type->as.array.element_type != NULL &&
            call->arguments[i]->expr_type->as.array.element_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            arg_strs[i] = arena_sprintf(gen->arena, "rt_managed_pin_string_array(%s, %s)",
                                         ARENA_VAR(gen), handle_expr);
        }
        else
        {
            arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
        }
    }
    gen->expr_as_handle = ns_outer_as_handle;

    /* Build args list - prepend arena if function has body (Sindarin function) */
    char *args_list;
    if (callee_has_body && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (callee_has_body)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        if (args_list[0] == '\0')
        {
            args_list = arg_strs[i];
        }
        else
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_strs[i]);
        }
    }

    /* Emit function call */
    char *nested_ns_call_expr = arena_sprintf(gen->arena, "%s(%s)", func_name_to_use, args_list);

    /* If the function returns a handle type and we need raw pointer, pin it */
    if (!gen->expr_as_handle && callee_has_body &&
        gen->current_arena_var != NULL &&
        expr->expr_type != NULL && is_handle_type(expr->expr_type))
    {
        if (expr->expr_type->kind == TYPE_STRING)
        {
            nested_ns_call_expr = arena_sprintf(gen->arena, "((char *)rt_managed_pin(%s, %s))",
                                                 ARENA_VAR(gen), nested_ns_call_expr);
        }
        else if (expr->expr_type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
            nested_ns_call_expr = arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                                 elem_c, ARENA_VAR(gen), nested_ns_call_expr);
        }
    }
    return nested_ns_call_expr;
}

/**
 * Generate code for a namespace struct static method call (namespace.StructType.staticMethod()).
 * Returns the generated C code, or NULL if not a namespace static method call.
 */
char *code_gen_namespace_static_method_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                             CallExpr *call)
{
    Type *struct_type = member->object->as.member.resolved_struct_type;
    StructMethod *method = member->resolved_method;

    if (method == NULL)
        return NULL;

    /* Generate static method call - static methods don't have a 'self' argument */
    bool callee_has_body = !method->is_native && method->body != NULL;

    /* Generate arguments */
    bool outer_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = (callee_has_body && gen->current_arena_var != NULL);
    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    for (int i = 0; i < call->arg_count; i++)
    {
        arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
    }
    gen->expr_as_handle = outer_as_handle;

    /* Build args list - prepend arena if Sindarin function with body */
    char *args_list;
    if (callee_has_body && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (callee_has_body)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        if (args_list[0] == '\0')
        {
            args_list = arg_strs[i];
        }
        else
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_strs[i]);
        }
    }

    /* Determine function name - mangle struct name and append method name */
    char *mangled_struct = sn_mangle_name(gen->arena, struct_type->as.struct_type.name);
    char *func_name = arena_sprintf(gen->arena, "%s_%s",
        mangled_struct, method->name);

    /* Emit static method call */
    char *static_call_expr = arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);

    /* If the method returns a handle type and we need raw pointer, pin it */
    if (!gen->expr_as_handle && callee_has_body &&
        gen->current_arena_var != NULL &&
        expr->expr_type != NULL && is_handle_type(expr->expr_type))
    {
        if (expr->expr_type->kind == TYPE_STRING)
        {
            static_call_expr = arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, %s)",
                                             ARENA_VAR(gen), static_call_expr);
        }
        else if (expr->expr_type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
            static_call_expr = arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                             elem_c, ARENA_VAR(gen), static_call_expr);
        }
    }
    return static_call_expr;
}
