/**
 * code_gen_expr_call_array_mutate.c - Code generation for array in-place mutation methods
 *
 * V2 Only: All functions use handle-based V2 API.
 */

/* Generate code for array.join(separator) method - returns RtHandleV2* */
static char *code_gen_array_join(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *separator)
{
    /* Join separator is now RtHandleV2* - evaluate in handle mode */
    bool prev_sep = gen->expr_as_handle;
    if (gen->current_arena_var != NULL) {
        gen->expr_as_handle = true;
    }
    char *sep_str = code_gen_expression(gen, separator);
    gen->expr_as_handle = prev_sep;

    const char *join_func_v2 = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            join_func_v2 = "rt_array_join_long_v2";
            break;
        case TYPE_INT32:
            join_func_v2 = "rt_array_join_int32_v2";
            break;
        case TYPE_UINT:
            join_func_v2 = "rt_array_join_uint_v2";
            break;
        case TYPE_UINT32:
            join_func_v2 = "rt_array_join_uint32_v2";
            break;
        case TYPE_FLOAT:
            join_func_v2 = "rt_array_join_float_v2";
            break;
        case TYPE_DOUBLE:
            join_func_v2 = "rt_array_join_double_v2";
            break;
        case TYPE_CHAR:
            join_func_v2 = "rt_array_join_char_v2";
            break;
        case TYPE_STRING:
            join_func_v2 = "rt_array_join_string_v2";
            break;
        case TYPE_BOOL:
            join_func_v2 = "rt_array_join_bool_v2";
            break;
        case TYPE_BYTE:
            join_func_v2 = "rt_array_join_byte_v2";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for join\n");
            exit(1);
    }

    bool saved = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = saved;

    return arena_sprintf(gen->arena, "%s(%s, %s)", join_func_v2, handle_str, sep_str);
}

/* Generate code for array.reverse() method - in-place reverse */
static char *code_gen_array_reverse(CodeGen *gen, Expr *object, Type *element_type)
{
    bool saved = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = saved;

    /* String arrays use specialized function */
    if (element_type->kind == TYPE_STRING) {
        /* If variable, assign result back */
        if (object->type == EXPR_VARIABLE) {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = rt_array_rev_string_v2(%s))",
                                 var_name, handle_str);
        }
        return arena_sprintf(gen->arena, "rt_array_rev_string_v2(%s)", handle_str);
    }

    /* All other types use generic reverse with sizeof */
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    if (object->type == EXPR_VARIABLE) {
        char *var_name = sn_mangle_name(gen->arena,
            get_var_name(gen->arena, object->as.variable.name));
        return arena_sprintf(gen->arena, "(%s = rt_array_rev_v2(%s, %s))",
                             var_name, handle_str, sizeof_expr);
    }
    return arena_sprintf(gen->arena, "rt_array_rev_v2(%s, %s)", handle_str, sizeof_expr);
}

/* Generate code for array.insert(element, index) method - in-place insert */
static char *code_gen_array_insert(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *element, Expr *index)
{
    /* String arrays: evaluate element in handle mode for RtHandleV2* parameter */
    bool prev_elem = gen->expr_as_handle;
    if (element_type->kind == TYPE_STRING && gen->current_arena_var != NULL) {
        gen->expr_as_handle = true;
    }
    char *elem_str = code_gen_expression(gen, element);
    gen->expr_as_handle = prev_elem;
    char *idx_str = code_gen_expression(gen, index);

    bool saved = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = saved;

    char *var_name = NULL;
    if (object->type == EXPR_VARIABLE) {
        var_name = sn_mangle_name(gen->arena,
            get_var_name(gen->arena, object->as.variable.name));
    }

    /* String arrays use specialized function */
    if (element_type->kind == TYPE_STRING) {
        if (var_name) {
            return arena_sprintf(gen->arena, "(%s = rt_array_ins_string_v2(%s, %s, %s))",
                                 var_name, handle_str, elem_str, idx_str);
        }
        return arena_sprintf(gen->arena, "rt_array_ins_string_v2(%s, %s, %s)",
                             handle_str, elem_str, idx_str);
    }

    /* Struct types: elem_str is already a compound literal */
    if (element_type->kind == TYPE_STRUCT) {
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
        if (var_name) {
            return arena_sprintf(gen->arena, "(%s = rt_array_ins_v2(%s, &(%s), %s, %s))",
                                 var_name, handle_str, elem_str, idx_str, sizeof_expr);
        }
        return arena_sprintf(gen->arena, "rt_array_ins_v2(%s, &(%s), %s, %s)",
                             handle_str, elem_str, idx_str, sizeof_expr);
    }

    /* Primitive types: wrap in compound literal to get address */
    const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    if (var_name) {
        return arena_sprintf(gen->arena, "(%s = rt_array_ins_v2(%s, &(%s){%s}, %s, %s))",
                             var_name, handle_str, elem_c, elem_str, idx_str, sizeof_expr);
    }
    return arena_sprintf(gen->arena, "rt_array_ins_v2(%s, &(%s){%s}, %s, %s)",
                         handle_str, elem_c, elem_str, idx_str, sizeof_expr);
}

/* Generate code for array.remove(index) method - in-place remove */
static char *code_gen_array_remove(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *index)
{
    char *idx_str = code_gen_expression(gen, index);

    bool saved = gen->expr_as_handle;
    gen->expr_as_handle = true;
    char *handle_str = code_gen_expression(gen, object);
    gen->expr_as_handle = saved;

    char *var_name = NULL;
    if (object->type == EXPR_VARIABLE) {
        var_name = sn_mangle_name(gen->arena,
            get_var_name(gen->arena, object->as.variable.name));
    }

    /* String arrays use specialized function */
    if (element_type->kind == TYPE_STRING) {
        if (var_name) {
            return arena_sprintf(gen->arena, "(%s = rt_array_rem_string_v2(%s, %s))",
                                 var_name, handle_str, idx_str);
        }
        return arena_sprintf(gen->arena, "rt_array_rem_string_v2(%s, %s)",
                             handle_str, idx_str);
    }

    /* All other types use generic remove with sizeof */
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    if (var_name) {
        return arena_sprintf(gen->arena, "(%s = rt_array_rem_v2(%s, %s, %s))",
                             var_name, handle_str, idx_str, sizeof_expr);
    }
    return arena_sprintf(gen->arena, "rt_array_rem_v2(%s, %s, %s)",
                         handle_str, idx_str, sizeof_expr);
}

