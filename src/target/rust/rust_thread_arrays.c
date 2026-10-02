/* Thread arrays keep value storage distinct from pending joins. The tagged
 * runtime puts handles in value slots; Rust needs no pointer/integer punning. */
static void rust_thread_array_collect(json_object *node, json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_array_collect(json_object_array_get_idx(node, i), names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    const char *name = NULL;
    json_object *callee = NULL, *object = NULL, *args = NULL, *handle = NULL;
    if (json_boolean_property(node, "is_array_sync") && json_object_object_get_ex(node, "handle", &handle))
        name = json_string_property(handle, "name");
    if (json_object_object_get_ex(node, "callee", &callee) &&
        json_string_property_equals(callee, "member_name", "push") &&
        json_object_object_get_ex(callee, "object", &object) &&
        json_string_property_equals(object, "kind", "variable") &&
        json_object_object_get_ex(node, "args", &args) && json_object_array_length(args) == 1 &&
        json_string_property_equals(json_object_array_get_idx(args, 0), "kind", "thread_spawn"))
        name = json_string_property(object, "name");
    if (name) json_object_object_add(names, name, json_object_new_boolean(true));
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_array_collect(value, names);
    }
}

/* Copies retain the same pending join slots, including intermediate aliases. */
static void rust_thread_array_aliases(json_object *node, json_object *names)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_array_aliases(json_object_array_get_idx(node, i), names);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *source = NULL, *found = NULL, *source_type = NULL;
    const char *target = NULL;
    if (json_string_property_equals(node, "kind", "var_decl")) {
        target = json_string_property(node, "name");
        json_object_object_get_ex(node, "initializer", &source);
    } else if (json_string_property_equals(node, "kind", "assign")) {
        target = json_string_property(node, "target");
        json_object_object_get_ex(node, "value", &source);
    }
    const char *name = json_string_property_equals(source, "kind", "variable") ?
        json_string_property(source, "name") : NULL;
    if (source) json_object_object_get_ex(source, "type", &source_type);
    if (target && name && json_string_property_equals(source_type, "kind", "array") && (json_object_object_get_ex(names, name, &found) ||
                          json_object_object_get_ex(names, target, &found))) {
        json_object_object_add(names, target, json_object_new_boolean(true));
        json_object_object_add(names, name, json_object_new_boolean(true));
    }
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_array_aliases(value, names);
    }
}

static void rust_thread_array_annotate(json_object *node, json_object *names,
                                       const char *prefix, json_object *model)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_array_annotate(json_object_array_get_idx(node, i), names, prefix, model);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_array_annotate(value, names, prefix, model);
    }
    const char *kind = json_string_property(node, "kind"), *name = NULL;
    if (!kind) return;
    json_object *type = NULL, *element = NULL, *place = NULL, *callee = NULL, *args = NULL;
    if (strcmp(kind, "var_decl") == 0) {
        json_object_object_get_ex(node, "type", &type);
        if (json_string_property_equals(type, "kind", "array") && !json_boolean_property(node, "is_thread_handle"))
            name = json_string_property(node, "name");
    } else if (strcmp(kind, "assign") == 0) {
        name = json_string_property(node, "target");
    } else if (strcmp(kind, "thread_sync") == 0) {
        json_object_object_get_ex(node, "handle", &place);
        if (json_boolean_property(node, "is_element_sync"))
            json_object_object_get_ex(place, "array", &place);
        name = json_string_property(place, "name");
    } else if (json_object_object_get_ex(node, "callee", &callee) &&
               json_string_property_equals(callee, "member_name", "push")) {
        json_object_object_get_ex(callee, "object", &place);
        name = json_string_property(place, "name");
        json_object_object_get_ex(place, "type", &type);
    }
    json_object *found = NULL;
    if (!name || !json_object_object_get_ex(names, name, &found)) return;
    char companion[1024], support[192];
    snprintf(companion, sizeof(companion), "%sarray_%s", prefix, name);
    snprintf(support, sizeof(support), "%sArrayJoins", prefix);
    rust_concurrency_string(node, "rust_thread_array_handle", companion);
    rust_concurrency_string(node, "rust_thread_array_name", name);
    rust_concurrency_string(node, "rust_thread_array_type", support);
    json_object *source = NULL;
    if (strcmp(kind, "var_decl") == 0) json_object_object_get_ex(node, "initializer", &source);
    if (strcmp(kind, "assign") == 0) json_object_object_get_ex(node, "value", &source);
    const char *source_name = json_string_property_equals(source, "kind", "variable") ?
        json_string_property(source, "name") : NULL;
    if (source_name && json_object_object_get_ex(names, source_name, &found)) {
        snprintf(companion, sizeof(companion), "%sarray_%s", prefix, source_name);
        rust_concurrency_string(node, "rust_thread_array_source", companion);
    }
    if (type && json_object_object_get_ex(type, "element_type", &element)) {
        json_object_object_add(node, "rust_thread_array_element", json_object_get(element));
        json_object_object_add(node, "rust_thread_array_default", rust_concurrency_default(element, model));
    }
    if (callee && json_object_object_get_ex(node, "args", &args) && json_object_array_length(args) == 1) {
        json_object *arg = json_object_array_get_idx(args, 0);
        json_object_object_add(node, "rust_thread_array_push", json_object_new_boolean(true));
        json_object_object_add(node, "rust_thread_array_pending",
            json_object_new_boolean(json_string_property_equals(arg, "kind", "thread_spawn")));
    }
}

static void rust_lower_thread_arrays(json_object *model)
{
    json_object *names = json_object_new_object();
    rust_thread_array_collect(model, names);
    size_t previous;
    do {
        previous = json_object_object_length(names);
        rust_thread_array_aliases(model, names);
    } while (previous != json_object_object_length(names));
    if (json_object_object_length(names)) {
        const char *prefix = json_string_property(model, "rust_concurrency_prefix");
        rust_thread_array_annotate(model, names, prefix, model);
        char support[192]; snprintf(support, sizeof(support), "%sArrayJoins", prefix);
        rust_concurrency_string(model, "rust_thread_array_type", support);
        snprintf(support, sizeof(support), "%sArrayPending", prefix);
        rust_concurrency_string(model, "rust_thread_array_pending_type", support);
        json_object_object_add(model, "rust_uses_thread_arrays", json_object_new_boolean(true));
        json_object_object_add(model, "rust_uses_threads", json_object_new_boolean(true));
    }
    json_object_put(names);
}
