static bool native_primitive_array_element(json_object *type)
{
    const char *kind = native_string(type, "kind");
    return kind && (strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
        strcmp(kind, "int32") == 0 || strcmp(kind, "uint") == 0 ||
        strcmp(kind, "uint32") == 0 || strcmp(kind, "byte") == 0 ||
        strcmp(kind, "bool") == 0 || strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0);
}

static bool native_primitive_array_boundary(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (native_primitive_array_boundary(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    if (native_bool(node, "is_native"))
    {
        json_object *result = native_record_child(node, "return_type");
        const char *result_kind = native_string(result, "kind");
        const char *result_element = native_string(native_record_child(result, "element_type"), "kind");
        if (result_kind && result_element && strcmp(result_kind, "array") == 0 &&
            native_primitive_array_element(native_record_child(result, "element_type"))) return true;
        json_object *params = native_record_child(node, "params");
        for (size_t i = 0; params && i < json_object_array_length(params); i++)
        {
            json_object *type = native_record_child(json_object_array_get_idx(params, i), "type");
            const char *kind = native_string(type, "kind");
            const char *element = native_string(native_record_child(type, "element_type"), "kind");
            if (kind && element && strcmp(kind, "array") == 0 && native_primitive_array_element(native_record_child(type, "element_type"))) return true;
        }
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && native_primitive_array_boundary(child)) return true;
    return false;
}

/* Native reference arrays use the actual C header and its ownership callbacks.
 * This pass only annotates the Rust projection, after canonical handle types. */
static bool native_handle_array_collect(json_object *node)
{
    if (!node) return false;
    bool found = false;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            found |= native_handle_array_collect(json_object_array_get_idx(node, i));
    } else if (json_object_is_type(node, json_type_object)) {
        if (native_string(node, "kind") && strcmp(native_string(node, "kind"), "array") == 0 &&
            native_handle_type(native_record_child(node, "element_type")) &&
            !native_bool(native_record_child(node, "element_type"), "rust_native_serial_handle")) found = true;
        json_object_object_foreach(node, key, child)
            if (strncmp(key, "rust_", 5) != 0) found |= native_handle_array_collect(child);
    }
    return found;
}

static void native_handle_array_mark(json_object *node, json_object *support)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            native_handle_array_mark(json_object_array_get_idx(node, i), support);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (native_string(node, "kind") && strcmp(native_string(node, "kind"), "array") == 0 &&
        ((native_handle_type(native_record_child(node, "element_type")) &&
          !native_bool(native_record_child(node, "element_type"), "rust_native_serial_handle")) ||
         (native_bool(support, "primitive_scalars") &&
          native_primitive_array_element(native_record_child(node, "element_type"))))) {
        json_object_object_add(node, "rust_native_handle_array", json_object_new_boolean(true));
        json_object_object_add(node, "rust_native_handle_array_name", json_object_get(native_record_child(support, "type")));
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) native_handle_array_mark(child, support);
    const char *kind = native_string(node, "kind");
    if (kind && strcmp(kind, "array_literal") == 0 &&
        native_bool(native_record_child(node, "type"), "rust_native_handle_array"))
        json_object_object_add(node, "rust_native_handle_array_value", json_object_new_boolean(true));
}

static bool native_prepare_handle_arrays(json_object *model, json_object *handles)
{
    bool primitive_scalars = native_primitive_array_boundary(model);
    if (!native_handle_array_collect(model) && !primitive_scalars) return true;
    json_object *support = json_object_new_object();
    if (primitive_scalars) json_object_object_add(support, "primitive_scalars", json_object_new_boolean(true));
    const char *roles[] = {"type", "trait", "untyped_new", "free", "copy", "length", "data", "width", "push", "pop", "insert", "remove", "clear", "reverse", "slice", "concat", NULL};
    for (int i = 0; roles[i]; i++) {
        char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_handle_array_%s", roles[i]);
        char *name = unique_private_name(native_record_child(model, "functions"), native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
        if (!name) { json_object_put(support); return false; }
        json_object_object_add(support, roles[i], json_object_new_string(name)); free(name);
    }
    if (primitive_scalars) {
        const char *extra[] = {"primitive_new", "primitive_width", "primitive_align", NULL};
        for (int i = 0; extra[i]; i++) {
            char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_array_%s", extra[i]);
            char *name = unique_private_name(native_record_child(model, "functions"), native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
            if (!name) { json_object_put(support); return false; }
            json_object_object_add(support, extra[i], json_object_new_string(name)); free(name);
        }
    }
    for (size_t i = 0; i < json_object_array_length(handles); i++) {
        json_object *handle = json_object_array_get_idx(handles, i);
        /* Keep support visible to the C renderer even when the first handle is
         * an opaque serialization object. It has no C refcount/copy callback. */
        json_object_object_add(handle, "rust_native_handle_array_support", json_object_get(support));
        if (native_bool(handle, "rust_native_serial_handle")) continue;
        char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_handle_%zu_array_new", i);
        char *name = unique_private_name(native_record_child(model, "functions"), native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
        if (!name) { json_object_put(support); return false; }
        json_object_object_add(handle, "rust_native_handle_array_new", json_object_new_string(name)); free(name);
    }
    native_handle_array_mark(model, support);
    json_object_object_add(model, "rust_native_handle_array_support", support);
    return true;
}
