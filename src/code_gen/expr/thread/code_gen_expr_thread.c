/**
 * code_gen_expr_thread.c - Code generation for thread spawn expressions
 *
 * Contains implementation for generating C code from thread spawn (&fn())
 * expressions. Thread sync expressions are in code_gen_expr_thread_sync.c.
 */

#include "code_gen/expr/thread/code_gen_expr_thread.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* ============================================================================
 * Thread Spawn Expression Code Generation
 * ============================================================================
 * Generates code for &fn() thread spawn expressions.
 * Sets is_shared and is_private flags based on function modifier.
 * ============================================================================ */

char *code_gen_thread_spawn_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_thread_spawn_expression");

    ThreadSpawnExpr *spawn = &expr->as.thread_spawn;

    /* Get caller arena for passing to thread args */
    const char *caller_arena = gen->current_arena_var ? gen->current_arena_var : "NULL";

    /* The call expression being spawned */
    Expr *call_expr = spawn->call;

    /* Track if this is a method call on an instance (self.method())
     * Method calls can be either:
     * 1. EXPR_METHOD_CALL (explicit method call syntax)
     * 2. EXPR_CALL with EXPR_MEMBER_ACCESS callee (self.method() pattern)
     */
    bool is_method_call = false;
    bool is_member_method_call = false;  /* EXPR_CALL with member access callee */
    MethodCallExpr *method_call = NULL;
    CallExpr *call = NULL;
    Type *self_struct_type = NULL;
    const char *self_struct_name = NULL;
    char *mangled_self_type = NULL;
    Expr *self_object = NULL;  /* The 'self' or object expression for method calls */
    const char *method_name = NULL;

    if (call_expr->type == EXPR_METHOD_CALL)
    {
        is_method_call = true;
        method_call = &call_expr->as.method_call;
        if (method_call->is_static || method_call->object == NULL)
        {
            fprintf(stderr, "Error: Thread spawn with static method calls not yet supported\n");
            exit(1);
        }
        self_struct_type = method_call->struct_type;
        self_struct_name = self_struct_type->as.struct_type.name;
        mangled_self_type = sn_mangle_name(gen->arena, self_struct_name);
        self_object = method_call->object;
        method_name = method_call->method->name;
    }
    else if (call_expr->type == EXPR_CALL)
    {
        call = &call_expr->as.call;

        /* Check if this is a method call via EXPR_MEMBER pattern (self.method())
         * EXPR_MEMBER is used for method calls resolved during type checking */
        if (call->callee->type == EXPR_MEMBER)
        {
            MemberExpr *member = &call->callee->as.member;
            /* Check if this has a resolved method (indicating it's a method call) */
            if (member->resolved_method != NULL && member->resolved_struct_type != NULL)
            {
                is_member_method_call = true;
                is_method_call = true;
                self_struct_type = member->resolved_struct_type;
                self_struct_name = self_struct_type->as.struct_type.name;
                mangled_self_type = sn_mangle_name(gen->arena, self_struct_name);
                self_object = member->object;
                method_name = member->resolved_method->name;
            }
        }
        /* Also check EXPR_MEMBER_ACCESS for field access patterns */
        else if (call->callee->type == EXPR_MEMBER_ACCESS)
        {
            MemberAccessExpr *member = &call->callee->as.member_access;
            /* Get the type of the object being accessed */
            if (member->object->expr_type != NULL)
            {
                Type *obj_type = member->object->expr_type;
                /* Handle pointer to struct (self inside method is a pointer) */
                if (obj_type->kind == TYPE_POINTER && obj_type->as.pointer.base_type != NULL)
                {
                    obj_type = obj_type->as.pointer.base_type;
                }
                if (obj_type->kind == TYPE_STRUCT)
                {
                    is_member_method_call = true;
                    is_method_call = true;
                    self_struct_type = obj_type;
                    self_struct_name = self_struct_type->as.struct_type.name;
                    mangled_self_type = sn_mangle_name(gen->arena, self_struct_name);
                    self_object = member->object;
                    /* Get method name from the field_name token */
                    method_name = arena_strndup(gen->arena, member->field_name.start, member->field_name.length);
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "Error: Thread spawn expression must be a function call or method call\n");
        exit(1);
    }

    /* Get parameter memory qualifiers from callee's function type */
    MemoryQualifier *param_quals = NULL;
    int param_count = 0;
    int arg_count = 0;
    Expr **arguments = NULL;

    if (is_method_call && !is_member_method_call)
    {
        /* Direct EXPR_METHOD_CALL */
        arg_count = method_call->arg_count;
        arguments = method_call->args;
        /* Methods don't have param_quals in StructMethod, so leave them as NULL.
         * This means 'as ref' parameters won't be handled specially for method spawns,
         * but that's an uncommon case. */
        param_count = method_call->method != NULL ? method_call->method->param_count : 0;
    }
    else
    {
        /* Regular EXPR_CALL (including member method calls via EXPR_MEMBER_ACCESS) */
        arg_count = call->arg_count;
        arguments = call->arguments;
        if (call->callee->expr_type && call->callee->expr_type->kind == TYPE_FUNCTION)
        {
            param_quals = call->callee->expr_type->as.function.param_mem_quals;
            param_count = call->callee->expr_type->as.function.param_count;
        }
    }

    /* Generate unique wrapper function ID */
    int wrapper_id = gen->thread_wrapper_count++;
    char *wrapper_name = arena_sprintf(gen->arena, "__thread_wrapper_%d__", wrapper_id);

    /* Get return type of the function being called */
    Type *return_type = call_expr->expr_type;

    /* Check if function returns a heap-allocated type (string, array, closure).
     * Such functions are implicitly shared in regular calls because the returned
     * value must survive the function's arena. For thread spawns, we need to
     * match this behavior so the wrapper correctly passes the arena parameter.
     * However, for thread spawning purposes, we want to treat it as the declared
     * modifier for arena mode (default mode spawns can return strings with promotion). */
    bool returns_heap_type = return_type != NULL && (
        return_type->kind == TYPE_FUNCTION ||
        return_type->kind == TYPE_STRING ||
        return_type->kind == TYPE_ARRAY);

    const char *is_shared_str = "false";
    const char *is_private_str = "false";

    /* Track if function is implicitly shared for wrapper generation */
    bool is_implicitly_shared = returns_heap_type;

    DEBUG_VERBOSE("Thread spawn: is_shared=%s, is_private=%s, implicit_shared=%d",
                  is_shared_str, is_private_str, is_implicitly_shared);
    const char *ret_c_type = get_c_type(gen->arena, return_type);
    bool is_void_return = (return_type == NULL || return_type->kind == TYPE_VOID);

    /* Generate the argument structure type name */
    char *args_struct_name = arena_sprintf(gen->arena, "__ThreadArgs_%d__", wrapper_id);

    /* Build argument struct definition - V2: only function-specific arguments
     * The struct is allocated in the thread arena after rt_thread_v3_create. */
    char *struct_def = arena_sprintf(gen->arena,
        "typedef struct {\n"
        "    /* Function-specific arguments */\n");

    /* For method calls, add a field to capture 'self' */
    if (is_method_call)
    {
        struct_def = arena_sprintf(gen->arena, "%s    %s *__sn__self;\n",
                                   struct_def, mangled_self_type);
    }

    /* Add fields for each argument.
     * For 'as ref' primitive parameters, store pointer type so thread
     * can modify the caller's variable. */
    for (int i = 0; i < arg_count; i++)
    {
        const char *arg_c_type = get_c_type(gen->arena, arguments[i]->expr_type);

        /* Check if this is an 'as ref' primitive parameter */
        bool is_ref_primitive = false;
        if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
            arguments[i]->expr_type != NULL)
        {
            TypeKind kind = arguments[i]->expr_type->kind;
            is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                               kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                               kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                               kind == TYPE_BYTE);
        }

        if (is_ref_primitive)
        {
            struct_def = arena_sprintf(gen->arena, "%s    %s *arg%d;\n",
                                       struct_def, arg_c_type, i);
        }
        else
        {
            struct_def = arena_sprintf(gen->arena, "%s    %s arg%d;\n",
                                       struct_def, arg_c_type, i);
        }
    }

    struct_def = arena_sprintf(gen->arena, "%s} %s;\n\n",
                               struct_def, args_struct_name);

    /* Generate wrapper function definition using V3 API.
     * V3 uses handle-first design: wrapper receives RtHandleV2* (not RtThread*),
     * all field access through transactions, and V3 TLS for panic capture. */
    char *wrapper_def = arena_sprintf(gen->arena,
        "static void *%s(void *arg) {\n"
        "    RtHandleV2 *__th__ = (RtHandleV2 *)arg;\n"
        "    rt_tls_thread_set_v3(__th__);\n"
        "    RtArenaV2 *__arena__ = rt_thread_v3_get_arena(__th__);\n"
        "    rt_safepoint_adopt_registration();\n"
        "\n"
        "    /* Unpack args from thread handle */\n"
        "    RtHandleV2 *__args_h__ = rt_thread_v3_get_args(__th__);\n"
        "    rt_handle_begin_transaction(__args_h__);\n"
        "    %s __args_copy__ = *(%s *)__args_h__->ptr;\n"
        "    rt_handle_end_transaction(__args_h__);\n"
        "    %s *args = &__args_copy__;\n"
        "\n",
        wrapper_name, args_struct_name, args_struct_name, args_struct_name);

    /* For method calls, extract self from the args struct */
    if (is_method_call)
    {
        wrapper_def = arena_sprintf(gen->arena,
            "%s    /* Extract self from thread arguments */\n"
            "    %s *__sn__self = args->__sn__self;\n"
            "\n",
            wrapper_def, mangled_self_type);
    }

    /* Generate the function call with arguments extracted from struct */
    char *callee_str = NULL;
    bool target_needs_arena_arg = false;

    if (is_method_call)
    {
        /* For method calls, generate: StructName_methodName */
        callee_str = arena_sprintf(gen->arena, "%s_%s",
                                   mangled_self_type, method_name);
        /* Method is a Sindarin function, so it needs arena arg.
         * Check if method is native - use resolved method if available */
        bool method_is_native = false;
        if (!is_member_method_call && method_call != NULL && method_call->method != NULL)
        {
            method_is_native = method_call->method->is_native;
        }
        else if (is_member_method_call && call != NULL && call->callee->type == EXPR_MEMBER)
        {
            /* Use the resolved method from EXPR_MEMBER */
            MemberExpr *member = &call->callee->as.member;
            if (member->resolved_method != NULL)
            {
                method_is_native = member->resolved_method->is_native;
            }
        }
        else if (is_member_method_call && self_struct_type != NULL)
        {
            /* Fallback: Look up the method in the struct to check if it's native */
            StructMethod *methods = self_struct_type->as.struct_type.methods;
            int method_count = self_struct_type->as.struct_type.method_count;
            for (int m = 0; m < method_count; m++)
            {
                if (strcmp(methods[m].name, method_name) == 0)
                {
                    method_is_native = methods[m].is_native;
                    break;
                }
            }
        }
        target_needs_arena_arg = !method_is_native;
    }
    else
    {
        callee_str = code_gen_expression(gen, call->callee);

        /* Check if this is a user-defined function (has body) that should receive arena.
         * With the new arena model, ALL Sindarin functions (with bodies) receive the
         * arena as first parameter, regardless of their modifier. */
        if (call->callee->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
            if (sym != NULL && sym->is_function && !sym->is_native)
            {
                target_needs_arena_arg = true;  /* All Sindarin functions get arena */
            }
            /* Also check has_body flag on function type */
            if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_FUNCTION &&
                sym->type->as.function.has_body)
            {
                target_needs_arena_arg = true;
            }
        }
    }

    /* Build argument list - prepend __arena__ for functions that need it */
    char *call_args;
    if (target_needs_arena_arg)
    {
        call_args = arena_strdup(gen->arena, "__arena__");
    }
    else
    {
        call_args = arena_strdup(gen->arena, "");
    }

    /* For method calls, add self as the second argument (after arena) */
    if (is_method_call)
    {
        if (call_args[0] != '\0')
        {
            call_args = arena_sprintf(gen->arena, "%s, __sn__self", call_args);
        }
        else
        {
            call_args = arena_strdup(gen->arena, "__sn__self");
        }
    }

    for (int i = 0; i < arg_count; i++)
    {
        bool need_comma = (i > 0) || target_needs_arena_arg;
        if (need_comma)
        {
            call_args = arena_sprintf(gen->arena, "%s, ", call_args);
        }

        /* For handle-type args in non-shared mode, clone from caller's arena
         * to thread's arena since handles are per-arena.
         * V2 handles carry their own arena reference, so no source arena needed. */
        Type *arg_type = arguments[i]->expr_type;
        bool is_handle_arg = gen->current_arena_var != NULL && arg_type != NULL &&
                             (arg_type->kind == TYPE_ARRAY || arg_type->kind == TYPE_STRING);
        if (is_handle_arg)
        {
            call_args = arena_sprintf(gen->arena, "%srt_arena_v2_clone(__arena__, args->arg%d)",
                                      call_args, i);
        }
        else
        {
            call_args = arena_sprintf(gen->arena, "%sargs->arg%d", call_args, i);
        }
    }

    /* Generate the function call in the wrapper */
    if (is_void_return)
    {
        /* Void function - just call it. V3: sync handles arena cleanup. */
        wrapper_def = arena_sprintf(gen->arena,
            "%s"
            "    /* Call the function */\n"
            "    %s(%s);\n"
            "\n"
            "%s"
            "    return NULL;\n"
            "}\n\n",
            wrapper_def, callee_str, call_args,
            gen->spawn_is_fire_and_forget
                ? "    rt_thread_v3_dispose(__th__);\n"
                  "    rt_safepoint_poll();\n"
                  "    rt_safepoint_thread_deregister();\n"
                  "    rt_tls_thread_set_v3(NULL);\n"
                : "    rt_thread_v3_signal_done(__th__);\n"
                  "    rt_safepoint_thread_deregister();\n"
                  "    rt_tls_thread_set_v3(NULL);\n");
    }
    else
    {
        /* Non-void function - store result. V2 API.
         * For handle types (string/array), the result is already RtHandleV2* - store directly.
         * For primitives/structs, wrap in a handle. */
        bool is_handle_result = return_type != NULL &&
            (return_type->kind == TYPE_STRING || return_type->kind == TYPE_ARRAY);
        bool is_struct = return_type != NULL && return_type->kind == TYPE_STRUCT;

        if (is_handle_result)
        {
            /* Handle type result - store directly without wrapping */
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function and store result */\n"
                "    %s __result__ = %s(%s);\n"
                "\n"
                "    /* V3: Handle type - store directly, sync site handles promotion */\n"
                "    rt_thread_v3_set_result(__th__, __result__);\n"
                "\n"
                "    rt_thread_v3_signal_done(__th__);\n"
                "    rt_safepoint_thread_deregister();\n"
                "    rt_tls_thread_set_v3(NULL);\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, ret_c_type, callee_str, call_args);
        }
        else if (is_struct)
        {
            /* Struct result - wrap in a handle */
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function and store result */\n"
                "    %s __result__ = %s(%s);\n"
                "\n"
                "    /* V3: Store result in handle - sync will promote to caller arena */\n"
                "    RtHandleV2 *__result_handle__ = rt_arena_v2_alloc(__arena__, sizeof(%s));\n"
                "    rt_handle_begin_transaction(__result_handle__);\n"
                "    *(%s *)__result_handle__->ptr = __result__;\n"
                "    rt_handle_end_transaction(__result_handle__);\n"
                "    rt_thread_v3_set_result(__th__, __result_handle__);\n"
                "\n"
                "    rt_thread_v3_signal_done(__th__);\n"
                "    rt_safepoint_thread_deregister();\n"
                "    rt_tls_thread_set_v3(NULL);\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, ret_c_type, callee_str, call_args, ret_c_type, ret_c_type);
        }
        else
        {
            /* Primitive result - wrap in a handle */
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function and store result */\n"
                "    %s __result__ = %s(%s);\n"
                "\n"
                "    /* V3: Store result in handle - sync will promote to caller arena */\n"
                "    RtHandleV2 *__result_handle__ = rt_arena_v2_alloc(__arena__, sizeof(%s));\n"
                "    rt_handle_begin_transaction(__result_handle__);\n"
                "    *(%s *)__result_handle__->ptr = __result__;\n"
                "    rt_handle_end_transaction(__result_handle__);\n"
                "    rt_thread_v3_set_result(__th__, __result_handle__);\n"
                "\n"
                "    rt_thread_v3_signal_done(__th__);\n"
                "    rt_safepoint_thread_deregister();\n"
                "    rt_tls_thread_set_v3(NULL);\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, ret_c_type, callee_str, call_args, ret_c_type, ret_c_type);
        }
    }

    /* Add struct and wrapper to forward declarations */
    gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s%s",
                                              gen->lambda_forward_decls,
                                              struct_def,
                                              wrapper_def);

    /* Generate unique temp variable name for spawning */
    int temp_id = gen->temp_count++;
    char *args_var = arena_sprintf(gen->arena, "__spawn_args_%d__", temp_id);
    char *handle_var = arena_sprintf(gen->arena, "__spawn_handle_%d__", temp_id);

    /* Generate arguments code.
     * For 'as ref' primitive parameters, pass address instead of value.
     * For function type arguments that are named function references, wrap in closure. */
    char *arg_assignments = arena_strdup(gen->arena, "");

    /* For method calls, capture self in the thread arguments */
    if (is_method_call && self_object != NULL)
    {
        /* Generate code to get self pointer from the object expression */
        char *self_code = code_gen_expression(gen, self_object);
        /* self_object is 'self' inside a method, which is already a pointer */
        if (self_object->expr_type != NULL &&
            self_object->expr_type->kind == TYPE_POINTER)
        {
            /* Already a pointer, use directly */
            arg_assignments = arena_sprintf(gen->arena, "%s->__sn__self = %s; ",
                                            args_var, self_code);
        }
        else
        {
            /* Take address of struct */
            arg_assignments = arena_sprintf(gen->arena, "%s->__sn__self = &%s; ",
                                            args_var, self_code);
        }
    }

    for (int i = 0; i < arg_count; i++)
    {
        Expr *arg_expr = arguments[i];

        char *arg_code = code_gen_expression(gen, arg_expr);

        /* Check if this is an 'as ref' primitive parameter */
        bool is_ref_primitive = false;
        if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
            arg_expr->expr_type != NULL)
        {
            TypeKind kind = arg_expr->expr_type->kind;
            is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                               kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                               kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                               kind == TYPE_BYTE);
        }

        /* Check if this is a function type argument that needs closure wrapping.
         * Named functions (not lambdas/closures) need to be wrapped in a closure
         * when passed as function type parameters. We also need to generate a thunk
         * because named functions don't use the closure calling convention. */
        bool needs_closure_wrap = false;
        char *fn_thunk_name = NULL;
        if (arg_expr->expr_type != NULL && arg_expr->expr_type->kind == TYPE_FUNCTION &&
            arg_expr->type == EXPR_VARIABLE)
        {
            /* Check if this is a named function (not a closure variable) */
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, arg_expr->as.variable.name);
            if (sym != NULL && sym->is_function)
            {
                needs_closure_wrap = true;

                /* Generate a thunk wrapper that adapts the function to closure calling convention.
                 * The thunk uses the closure pointer to get the arena for user-defined functions. */
                Type *fn_type = arg_expr->expr_type;
                int fn_thunk_id = gen->temp_count++;
                fn_thunk_name = arena_sprintf(gen->arena, "__fn_thunk_%d__", fn_thunk_id);

                /* Check if the target function needs an arena parameter.
                 * User-defined (non-native) functions need the arena as first argument. */
                bool target_needs_arena = sym->is_function && !sym->is_native;

                /* Build thunk parameter list with closure as first param, caller arena second */
                char *thunk_params = arena_strdup(gen->arena, "void *__cl__, RtArenaV2 *__caller_arena__");
                char *thunk_call_args;
                if (target_needs_arena)
                {
                    /* Prepend arena as first argument — use caller arena, fall back to closure's arena. */
                    thunk_call_args = arena_strdup(gen->arena, "__caller_arena__ ? __caller_arena__ : ((__Closure__ *)((RtHandleV2 *)__cl__)->ptr)->arena");
                }
                else
                {
                    thunk_call_args = arena_strdup(gen->arena, "");
                }
                for (int p = 0; p < fn_type->as.function.param_count; p++)
                {
                    const char *param_type = get_c_type(gen->arena, fn_type->as.function.param_types[p]);
                    thunk_params = arena_sprintf(gen->arena, "%s, %s __p%d__", thunk_params, param_type, p);
                    if (p > 0 || target_needs_arena) thunk_call_args = arena_sprintf(gen->arena, "%s, ", thunk_call_args);
                    thunk_call_args = arena_sprintf(gen->arena, "%s__p%d__", thunk_call_args, p);
                }

                const char *fn_ret_type = get_c_type(gen->arena, fn_type->as.function.return_type);
                char *fn_thunk_def;
                if (fn_type->as.function.return_type->kind == TYPE_VOID)
                {
                    fn_thunk_def = arena_sprintf(gen->arena,
                        "static %s %s(%s) { %s(%s); }\n",
                        fn_ret_type, fn_thunk_name, thunk_params, arg_code, thunk_call_args);
                }
                else
                {
                    fn_thunk_def = arena_sprintf(gen->arena,
                        "static %s %s(%s) { return %s(%s); }\n",
                        fn_ret_type, fn_thunk_name, thunk_params, arg_code, thunk_call_args);
                }

                /* Add forward declaration so the thunk can be used before its definition */
                char *fn_thunk_fwd = arena_sprintf(gen->arena, "static %s %s(%s);\n",
                                                fn_ret_type, fn_thunk_name, thunk_params);
                gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                          gen->lambda_forward_decls, fn_thunk_fwd);

                /* Add thunk definition */
                gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                        gen->lambda_definitions, fn_thunk_def);
            }
        }

        if (is_ref_primitive)
        {
            /* For 'as ref' params, store address of the argument */
            arg_assignments = arena_sprintf(gen->arena, "%s%s->arg%d = &%s; ",
                                            arg_assignments, args_var, i, arg_code);
        }
        else if (needs_closure_wrap)
        {
            /* Wrap named function in a closure using the thunk.
             * The thunk adapts the function to the closure calling convention. */
            arg_assignments = arena_sprintf(gen->arena,
                "%s%s->arg%d = ({ RtHandleV2 *__ah = rt_arena_v2_alloc(%s, sizeof(__Closure__)); "
                "rt_handle_begin_transaction(__ah); __Closure__ *__fn_cl__ = (__Closure__ *)__ah->ptr; "
                "__fn_cl__->fn = (void *)%s; __fn_cl__->arena = %s; rt_handle_end_transaction(__ah); __ah; }); ",
                arg_assignments, args_var, i, caller_arena, fn_thunk_name, caller_arena);
        }
        else
        {
            arg_assignments = arena_sprintf(gen->arena, "%s%s->arg%d = %s; ",
                                            arg_assignments, args_var, i, arg_code);
        }
    }

    /* All threads use default arena mode */
    const char *thread_mode = "RT_THREAD_MODE_DEFAULT";

    /* Generate the thread spawn expression using V3 API */
    char *result = arena_sprintf(gen->arena,
        "({\n"
        "    /* V3: Create thread with its own arena */\n"
        "    RtHandleV2 *%s = rt_thread_v3_create(%s, %s);\n"
        "    RtArenaV2 *__thread_arena__ = rt_thread_v3_get_arena(%s);\n"
        "\n"
        "    /* Allocate args in thread arena (min 1 byte for empty structs) */\n"
        "    RtHandleV2 *__args_h__ = rt_arena_v2_alloc(__thread_arena__, sizeof(%s) > 0 ? sizeof(%s) : 1);\n"
        "    rt_handle_begin_transaction(__args_h__);\n"
        "    %s *%s = (%s *)__args_h__->ptr;\n"
        "    %s"
        "    rt_handle_end_transaction(__args_h__);\n"
        "    rt_thread_v3_set_args(%s, __args_h__);\n"
        "\n"
        "    /* Start the thread */\n"
        "    rt_thread_v3_start(%s, %s);\n"
        "    %s;\n"
        "})",
        handle_var, caller_arena, thread_mode,
        handle_var,
        args_struct_name, args_struct_name,
        args_struct_name, args_var, args_struct_name,
        arg_assignments,
        handle_var,
        handle_var, wrapper_name,
        handle_var
    );

    return result;
}
