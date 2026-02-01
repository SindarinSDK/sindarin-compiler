/**
 * code_gen_expr_call_array_mutate.c - Code generation for array in-place mutation methods
 */

/* Generate code for array.join(separator) method */
static char *code_gen_array_join(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *separator)
{
    char *object_str = code_gen_expression(gen, object);
    char *sep_str = code_gen_expression(gen, separator);

    const char *join_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            join_func = "rt_array_join_long";
            break;
        case TYPE_INT32:
            join_func = "rt_array_join_int32";
            break;
        case TYPE_UINT:
            join_func = "rt_array_join_uint";
            break;
        case TYPE_UINT32:
            join_func = "rt_array_join_uint32";
            break;
        case TYPE_FLOAT:
            join_func = "rt_array_join_float";
            break;
        case TYPE_DOUBLE:
            join_func = "rt_array_join_double";
            break;
        case TYPE_CHAR:
            join_func = "rt_array_join_char";
            break;
        case TYPE_STRING:
            join_func = gen->current_arena_var ? "rt_array_join_string_h" : "rt_array_join_string";
            break;
        case TYPE_BOOL:
            join_func = "rt_array_join_bool";
            break;
        case TYPE_BYTE:
            join_func = "rt_array_join_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for join\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s, %s)", join_func, ARENA_VAR(gen), object_str, sep_str);
}

/* Generate code for array.reverse() method - in-place reverse */
static char *code_gen_array_reverse(CodeGen *gen, Expr *object, Type *element_type)
{
    char *object_str = code_gen_expression(gen, object);

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
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s))",
                                 var_name, rev_func, ARENA_VAR(gen), object_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, rev_func, ARENA_VAR(gen), object_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s)", rev_func, ARENA_VAR(gen), object_str);
}

/* Generate code for array.insert(element, index) method - in-place insert */
static char *code_gen_array_insert(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *element, Expr *index)
{
    char *object_str = code_gen_expression(gen, object);
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
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s, %s, %s))",
                                 var_name, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s, %s))",
                             object_str, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)",
                         ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
}

/* Generate code for array.remove(index) method - in-place remove */
static char *code_gen_array_remove(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *index)
{
    char *object_str = code_gen_expression(gen, object);
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
        /* In handle mode, use _h variant and assign to the handle variable */
        if (gen->current_arena_var != NULL && object->expr_type != NULL &&
            object->expr_type->kind == TYPE_ARRAY)
        {
            char *var_name = sn_mangle_name(gen->arena,
                get_var_name(gen->arena, object->as.variable.name));
            return arena_sprintf(gen->arena, "(%s = %s_h(%s, %s, %s))",
                                 var_name, rem_func, ARENA_VAR(gen), object_str, idx_str);
        }
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             object_str, rem_func, ARENA_VAR(gen), object_str, idx_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         rem_func, ARENA_VAR(gen), object_str, idx_str);
}

