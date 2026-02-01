/**
 * code_gen_expr_call_array_byte.c - Code generation for byte array methods and dispatcher
 */

/* Generate code for byte[].toString() - UTF-8 decoding */
static char *code_gen_byte_array_to_string(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_string(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toStringLatin1() - Latin-1/ISO-8859-1 decoding */
static char *code_gen_byte_array_to_string_latin1(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_string_latin1(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toHex() - hexadecimal encoding */
static char *code_gen_byte_array_to_hex(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_hex(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for byte[].toBase64() - Base64 encoding */
static char *code_gen_byte_array_to_base64(CodeGen *gen, Expr *object)
{
    char *object_str = code_gen_expression(gen, object);
    return arena_sprintf(gen->arena, "rt_byte_array_to_base64(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Main dispatcher for array method calls */
char *code_gen_array_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                  Expr *object, Type *element_type, int arg_count,
                                  Expr **arguments)
{
    (void)expr; /* May be used for future enhancements */

    /* Most array methods need the object as a raw pointer (pinned form).
     * Force expr_as_handle=false so object evaluates to pinned pointer.
     * Methods that need the handle form (push, pop) manage their own state. */
    bool saved_handle_mode = gen->expr_as_handle;
    gen->expr_as_handle = false;

    char *result = NULL;

    /* Handle push(element) */
    if (strcmp(method_name, "push") == 0 && arg_count == 1) {
        result = code_gen_array_push(gen, object, element_type, arguments[0]);
    }
    /* Handle clear() */
    else if (strcmp(method_name, "clear") == 0 && arg_count == 0) {
        result = code_gen_array_clear(gen, object);
    }
    /* Handle pop() */
    else if (strcmp(method_name, "pop") == 0 && arg_count == 0) {
        result = code_gen_array_pop(gen, object, element_type);
    }
    /* Handle concat(other_array) */
    else if (strcmp(method_name, "concat") == 0 && arg_count == 1) {
        result = code_gen_array_concat(gen, object, element_type, arguments[0], saved_handle_mode);
    }
    /* Handle indexOf(element) */
    else if (strcmp(method_name, "indexOf") == 0 && arg_count == 1) {
        result = code_gen_array_indexof(gen, object, element_type, arguments[0]);
    }
    /* Handle contains(element) */
    else if (strcmp(method_name, "contains") == 0 && arg_count == 1) {
        result = code_gen_array_contains(gen, object, element_type, arguments[0]);
    }
    /* Handle clone() */
    else if (strcmp(method_name, "clone") == 0 && arg_count == 0) {
        result = code_gen_array_clone(gen, object, element_type, saved_handle_mode);
    }
    /* Handle join(separator) */
    else if (strcmp(method_name, "join") == 0 && arg_count == 1) {
        result = code_gen_array_join(gen, object, element_type, arguments[0]);
    }
    /* Handle reverse() */
    else if (strcmp(method_name, "reverse") == 0 && arg_count == 0) {
        result = code_gen_array_reverse(gen, object, element_type);
    }
    /* Handle insert(element, index) */
    else if (strcmp(method_name, "insert") == 0 && arg_count == 2) {
        result = code_gen_array_insert(gen, object, element_type, arguments[0], arguments[1]);
    }
    /* Handle remove(index) */
    else if (strcmp(method_name, "remove") == 0 && arg_count == 1) {
        result = code_gen_array_remove(gen, object, element_type, arguments[0]);
    }
    /* Byte array extension methods - only for byte[] */
    else if (element_type->kind == TYPE_BYTE) {
        if (strcmp(method_name, "toString") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_string(gen, object);
        }
        else if (strcmp(method_name, "toStringLatin1") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_string_latin1(gen, object);
        }
        else if (strcmp(method_name, "toHex") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_hex(gen, object);
        }
        else if (strcmp(method_name, "toBase64") == 0 && arg_count == 0) {
            result = code_gen_byte_array_to_base64(gen, object);
        }
    }

    gen->expr_as_handle = saved_handle_mode;

    /* If handle mode was active and the method returns a string (raw char *),
     * wrap the result to produce an RtHandle. Methods like toHex, toBase64,
     * toString return raw char* from C runtime functions. */
    if (saved_handle_mode && result != NULL && gen->current_arena_var != NULL)
    {
        /* Check if this is a string-returning method (byte encoding, join, etc.) */
        if ((element_type && element_type->kind == TYPE_BYTE &&
             (strcmp(method_name, "toHex") == 0 ||
              strcmp(method_name, "toBase64") == 0 ||
              strcmp(method_name, "toString") == 0 ||
              strcmp(method_name, "toStringLatin1") == 0)) ||
            strcmp(method_name, "join") == 0)
        {
            result = arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)",
                                   ARENA_VAR(gen), result);
        }
    }

    return result;
}

