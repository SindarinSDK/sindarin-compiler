#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
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

            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                if (args_list[0] != '\0')
                {
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
                }
                else
                {
                    args_list = arg_str;
                }
            }

            return arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);
        }
        else
        {
            /* Non-native static method: StructName_methodName(arena, args) */
            char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
            }

            return arena_sprintf(gen->arena, "%s_%s(%s)",
                                 struct_name, method->name, args_list);
        }
    }

    /* Fallback for unimplemented static methods */
    return arena_sprintf(gen->arena,
        "(fprintf(stderr, \"Static method call not yet implemented: %.*s.%.*s\\n\"), exit(1), (void *)0)",
        type_name.length, type_name.start,
        method_name.length, method_name.start);
}
