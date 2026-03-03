#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Helper to compare Token with C string */
static bool codegen_token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

char *code_gen_static_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_static_call_expression");
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Generate argument expressions */
    char *arg0 = call->arg_count > 0 ? code_gen_expression(gen, call->arguments[0]) : NULL;
    char *arg1 = call->arg_count > 1 ? code_gen_expression(gen, call->arguments[1]) : NULL;

    /* Interceptor static methods */
    if (codegen_token_equals(type_name, "Interceptor"))
    {
        /* Interceptor.register(handler) -> rt_interceptor_register((RtInterceptHandler)handler) */
        if (codegen_token_equals(method_name, "register"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_register((RtInterceptHandler)%s), (void)0)", arg0);
        }
        /* Interceptor.registerWhere(handler, pattern) -> rt_interceptor_register_where((RtInterceptHandler)handler, pattern) */
        else if (codegen_token_equals(method_name, "registerWhere"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_register_where((RtInterceptHandler)%s, %s), (void)0)", arg0, arg1);
        }
        /* Interceptor.clearAll() -> rt_interceptor_clear_all() */
        else if (codegen_token_equals(method_name, "clearAll"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_clear_all(), (void)0)");
        }
        /* Interceptor.isActive() -> rt_interceptor_is_active() */
        else if (codegen_token_equals(method_name, "isActive"))
        {
            return arena_sprintf(gen->arena, "rt_interceptor_is_active()");
        }
        /* Interceptor.count() -> rt_interceptor_count() */
        else if (codegen_token_equals(method_name, "count"))
        {
            return arena_sprintf(gen->arena, "rt_interceptor_count()");
        }
    }

    /* Check for user-defined struct static methods */
    if (call->resolved_method != NULL && call->resolved_struct_type != NULL)
    {
        StructMethod *method = call->resolved_method;
        Type *struct_type = call->resolved_struct_type;
        const char *struct_name = struct_type->as.struct_type.name;

        if (method->is_native)
        {
            /* Native static method - use c_alias if present, else use naming convention */
            const char *func_name;
            if (method->c_alias != NULL)
            {
                /* Use explicit c_alias from #pragma alias */
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

            /* Track native string handle args for transactional FFI boundary */
            char **native_str_handle_exprs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
            int native_str_handle_count = 0;

            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str;
                /* For native methods receiving str[] args */
                if (gen->current_arena_var != NULL &&
                    call->arguments[i]->expr_type != NULL &&
                    call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
                    call->arguments[i]->expr_type->as.array.element_type != NULL &&
                    call->arguments[i]->expr_type->as.array.element_type->kind == TYPE_STRING)
                {
                    char *handle_expr = code_gen_expression(gen, call->arguments[i]);
                    arg_str = arena_sprintf(gen->arena,
                        "({ RtHandleV2 *__pin_h = rt_pin_string_array_v2(%s); (char **)rt_array_data_v2(__pin_h); })",
                        handle_expr);
                }
                /* For native methods receiving non-string array args (byte[], int[], etc.) */
                else if (gen->current_arena_var != NULL &&
                         call->arguments[i]->expr_type != NULL &&
                         call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
                         call->arguments[i]->expr_type->as.array.element_type != NULL &&
                         call->arguments[i]->expr_type->as.array.element_type->kind != TYPE_STRING)
                {
                    char *handle_expr = code_gen_expression(gen, call->arguments[i]);
                    const char *elem_c = get_c_array_elem_type(gen->arena,
                        call->arguments[i]->expr_type->as.array.element_type);
                    arg_str = arena_sprintf(gen->arena,
                        "((%s *)rt_array_data_v2(%s))", elem_c, handle_expr);
                }
                /* For native methods receiving individual str args */
                else if (gen->current_arena_var != NULL &&
                         call->arguments[i]->expr_type != NULL &&
                         call->arguments[i]->expr_type->kind == TYPE_STRING)
                {
                    char *handle_expr = code_gen_expression(gen, call->arguments[i]);
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

            return call_result;
        }
        else
        {
            /* Non-native static method: check for interception */
            if (should_intercept_method(method, struct_type, method->return_type))
            {
                return code_gen_intercepted_method_call(gen, struct_name, method,
                                                        struct_type, call->arg_count,
                                                        call->arguments, NULL,
                                                        false, method->return_type);
            }

            /* Direct call (no interception) */
            char *mangled_struct = sn_mangle_name(gen->arena, struct_name);
            char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
            }

            char *result = arena_sprintf(gen->arena, "%s_%s(%s)",
                                         mangled_struct, method->name, args_list);

            return result;
        }
    }

    /* Fallback for unimplemented static methods */
    return arena_sprintf(gen->arena,
        "(fprintf(stderr, \"Static method call not yet implemented: %.*s.%.*s\\n\"), exit(1), (void *)0)",
        type_name.length, type_name.start,
        method_name.length, method_name.start);
}

char *code_gen_method_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_method_call_expression");
    MethodCallExpr *call = &expr->as.method_call;
    StructMethod *method = call->method;
    Type *struct_type = call->struct_type;

    if (method == NULL || struct_type == NULL)
    {
        fprintf(stderr, "Error: Unresolved method call\n");
        exit(1);
    }

    const char *struct_name = struct_type->as.struct_type.name;

    if (method->is_native)
    {
        /* Native method - use c_alias if present, else use naming convention */
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

        /* Build args list */
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

        /* For instance methods, add self */
        if (!call->is_static && call->object != NULL)
        {
            char *self_str = code_gen_expression(gen, call->object);
            /* Handle pointer-to-struct (e.g., self inside method body) */
            if (call->object->expr_type != NULL &&
                call->object->expr_type->kind == TYPE_POINTER)
            {
                /* Already a pointer */
                if (args_list[0] != '\0')
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
                else
                    args_list = self_str;
            }
            else if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
            {
                /* Opaque handle: self is already a pointer */
                if (args_list[0] != '\0')
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
                else
                    args_list = self_str;
            }
            else if (struct_type->as.struct_type.pass_self_by_ref)
            {
                /* Native struct with 'as ref' - pass pointer to self */
                char *mangled_type = sn_mangle_name(gen->arena, struct_name);
                char *self_ref = code_gen_self_ref(gen, call->object, mangled_type, self_str);
                if (args_list[0] != '\0')
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_ref);
                else
                    args_list = self_ref;
            }
            else
            {
                /* Native struct by value */
                if (args_list[0] != '\0')
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
                else
                    args_list = self_str;
            }
        }

        /* Add regular arguments */
        for (int i = 0; i < call->arg_count; i++)
        {
            char *arg_str = code_gen_expression(gen, call->args[i]);
            if (args_list[0] != '\0')
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
            else
                args_list = arg_str;
        }

        char *call_result = arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);
        return call_result;
    }
    else
    {
        /* Non-native Sindarin method */

        /* Check for interception */
        if (should_intercept_method(method, struct_type, method->return_type))
        {
            char *self_ptr_str = NULL;
            bool is_self_pointer = false;

            if (!call->is_static && call->object != NULL)
            {
                char *self_str = code_gen_expression(gen, call->object);
                if (call->object->expr_type != NULL &&
                    call->object->expr_type->kind == TYPE_POINTER)
                {
                    self_ptr_str = self_str;
                    is_self_pointer = true;
                }
                else
                {
                    char *mangled_type = sn_mangle_name(gen->arena, struct_name);
                    self_ptr_str = code_gen_self_ref(gen, call->object, mangled_type, self_str);
                    is_self_pointer = false;
                }
            }

            char *intercept_result = code_gen_intercepted_method_call(gen, struct_name, method,
                                                    struct_type, call->arg_count,
                                                    call->args, self_ptr_str,
                                                    is_self_pointer, method->return_type);

            return intercept_result;
        }

        /* Direct call (no interception) */
        char *mangled_struct = sn_mangle_name(gen->arena, struct_name);
        char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

        /* For instance methods, pass self */
        if (!call->is_static && call->object != NULL)
        {
            char *self_str = code_gen_expression(gen, call->object);

            if (call->object->expr_type != NULL &&
                call->object->expr_type->kind == TYPE_POINTER)
            {
                /* Object is already a pointer (e.g., self inside a method body) */
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
            }
            else if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
            {
                /* Opaque handle: self is already a pointer */
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
            }
            else
            {
                /* Regular struct: take address of self (handle rvalue chaining) */
                char *mangled_type = sn_mangle_name(gen->arena, struct_name);
                char *self_ref = code_gen_self_ref(gen, call->object, mangled_type, self_str);
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_ref);
            }
        }

        for (int i = 0; i < call->arg_count; i++)
        {
            char *arg_str = code_gen_expression(gen, call->args[i]);
            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
        }

        char *result = arena_sprintf(gen->arena, "%s_%s(%s)",
                                     mangled_struct, method->name, args_list);

        return result;
    }
}
