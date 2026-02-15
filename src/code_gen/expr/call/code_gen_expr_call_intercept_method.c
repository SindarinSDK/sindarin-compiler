/**
 * code_gen_expr_call_intercept_method.c - Code generation for intercepted method calls
 *
 * Contains functions for generating intercepted struct method calls.
 */

/**
 * Check if a struct method should be intercepted.
 * Skips native methods and methods with unsupported parameter/return types.
 */
bool should_intercept_method(StructMethod *method, Type *struct_type, Type *return_type)
{
    /* Native methods are never intercepted */
    if (method->is_native)
        return false;

    /* Methods on native structs are never intercepted (no C typedef for sizeof/memcpy) */
    if (struct_type != NULL && struct_type->kind == TYPE_STRUCT &&
        struct_type->as.struct_type.is_native)
        return false;

    /* Check non-self parameters for unsupported types */
    for (int i = 0; i < method->param_count; i++)
    {
        if (method->params[i].type != NULL &&
            (method->params[i].type->kind == TYPE_POINTER ||
             method->params[i].type->kind == TYPE_STRUCT))
        {
            return false;
        }
    }

    /* Check return type */
    if (return_type != NULL &&
        (return_type->kind == TYPE_POINTER || return_type->kind == TYPE_STRUCT))
    {
        return false;
    }

    return true;
}

/**
 * Generate an intercepted struct method call.
 * Similar to code_gen_intercepted_call() but handles:
 * - Self boxing as args[0] for instance methods
 * - Self writeback after the call to propagate mutations
 * - Struct-qualified name ("StructName.methodName")
 *
 * @param gen             Code generator context
 * @param struct_name     Name of the struct (unmangled)
 * @param method          The struct method being called
 * @param struct_type     The struct's Type node
 * @param arg_count       Number of arguments (excluding self)
 * @param arguments       Argument expressions
 * @param self_ptr_str    Generated C expression for pointer to self (NULL for static)
 * @param is_self_pointer Whether self is already a pointer type (inside method body)
 * @param return_type     Return type of the method
 * @return Generated C code for the intercepted method call
 */
char *code_gen_intercepted_method_call(CodeGen *gen,
                                        const char *struct_name,
                                        StructMethod *method,
                                        Type *struct_type,
                                        int arg_count,
                                        Expr **arguments,
                                        const char *self_ptr_str,
                                        bool is_self_pointer,
                                        Type *return_type)
{
    Arena *arena = gen->arena;
    bool returns_void = (return_type && return_type->kind == TYPE_VOID);
    const char *ret_c = get_c_type(arena, return_type);
    bool is_instance = !method->is_static;
    int total_arg_count = is_instance ? (arg_count + 1) : arg_count;

    char *mangled_struct = sn_mangle_name(arena, struct_name);
    int type_id = get_struct_type_id(struct_type);

    /* Build the qualified method name: "StructName.methodName" */
    char *qualified_name = arena_sprintf(arena, "%s.%s", struct_name, method->name);

    /* Build the direct C callee: StructName_methodName */
    char *callee_str = arena_sprintf(arena, "%s_%s", mangled_struct, method->name);

    /* Generate unique thunk ID */
    int thunk_id = gen->thunk_count++;
    char *thunk_name = arena_sprintf(arena, "__thunk_%d", thunk_id);

    /* Generate thunk forward declaration */
    gen->thunk_forward_decls = arena_sprintf(arena, "%sstatic RtAny %s(void);\n",
                                             gen->thunk_forward_decls, thunk_name);

    /* Generate thunk definition */
    char *thunk_def = arena_sprintf(arena, "static RtAny %s(void) {\n", thunk_name);

    /* Build unboxed argument list for the thunk - always starts with arena */
    char *unboxed_args = arena_strdup(arena, "(RtArenaV2 *)__rt_thunk_arena");

    if (is_instance)
    {
        /* Unbox self from args[0] */
        thunk_def = arena_sprintf(arena, "%s    RtHandleV2 *__self_h = rt_unbox_struct(__rt_thunk_args[0], %d);\n"
                                  "    %s *__self = (%s *)__self_h->ptr;\n",
                                  thunk_def, type_id, mangled_struct, mangled_struct);
        unboxed_args = arena_sprintf(arena, "%s, __self", unboxed_args);
    }

    /* Unbox remaining arguments (offset by 1 for instance methods) */
    int arg_offset = is_instance ? 1 : 0;
    for (int i = 0; i < arg_count; i++)
    {
        Type *arg_type = arguments[i]->expr_type;
        const char *unbox_func = get_unboxing_function(arg_type);

        unboxed_args = arena_sprintf(arena, "%s, ", unboxed_args);

        if (unbox_func == NULL)
        {
            /* For 'any' type parameters, pass directly */
            unboxed_args = arena_sprintf(arena, "%s__rt_thunk_args[%d]", unboxed_args, i + arg_offset);
        }
        else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* In handle mode, wrap unboxed char* as RtHandleV2* */
            unboxed_args = arena_sprintf(arena, "%srt_arena_v2_strdup((RtArenaV2 *)__rt_thunk_arena, %s(__rt_thunk_args[%d]))",
                                          unboxed_args, unbox_func, i + arg_offset);
        }
        else if (arg_type && arg_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
        {
            /* In handle mode, unboxed array is stored as (void*)(uintptr_t)handle — cast back to V2 */
            unboxed_args = arena_sprintf(arena, "%s(RtHandleV2 *)(uintptr_t)%s(__rt_thunk_args[%d])",
                                          unboxed_args, unbox_func, i + arg_offset);
        }
        else
        {
            unboxed_args = arena_sprintf(arena, "%s%s(__rt_thunk_args[%d])", unboxed_args, unbox_func, i + arg_offset);
        }
    }

    /* Make the actual method call in the thunk */
    if (returns_void)
    {
        thunk_def = arena_sprintf(arena, "%s    %s(%s);\n", thunk_def, callee_str, unboxed_args);
    }
    else
    {
        const char *box_func = get_boxing_function(return_type);
        if (box_func == NULL)
        {
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

    /* Evaluate arguments into temporaries to avoid exponential code duplication
     * when intercepted calls are nested.
     * Struct methods are Sindarin functions, so args must be in handle mode. */
    bool saved_as_handle = gen->expr_as_handle;
    if (gen->current_arena_var != NULL)
    {
        gen->expr_as_handle = true;
    }
    char **arg_temps = arena_alloc(arena, arg_count * sizeof(char *));
    for (int i = 0; i < arg_count; i++)
    {
        char *arg_str = code_gen_expression(gen, arguments[i]);
        Type *arg_type = arguments[i]->expr_type;
        const char *arg_c_type = get_c_type(arena, arg_type);
        /* Check if argument needs closure wrapping for function-type parameter */
        if (i < method->param_count)
        {
            char *wrapped = code_gen_wrap_fn_arg_as_closure(gen,
                method->params[i].type, arguments[i], arg_str);
            if (wrapped != NULL)
            {
                arg_str = wrapped;
                arg_c_type = "__Closure__ *";
            }
        }
        char *temp_name = arena_sprintf(arena, "__iarg_%d_%d", thunk_id, i);
        result = arena_sprintf(arena, "%s    %s %s = %s;\n", result, arg_c_type, temp_name, arg_str);
        arg_temps[i] = temp_name;
    }
    gen->expr_as_handle = saved_as_handle;

    /* Declare result variable */
    if (!returns_void)
    {
        result = arena_sprintf(arena, "%s    %s __intercept_result;\n", result, ret_c);
    }

    /* Fast path check */
    result = arena_sprintf(arena, "%s    if (__rt_interceptor_count > 0) {\n", result);

    /* Box arguments into RtAny array */
    result = arena_sprintf(arena, "%s        RtAny __args[%d];\n", result, total_arg_count > 0 ? total_arg_count : 1);

    if (is_instance)
    {
        /* Box self as args[0] */
        result = arena_sprintf(arena,
            "%s        { RtHandleV2 *__bh = rt_arena_v2_alloc(%s, sizeof(%s)); rt_handle_begin_transaction(__bh); memcpy(__bh->ptr, %s, sizeof(%s)); rt_handle_end_transaction(__bh); __args[0] = rt_box_struct(%s, __bh, sizeof(%s), %d); }\n",
            result, ARENA_VAR(gen), mangled_struct, self_ptr_str, mangled_struct, ARENA_VAR(gen), mangled_struct, type_id);
    }

    /* Box remaining arguments using temporaries */
    for (int i = 0; i < arg_count; i++)
    {
        Type *arg_type = arguments[i]->expr_type;
        const char *box_func = get_boxing_function(arg_type);
        int arg_idx = i + arg_offset;

        if (box_func == NULL)
        {
            result = arena_sprintf(arena, "%s        __args[%d] = %s;\n",
                                   result, arg_idx, arg_temps[i]);
        }
        else if (arg_type && arg_type->kind == TYPE_ARRAY)
        {
            const char *elem_tag = get_element_type_tag(arg_type->as.array.element_type);
            if (gen->current_arena_var != NULL)
            {
                /* In handle mode, array temps are RtHandle — box as (void*)(uintptr_t) */
                result = arena_sprintf(arena, "%s        __args[%d] = %s((void *)(uintptr_t)%s, %s);\n",
                                       result, arg_idx, box_func, arg_temps[i], elem_tag);
            }
            else
            {
                result = arena_sprintf(arena, "%s        __args[%d] = %s(%s, %s);\n",
                                       result, arg_idx, box_func, arg_temps[i], elem_tag);
            }
        }
        else if (arg_type && arg_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            /* In V2 handle mode, string temps are RtHandleV2* — access ptr for boxing. */
            result = arena_sprintf(arena, "%s        __args[%d] = %s((char *)%s->ptr);\n",
                                   result, arg_idx, box_func, arg_temps[i]);
        }
        else
        {
            result = arena_sprintf(arena, "%s        __args[%d] = %s(%s);\n",
                                   result, arg_idx, box_func, arg_temps[i]);
        }
    }

    /* Set thread-local args and arena for thunk */
    result = arena_sprintf(arena, "%s        __rt_thunk_args = __args;\n", result);
    if (gen->current_arena_var != NULL)
    {
        result = arena_sprintf(arena, "%s        __rt_thunk_arena = %s;\n", result, gen->current_arena_var);
    }

    /* Call through interceptor chain */
    result = arena_sprintf(arena, "%s        RtAny __intercepted = rt_call_intercepted(\"%s\", __args, %d, %s);\n",
                           result, qualified_name, total_arg_count, thunk_name);

    /* Unbox result */
    if (!returns_void)
    {
        const char *unbox_func = get_unboxing_function(return_type);
        if (unbox_func == NULL)
        {
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

    /* Write back self mutations for instance methods */
    if (is_instance && !is_self_pointer)
    {
        /* self_ptr_str is like "&counter" - write back from boxed copy */
        result = arena_sprintf(arena, "%s        { RtHandleV2 *__wb = rt_unbox_struct(__args[0], %d); memcpy((void *)%s, __wb->ptr, sizeof(%s)); }\n",
                               result, type_id, self_ptr_str, mangled_struct);
    }
    else if (is_instance && is_self_pointer)
    {
        /* self is already a pointer (inside method body) - write back directly */
        /* self_ptr_str is the pointer itself, e.g., "self" */
        result = arena_sprintf(arena, "%s        { RtHandleV2 *__wb = rt_unbox_struct(__args[0], %d); memcpy((void *)%s, __wb->ptr, sizeof(%s)); }\n",
                               result, type_id, self_ptr_str, mangled_struct);
    }

    /* Close interceptor branch, add fast path using temporaries */
    result = arena_sprintf(arena, "%s    } else {\n", result);

    /* Build direct call args for fast path */
    char *direct_args = arena_strdup(arena, ARENA_VAR(gen));
    if (is_instance)
    {
        direct_args = arena_sprintf(arena, "%s, %s", direct_args, self_ptr_str);
    }
    for (int i = 0; i < arg_count; i++)
    {
        direct_args = arena_sprintf(arena, "%s, %s", direct_args, arg_temps[i]);
    }

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

