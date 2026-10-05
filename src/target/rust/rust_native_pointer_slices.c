/* C's actual char signedness determines the long long slice bound. Use a
 * typed C adapter rather than assuming Rust's target c_char matches SN_CC flags. */
static bool native_pointer_slice_char_bounds(json_object *node, const char *helper)
{
    if (!node) return false;
    bool found = false;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            found |= native_pointer_slice_char_bounds(json_object_array_get_idx(node, i), helper);
        return found;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object_object_foreach(node, key, value)
        if (strncmp(key, "rust_", 5) != 0)
            found |= native_pointer_slice_char_bounds(value, helper);
    if (!native_bool(node, "is_pointer_slice")) return found;
    const char *bounds[] = {"start", "end"};
    for (size_t i = 0; i < 2; i++) {
        json_object *value = native_record_child(node, bounds[i]);
        const char *kind = native_string(native_record_child(value, "type"), "kind");
        if (!kind || strcmp(kind, "char") != 0) continue;
        found = true;
        if (!helper) continue;
        json_object *type = native_primitive_type("function"), *params = json_object_new_array();
        json_object_object_add(type, "is_native", json_object_new_boolean(true));
        json_object_object_add(type, "return_type", native_primitive_type("int"));
        json_object_array_add(params, native_primitive_type("char"));
        json_object_object_add(type, "param_types", params);
        json_object *call = json_object_new_object(), *args = json_object_new_array();
        json_object_object_add(call, "kind", json_object_new_string("call"));
        json_object_object_add(call, "type", native_primitive_type("int"));
        json_object_object_add(call, "callee", native_handle_expression(helper, type));
        json_object_put(type);
        json_object_array_add(args, json_object_get(value));
        json_object_object_add(call, "args", args);
        json_object_object_add(node, bounds[i], call);
    }
    return found;
}

static bool native_prepare_pointer_slice_char_bounds(json_object *model,
                                                      json_object *private_model,
                                                      const char *source_file)
{
    if (!native_pointer_slice_char_bounds(model, NULL)) return true;
    char *name = unique_private_name(native_record_child(model, "functions"),
        native_record_child(model, "structs"), native_record_child(model, "globals"),
        "__sn_pointer_slice_char_offset");
    if (!name) return false;
    json_object *function = json_object_new_object(), *params = json_object_new_array();
    json_object_object_add(function, "name", json_object_new_string(name));
    json_object_object_add(function, "source_file", json_object_new_string(source_file));
    json_object_object_add(function, "is_native", json_object_new_boolean(true));
    json_object_object_add(function, "has_body", json_object_new_boolean(true));
    json_object_object_add(function, "return_type", native_primitive_type("int"));
    json_object *param = json_object_new_object();
    json_object_object_add(param, "name", json_object_new_string("value"));
    json_object_object_add(param, "type", native_primitive_type("char"));
    json_object_object_add(param, "mem_qual", json_object_new_string("default"));
    json_object_object_add(param, "sync_mod", json_object_new_string("none"));
    json_object_array_add(params, param);
    json_object_object_add(function, "params", params);
    json_object *statement = json_object_new_object(), *body = json_object_new_array();
    json_object_object_add(statement, "kind", json_object_new_string("return"));
    json_object *type = native_primitive_type("char");
    json_object_object_add(statement, "value", native_handle_expression("value", type));
    json_object_put(type);
    json_object_array_add(body, statement);
    json_object_object_add(function, "body", body);
    json_object_array_add(native_record_child(private_model, "functions"), deep_copy(function));
    json_object_array_add(native_record_child(model, "functions"), function);
    native_pointer_slice_char_bounds(model, name);
    free(name);
    return true;
}
