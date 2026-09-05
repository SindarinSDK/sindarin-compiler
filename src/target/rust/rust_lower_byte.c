/* Preserve the tagged fixed-width arithmetic contract in Rust without
 * changing the shared model or the C backend. These target-local node kinds
 * keep wrapping storage and C byte promotions separate from the checked
 * signed-integer helper path. */
static const char *rust_wrapping_expr_type(json_object *expr)
{
    json_object *type = NULL;
    return expr && json_object_object_get_ex(expr, "type", &type)
        ? json_string_property(type, "kind") : NULL;
}

static bool rust_wrapping_binary_op(const char *op)
{
    return op && (strcmp(op, "add") == 0 || strcmp(op, "subtract") == 0 ||
                  strcmp(op, "multiply") == 0 || strcmp(op, "bitand") == 0 ||
                  strcmp(op, "bitor") == 0 || strcmp(op, "bitxor") == 0 ||
                  strcmp(op, "shl") == 0 || strcmp(op, "shr") == 0);
}

static bool rust_byte_promoted_binary(json_object *node, const char *op)
{
    if (!op) return false;
    if (strcmp(op, "bitand") == 0 || strcmp(op, "bitor") == 0 ||
        strcmp(op, "bitxor") == 0 || strcmp(op, "shl") == 0 ||
        strcmp(op, "shr") == 0)
        return true;
    if (!json_string_property_equals(node, "arithmetic_mode", "unchecked"))
        return false;
    return strcmp(op, "add") == 0 || strcmp(op, "subtract") == 0 ||
           strcmp(op, "multiply") == 0 || strcmp(op, "divide") == 0 ||
           strcmp(op, "modulo") == 0;
}

static bool rust_byte_comparison_op(const char *op)
{
    return op && (strcmp(op, "eq") == 0 || strcmp(op, "ne") == 0 ||
                  strcmp(op, "lt") == 0 || strcmp(op, "le") == 0 ||
                  strcmp(op, "gt") == 0 || strcmp(op, "ge") == 0);
}

static void rust_mark_promoted_child(json_object *child, bool observed)
{
    if (!child || !json_boolean_property(child, "rust_byte_promoted")) return;
    json_object_object_add(child,
        observed ? "rust_byte_promoted_observed" : "rust_byte_promoted_inner",
        json_object_new_boolean(true));
}

static bool rust_wrapping_compound_op(json_object *node, const char *op)
{
    if (rust_wrapping_binary_op(op)) return true;
    return op && json_string_property_equals(
                     node, "mutation_arithmetic_mode", "unchecked") &&
           (strcmp(op, "divide") == 0 || strcmp(op, "modulo") == 0);
}

static bool rust_fixed_integral_kind(const char *kind)
{
    return kind && (strcmp(kind, "byte") == 0 || strcmp(kind, "int32") == 0 ||
                    strcmp(kind, "uint32") == 0 || strcmp(kind, "int") == 0 ||
                    strcmp(kind, "long") == 0 || strcmp(kind, "uint") == 0);
}

static const char *rust_integral_promotion_type(const char *left,
                                                 const char *right,
                                                 const char *op)
{
    bool shift = op && (strcmp(op, "shl") == 0 || strcmp(op, "shr") == 0);
    bool left_64 = strcmp(left, "int") == 0 || strcmp(left, "long") == 0 ||
                   strcmp(left, "uint") == 0;
    bool left_unsigned = strcmp(left, "uint32") == 0 || strcmp(left, "uint") == 0;
    if (shift || strcmp(left, right) == 0)
    {
        if (strcmp(left, "byte") == 0) return "i32";
        if (left_64) return left_unsigned ? "u64" : "i64";
        return left_unsigned ? "u32" : "i32";
    }

    bool right_64 = strcmp(right, "int") == 0 || strcmp(right, "long") == 0 ||
                    strcmp(right, "uint") == 0;
    bool right_unsigned =
        strcmp(right, "uint32") == 0 || strcmp(right, "uint") == 0;
    if (strcmp(left, "byte") == 0)
        left_unsigned = false;
    if (strcmp(right, "byte") == 0)
        right_unsigned = false;
    if (left_64 || right_64)
    {
        if ((left_64 && left_unsigned) || (right_64 && right_unsigned))
            return "u64";
        return "i64";
    }
    if (left_unsigned || right_unsigned) return "u32";
    return "i32";
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

    const char *type_kind = rust_wrapping_expr_type(node);
    bool byte_type = type_kind && strcmp(type_kind, "byte") == 0;
    bool unsigned_type = type_kind &&
        (strcmp(type_kind, "uint") == 0 || strcmp(type_kind, "uint32") == 0);
    bool checked_int32 = type_kind && strcmp(type_kind, "int32") == 0 &&
        json_string_property_equals(node, "arithmetic_mode", "checked");

    if (strcmp(kind, "binary") == 0 && byte_type &&
        rust_byte_promoted_binary(node, op))
    {
        json_object_object_add(node, "rust_byte_promoted",
                               json_object_new_boolean(true));
        json_object *left = NULL, *right = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        rust_mark_promoted_child(left, false);
        rust_mark_promoted_child(right, false);
        return;
    }

    if (strcmp(kind, "binary") == 0 && rust_wrapping_binary_op(op) &&
        (byte_type || unsigned_type ||
         (checked_int32 && op &&
          (strcmp(op, "add") == 0 || strcmp(op, "subtract") == 0 ||
           strcmp(op, "multiply") == 0))))
    {
        json_object_object_add(node, "rust_wrapping_binary",
                               json_object_new_boolean(true));
        return;
    }

    if (strcmp(kind, "unary") == 0 && byte_type && op &&
        (strcmp(op, "negate") == 0 || strcmp(op, "bitnot") == 0))
    {
        json_object_object_add(node, "rust_byte_promoted",
                               json_object_new_boolean(true));
        json_object *operand = NULL;
        json_object_object_get_ex(node, "operand", &operand);
        rust_mark_promoted_child(operand, false);
        return;
    }

    if (strcmp(kind, "unary") == 0 && unsigned_type && op &&
        (strcmp(op, "negate") == 0 || strcmp(op, "bitnot") == 0))
    {
        json_object_object_add(node, "rust_wrapping_unary",
                               json_object_new_boolean(true));
        return;
    }

    if ((strcmp(kind, "builtin_print") == 0 ||
         strcmp(kind, "builtin_println") == 0))
    {
        json_object *args = NULL;
        if (json_object_object_get_ex(node, "args", &args) &&
            json_object_is_type(args, json_type_array))
            for (size_t i = 0; i < json_object_array_length(args); i++)
                rust_mark_promoted_child(json_object_array_get_idx(args, i), true);
        return;
    }

    if (strcmp(kind, "binary") == 0 && rust_byte_comparison_op(op))
    {
        json_object *left = NULL, *right = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        if ((left && json_boolean_property(left, "rust_byte_promoted")) ||
            (right && json_boolean_property(right, "rust_byte_promoted")))
        {
            rust_mark_promoted_child(left, true);
            rust_mark_promoted_child(right, true);
            json_object_object_add(node, "rust_byte_promoted_comparison",
                                   json_object_new_boolean(true));
        }
        return;
    }

    json_object *place = NULL;
    if (strcmp(kind, "compound_assign") == 0)
    {
        json_object_object_get_ex(node, "target", &place);
        const char *place_kind = rust_wrapping_expr_type(place);
        json_object *value = NULL;
        json_object_object_get_ex(node, "value", &value);
        const char *value_kind = rust_wrapping_expr_type(value);
        if (rust_fixed_integral_kind(place_kind) &&
            rust_fixed_integral_kind(value_kind) &&
            strcmp(place_kind, value_kind) != 0)
        {
            const char *promotion =
                rust_integral_promotion_type(place_kind, value_kind, op);
            json_object_object_add(node, "rust_mixed_integral_compound",
                                   json_object_new_boolean(true));
            json_object_object_add(node, "rust_mixed_integral_type",
                                   json_object_new_string(promotion));
            json_object_object_add(
                node, "rust_mixed_integral_unsigned",
                json_object_new_boolean(promotion[0] == 'u'));
            json_object_object_add(
                node, "rust_mixed_integral_wrapping",
                json_object_new_boolean(
                    promotion[0] == 'u' ||
                    json_string_property_equals(
                        node, "mutation_arithmetic_mode", "unchecked")));
        }
        else if (place_kind &&
            (strcmp(place_kind, "byte") == 0 || strcmp(place_kind, "uint") == 0 ||
             strcmp(place_kind, "uint32") == 0) &&
            rust_wrapping_compound_op(node, op))
            json_object_object_add(node, "rust_wrapping_compound_assign",
                                   json_object_new_boolean(true));
        else if (place_kind &&
                 (strcmp(place_kind, "int") == 0 ||
                  strcmp(place_kind, "long") == 0 ||
                  strcmp(place_kind, "int32") == 0) &&
                 json_string_property_equals(
                     node, "mutation_arithmetic_mode", "unchecked"))
            json_object_object_add(node, "rust_unchecked_integral_compound",
                                   json_object_new_boolean(true));
        return;
    }

    if ((strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0) &&
        json_object_object_get_ex(node, "operand", &place) &&
        (type_kind = rust_wrapping_expr_type(place)) &&
        (strcmp(type_kind, "byte") == 0 || strcmp(type_kind, "uint") == 0 ||
         strcmp(type_kind, "uint32") == 0))
        json_object_object_add(node,
            strcmp(kind, "increment") == 0 ? "rust_wrapping_increment" :
                                              "rust_wrapping_decrement",
            json_object_new_boolean(true));
}
