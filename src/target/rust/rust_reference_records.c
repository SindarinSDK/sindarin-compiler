/* Contextual nil values for shared source record identities. Keep this pass
 * private to Rust: reference records retain the original C wire/owner rules. */
static bool rust_reference_record_type(json_object *model, json_object *type)
{
    return json_string_property_equals(type, "kind", "struct") &&
        json_boolean_property(rust_find_struct(model, json_string_property(type, "name")),
                              "rust_thread_reference_identity");
}

static void rust_prepare_reference_record_nodes(json_object *model, json_object *node,
                                                 json_object *expected, json_object *return_type)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_prepare_reference_record_nodes(model, json_object_array_get_idx(node, i), expected, return_type);
        return;
    }
    if (!json_object_is_type(node, json_type_object) ||
        json_string_property_equals(node, "kind", "sizeof")) return;
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "nil") &&
        rust_reference_record_type(model, expected))
    {
        json_object_object_add(node, "type", json_object_get(expected));
        json_object_object_add(node, "rust_reference_nil", json_object_new_boolean(true));
        return;
    }
    json_object *type = rust_nullable_child(node, "type");
    json_object *local_return = rust_nullable_child(node, "return_type");
    if (local_return) return_type = local_return;
    if (json_string_property_equals(node, "kind", "var_decl") &&
        rust_reference_record_type(model, type) && !rust_nullable_child(node, "initializer"))
    {
        json_object *nil = json_object_new_object();
        json_object_object_add(nil, "kind", json_object_new_string("literal"));
        json_object_object_add(nil, "value_kind", json_object_new_string("nil"));
        json_object_object_add(node, "initializer", nil);
    }
    json_object *left = rust_nullable_child(node, "left"), *right = rust_nullable_child(node, "right");
    json_object *callee_type = rust_nullable_child(rust_nullable_child(node, "callee"), "type");
    json_object *call_params = rust_nullable_child(callee_type, "param_types");
    if (json_string_property_equals(node, "kind", "static_call") ||
        json_string_property_equals(node, "kind", "method_call"))
    {
        const char *name = json_string_property(node, "type_name");
        if (!name) name = json_string_property(rust_nullable_child(node, "struct_type"), "name");
        json_object *method = rust_find_resolved_method(rust_find_struct(model, name),
            json_string_property(node, "method_name"),
            json_string_property_equals(node, "kind", "static_call") || json_boolean_property(node, "is_static"));
        call_params = rust_nullable_child(method, "params");
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0) continue;
        json_object *context = NULL;
        if (strcmp(key, "initializer") == 0 || strcmp(key, "default_value") == 0) context = type;
        else if (strcmp(key, "value") == 0)
            context = json_string_property_equals(node, "kind", "return") ? return_type : type;
        else if (strcmp(key, "left") == 0) context = rust_nullable_child(right, "type");
        else if (strcmp(key, "right") == 0) context = rust_nullable_child(left, "type");
        else if (strcmp(key, "elements") == 0) context = rust_nullable_child(type, "element_type");
        if (strcmp(key, "fields") == 0 && json_string_property_equals(node, "kind", "struct_literal"))
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *field = json_object_array_get_idx(child, i);
                rust_prepare_reference_record_nodes(model, rust_nullable_child(field, "value"),
                    rust_nullable_field_type(type, json_string_property(field, "name")), return_type);
            }
        }
        else if (strcmp(key, "args") == 0 && call_params)
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *param = i < json_object_array_length(call_params)
                    ? json_object_array_get_idx(call_params, i) : NULL;
                json_object *param_type = rust_nullable_child(param, "type");
                rust_prepare_reference_record_nodes(model, json_object_array_get_idx(child, i),
                    param_type ? param_type : param, return_type);
            }
        }
        else rust_prepare_reference_record_nodes(model, child, context, return_type);
    }
}
