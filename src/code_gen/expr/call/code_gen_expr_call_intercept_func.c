/**
 * code_gen_expr_call_intercept_func.c - Code generation for intercepted function calls
 *
 * Contains the function for generating intercepted regular function calls.
 */

/**
 * Generate an intercepted function call.
 * This wraps a user-defined function call with interception logic:
 * - Fast path when no interceptors registered
 * - When interceptors present: box args, create thunk, call through chain
 *
 * Uses GNU C extensions (nested functions, statement expressions) which work
 * with GCC and Clang but not MSVC or TinyCC.
 *
 * @param gen           Code generator context
 * @param func_name     Name of the function being called
 * @param callee_str    Generated C code for the callee
 * @param call          The call expression AST node
 * @param arg_strs      Array of raw generated argument expression strings
 * @param arg_names     Array of transformed argument strings (with boxing, as-ref, closure wrapping applied)
 * @param param_types   Array of parameter types from function signature
 * @param param_count   Number of parameters
 * @param return_type   Return type of the function
 * @param callee_has_body Whether the callee function has a body (Sindarin function, needs arena arg)
 * @return Generated C code for the intercepted call
 */
char *code_gen_intercepted_call(CodeGen *gen, const char *func_name,
                                       const char *callee_str,
                                       CallExpr *call, char **arg_strs, char **arg_names,
                                       Type **param_types, MemoryQualifier *param_quals,
                                       int param_count, Type *return_type, bool callee_has_body)
{
    Arena *arena = gen->arena;
    bool returns_void = (return_type && return_type->kind == TYPE_VOID);
    const char *ret_c = get_c_type(arena, return_type);

    /* Generate unique thunk ID */
    int thunk_id = gen->thunk_count++;
    char *thunk_name = arena_sprintf(arena, "__thunk_%d", thunk_id);

    /* Check if any parameters are 'as ref' */
    bool has_ref_params = false;
    if (param_quals != NULL)
    {
        for (int i = 0; i < param_count && i < call->arg_count; i++)
        {
            if (param_quals[i] == MEM_AS_REF)
            {
                has_ref_params = true;
                break;
            }
        }
    }

    /* Generate thunk forward declaration */
    gen->thunk_forward_decls = arena_sprintf(arena, "%sstatic RtAny %s(void);\n",
                                             gen->thunk_forward_decls, thunk_name);

    /* Generate thunk definition */
    char *thunk_def = arena_sprintf(arena, "static RtAny %s(void) {\n", thunk_name);

    /* For 'as ref' parameters, declare local variables to hold unboxed values */
    for (int i = 0; i < call->arg_count; i++)
    {
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
        if (is_ref)
        {
            Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
            const char *c_type = get_c_type(arena, arg_type);
            const char *unbox_func = get_unboxing_function(arg_type);
            if (unbox_func != NULL)
            {
                thunk_def = arena_sprintf(arena, "%s    %s __ref_%d = %s(__rt_thunk_args[%d]);\n",
                                          thunk_def, c_type, i, unbox_func, i);
            }
        }
    }

    /* Build unboxed argument list for the thunk body */
    char *unboxed_args;
    if (callee_has_body)
    {
        unboxed_args = arena_strdup(arena, "(RtArenaV2 *)__rt_thunk_arena");
    }
    else
    {
        unboxed_args = arena_strdup(arena, "");
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
        const char *unbox_func = get_unboxing_function(arg_type);
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);

        bool need_comma = (i > 0) || callee_has_body;
        if (need_comma)
        {
            unboxed_args = arena_sprintf(arena, "%s, ", unboxed_args);
        }

        if (is_ref)
        {
            /* Pass address of local variable */
            unboxed_args = arena_sprintf(arena, "%s&__ref_%d", unboxed_args, i);
        }
        else if (unbox_func == NULL)
        {
            /* For 'any' type parameters, pass directly (already RtAny) */
            unboxed_args = arena_sprintf(arena, "%s__rt_thunk_args[%d]", unboxed_args, i);
        }
        else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* In handle mode, wrap unboxed char* as RtHandleV2* */
            unboxed_args = arena_sprintf(arena, "%srt_arena_v2_strdup((RtArenaV2 *)__rt_thunk_arena, %s(__rt_thunk_args[%d]))",
                                          unboxed_args, unbox_func, i);
        }
        else if (arg_type && arg_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
        {
            /* In handle mode, unboxed array is (void*)(uintptr_t)handle — cast back to V2 handle */
            unboxed_args = arena_sprintf(arena, "%s(RtHandleV2 *)(uintptr_t)%s(__rt_thunk_args[%d])",
                                          unboxed_args, unbox_func, i);
        }
        else
        {
            unboxed_args = arena_sprintf(arena, "%s%s(__rt_thunk_args[%d])", unboxed_args, unbox_func, i);
        }
    }

    /* Make the actual function call */
    if (returns_void)
    {
        thunk_def = arena_sprintf(arena, "%s    %s(%s);\n", thunk_def, callee_str, unboxed_args);
    }
    else
    {
        const char *box_func = get_boxing_function(return_type);
        if (box_func == NULL)
        {
            /* Return type is 'any' - already boxed, no boxing needed */
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s);\n",
                                      thunk_def, callee_str, unboxed_args);
        }
        else if (return_type && return_type->kind == TYPE_ARRAY)
        {
            const char *elem_tag = get_element_type_tag(return_type->as.array.element_type);
            if (gen->current_arena_var != NULL)
            {
                /* In handle mode, array result is RtHandle — cast to void* for boxing */
                thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s((void *)(uintptr_t)%s(%s), %s);\n",
                                          thunk_def, box_func, callee_str, unboxed_args, elem_tag);
            }
            else
            {
                thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s(%s), %s);\n",
                                          thunk_def, box_func, callee_str, unboxed_args, elem_tag);
            }
        }
        else if (return_type && return_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* In V2 handle mode, string result is RtHandleV2* — access ptr for boxing */
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s((char *)(%s(%s))->ptr);\n",
                                      thunk_def, box_func, callee_str, unboxed_args);
        }
        else
        {
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s(%s));\n",
                                      thunk_def, box_func, callee_str, unboxed_args);
        }
    }

    /* For 'as ref' parameters, write modified values back to args array */
    for (int i = 0; i < call->arg_count; i++)
    {
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
        if (is_ref)
        {
            Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
            const char *box_func = get_boxing_function(arg_type);
            if (box_func != NULL)
            {
                thunk_def = arena_sprintf(arena, "%s    __rt_thunk_args[%d] = %s(__ref_%d);\n",
                                          thunk_def, i, box_func, i);
            }
        }
    }

    /* Return the result */
    if (returns_void)
    {
        thunk_def = arena_sprintf(arena, "%s    return rt_box_nil();\n", thunk_def);
    }
    else
    {
        thunk_def = arena_sprintf(arena, "%s    return __result;\n", thunk_def);
    }
    thunk_def = arena_sprintf(arena, "%s}\n", thunk_def);
    gen->thunk_definitions = arena_sprintf(arena, "%s%s\n", gen->thunk_definitions, thunk_def);

    /* Now generate the call site code */
    char *result = arena_strdup(arena, "({\n");

    /* Evaluate complex arguments into temporaries to avoid exponential code
     * duplication when intercepted calls are nested (each arg expression would
     * otherwise be duplicated in both the interceptor and fast-path branches).
     * Only function calls need temps - simple expressions (variables, literals)
     * are cheap to duplicate and may need special handling (lvalues, closures). */
    char **arg_temps = arena_alloc(arena, call->arg_count * sizeof(char *));
    for (int i = 0; i < call->arg_count; i++)
    {
        bool needs_temp = (call->arguments[i]->type == EXPR_CALL);
        if (needs_temp)
        {
            Type *arg_type = call->arguments[i]->expr_type;
            const char *arg_c_type = get_c_type(arena, arg_type);
            char *temp_name = arena_sprintf(arena, "__iarg_%d_%d", thunk_id, i);
            result = arena_sprintf(arena, "%s    %s %s = %s;\n", result, arg_c_type, temp_name, arg_strs[i]);
            arg_temps[i] = temp_name;
        }
        else
        {
            arg_temps[i] = arg_strs[i];
        }
    }

    /* Build direct-call args list. For temped args (EXPR_CALL), use the temp
     * with any-boxing applied. For non-temped args, use arg_names which already
     * has all transformations applied (closure wrapping, boxing, as-ref). */
    char *direct_args;
    if (callee_has_body)
    {
        if (gen->current_arena_var != NULL)
        {
            direct_args = arena_strdup(arena, gen->current_arena_var);
        }
        else
        {
            direct_args = arena_strdup(arena, "NULL");
        }
    }
    else
    {
        direct_args = arena_strdup(arena, "");
    }
    for (int i = 0; i < call->arg_count; i++)
    {
        char *arg_val;
        bool was_temped = (call->arguments[i]->type == EXPR_CALL);

        if (was_temped)
        {
            /* Temped args: use the temp name, box for 'any' params if needed */
            arg_val = arg_temps[i];
            if (param_types != NULL && i < param_count &&
                param_types[i] != NULL && param_types[i]->kind == TYPE_ANY &&
                call->arguments[i]->expr_type != NULL &&
                call->arguments[i]->expr_type->kind != TYPE_ANY)
            {
                arg_val = code_gen_box_value(gen, arg_val, call->arguments[i]->expr_type);
            }
        }
        else
        {
            /* Non-temped args: use pre-transformed arg_names (has closure
             * wrapping, any-boxing, as-ref already applied by caller) */
            arg_val = arg_names[i];
        }

        if (i == 0 && !callee_has_body)
        {
            direct_args = arena_strdup(arena, arg_val);
        }
        else
        {
            direct_args = arena_sprintf(arena, "%s, %s", direct_args, arg_val);
        }
    }

    /* Declare result variable */
    if (!returns_void)
    {
        result = arena_sprintf(arena, "%s    %s __intercept_result;\n", result, ret_c);
    }

    /* Fast path check */
    result = arena_sprintf(arena, "%s    if (__rt_interceptor_count > 0) {\n", result);

    /* Box arguments into RtAny array */
    result = arena_sprintf(arena, "%s        RtAny __args[%d];\n", result, call->arg_count > 0 ? call->arg_count : 1);
    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = call->arguments[i]->expr_type;
        const char *box_func = get_boxing_function(arg_type);
        if (box_func == NULL)
        {
            /* Argument is already 'any' - no boxing needed */
            result = arena_sprintf(arena, "%s        __args[%d] = %s;\n",
                                   result, i, arg_temps[i]);
        }
        else if (arg_type && arg_type->kind == TYPE_ARRAY)
        {
            const char *elem_tag = get_element_type_tag(arg_type->as.array.element_type);
            if (gen->current_arena_var != NULL)
            {
                /* In handle mode, array temp is RtHandle — cast to void* for boxing */
                result = arena_sprintf(arena, "%s        __args[%d] = %s((void *)(uintptr_t)%s, %s);\n",
                                       result, i, box_func, arg_temps[i], elem_tag);
            }
            else
            {
                result = arena_sprintf(arena, "%s        __args[%d] = %s(%s, %s);\n",
                                       result, i, box_func, arg_temps[i], elem_tag);
            }
        }
        else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* In V2 handle mode, string temp is RtHandleV2* — access ptr for boxing. */
            result = arena_sprintf(arena, "%s        __args[%d] = %s((char *)(%s)->ptr);\n",
                                   result, i, box_func, arg_temps[i]);
        }
        else
        {
            result = arena_sprintf(arena, "%s        __args[%d] = %s(%s);\n",
                                   result, i, box_func, arg_temps[i]);
        }
    }

    /* Set thread-local args and arena for thunk */
    result = arena_sprintf(arena, "%s        __rt_thunk_args = __args;\n", result);
    /* Set the thunk arena for interceptors - they return 'any' and need an arena.
     * Only set it when we have an arena; otherwise leave it unchanged so it can
     * inherit from an outer scope (e.g., thread wrapper) */
    if (gen->current_arena_var != NULL)
    {
        result = arena_sprintf(arena, "%s        __rt_thunk_arena = %s;\n", result, gen->current_arena_var);
    }

    /* Call through interceptor chain */
    result = arena_sprintf(arena, "%s        RtAny __intercepted = rt_call_intercepted(\"%s\", __args, %d, %s);\n",
                           result, func_name, call->arg_count, thunk_name);

    /* Unbox result */
    if (!returns_void)
    {
        const char *unbox_func = get_unboxing_function(return_type);
        if (unbox_func == NULL)
        {
            /* For 'any' return type, no unboxing needed */
            result = arena_sprintf(arena, "%s        __intercept_result = __intercepted;\n", result);
        }
        else if (return_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* String result: unbox to raw char*, then convert to V2 handle */
            result = arena_sprintf(arena, "%s        __intercept_result = rt_arena_v2_strdup(%s, %s(__intercepted));\n",
                                   result, ARENA_VAR(gen), unbox_func);
        }
        else if (return_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
        {
            /* Array result: unbox to raw pointer (which is actually the stored RtHandleV2* cast to void*) - cast back */
            result = arena_sprintf(arena, "%s        __intercept_result = (RtHandleV2 *)(uintptr_t)%s(__intercepted);\n",
                                   result, unbox_func);
        }
        else
        {
            result = arena_sprintf(arena, "%s        __intercept_result = %s(__intercepted);\n", result, unbox_func);
        }
    }

    /* Write back modified values for 'as ref' parameters */
    if (has_ref_params)
    {
        for (int i = 0; i < call->arg_count; i++)
        {
            bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
            if (is_ref)
            {
                Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
                const char *unbox_func = get_unboxing_function(arg_type);
                if (unbox_func != NULL)
                {
                    /* Write back to original variable: arg_strs[i] is the lvalue */
                    result = arena_sprintf(arena, "%s        %s = %s(__args[%d]);\n",
                                           result, arg_strs[i], unbox_func, i);
                }
            }
        }
    }

    /* Close interceptor branch, add fast path */
    result = arena_sprintf(arena, "%s    } else {\n", result);
    if (returns_void)
    {
        result = arena_sprintf(arena, "%s        %s(%s);\n", result, callee_str, direct_args);
    }
    else
    {
        result = arena_sprintf(arena, "%s        __intercept_result = %s(%s);\n", result, callee_str, direct_args);
    }
    result = arena_sprintf(arena, "%s    }\n", result);

    /* Return result */
    if (returns_void)
    {
        result = arena_sprintf(arena, "%s    (void)0;\n})", result);
    }
    else
    {
        result = arena_sprintf(arena, "%s    __intercept_result;\n})", result);
    }

    return result;
}

