/* Thread-only aggregate storage. Value copies snapshot; reference/receiver
 * transfers share field owners. No guard survives an expression or call. */
static json_object *rust_thread_receiver_base(json_object *type)
{
    json_object *base = NULL;
    if (json_string_property_equals(type, "kind", "pointer") &&
        json_object_object_get_ex(type, "base_type", &base)) return base;
    return type;
}

static void rust_thread_receiver_select(json_object *node, json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_receiver_select(json_object_array_get_idx(node, i), names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "thread_spawn")) {
        json_object *call = NULL, *callee = NULL, *object = NULL, *type = NULL, *args = NULL;
        json_object_object_get_ex(node, "call", &call);
        json_object_object_get_ex(call, "callee", &callee);
        json_object_object_get_ex(callee, "object", &object);
        if (object) json_object_object_get_ex(object, "type", &type);
        type = rust_thread_receiver_base(type);
        const char *name = json_string_property(type, "name");
        if (name && json_string_property_equals(type, "kind", "struct"))
            json_object_object_add(names, name, json_object_new_boolean(true));
        json_object_object_get_ex(call, "args", &args);
        for (size_t i = 0; args && i < json_object_array_length(args); i++) {
            json_object *arg = json_object_array_get_idx(args, i); type = NULL;
            json_object_object_get_ex(arg, "type", &type);
            name = json_string_property(type, "name");
            if (name && json_string_property_equals(type, "kind", "struct"))
                json_object_object_add(names, name, json_object_new_boolean(true));
        }
    }
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_receiver_select(value, names);
    }
}

static bool rust_thread_receiver_type(json_object *type, json_object *names)
{
    json_object *found = NULL;
    type = rust_thread_receiver_base(type);
    const char *name = json_string_property(type, "name");
    return name && json_string_property_equals(type, "kind", "struct") &&
        json_object_object_get_ex(names, name, &found);
}

static void rust_thread_receiver_prepare_nodes(json_object *node, json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_receiver_prepare_nodes(json_object_array_get_idx(node, i), names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    if (rust_thread_receiver_type(type, names) && json_string_property(node, "mem_qual"))
        json_object_object_add(node, "rust_thread_aggregate_param", json_object_new_boolean(true));
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_receiver_prepare_nodes(value, names);
    }
}

static void rust_prepare_thread_receivers(json_object *model)
{
    json_object *names = json_object_new_object();
    rust_thread_receiver_select(model, names);
    json_object_object_add(model, "rust_thread_receiver_names", names);
    rust_thread_receiver_prepare_nodes(model, names);
    json_object *structures = NULL;
    json_object_object_get_ex(model, "structs", &structures);
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i), *found = NULL;
        if (json_object_object_get_ex(names, json_string_property(structure, "name"), &found)) {
            json_object_object_add(structure, "rust_thread_fields", json_object_new_boolean(true));
            if (json_string_property_equals(structure, "mem_mode", "ref"))
                json_object_object_add(structure, "rust_thread_reference_identity", json_object_new_boolean(true));
        }
    }
}

static void rust_thread_receiver_lower(json_object *node, json_object *names, json_object *model)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_receiver_lower(json_object_array_get_idx(node, i), names, model);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_receiver_lower(value, names, model);
    }
    json_object *object = NULL, *type = NULL;
    json_object_object_get_ex(node, "object", &object);
    if (object) json_object_object_get_ex(object, "type", &type);
    if (rust_thread_receiver_type(type, names)) {
        json_object *structure = rust_find_struct(model, json_string_property(rust_thread_receiver_base(type), "name")), *fields = NULL;
        const char *field_name = json_string_property(node, "member_name");
        if (!field_name) field_name = json_string_property(node, "field_name");
        json_object_object_get_ex(structure, "fields", &fields);
        for (size_t i = 0; field_name && i < json_object_array_length(fields); i++)
            if (json_string_property_equals(json_object_array_get_idx(fields, i), "name", field_name))
                json_object_object_add(node, "rust_thread_field", json_object_new_boolean(true));
    }
    if (json_string_property_equals(node, "kind", "struct_literal")) {
        json_object *found = NULL;
        const char *name = json_string_property(node, "struct_name");
        if (name && json_object_object_get_ex(names, name, &found))
            json_object_object_add(node, "rust_thread_fields", json_object_new_boolean(true));
    }
    json_object_object_get_ex(node, "type", &type);
    if (rust_thread_receiver_type(type, names) && json_string_property_equals(node, "kind", "rust_thread_default"))
        json_object_object_add(node, "rust_thread_fields", json_object_new_boolean(true));
    if (rust_thread_receiver_type(type, names) && json_boolean_property(node, "is_ref_arg")) {
        json_object_object_del(node, "is_ref_arg");
        json_object_object_add(node, "rust_thread_aggregate_share", json_object_new_boolean(true));
    }
    const char *kind = json_string_property(node, "kind");
    const char *place_key = kind && strcmp(kind, "compound_assign") == 0 ? "target" :
        kind && (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0) ? "operand" : NULL;
    json_object *place = NULL;
    if (place_key && json_object_object_get_ex(node, place_key, &place) &&
        json_boolean_property(place, "rust_thread_field")) {
        json_object *inner = NULL, *read = NULL, *value = NULL;
        json_object_deep_copy(node, &inner, NULL);
        json_object_deep_copy(place, &read, NULL);
        rust_concurrency_string(read, "kind", "variable");
        rust_concurrency_string(read, "name", json_string_property(model, "rust_thread_field_guard"));
        json_object_object_add(read, "rust_cell_guard", json_object_new_boolean(true));
        json_object_object_del(read, "rust_thread_field");
        json_object_object_add(inner, place_key, read);
        if (json_object_object_get_ex(node, "value", &value)) {
            json_object *rhs = NULL;
            json_object_deep_copy(value, &rhs, NULL);
            rust_concurrency_string(rhs, "kind", "variable");
            rust_concurrency_string(rhs, "name", json_string_property(model, "rust_thread_field_rhs"));
            json_object_object_add(inner, "value", rhs);
        }
        rust_concurrency_string(node, "kind", "rust_receiver_mutation");
        json_object_object_add(node, "rust_field_place", json_object_get(place));
        json_object_object_add(node, "rust_field_inner", inner);
    }
    if (json_string_property_equals(node, "kind", "thread_spawn")) {
        json_object *call = NULL, *callee = NULL;
        json_object_object_get_ex(node, "call", &call);
        json_object_object_get_ex(call, "callee", &callee);
        object = NULL; type = NULL;
        json_object_object_get_ex(callee, "object", &object);
        if (object) json_object_object_get_ex(object, "type", &type);
        if (rust_thread_receiver_type(type, names)) {
            json_object *bindings = NULL, *binding = json_object_new_object(), *read = NULL;
            json_object_object_get_ex(node, "rust_spawn_bindings", &bindings);
            const char *name = json_string_property(model, "rust_thread_receiver_temp");
            rust_concurrency_string(binding, "name", name);
            json_object_object_add(binding, "value", json_object_get(object));
            json_object_object_add(binding, "rust_receiver_share", json_object_new_boolean(true));
            /* Receiver evaluates before arguments. */
            json_object *ordered = json_object_new_array();
            json_object_array_add(ordered, binding);
            for (size_t i = 0; i < json_object_array_length(bindings); i++)
                json_object_array_add(ordered, json_object_get(json_object_array_get_idx(bindings, i)));
            json_object_object_add(node, "rust_spawn_bindings", ordered);
            json_object_deep_copy(object, &read, NULL);
            rust_concurrency_string(read, "kind", "variable");
            rust_concurrency_string(read, "name", name);
            json_object_object_add(callee, "object", read);
        }
    }
}

static void rust_lower_thread_receivers(json_object *model)
{
    json_object *names = NULL, *structures = NULL;
    json_object_object_get_ex(model, "rust_thread_receiver_names", &names);
    if (!json_object_object_length(names)) return;
    const char *prefix = json_string_property(model, "rust_concurrency_prefix");
    char name[256];
    snprintf(name, sizeof(name), "%sField", prefix);
    rust_concurrency_string(model, "rust_thread_field_type", name);
    snprintf(name, sizeof(name), "%sshare", prefix);
    rust_concurrency_string(model, "rust_thread_share_method", name);
    snprintf(name, sizeof(name), "%sreceiver", prefix);
    rust_concurrency_string(model, "rust_thread_receiver_temp", name);
    snprintf(name, sizeof(name), "%sfield_guard", prefix);
    rust_concurrency_string(model, "rust_thread_field_guard", name);
    snprintf(name, sizeof(name), "%sfield_rhs", prefix);
    rust_concurrency_string(model, "rust_thread_field_rhs", name);
    json_object_object_add(model, "rust_uses_thread_fields", json_object_new_boolean(true));
    json_object_object_get_ex(model, "structs", &structures);
    for (size_t i = 0; i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i), *found = NULL;
        if (json_object_object_get_ex(names, json_string_property(structure, "name"), &found)) {
            json_object_object_add(structure, "rust_thread_fields", json_object_new_boolean(true));
            json_object *methods = NULL;
            json_object_object_get_ex(structure, "methods", &methods);
            for (size_t j = 0; methods && j < json_object_array_length(methods); j++)
                json_object_object_add(json_object_array_get_idx(methods, j), "rust_thread_receiver", json_object_new_boolean(true));
        }
    }
    rust_thread_receiver_lower(model, names, model);
}
