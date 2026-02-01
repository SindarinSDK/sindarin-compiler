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
            pop_func = "rt_array_pop_long_h";
            break;
        case TYPE_INT32:
            pop_func = "rt_array_pop_int32_h";
            break;
        case TYPE_UINT:
            pop_func = "rt_array_pop_uint_h";
            break;
        case TYPE_UINT32:
            pop_func = "rt_array_pop_uint32_h";
            break;
        case TYPE_FLOAT:
            pop_func = "rt_array_pop_float_h";
            break;
        case TYPE_DOUBLE:
            pop_func = "rt_array_pop_double_h";
            break;
        case TYPE_CHAR:
            pop_func = "rt_array_pop_char_h";
            break;
        case TYPE_STRING:
            pop_func = "rt_array_pop_string_h";
            break;
        case TYPE_BOOL:
            pop_func = "rt_array_pop_bool_h";
            break;
        case TYPE_BYTE:
            pop_func = "rt_array_pop_byte_h";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            pop_func = "rt_array_pop_ptr_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for pop\n");
            exit(1);
    }

    /* pop_h takes the RtHandle and the arena, returns the popped value */
    const char *arena_to_use = get_arena_for_mutation(gen, object);
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *elem_type_str = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "(%s)%s(%s, %s)",
                             elem_type_str, pop_func, arena_to_use, handle_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s)", pop_func, arena_to_use, handle_str);
}

/* Generate code for array.concat(other_array) method */
static char *code_gen_array_concat(CodeGen *gen, Expr *object, Type *element_type,
                                    Expr *arg, bool caller_wants_handle)
{
    /* Evaluate both arrays in raw pointer mode (pinned) for concat data */
    char *object_str = code_gen_expression(gen, object);
    char *arg_str = code_gen_expression(gen, arg);

    const char *concat_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            concat_func = "rt_array_concat_long_h";
            break;
        case TYPE_INT32:
            concat_func = "rt_array_concat_int32_h";
            break;
        case TYPE_UINT:
            concat_func = "rt_array_concat_uint_h";
            break;
        case TYPE_UINT32:
            concat_func = "rt_array_concat_uint32_h";
            break;
        case TYPE_FLOAT:
            concat_func = "rt_array_concat_float_h";
            break;
        case TYPE_DOUBLE:
            concat_func = "rt_array_concat_double_h";
            break;
        case TYPE_CHAR:
            concat_func = "rt_array_concat_char_h";
            break;
        case TYPE_STRING:
            concat_func = "rt_array_concat_string_h";
            break;
        case TYPE_BOOL:
            concat_func = "rt_array_concat_bool_h";
            break;
        case TYPE_BYTE:
            concat_func = "rt_array_concat_byte_h";
            break;
        case TYPE_FUNCTION:
        case TYPE_ARRAY:
            concat_func = "rt_array_concat_ptr_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for concat\n");
            exit(1);
    }

    /* concat_h takes two raw pointers and returns a new RtHandle.
     * If caller wants handle, return handle directly. Otherwise pin for raw pointer. */
    char *call_expr = arena_sprintf(gen->arena, "%s(%s, RT_HANDLE_NULL, %s, %s)",
                                    concat_func, ARENA_VAR(gen), object_str, arg_str);
    if (!caller_wants_handle && gen->current_arena_var != NULL)
    {
        const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
        return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                             elem_c, ARENA_VAR(gen), call_expr);
    }
    return call_expr;
}

/* Generate code for array.indexOf(element) method */
static char *code_gen_array_indexof(CodeGen *gen, Expr *object, Type *element_type,
                                     Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
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
            if (gen->current_arena_var) {
                return arena_sprintf(gen->arena, "rt_array_indexOf_string_h(%s, %s, %s)",
                                     ARENA_VAR(gen), object_str, arg_str);
            }
            indexof_func = "rt_array_indexOf_string";
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

    return arena_sprintf(gen->arena, "%s(%s, %s)", indexof_func, object_str, arg_str);
}

/* Generate code for array.contains(element) method */
static char *code_gen_array_contains(CodeGen *gen, Expr *object, Type *element_type,
                                      Expr *arg)
{
    char *object_str = code_gen_expression(gen, object);
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
            if (gen->current_arena_var) {
                return arena_sprintf(gen->arena, "rt_array_contains_string_h(%s, %s, %s)",
                                     ARENA_VAR(gen), object_str, arg_str);
            }
            contains_func = "rt_array_contains_string";
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

    return arena_sprintf(gen->arena, "%s(%s, %s)", contains_func, object_str, arg_str);
}

/* Generate code for array.clone() method */
static char *code_gen_array_clone(CodeGen *gen, Expr *object, Type *element_type, bool handle_mode)
{
    char *object_str = code_gen_expression(gen, object);

    const char *suffix = code_gen_type_suffix(element_type);
    if (suffix == NULL) {
        fprintf(stderr, "Error: Unsupported array element type for clone\n");
        exit(1);
    }

    if (handle_mode && gen->current_arena_var != NULL) {
        return arena_sprintf(gen->arena, "rt_array_clone_%s_h(%s, RT_HANDLE_NULL, %s)",
                             suffix, ARENA_VAR(gen), object_str);
    }
    return arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), object_str);
}

