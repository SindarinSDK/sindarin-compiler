/**
 * code_gen_expr_call_array_mutate.c - Code generation for array in-place mutation methods
 */

/* Generate code for array.join(separator) method */
static char *code_gen_array_join(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *separator)
{
    char *sep_str = code_gen_expression(gen, separator);

    const char *join_func = NULL;
    const char *join_func_v2 = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            join_func = "rt_array_join_long";
            join_func_v2 = "rt_array_join_long_v2";
            break;
        case TYPE_INT32:
            join_func = "rt_array_join_int32";
            join_func_v2 = "rt_array_join_int32_v2";
            break;
        case TYPE_UINT:
            join_func = "rt_array_join_uint";
            join_func_v2 = "rt_array_join_uint_v2";
            break;
        case TYPE_UINT32:
            join_func = "rt_array_join_uint32";
            join_func_v2 = "rt_array_join_uint32_v2";
            break;
        case TYPE_FLOAT:
            join_func = "rt_array_join_float";
            join_func_v2 = "rt_array_join_float_v2";
            break;
        case TYPE_DOUBLE:
            join_func = "rt_array_join_double";
            join_func_v2 = "rt_array_join_double_v2";
            break;
        case TYPE_CHAR:
            join_func = "rt_array_join_char";
            join_func_v2 = "rt_array_join_char_v2";
            break;
        case TYPE_STRING:
            join_func = "rt_array_join_string";
            join_func_v2 = "rt_array_join_string_v2";
            break;
        case TYPE_BOOL:
            join_func = "rt_array_join_bool";
            join_func_v2 = "rt_array_join_bool_v2";
            break;
        case TYPE_BYTE:
            join_func = "rt_array_join_byte";
            join_func_v2 = "rt_array_join_byte_v2";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for join\n");
            exit(1);
    }

    /* In V2 mode, use V2 join functions with data pointer from rt_array_data_v2() */
    if (gen->current_arena_var != NULL) {
        bool saved = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = saved;

        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        /* String arrays pass handle array directly, others pass data pointer */
        if (element_type->kind == TYPE_STRING) {
            return arena_sprintf(gen->arena, "%s(%s, (RtHandleV2 **)rt_array_data_v2(%s), %s)",
                                 join_func_v2, ARENA_VAR(gen), handle_str, sep_str);
        }
        return arena_sprintf(gen->arena, "%s(%s, (%s *)rt_array_data_v2(%s), %s)",
                             join_func_v2, ARENA_VAR(gen), elem_c, handle_str, sep_str);
    }

    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)", join_func, ARENA_VAR(gen), object_str, sep_str);
}

/* Generate code for array.reverse() method - in-place reverse */
static char *code_gen_array_reverse(CodeGen *gen, Expr *object, Type *element_type)
{
    const char *rev_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            rev_func = "rt_array_rev_long";
            break;
        case TYPE_INT32:
            rev_func = "rt_array_rev_int32";
            break;
        case TYPE_UINT:
            rev_func = "rt_array_rev_uint";
            break;
        case TYPE_UINT32:
            rev_func = "rt_array_rev_uint32";
            break;
        case TYPE_FLOAT:
            rev_func = "rt_array_rev_float";
            break;
        case TYPE_DOUBLE:
            rev_func = "rt_array_rev_double";
            break;
        case TYPE_CHAR:
            rev_func = "rt_array_rev_char";
            break;
        case TYPE_STRING:
            rev_func = "rt_array_rev_string";
            break;
        case TYPE_BOOL:
            rev_func = "rt_array_rev_bool";
            break;
        case TYPE_BYTE:
            rev_func = "rt_array_rev_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for reverse\n");
            exit(1);
    }

    /* reverse in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In V2 handle mode, use _v2 variant and assign to the handle variable.
         * V2 functions take raw data pointer, so use rt_array_data_v2() to convert. */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            /* Get handle for assignment and data pointer for function arg */
            bool saved = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_str = code_gen_expression(gen, object);
            gen->expr_as_handle = saved;

            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
            /* For string arrays, use handle-aware version since V2 arrays store RtHandleV2* elements */
            if (element_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(%s = rt_array_rev_string_handle_v2(%s, (%s *)rt_array_data_v2(%s)))",
                                     var_name, ARENA_VAR(gen), elem_c, handle_str);
            }
            return arena_sprintf(gen->arena, "(%s = %s_v2(%s, (%s *)rt_array_data_v2(%s)))",
                                 var_name, rev_func, ARENA_VAR(gen), elem_c, handle_str);
        }
        char *object_str = code_gen_expression(gen, object);
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, rev_func, ARENA_VAR(gen), object_str);
    }
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s)", rev_func, ARENA_VAR(gen), object_str);
}

/* Generate code for array.insert(element, index) method - in-place insert */
static char *code_gen_array_insert(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *element, Expr *index)
{
    char *elem_str = code_gen_expression(gen, element);
    char *idx_str = code_gen_expression(gen, index);

    const char *ins_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            ins_func = "rt_array_ins_long";
            break;
        case TYPE_INT32:
            ins_func = "rt_array_ins_int32";
            break;
        case TYPE_UINT:
            ins_func = "rt_array_ins_uint";
            break;
        case TYPE_UINT32:
            ins_func = "rt_array_ins_uint32";
            break;
        case TYPE_FLOAT:
            ins_func = "rt_array_ins_float";
            break;
        case TYPE_DOUBLE:
            ins_func = "rt_array_ins_double";
            break;
        case TYPE_CHAR:
            ins_func = "rt_array_ins_char";
            break;
        case TYPE_STRING:
            ins_func = "rt_array_ins_string";
            break;
        case TYPE_BOOL:
            ins_func = "rt_array_ins_bool";
            break;
        case TYPE_BYTE:
            ins_func = "rt_array_ins_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for insert\n");
            exit(1);
    }

    /* insert in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In V2 handle mode, use _v2 variant and assign to the handle variable.
         * V2 functions take raw data pointer, so use rt_array_data_v2() to convert. */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            /* Get handle for assignment and data pointer for function arg */
            bool saved = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_str = code_gen_expression(gen, object);
            gen->expr_as_handle = saved;

            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
            /* For string arrays, use handle-aware version since V2 arrays store RtHandleV2* elements */
            if (element_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(%s = rt_array_ins_string_handle_v2(%s, (%s *)rt_array_data_v2(%s), %s, %s))",
                                     var_name, ARENA_VAR(gen), elem_c, handle_str, elem_str, idx_str);
            }
            return arena_sprintf(gen->arena, "(%s = %s_v2(%s, (%s *)rt_array_data_v2(%s), %s, %s))",
                                 var_name, ins_func, ARENA_VAR(gen), elem_c, handle_str, elem_str, idx_str);
        }
        char *object_str = code_gen_expression(gen, object);
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s, %s))",
                             object_str, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
    }
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)",
                         ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
}

/* Generate code for array.remove(index) method - in-place remove */
static char *code_gen_array_remove(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *index)
{
    char *idx_str = code_gen_expression(gen, index);

    const char *rem_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            rem_func = "rt_array_rem_long";
            break;
        case TYPE_INT32:
            rem_func = "rt_array_rem_int32";
            break;
        case TYPE_UINT:
            rem_func = "rt_array_rem_uint";
            break;
        case TYPE_UINT32:
            rem_func = "rt_array_rem_uint32";
            break;
        case TYPE_FLOAT:
            rem_func = "rt_array_rem_float";
            break;
        case TYPE_DOUBLE:
            rem_func = "rt_array_rem_double";
            break;
        case TYPE_CHAR:
            rem_func = "rt_array_rem_char";
            break;
        case TYPE_STRING:
            rem_func = "rt_array_rem_string";
            break;
        case TYPE_BOOL:
            rem_func = "rt_array_rem_bool";
            break;
        case TYPE_BYTE:
            rem_func = "rt_array_rem_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for remove\n");
            exit(1);
    }

    /* remove in-place: assign result back to variable */
    if (object->type == EXPR_VARIABLE) {
        /* In V2 handle mode, use _v2 variant and assign to the handle variable.
         * V2 functions take raw data pointer, so use rt_array_data_v2() to convert. */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            /* Get handle for assignment and data pointer for function arg */
            bool saved = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_str = code_gen_expression(gen, object);
            gen->expr_as_handle = saved;

            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
            /* For string arrays, use handle-aware version since V2 arrays store RtHandleV2* elements */
            if (element_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(%s = rt_array_rem_string_handle_v2(%s, (%s *)rt_array_data_v2(%s), %s))",
                                     var_name, ARENA_VAR(gen), elem_c, handle_str, idx_str);
            }
            return arena_sprintf(gen->arena, "(%s = %s_v2(%s, (%s *)rt_array_data_v2(%s), %s))",
                                 var_name, rem_func, ARENA_VAR(gen), elem_c, handle_str, idx_str);
        }
        char *object_str = code_gen_expression(gen, object);
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             object_str, rem_func, ARENA_VAR(gen), object_str, idx_str);
    }
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         rem_func, ARENA_VAR(gen), object_str, idx_str);
}

