/**
 * code_gen_expr_call_array_query.c - Code generation for array query/copy methods
 */

/* Generate code for array.clear() method */
static char *code_gen_array_clear(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_array_clear(%s)", object_str);
}

/* Generate code for array.pop() method */
static char *code_gen_array_pop(CodeGen *gen, Expr *object, Type *element_type)
{
    /* Evaluate object in handle mode to get the RtHandle */
    bool prev_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = prev_as_handle;

    const char *pop_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            pop_func = "rt_array_pop_long_v2";
            break;
        case TYPE_INT32:
            pop_func = "rt_array_pop_int32_v2";
            break;
        case TYPE_UINT:
            pop_func = "rt_array_pop_uint_v2";
            break;
        case TYPE_UINT32:
            pop_func = "rt_array_pop_uint32_v2";
            break;
        case TYPE_FLOAT:
            pop_func = "rt_array_pop_float_v2";
            break;
        case TYPE_DOUBLE:
            pop_func = "rt_array_pop_double_v2";
            break;
        case TYPE_CHAR:
            pop_func = "rt_array_pop_char_v2";
            break;
        case TYPE_STRING:
            pop_func = "rt_array_pop_string_v2";
            break;
        case TYPE_BOOL:
            pop_func = "rt_array_pop_bool_v2";
            break;
        case TYPE_BYTE:
            pop_func = "rt_array_pop_byte_v2";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            pop_func = "rt_array_pop_ptr_v2";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for pop\n");
            exit(1);
    }

    /* pop_v2 takes just the handle, returns the popped value */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *elem_type_str = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "(%s)%s(%s)",
                             elem_type_str, pop_func, handle_str);
    }
    return arena_sprintf(gen->arena, "%s(%s)", pop_func, handle_str);
}

/* Generate code for array.concat(other_array) method */
static char *code_gen_array_concat(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *arg, bool caller_wants_handle)
{
    const char *concat_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            concat_func = "rt_array_concat_long_v2";
            break;
        case TYPE_INT32:
            concat_func = "rt_array_concat_int32_v2";
            break;
        case TYPE_UINT:
            concat_func = "rt_array_concat_uint_v2";
            break;
        case TYPE_UINT32:
            concat_func = "rt_array_concat_uint32_v2";
            break;
        case TYPE_FLOAT:
            concat_func = "rt_array_concat_float_v2";
            break;
        case TYPE_DOUBLE:
            concat_func = "rt_array_concat_double_v2";
            break;
        case TYPE_CHAR:
            concat_func = "rt_array_concat_char_v2";
            break;
        case TYPE_STRING:
            concat_func = "rt_array_concat_string_v2";
            break;
        case TYPE_BOOL:
            concat_func = "rt_array_concat_bool_v2";
            break;
        case TYPE_BYTE:
            concat_func = "rt_array_concat_byte_v2";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            concat_func = "rt_array_concat_ptr_v2";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for concat\n");
            exit(1);
    }

    /* In V2, evaluate arrays in handle mode and use rt_array_data_v2 to get data pointers.
     * concat_v2 takes two raw data pointers and returns a new RtHandleV2*. */
    char *call_expr;
    if (gen->current_arena_var != NULL)
    {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *object_h = code_gen_expression(gen, object);
        char *arg_h = code_gen_expression(gen, arg);
        gen->expr_as_handle = saved;

        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        call_expr = arena_sprintf(gen->arena, "%s(%s, (%s *)rt_array_data_v2(%s), (%s *)rt_array_data_v2(%s))",
                                  concat_func, ARENA_VAR(gen), elem_c, object_h, elem_c, arg_h);

        if (!caller_wants_handle)
        {
            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                 elem_c, call_expr);
        }
        return call_expr;
    }

    /* Non-arena fallback (legacy) */
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         concat_func, ARENA_VAR(gen), object_str, arg_str);
}

/* Generate code for array.indexOf(element) method */
static char *code_gen_array_indexof(CodeGen *gen, Expr *object, Type *element_type,
                                     Expr *arg)
{
    char *arg_str = code_gen_expression(gen, arg);

    const char *indexof_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            indexof_func = "rt_array_indexOf_long";
            break;
        case TYPE_INT32:
            indexof_func = "rt_array_indexOf_int32";
            break;
        case TYPE_UINT:
            indexof_func = "rt_array_indexOf_uint";
            break;
        case TYPE_UINT32:
            indexof_func = "rt_array_indexOf_uint32";
            break;
        case TYPE_FLOAT:
            indexof_func = "rt_array_indexOf_float";
            break;
        case TYPE_DOUBLE:
            indexof_func = "rt_array_indexOf_double";
            break;
        case TYPE_CHAR:
            indexof_func = "rt_array_indexOf_char";
            break;
        case TYPE_STRING:
            indexof_func = "rt_array_indexOf_string";
            /* String arrays in V2 use handle-based search function */
            if (gen->current_arena_var) {
                bool saved = gen->expr_as_handle;
                gen->expr_as_handle = true;
                char *handle_str = code_gen_expression(gen, object);
                gen->expr_as_handle = saved;
                return arena_sprintf(gen->arena, "rt_array_indexOf_string_v2((RtHandleV2 **)rt_array_data_v2(%s), %s)",
                                     handle_str, arg_str);
            }
            break;
        case TYPE_BOOL:
            indexof_func = "rt_array_indexOf_bool";
            break;
        case TYPE_BYTE:
            indexof_func = "rt_array_indexOf_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for indexOf\n");
            exit(1);
    }

    /* In V2 mode, get data pointer from handle for non-string types */
    if (gen->current_arena_var != NULL) {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = saved;
        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "%s((%s *)rt_array_data_v2(%s), %s)",
                             indexof_func, elem_c, handle_str, arg_str);
    }

    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s)", indexof_func, object_str, arg_str);
}

/* Generate code for array.contains(element) method */
static char *code_gen_array_contains(CodeGen *gen, Expr *object, Type *element_type,
                                      Expr *arg)
{
    char *arg_str = code_gen_expression(gen, arg);

    const char *contains_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            contains_func = "rt_array_contains_long";
            break;
        case TYPE_INT32:
            contains_func = "rt_array_contains_int32";
            break;
        case TYPE_UINT:
            contains_func = "rt_array_contains_uint";
            break;
        case TYPE_UINT32:
            contains_func = "rt_array_contains_uint32";
            break;
        case TYPE_FLOAT:
            contains_func = "rt_array_contains_float";
            break;
        case TYPE_DOUBLE:
            contains_func = "rt_array_contains_double";
            break;
        case TYPE_CHAR:
            contains_func = "rt_array_contains_char";
            break;
        case TYPE_STRING:
            contains_func = "rt_array_contains_string";
            /* String arrays in V2 use handle-based search function */
            if (gen->current_arena_var) {
                bool saved = gen->expr_as_handle;
                gen->expr_as_handle = true;
                char *handle_str = code_gen_expression(gen, object);
                gen->expr_as_handle = saved;
                return arena_sprintf(gen->arena, "rt_array_contains_string_v2((RtHandleV2 **)rt_array_data_v2(%s), %s)",
                                     handle_str, arg_str);
            }
            break;
        case TYPE_BOOL:
            contains_func = "rt_array_contains_bool";
            break;
        case TYPE_BYTE:
            contains_func = "rt_array_contains_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for contains\n");
            exit(1);
    }

    /* In V2 mode, get data pointer from handle for non-string types */
    if (gen->current_arena_var != NULL) {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = saved;
        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "%s((%s *)rt_array_data_v2(%s), %s)",
                             contains_func, elem_c, handle_str, arg_str);
    }

    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s)", contains_func, object_str, arg_str);
}

/* Generate code for array.clone() method */
static char *code_gen_array_clone(CodeGen *gen, Expr *object, Type *element_type, bool handle_mode)
{
    const char *suffix = code_gen_type_suffix(element_type);
    if (suffix == NULL) {
        fprintf(stderr, "Error: Unsupported array element type for clone\n");
        exit(1);
    }

    /* In V2 mode, clone takes raw data pointer. Get handle and wrap with rt_array_data_v2. */
    if (handle_mode && gen->current_arena_var != NULL) {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = saved;

        /* For string arrays, use special handle-based clone since V2 string arrays
         * contain RtHandleV2* elements, not char* elements */
        if (element_type->kind == TYPE_STRING) {
            return arena_sprintf(gen->arena, "rt_array_clone_string_handle_v2(%s, %s)",
                                 ARENA_VAR(gen), handle_str);
        }

        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "rt_array_clone_%s_v2(%s, (%s *)rt_array_data_v2(%s))",
                             suffix, ARENA_VAR(gen), elem_c, handle_str);
    }

    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), object_str);
}

