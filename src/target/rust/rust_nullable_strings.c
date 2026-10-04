/* Included by rust_target.c. Nil strings retain their C null state separately
 * from allocated empty byte buffers. Contextual typing stays Rust-private. */
static json_object *rust_nullable_child(json_object *node, const char *key)
{
    json_object *value = NULL;
    if (node && json_object_is_type(node, json_type_object))
        json_object_object_get_ex(node, key, &value);
    return value;
}

static bool rust_nullable_string_type(json_object *type)
{
    return json_string_property_equals(type, "kind", "string");
}

static json_object *rust_nullable_field_type(json_object *type,
                                             const char *name)
{
    json_object *fields = rust_nullable_child(type, "fields");
    for (size_t i = 0; fields && i < json_object_array_length(fields); i++)
    {
        json_object *field = json_object_array_get_idx(fields, i);
        if (name && json_string_property_equals(field, "name", name))
            return rust_nullable_child(field, "type");
    }
    return NULL;
}

static bool rust_prepare_nullable_string_nodes(json_object *node,
                                               json_object *expected,
                                               json_object *return_type,
                                               json_object *model)
{
    if (!node) return false;
    bool nullable = false;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            nullable |= rust_prepare_nullable_string_nodes(
                json_object_array_get_idx(node, i), expected, return_type, model);
        return nullable;
    }
    if (!json_object_is_type(node, json_type_object) ||
        json_string_property_equals(node, "kind", "sizeof")) return false;
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "nil") &&
        rust_nullable_string_type(expected))
    {
        json_object_object_add(node, "type", json_object_get(expected));
        json_object_object_add(node, "rust_string_nil", json_object_new_boolean(true));
        return true;
    }
    json_object *type = rust_nullable_child(node, "type");
    json_object *local_return = rust_nullable_child(node, "return_type");
    if (local_return) return_type = local_return;
    if (json_string_property_equals(node, "kind", "var_decl") &&
        rust_nullable_string_type(type) &&
        !rust_nullable_child(node, "initializer")) nullable = true;
    if (json_boolean_property(node, "is_native"))
    {
        nullable |= rust_nullable_string_type(local_return);
        json_object *params = rust_nullable_child(node, "params");
        for (size_t i = 0; params && i < json_object_array_length(params); i++)
            nullable |= rust_nullable_string_type(rust_nullable_child(
                json_object_array_get_idx(params, i), "type"));
    }
    json_object *left = rust_nullable_child(node, "left");
    json_object *right = rust_nullable_child(node, "right");
    json_object *callee_type = rust_nullable_child(
        rust_nullable_child(node, "callee"), "type");
    json_object *call_params = rust_nullable_child(callee_type, "param_types");
    if (json_string_property_equals(node, "kind", "static_call"))
    {
        json_object *structure = rust_find_struct(model, json_string_property(node, "type_name"));
        json_object *method = rust_find_resolved_method(
            structure, json_string_property(node, "method_name"), true);
        call_params = rust_nullable_child(method, "params");
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0) continue;
        json_object *context = NULL;
        if (strcmp(key, "initializer") == 0 || strcmp(key, "default_value") == 0)
            context = type;
        else if (strcmp(key, "value") == 0)
            context = json_string_property_equals(node, "kind", "return")
                ? return_type : type;
        else if (strcmp(key, "left") == 0)
            context = rust_nullable_child(right, "type");
        else if (strcmp(key, "right") == 0)
            context = rust_nullable_child(left, "type");
        else if (strcmp(key, "elements") == 0)
            context = rust_nullable_child(type, "element_type");
        if (strcmp(key, "fields") == 0 &&
            json_string_property_equals(node, "kind", "struct_literal"))
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *field = json_object_array_get_idx(child, i);
                nullable |= rust_prepare_nullable_string_nodes(
                    rust_nullable_child(field, "value"),
                    rust_nullable_field_type(type, json_string_property(field, "name")),
                    return_type, model);
            }
        }
        else if (strcmp(key, "args") == 0 && call_params)
        {
            json_object *params = call_params;
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *param = params && i < json_object_array_length(params)
                    ? json_object_array_get_idx(params, i) : NULL;
                json_object *param_type = rust_nullable_child(param, "type");
                nullable |= rust_prepare_nullable_string_nodes(
                    json_object_array_get_idx(child, i), param_type ? param_type : param,
                    return_type, model);
            }
        }
        else nullable |= rust_prepare_nullable_string_nodes(child, context, return_type, model);
    }
    return nullable;
}

static void rust_mark_nullable_string_types(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_mark_nullable_string_types(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (rust_nullable_string_type(node))
        json_object_object_add(node, "rust_nullable_string", json_object_new_boolean(true));
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_mark_nullable_string_types(child);
    }
}

static void rust_prepare_nullable_strings(json_object *model)
{
    if (!rust_prepare_nullable_string_nodes(model, NULL, NULL, model)) return;
    rust_mark_nullable_string_types(model);
    json_object_object_add(model, "rust_nullable_strings", json_object_new_boolean(true));
}
