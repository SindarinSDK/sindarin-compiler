/**
 * code_gen_expr_call_struct.c - Code generation for struct method calls
 *
 * Handles:
 * - Native struct method calls (method with is_native flag)
 * - Non-native struct method calls (Sindarin methods)
 * - Pointer-to-struct method calls (e.g., self.method() inside a method body)
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * Generate code for a native struct method call.
 * Returns the generated C code.
 */
char *code_gen_native_struct_method_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                          StructMethod *method, Type *struct_type,
                                          CallExpr *call)
{
    (void)expr; /* used for interface consistency */
    const char *struct_name = struct_type->as.struct_type.name;

    /* Native method call - use c_alias if present, else use naming convention */
    const char *func_name;
    if (method->c_alias != NULL)
    {
        func_name = method->c_alias;
    }
    else
    {
        /* Create lowercase struct name for native method naming */
        char *struct_name_lower = arena_strdup(gen->arena, struct_name);
        for (char *p = struct_name_lower; *p; p++)
        {
            *p = (char)tolower((unsigned char)*p);
        }
        func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
    }

    /* Build args list - prepend arena if method has has_arena_param */
    char *args_list;
    if (method->has_arena_param && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (method->has_arena_param)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    /* For instance native methods, pass self as first arg */
    if (!method->is_static)
    {
        char *self_str = code_gen_expression(gen, member->object);
        /* For opaque handle types (native struct with c_alias), self is already a pointer */
        if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
        {
            /* Opaque handle: self is already a pointer, pass directly */
            if (args_list[0] != '\0')
            {
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
            }
            else
            {
                args_list = arena_strdup(gen->arena, self_str);
            }
        }
        else if (struct_type->as.struct_type.pass_self_by_ref)
        {
            /* Pass by reference (pointer) - handle rvalue chaining */
            char *mangled_type = sn_mangle_name(gen->arena, struct_name);
            char *self_ref = code_gen_self_ref(gen, member->object, mangled_type, self_str);
            if (args_list[0] != '\0')
            {
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_ref);
            }
            else
            {
                args_list = arena_strdup(gen->arena, self_ref);
            }
        }
        else
        {
            /* Pass by value */
            if (args_list[0] != '\0')
            {
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
            }
            else
            {
                args_list = arena_strdup(gen->arena, self_str);
            }
        }
    }

    /* Track native string handle args for transactional FFI boundary */
    char **native_str_handle_exprs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    int native_str_handle_count = 0;

    /* Add remaining arguments */
    for (int i = 0; i < call->arg_count; i++)
    {
        char *arg_str;
        /* For native methods receiving str[] args: evaluate in handle mode
         * and convert RtHandle[] to char** */
        if (gen->current_arena_var != NULL &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
            call->arguments[i]->expr_type->as.array.element_type != NULL &&
            call->arguments[i]->expr_type->as.array.element_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            arg_str = arena_sprintf(gen->arena, "rt_pin_string_array_v2(%s)",
                                     handle_expr);
        }
        /* For native methods receiving individual str args: convert RtHandle to const char*
         * using transactional access that spans the entire native call. */
        else if (gen->current_arena_var != NULL &&
                 call->arguments[i]->expr_type != NULL &&
                 call->arguments[i]->expr_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            int idx = native_str_handle_count++;
            native_str_handle_exprs[idx] = handle_expr;
            arg_str = arena_sprintf(gen->arena, "(const char *)__nsh_%d__->ptr", idx);
        }
        else
        {
            arg_str = code_gen_expression(gen, call->arguments[i]);
        }
        if (args_list[0] != '\0')
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
        }
        else
        {
            args_list = arg_str;
        }
    }

    char *call_result = arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);

    /* Wrap with transactions for native string handle args */
    if (native_str_handle_count > 0)
    {
        bool returns_void = (method->return_type != NULL && method->return_type->kind == TYPE_VOID);
        char *wrapped = arena_strdup(gen->arena, "({\n");
        for (int i = 0; i < native_str_handle_count; i++)
            wrapped = arena_sprintf(gen->arena, "%s    RtHandleV2 *__nsh_%d__ = %s;\n",
                                    wrapped, i, native_str_handle_exprs[i]);
        for (int i = 0; i < native_str_handle_count; i++)
            wrapped = arena_sprintf(gen->arena, "%s    rt_handle_begin_transaction(__nsh_%d__);\n",
                                    wrapped, i);
        if (returns_void)
            wrapped = arena_sprintf(gen->arena, "%s    %s;\n", wrapped, call_result);
        else
        {
            const char *ret_c = get_c_type(gen->arena, method->return_type);
            wrapped = arena_sprintf(gen->arena, "%s    %s __nffi_r__ = %s;\n",
                                    wrapped, ret_c, call_result);
        }
        for (int i = native_str_handle_count - 1; i >= 0; i--)
            wrapped = arena_sprintf(gen->arena, "%s    rt_handle_end_transaction(__nsh_%d__);\n",
                                    wrapped, i);
        if (returns_void)
            wrapped = arena_sprintf(gen->arena, "%s    (void)0;\n})", wrapped);
        else
            wrapped = arena_sprintf(gen->arena, "%s    __nffi_r__;\n})", wrapped);
        call_result = wrapped;
    }

    /* Handle native methods returning str */
    if (method->return_type != NULL && method->return_type->kind == TYPE_STRING &&
        gen->current_arena_var != NULL)
    {
        if (!gen->expr_as_handle)
        {
            /* Need char* - extract string pointer from handle */
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)",
                call_result);
        }
    }
    return call_result;
}

/**
 * Generate code for a non-native (Sindarin) struct method call.
 * Returns the generated C code.
 */
char *code_gen_sindarin_struct_method_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                            StructMethod *method, Type *struct_type,
                                            CallExpr *call)
{
    (void)expr; /* used for interface consistency */
    const char *struct_name = struct_type->as.struct_type.name;
    char *mangled_struct = sn_mangle_name(gen->arena, struct_name);

    /* Check if this method should be intercepted */
    if (should_intercept_method(method, struct_type, method->return_type))
    {
        /* Compute self pointer expression for interception */
        char *self_ptr_str = NULL;
        bool is_self_pointer = false;
        if (!method->is_static)
        {
            char *self_str = code_gen_expression(gen, member->object);
            if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
            {
                self_ptr_str = self_str;
                is_self_pointer = true;
            }
            else if (member->object->expr_type != NULL &&
                     member->object->expr_type->kind == TYPE_POINTER)
            {
                self_ptr_str = self_str;
                is_self_pointer = true;
            }
            else
            {
                char *mangled_type = sn_mangle_name(gen->arena, struct_name);
                self_ptr_str = code_gen_self_ref(gen, member->object, mangled_type, self_str);
                is_self_pointer = false;
            }
        }

        char *intercept_result = code_gen_intercepted_method_call(gen, struct_name, method,
                                                struct_type, call->arg_count,
                                                call->arguments, self_ptr_str,
                                                is_self_pointer, method->return_type);
        /* Pin result if caller expects raw pointer */
        if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
            method->return_type != NULL && is_handle_type(method->return_type))
        {
            if (method->return_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena,
                    "((char *)(%s)->ptr)",
                    intercept_result);
            }
            else if (method->return_type->kind == TYPE_ARRAY)
            {
                Type *elem_type = resolve_struct_type(gen, method->return_type->as.array.element_type);
                const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
                return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                     elem_c, intercept_result);
            }
        }
        return intercept_result;
    }

    /* Direct call (no interception) */
    char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

    /* For instance methods, pass self */
    if (!method->is_static)
    {
        char *self_str = code_gen_expression(gen, member->object);
        if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
        }
        else if (member->object->expr_type != NULL &&
                 member->object->expr_type->kind == TYPE_POINTER)
        {
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
        }
        else
        {
            char *mangled_type = sn_mangle_name(gen->arena, struct_name);
            char *self_ref = code_gen_self_ref(gen, member->object, mangled_type, self_str);
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_ref);
        }
    }

    /* Generate other arguments in handle mode (struct methods are Sindarin functions) */
    bool saved_method_handle = gen->expr_as_handle;
    gen->expr_as_handle = (gen->current_arena_var != NULL);
    for (int i = 0; i < call->arg_count; i++)
    {
        char *arg_str = code_gen_expression(gen, call->arguments[i]);
        /* Check if argument needs closure wrapping for function-type parameter */
        if (i < method->param_count)
        {
            char *wrapped = code_gen_wrap_fn_arg_as_closure(gen,
                method->params[i].type, call->arguments[i], arg_str);
            if (wrapped != NULL)
            {
                arg_str = wrapped;
            }
        }
        args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
    }
    gen->expr_as_handle = saved_method_handle;

    char *method_call = arena_sprintf(gen->arena, "%s_%s(%s)",
                         mangled_struct, method->name, args_list);
    /* If method returns handle type and caller expects raw pointer, pin result */
    if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
        method->return_type != NULL && is_handle_type(method->return_type))
    {
        if (method->return_type->kind == TYPE_STRING)
        {
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)",
                method_call);
        }
        else if (method->return_type->kind == TYPE_ARRAY)
        {
            Type *elem_type = resolve_struct_type(gen, method->return_type->as.array.element_type);
            const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                 elem_c, method_call);
        }
    }
    return method_call;
}

/**
 * Generate code for a pointer-to-struct method call (e.g., self.method() inside a method body).
 * Returns the generated C code, or NULL if not handled.
 */
char *code_gen_pointer_struct_method_call(CodeGen *gen, Expr *expr, MemberExpr *member,
                                           Type *object_type, CallExpr *call)
{
    (void)expr; /* used for interface consistency */
    if (object_type->as.pointer.base_type == NULL ||
        object_type->as.pointer.base_type->kind != TYPE_STRUCT ||
        member->resolved_method == NULL)
    {
        return NULL;
    }

    StructMethod *method = member->resolved_method;
    Type *struct_type = member->resolved_struct_type;
    const char *struct_name = struct_type->as.struct_type.name;

    /* Check if this method should be intercepted */
    if (should_intercept_method(method, struct_type, method->return_type))
    {
        char *self_ptr_str = NULL;
        if (!method->is_static)
        {
            self_ptr_str = code_gen_expression(gen, member->object);
        }
        char *intercept_result = code_gen_intercepted_method_call(gen, struct_name, method,
                                                struct_type, call->arg_count,
                                                call->arguments, self_ptr_str,
                                                true, method->return_type);
        /* Pin result if caller expects raw pointer */
        if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
            method->return_type != NULL && is_handle_type(method->return_type))
        {
            if (method->return_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena,
                    "((char *)(%s)->ptr)",
                    intercept_result);
            }
            else if (method->return_type->kind == TYPE_ARRAY)
            {
                Type *elem_type = resolve_struct_type(gen, method->return_type->as.array.element_type);
                const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
                return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                     elem_c, intercept_result);
            }
        }
        return intercept_result;
    }

    /* Direct call (no interception) */
    char *mangled_struct = sn_mangle_name(gen->arena, struct_name);
    char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

    /* For instance methods, pass self (already a pointer) */
    if (!method->is_static)
    {
        char *self_str = code_gen_expression(gen, member->object);
        args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
    }

    /* Generate other arguments in handle mode (struct methods are Sindarin functions) */
    bool saved_method_handle = gen->expr_as_handle;
    gen->expr_as_handle = (gen->current_arena_var != NULL);
    for (int i = 0; i < call->arg_count; i++)
    {
        char *arg_str = code_gen_expression(gen, call->arguments[i]);
        /* Check if argument needs closure wrapping for function-type parameter */
        if (i < method->param_count)
        {
            char *wrapped = code_gen_wrap_fn_arg_as_closure(gen,
                method->params[i].type, call->arguments[i], arg_str);
            if (wrapped != NULL)
            {
                arg_str = wrapped;
            }
        }
        args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
    }
    gen->expr_as_handle = saved_method_handle;

    char *method_call = arena_sprintf(gen->arena, "%s_%s(%s)",
                         mangled_struct, method->name, args_list);
    /* If method returns handle type and caller expects raw pointer, pin result */
    if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
        method->return_type != NULL && is_handle_type(method->return_type))
    {
        if (method->return_type->kind == TYPE_STRING)
        {
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)",
                method_call);
        }
        else if (method->return_type->kind == TYPE_ARRAY)
        {
            Type *elem_type = resolve_struct_type(gen, method->return_type->as.array.element_type);
            const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                 elem_c, method_call);
        }
    }
    return method_call;
}
