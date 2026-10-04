/* Source character values remain Rust char; only fields in persistent native
 * record layouts occupy C char storage. Annotate both source expressions and
 * the private read/store projections made by earlier lowering passes. */
static bool rust_native_record_char_field(json_object *model, json_object *type,
                                          const char *name)
{
    json_object *base = NULL;
    if (json_string_property_equals(type, "kind", "pointer") &&
        json_object_object_get_ex(type, "base_type", &base)) type = base;
    if (!json_string_property_equals(type, "kind", "struct") || !name) return false;
    json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
    json_object *fields = NULL;
    if (!json_object_object_get_ex(structure, "fields", &fields)) return false;
    for (size_t i = 0; i < json_object_array_length(fields); i++)
    {
        json_object *field = json_object_array_get_idx(fields, i);
        if (json_string_property_equals(field, "name", name))
            return json_boolean_property(field, "rust_native_c_char_storage");
    }
    return false;
}

static void rust_lower_native_record_storage(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_native_record_storage(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        rust_lower_native_record_storage(model, child);
    }
    json_object *object = NULL, *type = NULL;
    json_object_object_get_ex(node, "object", &object);
    json_object_object_get_ex(object, "type", &type);
    bool member = json_string_property_equals(node, "kind", "member");
    bool assignment = json_string_property_equals(node, "kind", "member_assign");
    if ((member || assignment) && rust_native_record_char_field(model, type,
        json_string_property(node, member ? "member_name" : "field_name")))
        json_object_object_add(node, "rust_native_c_char_storage", json_object_new_boolean(true));

    if (json_string_property_equals(node, "kind", "struct_literal") ||
        json_string_property_equals(node, "kind", "rust_thread_default"))
    {
        json_object *fields = NULL;
        json_object_object_get_ex(node, "type", &type);
        json_object_object_get_ex(node, "fields", &fields);
        for (size_t i = 0; fields && i < json_object_array_length(fields); i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            if (rust_native_record_char_field(model, type, json_string_property(field, "name")))
                json_object_object_add(field, "rust_native_c_char_storage", json_object_new_boolean(true));
        }
    }
    bool compound = json_string_property_equals(node, "kind", "compound_assign");
    bool postfix = json_string_property_equals(node, "kind", "increment") ||
                   json_string_property_equals(node, "kind", "decrement");
    json_object *place = NULL;
    if ((compound || postfix) && json_object_object_get_ex(node, compound ? "target" : "operand", &place) &&
        json_boolean_property(place, "rust_native_c_char_storage"))
    {
        json_object_object_add(node, "rust_native_c_char_mutation", json_object_new_boolean(true));
        json_object_object_add(place, "rust_native_c_char_place", json_object_new_boolean(true));
        const char *keys[] = {"rust_numeric_store", "rust_numeric_read"};
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        {
            json_object *projection = NULL;
            if (json_object_object_get_ex(node, keys[i], &projection))
                json_object_object_add(projection, "rust_native_c_char_place", json_object_new_boolean(true));
        }
    }
}
