/**
 * code_gen_expr_call_array_query.c - Code generation for array query/copy methods
 *
 * V2 Only: All functions use handle-based V2 API.
 */

/* Generate code for array.clear() method */
static char *code_gen_array_clear(CodeGen *gen, Expr *object)
{
    char *handle_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_array_clear_v2(%s)", handle_str);
}

/* Generate code for array.pop() method */
static char *code_gen_array_pop(CodeGen *gen, Expr *object, Type *element_type)
{
    /* Evaluate object in handle mode to get the RtHandle */
    char *handle_str = code_gen_expression(gen, object);

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
        case TYPE_STRUCT:
        {
            /* Struct pop uses generic rt_array_pop_v2 with a temp buffer */
            const char *struct_c = get_c_type(gen->arena, element_type);
            return arena_sprintf(gen->arena,
                "({ %s __pop_tmp__; rt_array_pop_v2(%s, &__pop_tmp__, sizeof(%s)); __pop_tmp__; })",
                struct_c, handle_str, struct_c);
        }
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
                                    Expr *arg)
{
    char *object_h = code_gen_expression(gen, object);
    char *arg_h = code_gen_expression(gen, arg);

    /* String arrays need special concat (strdup each element) */
    if (element_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_concat_string_v2(%s, %s)",
                             object_h, arg_h);
    }
    /* All other types use generic concat with sizeof */
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    return arena_sprintf(gen->arena, "rt_array_concat_v2(%s, %s, %s)",
                         object_h, arg_h, sizeof_expr);
}

/* Generate code for array.indexOf(element) method */
static char *code_gen_array_indexof(CodeGen *gen, Expr *object, Type *element_type,
                                     Expr *arg)
{
    char *arg_str = code_gen_expression(gen, arg);

    char *handle_str = code_gen_expression(gen, object);

    /* String arrays use specialized function (strcmp comparison) */
    if (element_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_indexOf_string_v2(%s, %s)",
                             handle_str, arg_str);
    }

    /* Struct types: arg_str is already a compound literal */
    if (element_type->kind == TYPE_STRUCT) {
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
        return arena_sprintf(gen->arena, "rt_array_indexOf_v2(%s, &(%s), %s)",
                             handle_str, arg_str, sizeof_expr);
    }

    /* Primitive types: wrap in compound literal to get address */
    const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    return arena_sprintf(gen->arena, "rt_array_indexOf_v2(%s, &(%s){%s}, %s)",
                         handle_str, elem_c, arg_str, sizeof_expr);
}

/* Generate code for array.contains(element) method */
static char *code_gen_array_contains(CodeGen *gen, Expr *object, Type *element_type,
                                      Expr *arg)
{
    char *arg_str = code_gen_expression(gen, arg);

    char *handle_str = code_gen_expression(gen, object);

    /* String arrays use specialized function (strcmp comparison) */
    if (element_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_contains_string_v2(%s, %s)",
                             handle_str, arg_str);
    }

    /* Struct types: arg_str is already a compound literal */
    if (element_type->kind == TYPE_STRUCT) {
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
        return arena_sprintf(gen->arena, "rt_array_contains_v2(%s, &(%s), %s)",
                             handle_str, arg_str, sizeof_expr);
    }

    /* Primitive types: wrap in compound literal to get address */
    const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    return arena_sprintf(gen->arena, "rt_array_contains_v2(%s, &(%s){%s}, %s)",
                         handle_str, elem_c, arg_str, sizeof_expr);
}

/* Generate code for array.clone() method */
static char *code_gen_array_clone(CodeGen *gen, Expr *object, Type *element_type)
{
    char *handle_str = code_gen_expression(gen, object);

    /* String arrays need special clone (strdup each element) */
    if (element_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_clone_string_v2(%s)", handle_str);
    }
    /* All other types use generic clone with sizeof */
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
    return arena_sprintf(gen->arena, "rt_array_clone_v2(%s, %s)", handle_str, sizeof_expr);
}

