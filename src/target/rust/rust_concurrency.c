/* Private concurrency projection. No source-language or C-model mutation. */
static void rust_concurrency_string(json_object *node, const char *key, const char *value)
{
    json_object_object_add(node, key, json_object_new_string(value));
}

static bool rust_concurrency_prefix_used(json_object *node, const char *prefix)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_string))
        return strncmp(json_object_get_string(node), prefix, strlen(prefix)) == 0;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (rust_concurrency_prefix_used(json_object_array_get_idx(node, i), prefix)) return true;
    } else if (json_object_is_type(node, json_type_object)) {
        json_object_object_foreach(node, key, value) {
            (void)key;
            if (rust_concurrency_prefix_used(value, prefix)) return true;
        }
    }
    return false;
}

/* Thread result slots have the tagged zero value before a join. Build the
 * value privately; deriving Default would use field initializers differently. */
static json_object *rust_concurrency_default(json_object *type, json_object *model)
{
    json_object *value = json_object_new_object();
    rust_concurrency_string(value, "kind", "rust_thread_default");
    json_object_object_add(value, "type", json_object_get(type));
    if (json_string_property_equals(type, "kind", "struct")) {
        json_object *structures = NULL;
        json_object_object_get_ex(model, "structs", &structures);
        for (size_t i = 0; structures && i < json_object_array_length(structures); i++) {
            json_object *structure = json_object_array_get_idx(structures, i);
            if (!json_string_property_equals(structure, "name", json_string_property(type, "name"))) continue;
            json_object *fields = NULL, *defaults = json_object_new_array();
            json_object_object_get_ex(structure, "fields", &fields);
            for (size_t j = 0; fields && j < json_object_array_length(fields); j++) {
                json_object *field = json_object_array_get_idx(fields, j), *field_type = NULL;
                json_object_object_get_ex(field, "type", &field_type);
                json_object *entry = json_object_new_object();
                rust_concurrency_string(entry, "name", json_string_property(field, "name"));
                json_object_object_add(entry, "value", rust_concurrency_default(field_type, model));
                json_object_array_add(defaults, entry);
            }
            json_object_object_add(value, "fields", defaults);
            break;
        }
    }
    return value;
}

/* Capture call operands before acquiring a cell guard or starting a worker. */
static json_object *rust_concurrency_bind_args(json_object *call, const char *prefix)
{
    json_object *args = NULL, *bindings = json_object_new_array();
    json_object_object_get_ex(call, "args", &args);
    for (size_t i = 0; args && i < json_object_array_length(args); i++) {
        json_object *arg = json_object_array_get_idx(args, i), *read = NULL;
        char name[256];
        snprintf(name, sizeof(name), "%sarg%zu", prefix, i);
        json_object *binding = json_object_new_object();
        rust_concurrency_string(binding, "name", name);
        json_object_object_add(binding, "value", json_object_get(arg));
        json_object_array_add(bindings, binding);
        /* Preserve call-site ownership and conversion metadata. */
        json_object_deep_copy(arg, &read, NULL);
        rust_concurrency_string(read, "kind", "variable");
        rust_concurrency_string(read, "name", name);
        json_object_object_del(read, "rust_cell");
        json_object_object_del(read, "rust_global");
        json_object_array_put_idx(args, i, read);
    }
    return bindings;
}

static void rust_concurrency_annotate(json_object *node, const char *prefix,
                                     const char *join_name, json_object *model)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_concurrency_annotate(json_object_array_get_idx(node, i), prefix, join_name, model);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value) {
        (void)key;
        rust_concurrency_annotate(value, prefix, join_name, model);
    }
    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    const char *temps[] = {"value", "rhs", "previous", "handle", "gate"};
    for (size_t i = 0; i < sizeof(temps) / sizeof(temps[0]); i++) {
        char key[64], name[192];
        snprintf(key, sizeof(key), "rust_temp_%s", temps[i]);
        snprintf(name, sizeof(name), "%s%s", prefix, temps[i]);
        rust_concurrency_string(node, key, name);
    }
    if (json_boolean_property(node, "rust_global")) {
        const char *key = strcmp(kind, "assign") == 0 ? "target" : "name";
        const char *original = json_string_property(node, key);
        if (original && strncmp(original, prefix, strlen(prefix)) != 0) {
            char name[1024]; snprintf(name, sizeof(name), "%sglobal_%s", prefix, original);
            rust_concurrency_string(node, key, name);
        }
    }
    char cell_type[160], guard_name[160];
    snprintf(cell_type, sizeof(cell_type), "%sCell", prefix);
    snprintf(guard_name, sizeof(guard_name), "%slock_guard", prefix);
    if (strcmp(kind, "var_decl") == 0) rust_concurrency_string(node, "rust_cell_type", cell_type);
    if (strcmp(kind, "lock") == 0) rust_concurrency_string(node, "rust_lock_guard", guard_name);
    json_object *child = NULL;
    if (strcmp(kind, "thread_spawn") == 0) {
        rust_concurrency_string(node, "rust_join_type", join_name);
        json_object *call = NULL;
        if (json_object_object_get_ex(node, "call", &call))
            json_object_object_add(node, "rust_spawn_bindings", rust_concurrency_bind_args(call, prefix));
    }
    if (rust_is_mutating_array_call(node)) {
        json_object *callee = NULL, *object = NULL;
        json_object_object_get_ex(node, "callee", &callee);
        if (callee && json_object_object_get_ex(callee, "object", &object) &&
            json_boolean_property(object, "rust_cell")) {
            rust_concurrency_string(node, "rust_cell_owner", json_string_property(object, "name"));
            char guard[256]; snprintf(guard, sizeof(guard), "%svalue_guard", prefix);
            rust_concurrency_string(node, "rust_value_guard", guard);
            rust_concurrency_string(object, "name", guard);
            json_object_object_del(object, "rust_cell");
            json_object_object_add(object, "rust_cell_guard", json_object_new_boolean(true));
            json_object_object_add(node, "rust_cell_bindings", rust_concurrency_bind_args(node, prefix));
        }
    }

    if (strcmp(kind, "index_assign") == 0 || strcmp(kind, "member_assign") == 0) {
        const char *place_key = strcmp(kind, "index_assign") == 0 ? "array" : "object";
        json_object *place = NULL;
        if (json_object_object_get_ex(node, place_key, &place) && json_boolean_property(place, "rust_cell")) {
            json_object *inner = NULL, *bindings = json_object_new_array();
            json_object_deep_copy(node, &inner, NULL);
            rust_concurrency_string(node, "rust_cell_owner", json_string_property(place, "name"));
            char guard[256]; snprintf(guard, sizeof(guard), "%svalue_guard", prefix);
            rust_concurrency_string(node, "rust_value_guard", guard);
            json_object *inner_place = NULL;
            json_object_object_get_ex(inner, place_key, &inner_place);
            json_object_object_del(inner_place, "rust_cell");
            json_object_object_del(inner_place, "rust_global");
            json_object_object_add(inner_place, "rust_cell_guard", json_object_new_boolean(true));
            rust_concurrency_string(inner_place, "name", guard);
            const char *keys[] = {"index", "value"};
            for (size_t i = 0; i < 2; i++) {
                json_object *operand = NULL, *read = NULL;
                if (!json_object_object_get_ex(inner, keys[i], &operand)) continue;
                char name[256]; snprintf(name, sizeof(name), "%soperand_%s", prefix, keys[i]);
                json_object *binding = json_object_new_object();
                rust_concurrency_string(binding, "name", name);
                json_object_object_add(binding, "value", json_object_get(operand));
                json_object_array_add(bindings, binding);
                json_object_deep_copy(operand, &read, NULL);
                rust_concurrency_string(read, "kind", "variable");
                rust_concurrency_string(read, "name", name);
                json_object_object_del(read, "rust_cell");
                json_object_object_del(read, "rust_global");
                json_object_object_add(inner, keys[i], read);
            }
            rust_concurrency_string(node, "kind", "rust_cell_operation");
            kind = json_string_property(node, "kind");
            json_object_object_add(node, "rust_cell_bindings", bindings);
            json_object_object_add(node, "rust_cell_inner", inner);
        }
    }
    if (strcmp(kind, "var_decl") == 0 &&
        (json_boolean_property(node, "is_thread_handle") || json_boolean_property(node, "needs_thread_handle"))) {
        char name[512];
        snprintf(name, sizeof(name), "%shandle_%s", prefix, json_string_property(node, "name"));
        rust_concurrency_string(node, "rust_thread_handle", name);
        rust_concurrency_string(node, "rust_join_type", join_name);
        json_object *type = NULL;
        json_object_object_get_ex(node, "type", &type);
        json_object_object_add(node, "rust_thread_default", rust_concurrency_default(type, model));
    }
    if (strcmp(kind, "assign") == 0 && json_object_object_get_ex(node, "value", &child) &&
        json_string_property_equals(child, "kind", "thread_spawn")) {
        char name[512];
        snprintf(name, sizeof(name), "%shandle_%s", prefix, json_string_property(node, "target"));
        rust_concurrency_string(node, "rust_thread_handle", name);
    }
    if ((strcmp(kind, "thread_sync") == 0 || strcmp(kind, "thread_detach") == 0) &&
        json_object_object_get_ex(node, "handle", &child)) {
        const char *name = json_string_property(child, "name");
        if (name) {
            char handle[512];
            snprintf(handle, sizeof(handle), "%shandle_%s", prefix, name);
            rust_concurrency_string(node, "rust_thread_handle", handle);
        }
        if (json_string_property_equals(child, "kind", "sync_list")) {
            json_object *elements = NULL;
            if (json_object_object_get_ex(child, "elements", &elements))
                for (size_t i = 0; i < json_object_array_length(elements); i++) {
                    json_object *element = json_object_array_get_idx(elements, i);
                    char handle[512];
                    snprintf(handle, sizeof(handle), "%shandle_%s", prefix, json_string_property(element, "name"));
                    rust_concurrency_string(element, "rust_thread_handle", handle);
                }
        }
    }
}

static void rust_lower_concurrency(json_object *model)
{
    char prefix[128], join_name[160];
    unsigned int index = 0;
    do { snprintf(prefix, sizeof(prefix), "__sn_concurrency%u_", index++); }
    while (rust_concurrency_prefix_used(model, prefix));
    snprintf(join_name, sizeof(join_name), "%sJoin", prefix);
    json_object *declared_globals = NULL;
    if (json_object_object_get_ex(model, "globals", &declared_globals))
        for (size_t i = 0; i < json_object_array_length(declared_globals); i++)
            json_object_object_add(json_object_array_get_idx(declared_globals, i),
                "rust_global", json_object_new_boolean(true));
    rust_concurrency_annotate(model, prefix, join_name, model);
    rust_concurrency_string(model, "rust_join_type", join_name);
    char cell_type[160];
    snprintf(cell_type, sizeof(cell_type), "%sCell", prefix);
    rust_concurrency_string(model, "rust_cell_type", cell_type);
    /* Emitting support is conditional, preserving existing non-concurrency snapshots. */
    json_object_object_add(model, "rust_uses_cells", json_object_new_boolean(
        !array_is_empty(model, "globals") || rust_model_contains_string(model, "atomic")));
    json_object_object_add(model, "rust_uses_threads",
        json_object_new_boolean(!array_is_empty(model, "threads")));
    json_object *globals = NULL, *functions = NULL;
    json_object_object_get_ex(model, "globals", &globals);
    json_object_object_get_ex(model, "functions", &functions);
    if (functions && globals && json_object_array_length(globals))
        for (size_t i = 0; i < json_object_array_length(functions); i++) {
            json_object *function = json_object_array_get_idx(functions, i);
            if (json_string_property_equals(function, "name", "main"))
                json_object_object_add(function, "rust_initialize_globals", json_object_get(globals));
        }
}
