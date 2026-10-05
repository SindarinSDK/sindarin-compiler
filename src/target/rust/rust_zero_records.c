/* C zeroes an uninitialized value record. Construct that value before nullable
 * and ownership preparation so strings, arrays and reference children receive
 * their nil representations rather than source field initializers. */
static json_object *rust_zero_record_value(json_object *model, json_object *type)
{
    json_object *value = json_object_new_object();
    json_object_object_add(value, "type", json_object_get(type));
    const char *kind = json_string_property(type, "kind");
    if (kind && strcmp(kind, "struct") == 0 &&
        !json_boolean_property(type, "pass_self_by_ref"))
    {
        json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
        json_object *fields = json_object_object_get(structure, "fields"), *values = json_object_new_array();
        json_object_object_add(value, "kind", json_object_new_string("struct_literal"));
        json_object_object_add(value, "struct_name", json_object_new_string(json_string_property(type, "name")));
        json_object_object_add(value, "fields", values);
        for (size_t i = 0; fields && i < json_object_array_length(fields); i++)
        {
            json_object *source = json_object_array_get_idx(fields, i), *field = json_object_new_object();
            json_object_object_add(field, "name", json_object_get(json_object_object_get(source, "name")));
            json_object_object_add(field, "value", rust_zero_record_value(model, json_object_object_get(source, "type")));
            json_object_array_add(values, field);
        }
        return value;
    }
    json_object_object_add(value, "kind", json_object_new_string("literal"));
    bool nil = kind && (strcmp(kind, "string") == 0 || strcmp(kind, "array") == 0 ||
        strcmp(kind, "struct") == 0 || strcmp(kind, "function") == 0 ||
        strcmp(kind, "interface") == 0 || strcmp(kind, "opaque") == 0 || strcmp(kind, "pointer") == 0);
    json_object_object_add(value, "value_kind", json_object_new_string(nil ? "nil" : kind ? kind : "int"));
    if (!nil)
        json_object_object_add(value, "value", kind && strcmp(kind, "bool") == 0
            ? json_object_new_boolean(false) : kind &&
              (strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0)
            ? json_object_new_double(0.0) : json_object_new_int64(0));
    return value;
}

static void rust_prepare_zero_record_bindings(json_object *model, json_object *node)
{
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_prepare_zero_record_bindings(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *type = json_object_object_get(node, "type");
    if (json_string_property_equals(node, "kind", "var_decl") &&
        json_string_property_equals(type, "kind", "struct") &&
        !json_boolean_property(type, "pass_self_by_ref") &&
        !json_boolean_property(type, "rust_native_record_storage") &&
        !json_object_object_get(node, "initializer"))
        json_object_object_add(node, "initializer", rust_zero_record_value(model, type));
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_prepare_zero_record_bindings(model, child);
}
