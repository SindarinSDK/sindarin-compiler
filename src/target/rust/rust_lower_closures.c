/* All lexical/signature decisions are made by closure validation. This pass
 * only installs rendering flags; the shared C model is never changed. */
static void rust_lower_closure_node(json_object *node, bool *uses)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_closure_node(json_object_array_get_idx(node, i), uses);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "lambdas") != 0) rust_lower_closure_node(value, uses);
    }
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    if (!json_string_property_equals(type, "kind", "function")) return;
    if (json_boolean_property(node, "rust_direct_callee")) return;
    *uses = true;
    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    if (strcmp(kind, "variable") == 0 &&
        !json_boolean_property(node, "rust_direct_callee") &&
        !json_boolean_property(node, "rust_named_function_value"))
        json_object_object_add(node, "rust_function_read", json_object_new_boolean(true));
    if (strcmp(kind, "assign") == 0)
        json_object_object_add(node, "rust_function_assign", json_object_new_boolean(true));
    if (strcmp(kind, "member") == 0)
        json_object_object_add(node, "rust_needs_clone", json_object_new_boolean(true));
    if (strcmp(kind, "array_access") == 0)
        json_object_object_add(node, "rust_function_index", json_object_new_boolean(true));
}

static void rust_lower_closures(json_object *model)
{
    bool uses = false;
    rust_lower_closure_node(model, &uses);
    if (uses) json_object_object_add(model, "rust_has_closures", json_object_new_boolean(true));
}
