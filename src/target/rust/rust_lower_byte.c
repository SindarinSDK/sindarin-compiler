/* Preserve the tagged byte arithmetic contract in Rust without changing the
 * shared model or the C backend. Byte storage is modulo 2^8 in every
 * arithmetic mode. These target-local node kinds keep that rule separate
 * from the checked signed-integer helper path. */
static bool rust_byte_expr_type(json_object *expr)
{
    json_object *type = NULL;
    return expr && json_object_object_get_ex(expr, "type", &type) &&
           json_string_property_equals(type, "kind", "byte");
}

static bool rust_byte_binary_op(const char *op)
{
    return op && (strcmp(op, "add") == 0 || strcmp(op, "subtract") == 0 ||
                  strcmp(op, "multiply") == 0 || strcmp(op, "bitand") == 0 ||
                  strcmp(op, "bitor") == 0 || strcmp(op, "bitxor") == 0 ||
                  strcmp(op, "shl") == 0 || strcmp(op, "shr") == 0);
}

static bool rust_byte_compound_op(json_object *node, const char *op)
{
    if (rust_byte_binary_op(op)) return true;
    return op && json_string_property_equals(
                     node, "mutation_arithmetic_mode", "unchecked") &&
           (strcmp(op, "divide") == 0 || strcmp(op, "modulo") == 0);
}

static void rust_lower_byte_arithmetic(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_byte_arithmetic(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_byte_arithmetic(value);
    }

    const char *kind = json_string_property(node, "kind");
    const char *op = json_string_property(node, "op");
    if (!kind) return;

    if (strcmp(kind, "binary") == 0 && rust_byte_expr_type(node) &&
        rust_byte_binary_op(op))
    {
        json_object_object_add(node, "rust_byte_binary",
                               json_object_new_boolean(true));
        return;
    }

    if (strcmp(kind, "unary") == 0 && rust_byte_expr_type(node) && op &&
        (strcmp(op, "negate") == 0 || strcmp(op, "bitnot") == 0))
    {
        json_object_object_add(node, "rust_byte_unary",
                               json_object_new_boolean(true));
        return;
    }

    json_object *place = NULL;
    if (strcmp(kind, "compound_assign") == 0)
    {
        json_object_object_get_ex(node, "target", &place);
        if (rust_byte_expr_type(place) && rust_byte_compound_op(node, op))
            json_object_object_add(node, "rust_byte_compound_assign",
                                   json_object_new_boolean(true));
        return;
    }

    if ((strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0) &&
        json_object_object_get_ex(node, "operand", &place) &&
        rust_byte_expr_type(place))
        json_object_object_add(node,
            strcmp(kind, "increment") == 0 ? "rust_byte_increment" :
                                              "rust_byte_decrement",
            json_object_new_boolean(true));
}
