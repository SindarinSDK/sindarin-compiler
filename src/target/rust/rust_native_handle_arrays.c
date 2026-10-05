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
        native_handle_type(native_record_child(node, "element_type")) &&
            !native_bool(native_record_child(node, "element_type"), "rust_native_serial_handle")) {
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
    if (!native_handle_array_collect(model)) return true;
    json_object *support = json_object_new_object();
    const char *roles[] = {"type", "trait", "untyped_new", "free", "copy", "length", "data", "width", "push", "pop", "insert", "remove", "clear", "reverse", "slice", "concat", NULL};
    for (int i = 0; roles[i]; i++) {
        char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_handle_array_%s", roles[i]);
        char *name = unique_private_name(native_record_child(model, "functions"), native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
        if (!name) { json_object_put(support); return false; }
        json_object_object_add(support, roles[i], json_object_new_string(name)); free(name);
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
