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
    json_object *comparison = NULL;
    if (json_object_object_get_ex(node, "rust_named_function_comparison", &comparison))
    {
        json_object_object_add(node, "value", json_object_new_boolean(json_object_get_boolean(comparison)));
        json_object_object_add(node, "kind", json_object_new_string("literal"));
        json_object_object_add(node, "value_kind", json_object_new_string("bool"));
        json_object_object_del(node, "left");
        json_object_object_del(node, "right");
    }
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

/* Shared model-wide name scan is defined later in rust_lower.c. */
static bool rust_model_contains_string(json_object *node, const char *wanted);

static void rust_closure_name_types(json_object *node, const char *name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_closure_name_types(json_object_array_get_idx(node, i), name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "lambdas") != 0) rust_closure_name_types(value, name);
    }
    if (json_string_property_equals(node, "kind", "function"))
        json_object_object_add(node, "rust_closure_handle_name", json_object_new_string(name));
}

static void rust_lower_closures(json_object *model)
{
    bool uses = false;
    rust_lower_closure_node(model, &uses);
    if (!uses) return;
    /* Choose the name before installing annotations. No user identifier is
     * reserved, and every nested signature uses the same module-local name. */
    char name[64] = "__SnClosure";
    unsigned int suffix = 0;
    while (rust_model_contains_string(model, name))
        snprintf(name, sizeof(name), "__SnClosure_%u", suffix++);
    rust_closure_name_types(model, name);
    json_object_object_add(model, "rust_closure_handle_name", json_object_new_string(name));
    json_object_object_add(model, "rust_has_closures", json_object_new_boolean(true));
}
