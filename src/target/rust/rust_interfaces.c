static void rust_interface_annotate_nodes(json_object *model, json_object *node)
{
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_interface_annotate_nodes(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "interface"))
        json_object_object_add(node, "rust_interface_type_name", json_object_get(json_object_object_get(model, "rust_interface_identity_type")));
    if (json_string_property_equals(node, "kind", "struct"))
    {
        json_object *structure = rust_find_struct(model, json_string_property(node, "name"));
        if (json_boolean_property(structure, "rust_interface_record"))
        {
            json_object_object_add(node, "rust_interface_record", json_object_new_boolean(true));
            json_object_object_add(node, "rust_interface_owned_metadata", json_object_new_boolean(
                json_boolean_property(structure, "rust_interface_owned_metadata")));
            json_object_object_add(node, "rust_interface_layout_type", json_object_get(json_object_object_get(structure, "rust_interface_layout_type")));
            if (json_boolean_property(structure, "rust_interface_empty"))
                json_object_object_add(node, "rust_interface_empty", json_object_new_boolean(true));
            if (json_boolean_property(structure, "rust_interface_borrow_origin"))
                json_object_object_add(node, "rust_interface_borrow_origin", json_object_new_boolean(true));
        }
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_interface_annotate_nodes(model, child);
    json_object *type = json_object_object_get(node, "type");
    if (json_string_property_equals(node, "kind", "variable") && json_boolean_property(node, "rust_interface_argument"))
        json_object_object_add(node, "rust_interface_root_read", json_object_new_boolean(
            !json_boolean_property(node, "is_ref_var") && !json_boolean_property(type, "pass_self_by_ref")));
    if (json_string_property_equals(node, "kind", "array_access") &&
        json_boolean_property(node, "rust_interface_argument") &&
        json_boolean_property(type, "rust_interface_record") &&
        !json_boolean_property(type, "pass_self_by_ref"))
        json_object_object_add(node, "rust_interface_array_element", json_object_new_boolean(true));
    if (json_string_property_equals(node, "kind", "var_decl") && json_boolean_property(type, "rust_interface_record"))
        json_object_object_add(node, "rust_interface_record_binding", json_object_new_boolean(true));
    if (json_string_property_equals(node, "kind", "call") ||
        json_string_property_equals(node, "kind", "static_call"))
    {
        json_object *args = json_object_object_get(node, "args");
        for (size_t i = 0; args && i < json_object_array_length(args); i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            json_object *argument_type = json_object_object_get(arg, "type");
            if (json_boolean_property(argument_type, "rust_interface_owned_metadata") &&
                !json_boolean_property(arg, "rust_interface_argument") &&
                !json_boolean_property(arg, "is_ref_arg") &&
                !json_boolean_property(arg, "is_copy_arg") &&
                (json_string_property_equals(arg, "kind", "variable") ||
                 json_string_property_equals(arg, "kind", "member") ||
                 json_string_property_equals(arg, "kind", "array_access")))
                json_object_object_add(arg, "rust_needs_clone", json_object_new_boolean(true));
        }
    }
    if (json_string_property_equals(node, "kind", "var_decl") &&
        json_boolean_property(type, "rust_interface_owned_metadata") &&
        !json_boolean_property(type, "has_heap_fields"))
    {
        json_object *initializer = json_object_object_get(node, "initializer");
        if (json_string_property_equals(initializer, "kind", "variable") ||
            json_string_property_equals(initializer, "kind", "member") ||
            json_string_property_equals(initializer, "kind", "array_access"))
            json_object_object_add(node, "source_is_borrow", json_object_new_boolean(true));
    }
    json_object *binding_element = json_object_object_get(type, "element_type");
    if (json_string_property_equals(node, "kind", "var_decl") &&
        json_string_property_equals(type, "kind", "array") &&
        json_boolean_property(binding_element, "rust_interface_record") &&
        !json_boolean_property(binding_element, "pass_self_by_ref") &&
        !json_boolean_property(binding_element, "rust_interface_owned_metadata") &&
        !json_boolean_property(node, "rust_shared_cell") &&
        !json_boolean_property(node, "rust_thread_array_storage"))
    {
        char name[100];
        if (rust_allocate_helper_name(model, "__sn_interface_array_scope", name, sizeof(name)))
            json_object_object_add(node, "rust_interface_array_scope", json_object_new_string(name));
    }
    if (json_string_property_equals(node, "kind", "member") && json_boolean_property(node, "rust_interface_argument"))
    {
        json_object *object = json_object_object_get(node, "object"), *owner_type = json_object_object_get(object, "type");
        json_object *owner = rust_find_struct(model, json_string_property(owner_type, "name"));
        json_object *fields = json_object_object_get(owner, "fields");
        for (size_t i = 0; fields && i < json_object_array_length(fields); i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            if (json_string_property_equals(field, "name", json_string_property(node, "member_name")) &&
                json_object_object_get(field, "rust_interface_field_getter"))
                json_object_object_add(node, "rust_interface_field_getter", json_object_get(json_object_object_get(field, "rust_interface_field_getter")));
        }
    }
    if (json_string_property_equals(node, "kind", "call"))
    {
        json_object *callee = json_object_object_get(node, "callee"), *object = json_object_object_get(callee, "object");
        json_object *array_type = json_object_object_get(object, "type"), *element = json_object_object_get(array_type, "element_type");
        const char *method = json_string_property(callee, "member_name");
        if (json_string_property_equals(callee, "kind", "member") &&
            json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(array_type, "kind", "array") &&
            json_boolean_property(element, "rust_interface_record") &&
            !json_boolean_property(element, "pass_self_by_ref") && method &&
            (strcmp(method, "push") == 0 || strcmp(method, "insert") == 0 ||
             strcmp(method, "remove") == 0 || strcmp(method, "pop") == 0 || strcmp(method, "clear") == 0))
            json_object_object_add(node, "rust_interface_array_refresh", json_object_new_boolean(true));
    }
    json_object *element = json_object_object_get(type, "element_type");
    if (json_string_property_equals(type, "kind", "array") &&
        json_boolean_property(element, "rust_interface_record") &&
        !json_boolean_property(element, "pass_self_by_ref") &&
        (json_string_property_equals(node, "kind", "copy_of") ||
         json_string_property_equals(node, "kind", "array_slice") ||
         json_string_property_equals(node, "kind", "binary") ||
         (json_string_property_equals(node, "kind", "call") &&
          !json_boolean_property(node, "rust_interface_array_refresh"))))
        json_object_object_add(node, "rust_interface_owned_array", json_object_new_boolean(true));
}

static bool rust_prepare_interface_identities(json_object *model)
{
    if (!json_boolean_property(model, "rust_has_interface_arguments")) return true;
    json_object_object_add(model, "rust_uses_arrays", json_object_new_boolean(true));
    const char *keys[] = {"identity_type", "key_type", "registry_type", "trait", "key_method", "links_method", "getter_function", "root_function", "array_function", "array_refresh_function", "array_trait", "array_prepare_function", "array_element_function", "array_scope_type", "owner_type", "owner_field", "owner_method", "links_owned_method", "links_physical_method", "array_read_trait", "mutation_temp", "metadata_type", "empty_field", "origin_field", "value_temp", NULL};
    for (size_t i = 0; keys[i]; i++)
    {
        char key[100], stem[100], name[100];
        snprintf(key, sizeof(key), "rust_interface_%s", keys[i]);
        snprintf(stem, sizeof(stem), "__sn_interface_%s", keys[i]);
        if (!rust_allocate_helper_name(model, stem, name, sizeof(name))) return false;
        json_object_object_add(model, key, json_object_new_string(name));
    }
    json_object *structures = json_object_object_get(model, "structs");
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++)
    {
        json_object *structure = json_object_array_get_idx(structures, i);
        if (json_boolean_property(structure, "rust_native_reference_handle") ||
            !json_object_object_get(structure, "rust_interface_layout_type")) continue;
        json_object_object_add(structure, "rust_interface_record", json_object_new_boolean(true));
        json_object_object_add(structure, "rust_interface_owned_metadata", json_object_new_boolean(
            !json_boolean_property(structure, "rust_native_record_storage")));
        if (json_boolean_property(structure, "rust_thread_fields") &&
            !json_boolean_property(structure, "rust_thread_reference_identity"))
            json_object_object_add(structure, "rust_interface_borrow_origin", json_object_new_boolean(true));
        json_object *fields = json_object_object_get(structure, "fields");
        if (!json_boolean_property(structure, "pass_self_by_ref") &&
            !json_boolean_property(structure, "is_native") && json_object_array_length(fields) == 0)
            json_object_object_add(structure, "rust_interface_empty", json_object_new_boolean(true));
    }
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++)
    {
        json_object *structure = json_object_array_get_idx(structures, i);
        json_object *fields = json_object_object_get(structure, "fields");
        for (size_t f = 0; fields && f < json_object_array_length(fields); f++)
        {
            json_object *field = json_object_array_get_idx(fields, f), *type = json_object_object_get(field, "type");
            json_object *child = rust_find_struct(model, json_string_property(type, "name"));
            if (json_string_property_equals(type, "kind", "struct") &&
                !json_boolean_property(type, "pass_self_by_ref") && json_boolean_property(child, "rust_interface_record"))
            {
                char wire[48];
                snprintf(wire, sizeof(wire), "field_%zu", f + (json_boolean_property(structure, "pass_self_by_ref") ? 1 : 0));
                json_object_object_add(field, "rust_interface_inline_record", json_object_new_boolean(true));
                json_object_object_add(field, "rust_interface_wire_field", json_object_new_string(wire));
                char getter[100];
                if (!rust_allocate_helper_name(model, "__sn_interface_field_identity", getter, sizeof(getter))) return false;
                json_object_object_add(field, "rust_interface_field_getter", json_object_new_string(getter));
            }
        }
    }
    rust_interface_annotate_nodes(model, model);
    return true;
}
