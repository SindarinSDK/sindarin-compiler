/* Shared aggregate storage. Value copies snapshot; reference/receiver
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

static void rust_qualified_record_assignments(json_object *model, json_object *node, json_object *params,
                                              json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_qualified_record_assignments(model, json_object_array_get_idx(node, i), params, names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *own_params = NULL;
    if (json_object_object_get_ex(node, "params", &own_params)) params = own_params;
    if (json_string_property_equals(node, "kind", "assign") && json_boolean_property(node, "is_captured"))
        for (size_t i = 0; params && i < json_object_array_length(params); i++)
        {
            json_object *param = json_object_array_get_idx(params, i), *type = NULL;
            json_object_object_get_ex(param, "type", &type);
            if (json_string_property_equals(param, "name", json_string_property(node, "target")) &&
                json_string_property_equals(param, "mem_qual", "as_ref") &&
                rust_thread_receiver_type(type, names))
            {
                json_object_object_add(node, "rust_record_reference_assign", json_object_new_boolean(true));
                json_object_object_add(model, "rust_uses_record_reference_assign", json_object_new_boolean(true));
            }
        }
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        if (strncmp(key, "rust_", 5) != 0) rust_qualified_record_assignments(model, child, params, names);
    }
}

static void rust_thread_receiver_prepare_nodes(json_object *model, json_object *node, json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_receiver_prepare_nodes(model, json_object_array_get_idx(node, i), names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    if (rust_thread_receiver_type(type, names) && json_string_property(node, "mem_qual"))
        json_object_object_add(node, "rust_thread_aggregate_param", json_object_new_boolean(true));
    json_object *object = NULL, *object_type = NULL;
    json_object_object_get_ex(node, "object", &object);
    json_object_object_get_ex(object, "type", &object_type);
    if (json_string_property_equals(node, "kind", "member") && rust_thread_receiver_type(object_type, names))
    {
        json_object *structure = rust_find_struct(model, json_string_property(rust_thread_receiver_base(object_type), "name")), *fields = NULL;
        json_object_object_get_ex(structure, "fields", &fields);
        for (size_t i = 0; fields && i < json_object_array_length(fields); i++)
            if (json_string_property_equals(json_object_array_get_idx(fields, i), "name", json_string_property(node, "member_name")))
            {
                json_object_object_add(node, "rust_thread_field", json_object_new_boolean(true));
                if (json_string_property_equals(structure, "mem_mode", "ref"))
                    json_object_object_add(node, "rust_reference_field", json_object_new_boolean(true));
            }
    }
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_receiver_prepare_nodes(model, value, names);
    }
    json_object *place = NULL;
    const char *kind = json_string_property(node, "kind");
    const char *place_key = kind && strcmp(kind, "compound_assign") == 0 ? "target" :
        kind && (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0) ? "operand" : NULL;
    if (place_key && json_object_object_get_ex(node, place_key, &place) &&
        json_boolean_property(place, "rust_thread_field") &&
        json_string_property_equals(node, "mutation_storage", "parameter") &&
        json_string_property_equals(node, "mutation_arithmetic_mode", "unchecked"))
        json_object_object_add(node, "mutation_place", json_object_new_string("computed"));
}

static void rust_thread_field_store_places(json_object *place)
{
    if (!place) return;
    json_object_object_del(place, "rust_needs_clone");
    if (json_boolean_property(place, "rust_thread_field"))
        json_object_object_add(place, "rust_thread_field_place", json_object_new_boolean(true));
    if (json_boolean_property(place, "rust_field_array_value"))
        json_object_object_add(place, "rust_field_array_place", json_object_new_boolean(true));
    if (json_string_property_equals(place, "kind", "array_access"))
        json_object_object_add(place, "rust_cleanup_length_read", json_object_new_boolean(true));
    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "array_access"))
        json_object_object_get_ex(place, "array", &parent);
    else if (json_string_property_equals(place, "kind", "member"))
        json_object_object_get_ex(place, "object", &parent);
    rust_thread_field_store_places(parent);
}

static void rust_prepare_thread_receivers(json_object *model)
{
    json_object *names = json_object_new_object();
    rust_thread_receiver_select(model, names);
    /* Qualified owned value records need shared field owners for reference
     * calls, including repeated aliases, and independent owners for copies.
     * The same storage already supplies that contract to threaded receivers. */
    json_object *declarations = json_object_new_array();
    json_object *functions = NULL, *declared_structs = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
        json_object_array_add(declarations, json_object_get(json_object_array_get_idx(functions, i)));
    json_object_object_get_ex(model, "structs", &declared_structs);
    for (size_t i = 0; declared_structs && i < json_object_array_length(declared_structs); i++)
    {
        json_object *methods = NULL;
        json_object_object_get_ex(json_object_array_get_idx(declared_structs, i), "methods", &methods);
        for (size_t j = 0; methods && j < json_object_array_length(methods); j++)
            json_object_array_add(declarations, json_object_get(json_object_array_get_idx(methods, j)));
    }
    for (size_t i = 0; i < json_object_array_length(declarations); i++)
    {
        json_object *declaration = json_object_array_get_idx(declarations, i), *params = NULL;
        if (json_boolean_property(declaration, "is_native")) continue;
        json_object_object_get_ex(declaration, "params", &params);
        for (size_t j = 0; params && j < json_object_array_length(params); j++)
        {
            json_object *param = json_object_array_get_idx(params, j), *type = NULL;
            if (!json_string_property_equals(param, "mem_qual", "as_ref") &&
                !json_string_property_equals(param, "mem_qual", "as_val")) continue;
            json_object_object_get_ex(param, "type", &type);
            if (!json_string_property_equals(type, "kind", "struct")) continue;
            const char *name = json_string_property(type, "name");
            json_object *structure = rust_find_struct(model, name);
            if (!structure || !json_string_property_equals(structure, "mem_mode", "val") ||
                !json_boolean_property(structure, "has_heap_fields") ||
                json_boolean_property(structure, "is_native") ||
                json_boolean_property(structure, "is_packed") ||
                json_boolean_property(structure, "is_serializable") ||
                json_boolean_property(structure, "has_user_copy_method")) continue;
            json_object_object_add(names, name, json_object_new_boolean(true));
            json_object_object_add(structure, "rust_qualified_record_fields", json_object_new_boolean(true));
        }
    }
    json_object_put(declarations);
    /* Ordinary reference records retain shared field owners through local
     * aliases, container reads and return edges, just like thread transfers. */
    for (size_t i = 0; declared_structs && i < json_object_array_length(declared_structs); i++)
    {
        json_object *structure = json_object_array_get_idx(declared_structs, i);
        const char *name = json_string_property(structure, "name");
        if (name && json_string_property_equals(structure, "mem_mode", "ref") &&
            !json_boolean_property(structure, "is_native") &&
            !json_boolean_property(structure, "is_packed") &&
            !json_boolean_property(structure, "is_serializable") &&
            !json_boolean_property(structure, "has_user_copy_method"))
            json_object_object_add(names, name, json_object_new_boolean(true));
    }
    json_object_object_add(model, "rust_thread_receiver_names", names);
    rust_thread_receiver_prepare_nodes(model, model, names);
    rust_qualified_record_assignments(model, model, NULL, names);
    json_object *structures = NULL;
    json_object_object_get_ex(model, "structs", &structures);
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i), *found = NULL;
        if (json_object_object_get_ex(names, json_string_property(structure, "name"), &found)) {
            json_object_object_add(structure, "rust_thread_fields", json_object_new_boolean(true));
            if (json_string_property_equals(structure, "mem_mode", "ref"))
            {
                json_object_object_add(structure, "rust_thread_reference_identity", json_object_new_boolean(true));
                json_object_object_add(model, "rust_uses_reference_records", json_object_new_boolean(true));
            }
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
            {
                json_object_object_add(node, "rust_thread_field", json_object_new_boolean(true));
                if (json_boolean_property(structure, "rust_thread_reference_identity"))
                    json_object_object_add(node, "rust_reference_field", json_object_new_boolean(true));
                /* A field read snapshots the field, not its containing owner. */
                if (json_string_property_equals(object, "kind", "variable"))
                    json_object_object_del(object, "rust_needs_clone");
            }
    }
    if (json_string_property_equals(node, "kind", "struct_literal")) {
        json_object *found = NULL;
        const char *name = json_string_property(node, "struct_name");
        if (name && json_object_object_get_ex(names, name, &found))
        {
            json_object_object_add(node, "rust_thread_fields", json_object_new_boolean(true));
            json_object *structure = rust_find_struct(model, name);
            if (json_boolean_property(structure, "rust_thread_reference_identity"))
                json_object_object_add(node, "rust_thread_reference_identity", json_object_new_boolean(true));
        }
    }
    json_object_object_get_ex(node, "type", &type);
    if (rust_thread_receiver_type(type, names))
    {
        json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
        if (json_boolean_property(structure, "rust_thread_reference_identity") &&
            json_boolean_property(node, "is_copy_arg"))
        {
            json_object_object_add(node, "rust_record_value_copy", json_object_new_boolean(true));
            json_object_object_del(node, "is_copy_arg");
            json_object_object_del(node, "rust_needs_clone");
            json_object_object_del(node, "rust_resolved_clone");
        }
    }
    if (rust_thread_receiver_type(type, names) && json_string_property_equals(node, "kind", "rust_thread_default"))
    {
        json_object_object_add(node, "rust_thread_fields", json_object_new_boolean(true));
        json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
        if (json_boolean_property(structure, "rust_thread_reference_identity"))
            json_object_object_add(node, "rust_thread_reference_identity", json_object_new_boolean(true));
    }
    if (rust_thread_receiver_type(type, names) &&
        (json_boolean_property(node, "is_ref_arg") || json_boolean_property(node, "is_borrow_tmp"))) {
        json_object_object_del(node, "is_ref_arg");
        json_object_object_del(node, "is_borrow_tmp");
        json_object_object_del(node, "rust_needs_clone");
        json_object_object_del(node, "rust_resolved_clone");
        json_object_object_del(node, "is_copy_arg");
        json_object_object_add(node, "rust_thread_aggregate_share", json_object_new_boolean(true));
        if (json_string_property_equals(node, "kind", "member") ||
            json_string_property_equals(node, "kind", "array_access"))
        {
            json_object *store = NULL, *indices = json_object_new_array(), *owners = json_object_new_array();
            json_object_deep_copy(node, &store, NULL);
            json_object_object_del(store, "rust_thread_aggregate_share");
            size_t next_id = 0;
            /* Resolve callbacks and negative-index lengths before borrowing
             * the original owners. The final projection contains no call. */
            if (!rust_numeric_place_owners(model, store, owners) ||
                !rust_collect_place_indices_mode(model, store, indices, &next_id, true))
            {
                json_object_put(store); json_object_put(indices); json_object_put(owners);
                return;
            }
            rust_thread_field_store_places(store);
            json_object_object_add(node, "rust_record_reference_store", store);
            json_object_object_add(node, "rust_record_reference_indices", indices);
            json_object_object_add(node, "rust_record_reference_owners", owners);
        }
    }
    const char *kind = json_string_property(node, "kind");
    if (kind && (strcmp(kind, "call") == 0 || strcmp(kind, "static_call") == 0 || strcmp(kind, "method_call") == 0))
    {
        json_object *call_args = NULL;
        json_object_object_get_ex(node, "args", &call_args);
        for (size_t i = 0; call_args && i < json_object_array_length(call_args); i++)
        {
            json_object *arg = json_object_array_get_idx(call_args, i), *arg_type = NULL;
            json_object_object_get_ex(arg, "type", &arg_type);
            if (rust_thread_receiver_type(arg_type, names) &&
                !json_boolean_property(arg, "rust_thread_aggregate_share") &&
                (json_string_property_equals(arg, "kind", "variable") ||
                 json_string_property_equals(arg, "kind", "member") ||
                 json_string_property_equals(arg, "kind", "array_access")))
                json_object_object_add(arg, "rust_needs_clone", json_object_new_boolean(true));
        }
    }
    if (json_boolean_property(node, "rust_field_nested_assignment"))
        rust_thread_field_store_places(object);
    if (kind && strcmp(kind, "index_assign") == 0)
    {
        json_object *array = NULL;
        json_object_object_get_ex(node, "array", &array);
        if (rust_field_array_owner(array))
            json_object_object_add(node, "rust_field_index_assign", json_object_new_boolean(true));
        json_object *store = NULL;
        if (json_object_object_get_ex(node, "rust_nullable_array_store", &store))
            rust_thread_field_store_places(store);
    }
    json_object *numeric_store = NULL;
    if (json_object_object_get_ex(node, "rust_numeric_store", &numeric_store))
        rust_thread_field_store_places(numeric_store);
    json_object *callee = NULL, *args = NULL;
    if (kind && strcmp(kind, "call") == 0 &&
        json_object_object_get_ex(node, "callee", &callee))
    {
        json_object *receiver = NULL, *receiver_type = NULL;
        json_object_object_get_ex(callee, "object", &receiver);
        json_object_object_get_ex(receiver, "type", &receiver_type);
        const char *method = json_string_property(callee, "member_name");
        if (rust_field_array_owner(receiver) &&
            json_string_property_equals(receiver_type, "kind", "array") && method &&
            (strcmp(method, "push") == 0 || strcmp(method, "pop") == 0 ||
             strcmp(method, "insert") == 0 || strcmp(method, "remove") == 0 ||
             strcmp(method, "reverse") == 0 || strcmp(method, "clear") == 0))
        {
            json_object_object_add(node, "rust_field_array_mutation", json_object_new_boolean(true));
            json_object_object_get_ex(node, "args", &args);
            const char *prefix = json_string_property(model, "rust_concurrency_prefix");
            for (size_t i = 0; args && i < json_object_array_length(args); i++)
            {
                char name[256];
                snprintf(name, sizeof(name), "%sfield_arg%zu", prefix, i);
                rust_concurrency_string(json_object_array_get_idx(args, i), "rust_field_arg_name", name);
            }
        }
    }
    const char *place_key = kind && strcmp(kind, "compound_assign") == 0 ? "target" :
        kind && (strcmp(kind, "increment") == 0 || strcmp(kind, "decrement") == 0) ? "operand" : NULL;
    json_object *place = NULL;
    if (place_key && !json_boolean_property(node, "rust_numeric_computed_mutation") &&
        json_object_object_get_ex(node, place_key, &place) &&
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
    snprintf(name, sizeof(name), "%sReferenceField", prefix);
    rust_concurrency_string(model, "rust_reference_field_type", name);
    snprintf(name, sizeof(name), "%snil", prefix);
    rust_concurrency_string(model, "rust_record_nil_method", name);
    snprintf(name, sizeof(name), "%srecord_identity", prefix);
    rust_concurrency_string(model, "rust_record_identity_field", name);
    snprintf(name, sizeof(name), "%ssnapshot", prefix);
    rust_concurrency_string(model, "rust_record_snapshot_method", name);
    snprintf(name, sizeof(name), "%sassign", prefix);
    rust_concurrency_string(model, "rust_record_assign_method", name);
    snprintf(name, sizeof(name), "%sreceiver", prefix);
    rust_concurrency_string(model, "rust_thread_receiver_temp", name);
    snprintf(name, sizeof(name), "%sfield_guard", prefix);
    rust_concurrency_string(model, "rust_thread_field_guard", name);
    snprintf(name, sizeof(name), "%sfield_rhs", prefix);
    rust_concurrency_string(model, "rust_thread_field_rhs", name);
    snprintf(name, sizeof(name), "%sfield_index", prefix);
    rust_concurrency_string(model, "rust_thread_field_index", name);
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
