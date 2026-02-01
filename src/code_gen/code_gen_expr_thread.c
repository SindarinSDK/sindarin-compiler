/**
 * code_gen_expr_thread.c - Code generation for thread spawn expressions
 *
 * Contains implementation for generating C code from thread spawn (&fn())
 * expressions. Thread sync expressions are in code_gen_expr_thread_sync.c.
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

char *code_gen_thread_spawn_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_thread_spawn_expression");

    ThreadSpawnExpr *spawn = &expr->as.thread_spawn;
    FunctionModifier modifier = spawn->modifier;

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
        "    /* Set thread arena for closures called from this thread */\n"
        "    rt_set_thread_arena(__arena__);\n"
        "\n"
        "    /* Set up panic context to catch panics in this thread */\n"
        "    RtThreadPanicContext __panic_ctx__;\n"
        "    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);\n"
        "    if (setjmp(__panic_ctx__.jump_buffer) != 0) {\n"
        "        /* Panic occurred - cleanup and return */\n"
        "        rt_set_thread_arena(NULL);\n"
        "        rt_thread_panic_context_clear();\n"
        "        return NULL;\n"
        "    }\n"
        "\n",
        wrapper_name, args_struct_name, args_struct_name);

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
    bool is_user_function = false;
    bool target_needs_arena_arg = false;
    const char *func_name_for_intercept = NULL;

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
        is_user_function = !method_is_native;
        target_needs_arena_arg = !method_is_native;
        func_name_for_intercept = arena_sprintf(gen->arena, "%s.%s",
                                                 self_struct_name, method_name);
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

        /* For handle-type args in non-shared mode, promote from caller's arena
         * to thread's arena since handles are per-arena */
        Type *arg_type = arguments[i]->expr_type;
        bool is_handle_arg = gen->current_arena_var != NULL && arg_type != NULL &&
                             (arg_type->kind == TYPE_ARRAY || arg_type->kind == TYPE_STRING);
        if (is_handle_arg && modifier != FUNC_SHARED)
        {
            call_args = arena_sprintf(gen->arena, "%srt_managed_clone(__arena__, args->caller_arena, args->arg%d)",
                                      call_args, i);
        }
        else
        {
            call_args = arena_sprintf(gen->arena, "%sargs->arg%d", call_args, i);
        }
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

        /* For method calls, self is at __rt_thunk_args[0], so regular args start at index 1 */
        int arg_offset = is_method_call ? 1 : 0;

        /* For 'as ref' primitives, declare local variables to hold unboxed values */
        for (int i = 0; i < arg_count; i++)
        {
            Type *arg_type = arguments[i]->expr_type;
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
                    thunk_def, c_type, i, unbox_func, i + arg_offset);
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

        /* For method calls, add self after arena (unboxed from __rt_thunk_args[0]) */
        if (is_method_call)
        {
            int type_id = get_struct_type_id(self_struct_type);
            if (unboxed_args[0] != '\0')
            {
                unboxed_args = arena_sprintf(gen->arena,
                    "%s, ((%s *)rt_unbox_struct(__rt_thunk_args[0], %d))",
                    unboxed_args, mangled_self_type, type_id);
            }
            else
            {
                unboxed_args = arena_sprintf(gen->arena,
                    "((%s *)rt_unbox_struct(__rt_thunk_args[0], %d))",
                    mangled_self_type, type_id);
            }
        }

        for (int i = 0; i < arg_count; i++)
        {
            Type *arg_type = arguments[i]->expr_type;
            const char *unbox_func = get_unboxing_function(arg_type);
            /* Need comma if not first arg, or if we have arena/self before us */
            bool need_comma = (i > 0) || target_needs_arena_arg || is_method_call;
            if (need_comma)
            {
                unboxed_args = arena_sprintf(gen->arena, "%s, ", unboxed_args);
            }

            /* Index into __rt_thunk_args, offset by arg_offset for method calls */
            int thunk_arg_idx = i + arg_offset;

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
                unboxed_args = arena_sprintf(gen->arena, "%s__rt_thunk_args[%d]", unboxed_args, thunk_arg_idx);
            }
            else if (arg_type && arg_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
            {
                /* In handle mode, clone the unboxed array into the thunk's arena */
                const char *suffix = code_gen_type_suffix(arg_type->as.array.element_type);
                const char *elem_c = get_c_array_elem_type(gen->arena, arg_type->as.array.element_type);
                unboxed_args = arena_sprintf(gen->arena,
                    "%srt_array_clone_%s_h((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, (%s *)%s(__rt_thunk_args[%d]))",
                    unboxed_args, suffix, elem_c, unbox_func, thunk_arg_idx);
            }
            else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
            {
                /* In handle mode, convert unboxed char* back to RtHandle */
                unboxed_args = arena_sprintf(gen->arena,
                    "%srt_managed_strdup((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, %s(__rt_thunk_args[%d]))",
                    unboxed_args, unbox_func, thunk_arg_idx);
            }
            else if (arg_type && arg_type->kind == TYPE_STRUCT)
            {
                /* Struct type: unbox with type_id and dereference */
                int type_id = get_struct_type_id(arg_type);
                const char *struct_name = get_c_type(gen->arena, arg_type);
                unboxed_args = arena_sprintf(gen->arena,
                    "%s*((%s *)rt_unbox_struct(__rt_thunk_args[%d], %d))",
                    unboxed_args, struct_name, thunk_arg_idx, type_id);
            }
            else
            {
                unboxed_args = arena_sprintf(gen->arena, "%s%s(__rt_thunk_args[%d])", unboxed_args, unbox_func, thunk_arg_idx);
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
                if (gen->current_arena_var != NULL)
                {
                    /* In handle mode, array result is RtHandle — cast to void* for boxing */
                    thunk_def = arena_sprintf(gen->arena,
                        "%s    RtAny __result = %s((void *)(uintptr_t)%s(%s), %s);\n",
                        thunk_def, box_func, callee_str, unboxed_args, elem_tag);
                }
                else
                {
                    thunk_def = arena_sprintf(gen->arena,
                        "%s    RtAny __result = %s(%s(%s), %s);\n",
                        thunk_def, box_func, callee_str, unboxed_args, elem_tag);
                }
            }
            else if (return_type && return_type->kind == TYPE_STRUCT)
            {
                int type_id = get_struct_type_id(return_type);
                const char *struct_name = get_c_type(gen->arena, return_type);
                thunk_def = arena_sprintf(gen->arena,
                    "%s    %s __tmp_result = %s(%s);\n"
                    "    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(%s), %d);\n",
                    thunk_def, struct_name, callee_str, unboxed_args,
                    struct_name, type_id);
            }
            else if (return_type && return_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
            {
                /* In handle mode, string result is RtHandle — pin to get char* for boxing */
                thunk_def = arena_sprintf(gen->arena,
                    "%s    RtAny __result = %s((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, %s(%s)));\n",
                    thunk_def, box_func, callee_str, unboxed_args);
            }
            else
            {
                thunk_def = arena_sprintf(gen->arena,
                    "%s    RtAny __result = %s(%s(%s));\n",
                    thunk_def, box_func, callee_str, unboxed_args);
            }
        }

        /* Write back modified values for 'as ref' primitives */
        for (int i = 0; i < arg_count; i++)
        {
            Type *arg_type = arguments[i]->expr_type;
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
        /* For method calls, we need one extra slot for self */
        int total_intercept_args = arg_count + (is_method_call ? 1 : 0);

        if (is_void_return)
        {
            wrapper_def = arena_sprintf(gen->arena,
                "%s"
                "    /* Call the function with interceptor support */\n"
                "    if (__rt_interceptor_count > 0) {\n"
                "        RtAny __args[%d];\n",
                wrapper_def, total_intercept_args > 0 ? total_intercept_args : 1);

            /* For method calls, box self as __args[0] */
            if (is_method_call)
            {
                int type_id = get_struct_type_id(self_struct_type);
                wrapper_def = arena_sprintf(gen->arena,
                    "%s        __args[0] = rt_box_struct(__arena__, (void *)args->__sn__self, sizeof(%s), %d);\n",
                    wrapper_def, mangled_self_type, type_id);
            }

            /* Box arguments - for 'as ref' primitives, dereference the pointer */
            /* For method calls, args start at index 1 in __args */
            int args_start_idx = is_method_call ? 1 : 0;
            for (int i = 0; i < arg_count; i++)
            {
                Type *arg_type = arguments[i]->expr_type;
                const char *box_func = get_boxing_function(arg_type);
                int arg_idx = args_start_idx + i;

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
                    if (gen->current_arena_var != NULL)
                    {
                        /* In handle mode, pin the handle to get void* for boxing */
                        wrapper_def = arena_sprintf(gen->arena,
                            "%s        __args[%d] = %s(rt_managed_pin_array_any(args->caller_arena, args->arg%d), %s);\n",
                            wrapper_def, arg_idx, box_func, i, elem_tag);
                    }
                    else
                    {
                        wrapper_def = arena_sprintf(gen->arena,
                            "%s        __args[%d] = %s(args->arg%d, %s);\n",
                            wrapper_def, arg_idx, box_func, i, elem_tag);
                    }
                }
                else if (arg_type && arg_type->kind == TYPE_STRUCT)
                {
                    int type_id = get_struct_type_id(arg_type);
                    const char *struct_name = get_c_type(gen->arena, arg_type);
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = rt_box_struct(__arena__, &(args->arg%d), sizeof(%s), %d);\n",
                        wrapper_def, arg_idx, i, struct_name, type_id);
                }
                else if (is_ref_primitive)
                {
                    /* Dereference pointer for as ref primitives */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(*args->arg%d);\n",
                        wrapper_def, arg_idx, box_func, i);
                }
                else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
                {
                    /* In handle mode, pin the string handle to get char* for boxing */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s((char *)rt_managed_pin(args->caller_arena, args->arg%d));\n",
                        wrapper_def, arg_idx, box_func, i);
                }
                else
                {
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d);\n",
                        wrapper_def, arg_idx, box_func, i);
                }
            }

            /* Generate write-back code for as ref primitives after interception */
            char *writeback_code = arena_strdup(gen->arena, "");
            for (int i = 0; i < arg_count; i++)
            {
                Type *arg_type = arguments[i]->expr_type;
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
                "    /* Clear thread arena and panic context on successful completion */\n"
                "    rt_set_thread_arena(NULL);\n"
                "    rt_thread_panic_context_clear();\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, func_name_for_intercept, total_intercept_args, thunk_name,
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
                wrapper_def, ret_c_type, total_intercept_args > 0 ? total_intercept_args : 1);

            /* For method calls, box self as __args[0] */
            if (is_method_call)
            {
                int type_id = get_struct_type_id(self_struct_type);
                wrapper_def = arena_sprintf(gen->arena,
                    "%s        __args[0] = rt_box_struct(__arena__, (void *)args->__sn__self, sizeof(%s), %d);\n",
                    wrapper_def, mangled_self_type, type_id);
            }

            /* Box arguments - for 'as ref' primitives, dereference the pointer */
            /* For method calls, args start at index 1 in __args */
            int args_start_idx = is_method_call ? 1 : 0;
            for (int i = 0; i < arg_count; i++)
            {
                Type *arg_type = arguments[i]->expr_type;
                const char *box_func = get_boxing_function(arg_type);
                int arg_idx = args_start_idx + i;

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
                    if (gen->current_arena_var != NULL)
                    {
                        /* In handle mode, pin the handle to get void* for boxing */
                        wrapper_def = arena_sprintf(gen->arena,
                            "%s        __args[%d] = %s(rt_managed_pin_array_any(args->caller_arena, args->arg%d), %s);\n",
                            wrapper_def, arg_idx, box_func, i, elem_tag);
                    }
                    else
                    {
                        wrapper_def = arena_sprintf(gen->arena,
                            "%s        __args[%d] = %s(args->arg%d, %s);\n",
                            wrapper_def, arg_idx, box_func, i, elem_tag);
                    }
                }
                else if (arg_type && arg_type->kind == TYPE_STRUCT)
                {
                    int type_id = get_struct_type_id(arg_type);
                    const char *struct_name = get_c_type(gen->arena, arg_type);
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = rt_box_struct(__arena__, &(args->arg%d), sizeof(%s), %d);\n",
                        wrapper_def, arg_idx, i, struct_name, type_id);
                }
                else if (is_ref_primitive)
                {
                    /* Dereference pointer for as ref primitives */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(*args->arg%d);\n",
                        wrapper_def, arg_idx, box_func, i);
                }
                else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
                {
                    /* In handle mode, pin the string handle to get char* for boxing */
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s((char *)rt_managed_pin(args->caller_arena, args->arg%d));\n",
                        wrapper_def, arg_idx, box_func, i);
                }
                else
                {
                    wrapper_def = arena_sprintf(gen->arena,
                        "%s        __args[%d] = %s(args->arg%d);\n",
                        wrapper_def, arg_idx, box_func, i);
                }
            }

            /* Generate write-back code for as ref primitives after interception */
            char *writeback_code = arena_strdup(gen->arena, "");
            for (int i = 0; i < arg_count; i++)
            {
                Type *arg_type = arguments[i]->expr_type;
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

            /* Generate the unbox expression for the intercepted result */
            char *unbox_expr;
            if (return_type && return_type->kind == TYPE_STRUCT)
            {
                int type_id = get_struct_type_id(return_type);
                const char *struct_name = get_c_type(gen->arena, return_type);
                unbox_expr = arena_sprintf(gen->arena,
                    "*((%s *)rt_unbox_struct(__intercepted, %d))",
                    struct_name, type_id);
            }
            else if (return_type && return_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
            {
                /* String result: unbox to raw char*, then convert to handle */
                unbox_expr = arena_sprintf(gen->arena, "rt_managed_strdup(__arena__, RT_HANDLE_NULL, %s(__intercepted))",
                                           unbox_func);
            }
            else if (return_type && return_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
            {
                /* Array result: unbox to raw pointer, cast back to RtHandle */
                unbox_expr = arena_sprintf(gen->arena, "(RtHandle)(uintptr_t)%s(__intercepted)", unbox_func);
            }
            else
            {
                unbox_expr = arena_sprintf(gen->arena, "%s(__intercepted)", unbox_func);
            }

            wrapper_def = arena_sprintf(gen->arena,
                "%s        __rt_thunk_args = __args;\n"
                "        __rt_thunk_arena = __arena__;\n"
                "        RtAny __intercepted = rt_call_intercepted(\"%s\", __args, %d, %s);\n"
                "%s"
                "        __result__ = %s;\n"
                "    } else {\n"
                "        __result__ = %s(%s);\n"
                "    }\n"
                "\n"
                "    /* Store result in thread result structure using runtime function */\n"
                "    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;\n"
                "    rt_thread_result_set_value(args->result, &__result__, sizeof(%s), __result_arena__);\n"
                "\n"
                "    /* Clear thread arena and panic context on successful completion */\n"
                "    rt_set_thread_arena(NULL);\n"
                "    rt_thread_panic_context_clear();\n"
                "    return NULL;\n"
                "}\n\n",
                wrapper_def, func_name_for_intercept, total_intercept_args, thunk_name,
                writeback_code, unbox_expr, callee_str, call_args, ret_c_type);
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
            "    /* Clear thread arena and panic context on successful completion */\n"
            "    rt_set_thread_arena(NULL);\n"
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
            "    /* Clear thread arena and panic context on successful completion */\n"
            "    rt_set_thread_arena(NULL);\n"
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

        /* For handle-type args (array/string in arena mode), generate as handle
         * so we store the RtHandle value directly instead of pinning */
        bool is_handle_arg = gen->current_arena_var != NULL && arg_expr->expr_type != NULL &&
                             (arg_expr->expr_type->kind == TYPE_ARRAY || arg_expr->expr_type->kind == TYPE_STRING);
        bool saved_handle = gen->expr_as_handle;
        if (is_handle_arg) gen->expr_as_handle = true;
        char *arg_code = code_gen_expression(gen, arg_expr);
        gen->expr_as_handle = saved_handle;

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

                /* Build thunk parameter list with closure as first param */
                char *thunk_params = arena_strdup(gen->arena, "void *__cl__");
                char *thunk_call_args;
                if (target_needs_arena)
                {
                    /* Prepend arena from closure as first argument.
                     * Use rt_get_thread_arena_or() to prefer thread arena when called from thread context. */
                    thunk_call_args = arena_strdup(gen->arena, "(RtManagedArena *)rt_get_thread_arena_or(((__Closure__ *)__cl__)->arena)");
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
                "%s%s->arg%d = ({ __Closure__ *__fn_cl__ = rt_arena_alloc(%s, sizeof(__Closure__)); "
                "__fn_cl__->fn = (void *)%s; __fn_cl__->arena = %s; __fn_cl__; }); ",
                arg_assignments, args_var, i, caller_arena, fn_thunk_name, caller_arena);
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
