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

    /* push_h takes the RtHandle and returns the new handle.
     * Assign back to the lvalue so the handle stays valid after reallocation. */
    bool is_lvalue = (object->type == EXPR_VARIABLE ||
                      object->type == EXPR_MEMBER_ACCESS ||
                      object->type == EXPR_MEMBER);

    /* String arrays use specialized push (strdup) */
    if (element_type->kind == TYPE_STRING) {
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = rt_array_push_string_v2(%s, %s, %s))",
                                 lvalue_str, arena_to_use, handle_str, arg_str);
        }
        return arena_sprintf(gen->arena, "rt_array_push_string_v2(%s, %s, %s)",
                             arena_to_use, handle_str, arg_str);
    }

    /* Any type uses specialized push (boxing) */
    if (element_type->kind == TYPE_ANY) {
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = rt_array_push_any_v2(%s, %s, %s))",
                                 lvalue_str, arena_to_use, handle_str, arg_str);
        }
        return arena_sprintf(gen->arena, "rt_array_push_any_v2(%s, %s, %s)",
                             arena_to_use, handle_str, arg_str);
    }

    /* Pointer types (function/array) need casting */
    if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
        const char *cast = (element_type->kind == TYPE_ARRAY && gen->current_arena_var != NULL)
            ? "(void *)(uintptr_t)" : "(void *)";
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = rt_array_push_v2(%s, %s, &(void *){%s%s}, %s))",
                                 lvalue_str, arena_to_use, handle_str, cast, arg_str, sizeof_expr);
        }
        return arena_sprintf(gen->arena, "rt_array_push_v2(%s, %s, &(void *){%s%s}, %s)",
                             arena_to_use, handle_str, cast, arg_str, sizeof_expr);
    }

    /* Struct types: arg_str is already a compound literal, just take its address */
    if (element_type->kind == TYPE_STRUCT) {
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);
        if (is_lvalue) {
            return arena_sprintf(gen->arena, "(%s = rt_array_push_v2(%s, %s, &(%s), %s))",
                                 lvalue_str, arena_to_use, handle_str, arg_str, sizeof_expr);
        }
        return arena_sprintf(gen->arena, "rt_array_push_v2(%s, %s, &(%s), %s)",
                             arena_to_use, handle_str, arg_str, sizeof_expr);
    }

    /* Primitive types: wrap in compound literal to get address */
    const char *elem_c = get_c_array_elem_type(gen->arena, element_type);
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, element_type);

    if (is_lvalue) {
        return arena_sprintf(gen->arena, "(%s = rt_array_push_v2(%s, %s, &(%s){%s}, %s))",
                             lvalue_str, arena_to_use, handle_str, elem_c, arg_str, sizeof_expr);
    }
    return arena_sprintf(gen->arena, "rt_array_push_v2(%s, %s, &(%s){%s}, %s)",
                         arena_to_use, handle_str, elem_c, arg_str, sizeof_expr);
}

