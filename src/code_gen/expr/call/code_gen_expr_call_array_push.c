/**
 * code_gen_expr_call_array_push.c - Code generation for array push method
 */

/* Helper to get the arena to use for array mutations.
 * Mutations (push/pop/insert/remove/reverse) must allocate in the arena that
 * owns the array handle. For globals, that's __main_arena__; for locals/params,
 * it's the function's arena. */
static const char *get_arena_for_mutation(CodeGen *gen, Expr *object)
{
    if (object->type == EXPR_VARIABLE) {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, object->as.variable.name);
        if (sym && is_handle_type(sym->type)) {
            if (sym->kind == SYMBOL_GLOBAL) {
                /* Global variables must be mutated using __main_arena__ so that
                 * reallocated handles persist across function calls. */
                return "__main_arena__";
            }
        }
    }
    return ARENA_VAR(gen);
}

/* Generate code for array.push(element) method */
static char *code_gen_array_push(CodeGen *gen, Expr *object, Type *element_type,
                                  Expr *arg)
{
    char *handle_str = NULL;
    char *lvalue_str = NULL;

    /* For global handle-type variables in a local arena context, we must NOT
     * use rt_managed_clone because:
     * 1. Clone creates a handle in the local arena
     * 2. But push_h expects a handle in the mutation arena (main arena for globals)
     * 3. The returned handle must be assigned back to the global variable
     * So we use the raw global variable directly for both reading and writing. */
    if (object->type == EXPR_VARIABLE && gen->current_arena_var != NULL) {
        Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, object->as.variable.name);
        if (sym && sym->kind == SYMBOL_GLOBAL && is_handle_type(sym->type)) {
            char *var_name = get_var_name(gen->arena, object->as.variable.name);
            char *mangled = sn_mangle_name(gen->arena, var_name);
            handle_str = mangled;
            lvalue_str = mangled;
        }
    }

    /* For non-global variables, evaluate in handle mode as before */
    if (handle_str == NULL) {
        bool prev_as_handle = gen->expr_as_handle;
        gen->expr_as_handle = true;
        handle_str = code_gen_expression(gen, object);
        gen->expr_as_handle = prev_as_handle;
        lvalue_str = handle_str;
    }

    /* For nested arrays in handle mode, generate arg in handle mode to get RtHandle */
    bool prev_arg_as_handle = gen->expr_as_handle;
    if (element_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL) {
        gen->expr_as_handle = true;
    }
    char *arg_str = code_gen_expression(gen, arg);
    gen->expr_as_handle = prev_arg_as_handle;

    const char *arena_to_use = get_arena_for_mutation(gen, object);

    const char *push_func = NULL;
    switch (element_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            push_func = "rt_array_push_long_h";
            break;
        case TYPE_INT32:
            push_func = "rt_array_push_int32_h";
            break;
        case TYPE_UINT:
            push_func = "rt_array_push_uint_h";
            break;
        case TYPE_UINT32:
            push_func = "rt_array_push_uint32_h";
            break;
        case TYPE_FLOAT:
            push_func = "rt_array_push_float_h";
            break;
        case TYPE_DOUBLE:
            push_func = "rt_array_push_double_h";
            break;
        case TYPE_CHAR:
            push_func = "rt_array_push_char_h";
            break;
        case TYPE_STRING:
            push_func = "rt_array_push_string_h";
            break;
        case TYPE_BOOL:
            push_func = "rt_array_push_bool_h";
            break;
        case TYPE_BYTE:
            push_func = "rt_array_push_byte_h";
            break;
        case TYPE_FUNCTION:
            push_func = "rt_array_push_voidptr_h";
            break;
        case TYPE_ARRAY:
            push_func = "rt_array_push_ptr_h";
            break;
        case TYPE_ANY:
            push_func = "rt_array_push_any_h";
            break;
        case TYPE_STRUCT:
            /* Struct types use a generic push with element size parameter.
             * The element is passed by pointer (address-of). */
            push_func = "rt_array_push_struct_h";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for push\n");
            exit(1);
    }

    /* push_h takes the RtHandle and returns the new handle.
     * Assign back to the lvalue so the handle stays valid after reallocation. */
    bool is_lvalue = (object->type == EXPR_VARIABLE ||
                      object->type == EXPR_MEMBER_ACCESS ||
                      object->type == EXPR_MEMBER);

    /* For struct types, use the struct push with element size.
     * The struct is passed by pointer (address-of). */
    if (element_type->kind == TYPE_STRUCT) {
        /* Get C type name for sizeof() */
        const char *c_type = get_c_type(gen->arena, element_type);
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = %s(%s, %s, &%s, sizeof(%s)))",
                                 lvalue_str, push_func, arena_to_use, handle_str, arg_str, c_type);
        }
        return arena_sprintf(gen->arena, "%s(%s, %s, &%s, sizeof(%s))",
                             push_func, arena_to_use, handle_str, arg_str, c_type);
    }

    /* For pointer types (function/array), cast element to void*.
     * For nested arrays in handle mode, arg_str is RtHandle (uint32_t) so use uintptr_t. */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *cast = (element_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
            ? "(void *)(uintptr_t)" : "(void *)";
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s%s))",
                                 lvalue_str, push_func, arena_to_use, handle_str, cast, arg_str);
        }
        return arena_sprintf(gen->arena, "%s(%s, %s, %s%s)",
                             push_func, arena_to_use, handle_str, cast, arg_str);
    }

    if (is_lvalue) {
        return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))",
                             lvalue_str, push_func, arena_to_use, handle_str, arg_str);
    }
    return arena_sprintf(gen->arena, "%s(%s, %s, %s)",
                         push_func, arena_to_use, handle_str, arg_str);
}

