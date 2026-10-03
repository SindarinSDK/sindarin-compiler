static bool rust_numeric_floating_kind(const char *kind);
static bool rust_numeric_checked_left_typed_op(const char *op);
static const char *rust_numeric_type_name(const char *kind);

/* Floating array macros copy/compare an object whose type is the rendered
 * argument's type, rather than the array element's type. Preserve
 * that representation in a private copy of the value expression. In the
 * restored C renderer, source float literals are C double expressions. */
static const char *rust_float_array_c_kind(json_object *node)
{
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    const char *kind = json_string_property(type, "kind");
    if (!rust_numeric_floating_kind(kind)) return NULL;
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "double")) return "double";
    if (json_string_property_equals(node, "kind", "unary"))
    {
        json_object *operand = NULL;
        json_object_object_get_ex(node, "operand", &operand);
        const char *operand_kind = rust_float_array_c_kind(operand);
        if (operand_kind) return operand_kind;
    }
    if (json_string_property_equals(node, "kind", "binary") &&
        !json_string_property_equals(node, "arithmetic_mode", "checked"))
    {
        json_object *left = NULL, *right = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        const char *left_kind = rust_float_array_c_kind(left);
        const char *right_kind = rust_float_array_c_kind(right);
        if ((left_kind && strcmp(left_kind, "double") == 0) ||
            (right_kind && strcmp(right_kind, "double") == 0)) return "double";
    }
    /* Calls, fields, indexing, assignment, mutation and value-match results
     * have an actual C signature/storage boundary at their resolved type. */
    return kind;
}

static void rust_float_array_c_values(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_float_array_c_values(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        rust_float_array_c_values(child);
    }
    const char *c_kind = rust_float_array_c_kind(node);
    if (c_kind)
    {
        json_object_object_add(node, "rust_c_float_expression_kind", json_object_new_string(c_kind));
        if (json_string_property_equals(node, "kind", "literal") &&
            json_string_property_equals(node, "value_kind", "double"))
            json_object_object_add(node, "rust_c_float_literal", json_object_new_boolean(true));
    }
    if (json_string_property_equals(node, "kind", "binary"))
    {
        json_object *left = NULL, *right = NULL, *left_type = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        json_object_object_get_ex(left, "type", &left_type);
        const char *left_kind = rust_float_array_c_kind(left);
        const char *right_kind = rust_float_array_c_kind(right);
        if (!left_kind && !right_kind) return;
        const char *op = json_string_property(node, "op");
        const char *common = c_kind;
        if (json_string_property_equals(node, "arithmetic_mode", "checked") &&
            rust_numeric_checked_left_typed_op(op))
            common = json_string_property(left_type, "kind");
        if (!common)
            common = (left_kind && strcmp(left_kind, "double") == 0) ||
                     (right_kind && strcmp(right_kind, "double") == 0) ? "double" : "float";
        if (rust_numeric_floating_kind(common))
            json_object_object_add(node, "rust_numeric_binary_type",
                                   json_object_new_string(rust_numeric_type_name(common)));
    }
}
