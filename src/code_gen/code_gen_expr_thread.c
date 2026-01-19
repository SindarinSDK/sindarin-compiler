/**
 * code_gen_expr_thread.c - Code generation for thread expressions
 *
 * Contains implementation for generating C code from thread spawn (&fn())
 * and thread sync (var!) expressions.
 */

#include "code_gen/code_gen_expr_thread.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
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

/* Helper function to generate RtResultType constant from Type */
const char *get_rt_result_type(Type *type)
{
    if (type == NULL || type->kind == TYPE_VOID)
    {
        return "RT_TYPE_VOID";
    }

    switch (type->kind)
    {
        case TYPE_INT:
            return "RT_TYPE_INT";
        case TYPE_LONG:
            return "RT_TYPE_LONG";
        case TYPE_DOUBLE:
            return "RT_TYPE_DOUBLE";
        case TYPE_BOOL:
            return "RT_TYPE_BOOL";
        case TYPE_BYTE:
            return "RT_TYPE_BYTE";
        case TYPE_CHAR:
            return "RT_TYPE_CHAR";
        case TYPE_STRING:
            return "RT_TYPE_STRING";
        case TYPE_ARRAY:
        {
            Type *elem = type->as.array.element_type;
            switch (elem->kind)
            {
                case TYPE_INT:
                case TYPE_LONG:
                    return "RT_TYPE_ARRAY_LONG";
                case TYPE_DOUBLE:
                    return "RT_TYPE_ARRAY_DOUBLE";
                case TYPE_BOOL:
                    return "RT_TYPE_ARRAY_BOOL";
                case TYPE_BYTE:
                    return "RT_TYPE_ARRAY_BYTE";
                case TYPE_CHAR:
                    return "RT_TYPE_ARRAY_CHAR";
                case TYPE_STRING:
                    return "RT_TYPE_ARRAY_STRING";
                default:
                    return "RT_TYPE_VOID";
            }
        }
        default:
            return "RT_TYPE_VOID";
    }
}

char *code_gen_thread_spawn_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_thread_spawn_expression");

    ThreadSpawnExpr *spawn = &expr->as.thread_spawn;
    FunctionModifier modifier = spawn->modifier;

    /* Get caller arena for passing to thread args */
    const char *caller_arena = gen->current_arena_var ? gen->current_arena_var : "NULL";

    /* The call expression being spawned */
    Expr *call_expr = spawn->call;

    /* Regular function call handling */
    if (call_expr->type != EXPR_CALL)
    {
        fprintf(stderr, "Error: Thread spawn expression must be a function call\n");
        exit(1);
    }

    CallExpr *call = &call_expr->as.call;

    /* Get parameter memory qualifiers from callee's function type */
    MemoryQualifier *param_quals = NULL;
    int param_count = 0;
    if (call->callee->expr_type && call->callee->expr_type->kind == TYPE_FUNCTION)
    {
        param_quals = call->callee->expr_type->as.function.param_mem_quals;
        param_count = call->callee->expr_type->as.function.param_count;
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

    /* For is_shared flag: use the declared modifier, NOT the implicit conversion.
     * This allows default mode threads to return strings/arrays with promotion.
     * The wrapper will still pass __arena__ as first arg if function is implicitly shared,
     * but the thread arena mode will be based on the declared modifier. */
    const char *is_shared_str = (modifier == FUNC_SHARED) ? "true" : "false";
    const char *is_private_str = (modifier == FUNC_PRIVATE) ? "true" : "false";

    /* Track if function is implicitly shared for wrapper generation */
    bool is_implicitly_shared = returns_heap_type && modifier != FUNC_SHARED && modifier != FUNC_PRIVATE;

    DEBUG_VERBOSE("Thread spawn: is_shared=%s, is_private=%s, implicit_shared=%d",
                  is_shared_str, is_private_str, is_implicitly_shared);
    const char *ret_c_type = get_c_type(gen->arena, return_type);
    const char *result_type_enum = get_rt_result_type(return_type);
    bool is_void_return = (return_type == NULL || return_type->kind == TYPE_VOID);

    /* Generate the argument structure type name */
    char *args_struct_name = arena_sprintf(gen->arena, "__ThreadArgs_%d__", wrapper_id);

    /* Build argument struct definition
     * IMPORTANT: The first fields must match RtThreadArgs layout exactly
     * so that rt_thread_spawn can cast to RtThreadArgs* and access fields.
     * Additional function arguments are appended after the common fields.
     */
    char *struct_def = arena_sprintf(gen->arena,
        "typedef struct {\n"
        "    /* These fields match RtThreadArgs layout */\n"
        "    void *func_ptr;\n"
        "    void *args_data;\n"
        "    size_t args_size;\n"
        "    RtThreadResult *result;\n"
        "    RtArena *caller_arena;\n"
        "    RtArena *thread_arena;\n"
        "    bool is_shared;\n"
        "    bool is_private;\n"
        "    /* Function-specific arguments follow */\n");

    /* Add fields for each argument.
     * For 'as ref' primitive parameters, store pointer type so thread
     * can modify the caller's variable. */
    for (int i = 0; i < call->arg_count; i++)
    {
        const char *arg_c_type = get_c_type(gen->arena, call->arguments[i]->expr_type);

        /* Check if this is an 'as ref' primitive parameter */
        bool is_ref_primitive = false;
        if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
            call->arguments[i]->expr_type != NULL)
        {
            TypeKind kind = call->arguments[i]->expr_type->kind;
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

    /* Generate wrapper function definition with panic handling.
     * We set up a panic context with setjmp so that panics in the thread
     * are captured and can be propagated to the caller on sync.
     *
     * Note: The arena is already created by rt_thread_spawn() and stored in
     * args->thread_arena. For shared mode, thread_arena == caller_arena.
     * We just use args->thread_arena directly. */
    char *wrapper_def = arena_sprintf(gen->arena,
        "static void *%s(void *args_ptr) {\n"
        "    %s *args = (%s *)args_ptr;\n"
        "\n"
        "    /* Use arena created by rt_thread_spawn(). For shared mode, this is\n"
        "     * the caller's arena. For default/private modes, it's a new arena. */\n"
        "    RtArena *__arena__ = args->thread_arena;\n"
        "\n"
        "    /* For shared mode, set ourselves as the frozen arena owner so we can allocate.\n"
        "     * This must be done before any allocation to avoid race with rt_thread_spawn. */\n"
        "    if (args->is_shared && args->caller_arena != NULL) {\n"
        "        args->caller_arena->frozen_owner = pthread_self();\n"
        "    }\n"
        "\n"
        "    /* Set up panic context to catch panics in this thread */\n"
        "    RtThreadPanicContext __panic_ctx__;\n"
        "    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);\n"
        "    if (setjmp(__panic_ctx__.jump_buffer) != 0) {\n"
        "        /* Panic occurred - cleanup and return */\n"
        "        rt_thread_panic_context_clear();\n"
        "        return NULL;\n"
        "    }\n"
        "\n",
        wrapper_name, args_struct_name, args_struct_name);

    /* Generate the function call with arguments extracted from struct */
    char *callee_str = code_gen_expression(gen, call->callee);

    /* Check if this is a user-defined function (has body) that should receive arena.
     * With the new arena model, ALL Sindarin functions (with bodies) receive the
     * arena as first parameter, regardless of their modifier. */
    bool is_user_function = false;
    bool target_needs_arena_arg = false;
    const char *func_name_for_intercept = NULL;
    if (call->callee->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
        if (sym != NULL && sym->is_function && !sym->is_native)
        {
            is_user_function = true;
            target_needs_arena_arg = true;  /* All Sindarin functions get arena */
            func_name_for_intercept = get_var_name(gen->arena, call->callee->as.variable.name);
        }
        /* Also check has_body flag on function type */
        if (sym != NULL && sym->type != NULL && sym->type->kind == TYPE_FUNCTION &&
            sym->type->as.function.has_body)
        {
            target_needs_arena_arg = true;
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

    for (int i = 0; i < call->arg_count; i++)
    {
        bool need_comma = (i > 0) || target_needs_arena_arg;
        if (need_comma)
        {
            call_args = arena_sprintf(gen->arena, "%s, ", call_args);
        }
        call_args = arena_sprintf(gen->arena, "%sargs->arg%d", call_args, i);
    }

    /* Generate thunk for interceptor support if this is a user function */
    char *thunk_name = NULL;
    if (is_user_function)
    {
        int thunk_id = gen->thunk_count++;
        thunk_name = arena_sprintf(gen->arena, "__thread_thunk_%d", thunk_id);

        /* Generate thunk forward declaration - add to lambda_forward_decls so it comes
         * before the thread wrapper that uses it */
        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic RtAny %s(void);\n",
                                                  gen->lambda_forward_decls, thunk_name);

        /* Generate thunk definition - reads from __rt_thunk_args */
        char *thunk_def = arena_sprintf(gen->arena, "static RtAny %s(void) {\n", thunk_name);

        /* For 'as ref' primitives, declare local variables to hold unboxed values */
        for (int i = 0; i < call->arg_count; i++)
        {
            Type *arg_type = call->arguments[i]->expr_type;
            bool is_ref_primitive = false;
            if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                arg_type != NULL)
            {
                TypeKind kind = arg_type->kind;
                is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                   kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                   kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                   kind == TYPE_BYTE);
            }
            if (is_ref_primitive)
            {
                const char *c_type = get_c_type(gen->arena, arg_type);
                const char *unbox_func = get_unboxing_function(arg_type);
                thunk_def = arena_sprintf(gen->arena,
                    "%s    %s __ref_%d = %s(__rt_thunk_args[%d]);\n",
                    thunk_def, c_type, i, unbox_func, i);
            }
        }

        /* Build unboxed argument list for the thunk */
        char *unboxed_args;
        if (target_needs_arena_arg)
        {
            unboxed_args = arena_strdup(gen->arena, "(RtArena *)__rt_thunk_arena");
        }
        else
        {
            unboxed_args = arena_strdup(gen->arena, "");
        }

        for (int i = 0; i < call->arg_count; i++)
        {
            Type *arg_type = call->arguments[i]->expr_type;
            const char *unbox_func = get_unboxing_function(arg_type);
            bool need_comma = (i > 0) || target_needs_arena_arg;
            if (need_comma)
            {
                unboxed_args = arena_sprintf(gen->arena, "%s, ", unboxed_args);
            }

            /* Check if this is an 'as ref' primitive parameter */
            bool is_ref_primitive = false;
            if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                arg_type != NULL)
            {
                TypeKind kind = arg_type->kind;
                is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                   kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                   kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                   kind == TYPE_BYTE);
            }

            if (is_ref_primitive)
            {
                /* Pass address of local variable for as ref primitives */
                unboxed_args = arena_sprintf(gen->arena, "%s&__ref_%d", unboxed_args, i);
            }
            else if (unbox_func == NULL)
            {
                /* For 'any' type or unknown, pass directly */
                unboxed_args = arena_sprintf(gen->arena, "%s__rt_thunk_args[%d]", unboxed_args, i);
            }
            else
            {
                unboxed_args = arena_sprintf(gen->arena, "%s%s(__rt_thunk_args[%d])", unboxed_args, unbox_func, i);
            }
        }

        /* Make the actual function call in the thunk */
        if (is_void_return)
        {
            thunk_def = arena_sprintf(gen->arena, "%s    %s(%s);\n",
                                      thunk_def, callee_str, unboxed_args);
        }
        else
        {
            const char *box_func = get_boxing_function(return_type);
            if (return_type && return_type->kind == TYPE_ARRAY)
            {
                const char *elem_tag = get_element_type_tag(return_type->as.array.element_type);
                thunk_def = arena_sprintf(gen->arena,
                    "%s    RtAny __result = %s(%s(%s), %s);\n",
                    thunk_def, box_func, callee_str, unboxed_args, elem_tag);
            }
            else
            {
                thunk_def = arena_sprintf(gen->arena,
                    "%s    RtAny __result = %s(%s(%s));\n",
                    thunk_def, box_func, callee_str, unboxed_args);
            }
        }

        /* Write back modified values for 'as ref' primitives */
        for (int i = 0; i < call->arg_count; i++)
        {
            Type *arg_type = call->arguments[i]->expr_type;
            bool is_ref_primitive = false;
            if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                arg_type != NULL)
            {
                TypeKind kind = arg_type->kind;
                is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                   kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                   kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                   kind == TYPE_BYTE);
            }
            if (is_ref_primitive)
            {
                const char *box_func = get_boxing_function(arg_type);
                thunk_def = arena_sprintf(gen->arena,
                    "%s    __rt_thunk_args[%d] = %s(__ref_%d);\n",
                    thunk_def, i, box_func, i);
            }
        }

        /* Return result */
        if (is_void_return)
        {
            thunk_def = arena_sprintf(gen->arena, "%s    return rt_box_nil();\n", thunk_def);
        }
        else
        {
            thunk_def = arena_sprintf(gen->arena, "%s    return __result;\n", thunk_def);
        }
        thunk_def = arena_sprintf(gen->arena, "%s}\n", thunk_def);
        gen->thunk_definitions = arena_sprintf(gen->arena, "%s%s\n", gen->thunk_definitions, thunk_def);
    }

    /* Generate the function call code with interceptor support */
    if (is_user_function)
    {
        /* Generate interceptor-aware call */
        if (is_void_return)
        {
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function with interceptor support */\n"
                "    if (__rt_interceptor_count > 0) {\n"
                "        RtAny __args[%d];\n",
                wrapper_def, call->arg_count > 0 ? call->arg_count : 1);

            /* Box arguments - for 'as ref' primitives, dereference the pointer */
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                const char *box_func = get_boxing_function(arg_type);

                /* Check if this is an 'as ref' primitive parameter */
                bool is_ref_primitive = false;
                if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                    arg_type != NULL)
                {
                    TypeKind kind = arg_type->kind;
                    is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                       kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                       kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                       kind == TYPE_BYTE);
                }

                if (arg_type && arg_type->kind == TYPE_ARRAY)
                {
                    const char *elem_tag = get_element_type_tag(arg_type->as.array.element_type);
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d, %s);\n",
                        wrapper_def, i, box_func, i, elem_tag);
                }
                else if (is_ref_primitive)
                {
                    /* Dereference pointer for as ref primitives */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(*args->arg%d);\n",
                        wrapper_def, i, box_func, i);
                }
                else
                {
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d);\n",
                        wrapper_def, i, box_func, i);
                }
            }

            /* Generate write-back code for as ref primitives after interception */
            char *writeback_code = arena_strdup(gen->arena, "");
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                bool is_ref_primitive = false;
                if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                    arg_type != NULL)
                {
                    TypeKind kind = arg_type->kind;
                    is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                       kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                       kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                       kind == TYPE_BYTE);
                }
                if (is_ref_primitive)
                {
                    const char *unbox_func = get_unboxing_function(arg_type);
                    writeback_code = arena_sprintf(gen->arena,
                        "%s        *args->arg%d = %s(__args[%d]);\n",
                        writeback_code, i, unbox_func, i);
                }
            }

            wrapper_def = arena_sprintf(gen->arena,
                "%s        __rt_thunk_args = __args;\n"
                "        __rt_thunk_arena = __arena__;\n"
                "        rt_call_intercepted(\"%s\", __args, %d, %s);\n"
                "%s"
                "    } else {\n"
                "        %s(%s);\n"
                "    }\n"
                "\n"
                "    /* Clear panic context on successful completion */\n"
                "    rt_thread_panic_context_clear();\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, func_name_for_intercept, call->arg_count, thunk_name,
                writeback_code, callee_str, call_args);
        }
        else
        {
            const char *unbox_func = get_unboxing_function(return_type);
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function with interceptor support */\n"
                "    %s __result__;\n"
                "    if (__rt_interceptor_count > 0) {\n"
                "        RtAny __args[%d];\n",
                wrapper_def, ret_c_type, call->arg_count > 0 ? call->arg_count : 1);

            /* Box arguments - for 'as ref' primitives, dereference the pointer */
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                const char *box_func = get_boxing_function(arg_type);

                /* Check if this is an 'as ref' primitive parameter */
                bool is_ref_primitive = false;
                if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                    arg_type != NULL)
                {
                    TypeKind kind = arg_type->kind;
                    is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                       kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                       kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                       kind == TYPE_BYTE);
                }

                if (arg_type && arg_type->kind == TYPE_ARRAY)
                {
                    const char *elem_tag = get_element_type_tag(arg_type->as.array.element_type);
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d, %s);\n",
                        wrapper_def, i, box_func, i, elem_tag);
                }
                else if (is_ref_primitive)
                {
                    /* Dereference pointer for as ref primitives */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(*args->arg%d);\n",
                        wrapper_def, i, box_func, i);
                }
                else
                {
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d);\n",
                        wrapper_def, i, box_func, i);
                }
            }

            /* Generate write-back code for as ref primitives after interception */
            char *writeback_code = arena_strdup(gen->arena, "");
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                bool is_ref_primitive = false;
                if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
                    arg_type != NULL)
                {
                    TypeKind kind = arg_type->kind;
                    is_ref_primitive = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                                       kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                                       kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                                       kind == TYPE_BYTE);
                }
                if (is_ref_primitive)
                {
                    const char *unbox = get_unboxing_function(arg_type);
                    writeback_code = arena_sprintf(gen->arena,
                        "%s        *args->arg%d = %s(__args[%d]);\n",
                        writeback_code, i, unbox, i);
                }
            }

            wrapper_def = arena_sprintf(gen->arena,
                "%s        __rt_thunk_args = __args;\n"
                "        __rt_thunk_arena = __arena__;\n"
                "        RtAny __intercepted = rt_call_intercepted(\"%s\", __args, %d, %s);\n"
                "%s"
                "        __result__ = %s(__intercepted);\n"
                "    } else {\n"
                "        __result__ = %s(%s);\n"
                "    }\n"
                "\n"
                "    /* Store result in thread result structure using runtime function */\n"
                "    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;\n"
                "    rt_thread_result_set_value(args->result, &__result__, sizeof(%s), __result_arena__);\n"
                "\n"
                "    /* Clear panic context on successful completion */\n"
                "    rt_thread_panic_context_clear();\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, func_name_for_intercept, call->arg_count, thunk_name,
                writeback_code, unbox_func, callee_str, call_args, ret_c_type);
        }
    }
    else if (is_void_return)
    {
        /* Non-interceptable void function - just call it */
        wrapper_def = arena_sprintf(gen->arena,
            "%s"
            "    /* Call the function */\n"
            "    %s(%s);\n"
            "\n"
            "    /* Clear panic context on successful completion */\n"
            "    rt_thread_panic_context_clear();\n"
            "    return NULL;\n"
            "}\n\n",
            wrapper_def, callee_str, call_args);
    }
    else
    {
        /* Non-interceptable non-void function - store result */
        wrapper_def = arena_sprintf(gen->arena,
            "%s"
            "    /* Call the function and store result */\n"
            "    %s __result__ = %s(%s);\n"
            "\n"
            "    /* Store result in thread result structure using runtime function */\n"
            "    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;\n"
            "    rt_thread_result_set_value(args->result, &__result__, sizeof(%s), __result_arena__);\n"
            "\n"
            "    /* Clear panic context on successful completion */\n"
            "    rt_thread_panic_context_clear();\n"
            "    return NULL;\n"
            "}\n\n",
            wrapper_def, ret_c_type, callee_str, call_args, ret_c_type);
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
    for (int i = 0; i < call->arg_count; i++)
    {
        Expr *arg_expr = call->arguments[i];
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
        char *thunk_name = NULL;
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
                int thunk_id = gen->temp_count++;
                thunk_name = arena_sprintf(gen->arena, "__fn_thunk_%d__", thunk_id);

                /* Check if the target function needs an arena parameter.
                 * User-defined (non-native) functions need the arena as first argument. */
                bool target_needs_arena = sym->is_function && !sym->is_native;

                /* Build thunk parameter list with closure as first param */
                char *thunk_params = arena_strdup(gen->arena, "void *__cl__");
                char *thunk_call_args;
                if (target_needs_arena)
                {
                    /* Prepend arena from closure as first argument */
                    thunk_call_args = arena_strdup(gen->arena, "((__Closure__ *)__cl__)->arena");
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

                const char *ret_type = get_c_type(gen->arena, fn_type->as.function.return_type);
                char *thunk_def;
                if (fn_type->as.function.return_type->kind == TYPE_VOID)
                {
                    thunk_def = arena_sprintf(gen->arena,
                        "static %s %s(%s) { %s(%s); }\n",
                        ret_type, thunk_name, thunk_params, arg_code, thunk_call_args);
                }
                else
                {
                    thunk_def = arena_sprintf(gen->arena,
                        "static %s %s(%s) { return %s(%s); }\n",
                        ret_type, thunk_name, thunk_params, arg_code, thunk_call_args);
                }

                /* Add forward declaration so the thunk can be used before its definition */
                char *thunk_fwd = arena_sprintf(gen->arena, "static %s %s(%s);\n",
                                                ret_type, thunk_name, thunk_params);
                gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                          gen->lambda_forward_decls, thunk_fwd);

                /* Add thunk definition */
                gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                        gen->lambda_definitions, thunk_def);
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
                "%s%s->arg%d = ({ __Closure__ *__fn_cl__ = rt_arena_alloc(%s, sizeof(__Closure__)); "
                "__fn_cl__->fn = (void *)%s; __fn_cl__->arena = %s; __fn_cl__; }); ",
                arg_assignments, args_var, i, caller_arena, thunk_name, caller_arena);
        }
        else
        {
            arg_assignments = arena_sprintf(gen->arena, "%s%s->arg%d = %s; ",
                                            arg_assignments, args_var, i, arg_code);
        }
    }

    /* Generate the thread spawn expression */
    char *result = arena_sprintf(gen->arena,
        "({\n"
        "    /* Allocate thread arguments structure */\n"
        "    %s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n"
        "    %s->caller_arena = %s;\n"
        "    %s->thread_arena = NULL;\n"
        "    %s->result = rt_thread_result_create(%s);\n"
        "    %s->is_shared = %s;\n"
        "    %s->is_private = %s;\n"
        "    %s"
        "\n"
        "    /* Spawn the thread */\n"
        "    RtThreadHandle *%s = rt_thread_spawn(%s, %s, %s);\n"
        "    %s->result_type = %s;\n"
        "    %s;\n"
        "})",
        args_struct_name, args_var, args_struct_name, caller_arena, args_struct_name,
        args_var, caller_arena,
        args_var,
        args_var, caller_arena,
        args_var, is_shared_str,
        args_var, is_private_str,
        arg_assignments,
        handle_var, caller_arena, wrapper_name, args_var,
        handle_var, result_type_enum,
        handle_var
    );

    return result;
}

/* ============================================================================
 * Thread Sync Expression Code Generation
 * ============================================================================
 * Generates code for var! sync expressions.
 * ============================================================================ */

char *code_gen_thread_sync_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_thread_sync_expression");

    ThreadSyncExpr *sync = &expr->as.thread_sync;

    if (sync->is_array)
    {
        /* Sync list: [r1, r2, r3]!
         * This generates code to:
         * 1. Build an array of RtThreadHandle* pointers
         * 2. Call rt_thread_sync_all to sync all handles
         * 3. Return void
         */
        DEBUG_VERBOSE("Thread sync: sync list");

        /* The handle expression should be a sync list expression */
        Expr *list_expr = sync->handle;
        if (list_expr->type != EXPR_SYNC_LIST)
        {
            fprintf(stderr, "Error: Multi-sync requires sync list expression\n");
            exit(1);
        }

        SyncListExpr *sync_list = &list_expr->as.sync_list;
        int count = sync_list->element_count;

        if (count == 0)
        {
            /* Empty sync list - no-op */
            return arena_strdup(gen->arena, "((void)0)");
        }

        /* Generate a unique temp ID for the handles array */
        int temp_id = gen->temp_count++;

        /* Build the array initializer with handle expressions */
        char *handles_init = arena_strdup(gen->arena, "");
        for (int i = 0; i < count; i++)
        {
            char *handle_code = code_gen_expression(gen, sync_list->elements[i]);
            if (i > 0)
            {
                handles_init = arena_sprintf(gen->arena, "%s, ", handles_init);
            }
            handles_init = arena_sprintf(gen->arena, "%s%s", handles_init, handle_code);
        }

        /* Generate the sync_all call with inline array
         * Result:
         * ({
         *     RtThreadHandle *__sync_handles_N__[] = {r1, r2, r3};
         *     rt_thread_sync_all(__sync_handles_N__, count);
         *     (void)0;
         * })
         */
        return arena_sprintf(gen->arena,
            "({\n"
            "    RtThreadHandle *__sync_handles_%d__[] = {%s};\n"
            "    rt_thread_sync_all(__sync_handles_%d__, %d);\n"
            "    (void)0;\n"
            "})",
            temp_id, handles_init,
            temp_id, count);
    }
    else
    {
        /* Single variable sync: r!
         * This generates code to:
         * 1. Call rt_thread_join to wait for thread and get result pointer
         * 2. Cast the result to the correct type
         * 3. Dereference for primitives, return pointer for reference types
         */
        DEBUG_VERBOSE("Thread sync: single variable sync");

        /* Generate the handle expression */
        char *handle_code = code_gen_expression(gen, sync->handle);

        /* Get the result type from the sync expression */
        Type *result_type = expr->expr_type;

        /* Check if the result is void - no value to retrieve */
        if (result_type == NULL || result_type->kind == TYPE_VOID)
        {
            /* Void sync - call rt_thread_sync for synchronization with panic propagation */
            return arena_sprintf(gen->arena,
                "({\n"
                "    rt_thread_sync(%s);\n"
                "    (void)0;\n"
                "})",
                handle_code);
        }

        /* Get the C type for casting */
        const char *c_type = get_c_type(gen->arena, result_type);

        /* Get the RtResultType for proper result promotion */
        const char *rt_type = get_rt_result_type(result_type);

        /* Determine if this is a primitive type (needs dereferencing) or reference type */
        bool is_primitive = (result_type->kind == TYPE_INT ||
                             result_type->kind == TYPE_LONG ||
                             result_type->kind == TYPE_DOUBLE ||
                             result_type->kind == TYPE_BOOL ||
                             result_type->kind == TYPE_BYTE ||
                             result_type->kind == TYPE_CHAR);

        /* Check if the handle is a variable - if so, we need to update it after sync
         * This ensures that after x! is used, subsequent uses of x return the synced value */
        bool is_variable_handle = (sync->handle->type == EXPR_VARIABLE);

        if (is_primitive)
        {
            /* Primitive type: cast pointer and dereference
             * Uses rt_thread_sync_with_result for panic propagation + result promotion
             * For variable handles, we have TWO variables: __var_pending__ (RtThreadHandle*)
             * and var (actual type). Sync uses the pending handle, assigns to the typed var. */
            if (is_variable_handle)
            {
                /* For primitive thread spawn variables, we declared:
                 *   RtThreadHandle *__var_pending__ = &fn();
                 *   type var;
                 * Now sync using __var_pending__ and assign result to var.
                 * Pattern: ({ var = *(type*)sync(__var_pending__, ...); var; }) */
                char *var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    %s = *(%s *)rt_thread_sync_with_result(%s, %s, %s);\n"
                    "    %s;\n"
                    "})",
                    var_name, c_type, pending_var, ARENA_VAR(gen), rt_type,
                    var_name);
            }
            else
            {
                /* Non-variable (e.g., inline spawn): just return the value */
                return arena_sprintf(gen->arena,
                    "(*(%s *)rt_thread_sync_with_result(%s, %s, %s))",
                    c_type, handle_code, ARENA_VAR(gen), rt_type);
            }
        }
        else
        {
            /* Reference type (string, array, etc.): cast pointer directly
             * Uses rt_thread_sync_with_result for panic propagation + result promotion */
            if (is_variable_handle)
            {
                /* Update variable and return value */
                return arena_sprintf(gen->arena,
                    "(%s = (%s)rt_thread_sync_with_result(%s, %s, %s))",
                    handle_code, c_type, handle_code, ARENA_VAR(gen), rt_type);
            }
            else
            {
                /* Non-variable: just return the value */
                return arena_sprintf(gen->arena,
                    "((%s)rt_thread_sync_with_result(%s, %s, %s))",
                    c_type, handle_code, ARENA_VAR(gen), rt_type);
            }
        }
    }
}
