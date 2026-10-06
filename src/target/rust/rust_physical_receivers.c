/* Copy-only records can be accessed through a physical receiver pointer.
 * No Rust reference to the slot spans an operation on its containing array. */
static bool rust_physical_record(json_object *structure)
{
    if (!structure || json_boolean_property(structure, "is_native") ||
        json_boolean_property(structure, "pass_self_by_ref") ||
        json_boolean_property(structure, "has_heap_fields") ||
        json_boolean_property(structure, "rust_thread_fields")) return false;
    json_object *fields = rust_closure_property(structure, "fields");
    if (!fields || json_object_array_length(fields) == 0) return false;
    for (size_t i = 0; i < json_object_array_length(fields); i++)
        if (!rust_closure_scalar_type(rust_closure_property(json_object_array_get_idx(fields, i), "type"))) return false;
    return true;
}

static void rust_physical_self_nodes(json_object *node, const char *name, json_object *pointer_type)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_physical_self_nodes(json_object_array_get_idx(node, i), name, pointer_type);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "variable") && json_string_property_equals(node, "name", "self")) {
        json_object_object_add(node, "name", json_object_new_string(name));
        json_object_object_add(node, "type", json_object_get(pointer_type));
        json_object_object_add(node, "rust_deref", json_object_new_boolean(true));
    }
    /* Lowered RHS and numeric-place bodies are executable metadata too. */
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") != 0) rust_physical_self_nodes(child, name, pointer_type);
}

static void rust_physical_capacity_nodes(json_object *node, const char *record, const char *array_name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++) rust_physical_capacity_nodes(json_object_array_get_idx(node, i), record, array_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *type = rust_closure_property(node, "type");
    if (json_string_property_equals(node, "kind", "for_each") &&
        json_string_property_equals(rust_closure_property(rust_closure_property(rust_closure_property(node, "iterable"), "type"), "element_type"), "name", record)) {
        json_object_object_add(node, "rust_physical_iteration", json_object_new_boolean(true));
        json_object *iterable = rust_closure_property(node, "iterable");
        json_object_object_del(iterable, "rust_needs_clone");
        if (json_boolean_property(iterable, "rust_closure_array_parameter") ||
            json_boolean_property(iterable, "rust_array_snapshot_cell"))
            json_object_object_add(node, "rust_physical_iteration_cell", json_object_get(rust_closure_property(iterable, "name")));
    }

    if (json_string_property_equals(node, "kind", "call")) {
        json_object *callee = rust_closure_property(node, "callee");
        json_object *owner_type = rust_closure_property(rust_closure_property(callee, "object"), "type");
        json_object *arguments = rust_closure_property(node, "args");
        if (json_string_property_equals(callee, "member_name", "concat") &&
            json_string_property_equals(rust_closure_property(owner_type, "element_type"), "name", record)) {
            for (size_t i = 0; i < rust_closure_length(arguments); i++) {
                json_object *argument = json_object_array_get_idx(arguments, i);
                if (json_string_property_equals(argument, "kind", "array_literal") &&
                    rust_closure_length(rust_closure_property(argument, "elements")) == 0) {
                    json_object_object_add(argument, "type", json_object_get(owner_type));
                    json_object_object_del(argument, "rust_nullable_array_value");
                    json_object_object_add(argument, "rust_physical_array_capacity", json_object_new_boolean(true));
                }
            }
        }
    }
    if (json_string_property_equals(node, "kind", "array") &&
        json_string_property_equals(rust_closure_property(node, "element_type"), "name", record))
        json_object_object_add(node, "rust_physical_array_name", json_object_new_string(array_name));
    if ((json_string_property_equals(node, "kind", "array_literal") ||
         json_string_property_equals(node, "kind", "sized_array") ||
         json_string_property_equals(node, "kind", "array_slice")) &&
        json_string_property_equals(rust_closure_property(type, "element_type"), "name", record)) {
        json_object_object_del(node, "rust_nullable_array_value");
        json_object_object_add(node, "rust_physical_array_capacity", json_object_new_boolean(true));
    }
    json_object_object_foreach(node, key, child)
        { (void)key; rust_physical_capacity_nodes(child, record, array_name); }
}

static const char *rust_physical_method_helper(json_object *model,
    json_object *structure, json_object *method, json_object *record_type,
    json_object *helpers);

static bool rust_physical_self_calls(json_object *model, json_object *node,
    json_object *structure, json_object *record_type, json_object *helpers,
    const char *self_name, json_object *pointer)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_physical_self_calls(model, json_object_array_get_idx(node, i),
                    structure, record_type, helpers, self_name, pointer)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") != 0 && !rust_physical_self_calls(model, child,
                structure, record_type, helpers, self_name, pointer)) return false;
    if (!json_string_property_equals(node, "kind", "call")) return true;
    json_object *callee = rust_closure_property(node, "callee");
    json_object *receiver = rust_closure_property(callee, "object");
    if (!json_string_property_equals(receiver, "kind", "variable") ||
        !json_string_property_equals(receiver, "name", self_name)) return true;
    json_object *method = rust_find_resolved_method(structure,
        json_string_property(callee, "member_name"), false);
    if (!method) return true;
    const char *helper = rust_physical_method_helper(model, structure, method,
        record_type, helpers);
    if (!helper) return false;
    json_object *args = json_object_new_array(), *self = json_object_new_object();
    json_object_object_add(self, "kind", json_object_new_string("variable"));
    json_object_object_add(self, "name", json_object_new_string(self_name));
    json_object_object_add(self, "type", json_object_get(pointer));
    json_object_array_add(args, self);
    json_object *old_args = rust_closure_property(node, "args");
    for (size_t i = 0; i < rust_closure_length(old_args); i++)
        json_object_array_add(args, json_object_get(json_object_array_get_idx(old_args, i)));
    json_object_object_add(node, "args", args);
    json_object *function = json_object_new_object();
    json_object_object_add(function, "kind", json_object_new_string("variable"));
    json_object_object_add(function, "name", json_object_new_string(helper));
    json_object_object_add(function, "rust_direct_callee", json_object_new_boolean(true));
    json_object_object_add(node, "callee", function);
    return true;
}

static const char *rust_physical_method_helper(json_object *model,
    json_object *structure, json_object *method, json_object *record_type,
    json_object *helpers)
{
    const char *cached = json_string_property(method, "rust_physical_method_name");
    if (cached) return cached;
    char helper_name[96], self_name[96];
    if (!rust_allocate_helper_name(model, "__sn_physical_method", helper_name, sizeof(helper_name)) ||
        !rust_allocate_helper_name(model, "__sn_physical_self", self_name, sizeof(self_name))) return NULL;
    json_object *helper = NULL;
    if (json_object_deep_copy(method, &helper, NULL) != 0 || !helper) return NULL;
    /* Register before rewriting nested calls, so recursion reuses this body. */
    json_object_object_add(method, "rust_physical_method_name", json_object_new_string(helper_name));
    json_object_object_add(helper, "name", json_object_new_string(helper_name));
    json_object_object_add(helper, "rust_physical_receiver_body", json_object_new_boolean(true));
    json_object *pointer = json_object_new_object();
    json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
    json_object_object_add(pointer, "base_type", json_object_get(record_type));
    json_object *params = json_object_new_array(), *self = json_object_new_object();
    json_object_object_add(self, "name", json_object_new_string(self_name));
    json_object_object_add(self, "type", json_object_get(pointer));
    json_object_object_add(self, "mem_qual", json_object_new_string("default"));
    json_object_array_add(params, self);
    json_object *old_params = rust_closure_property(helper, "params");
    for (size_t i = 0; i < rust_closure_length(old_params); i++)
        json_object_array_add(params, json_object_get(json_object_array_get_idx(old_params, i)));
    json_object_object_add(helper, "params", params);
    json_object_array_add(helpers, helper);
    json_object *body = rust_closure_property(helper, "body");
    rust_physical_self_nodes(body, self_name, pointer);
    bool ok = rust_physical_self_calls(model, body, structure, record_type, helpers, self_name, pointer);
    rust_physical_capacity_nodes(helper, json_string_property(record_type, "name"),
        json_string_property(model, "rust_physical_array_name"));
    json_object_put(pointer);
    return ok ? json_string_property(method, "rust_physical_method_name") : NULL;
}

static bool rust_physical_receiver_calls(json_object *model, json_object *node, json_object *helpers)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_physical_receiver_calls(model, json_object_array_get_idx(node, i), helpers)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    /* Dispatch renders its two prepared branches, not the original call.
     * Only visit these executable metadata nodes; types and cached places
     * are shared model data and must not generate duplicate helpers. */
    if (json_boolean_property(node, "rust_dynamic_array_alias_call"))
        return rust_physical_receiver_calls(model,
                   rust_closure_property(node, "rust_array_alias_branch"), helpers) &&
               rust_physical_receiver_calls(model,
                   rust_closure_property(node, "rust_array_alias_distinct_branch"), helpers);
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && !rust_physical_receiver_calls(model, child, helpers)) return false;
    if (!json_string_property_equals(node, "kind", "call") ||
        json_string_property(node, "rust_closure_array_reference_receiver")) return true;
    json_object *callee = rust_closure_property(node, "callee"), *receiver = rust_closure_property(callee, "object");
    json_object *type = rust_closure_property(receiver, "type");
    if (!json_string_property_equals(receiver, "kind", "array_access") ||
        !json_string_property_equals(type, "kind", "struct")) return true;
    json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
    if (!rust_physical_record(structure)) return true;
    json_object *method = rust_find_resolved_method(structure, json_string_property(callee, "member_name"), false);
    if (!method) return true;
    if (!json_string_property(node, "rust_closure_array_method_guard")) {
        size_t owner_id = 0, index_id = 0;
        if (!rust_capture_array_join_place_owner(model, node, receiver, &owner_id)) return false;
        if (!json_string_property(receiver, "rust_array_join_index_name") &&
            !rust_assign_array_join_place_index_names(model, receiver, &index_id)) return false;
        json_object_object_add(node, "rust_physical_plain_receiver", json_object_new_boolean(true));
    }
    const char *helper_name = rust_physical_method_helper(model, structure, method, type, helpers);
    char receiver_name[96];
    if (!helper_name || !rust_allocate_helper_name(model, "__sn_physical_receiver", receiver_name, sizeof(receiver_name))) return false;
    json_object *pointer = json_object_new_object();
    json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
    json_object_object_add(pointer, "base_type", json_object_get(type));
    json_object *args = json_object_new_array(), *ref = json_object_new_object();
    json_object_object_add(ref, "kind", json_object_new_string("variable"));
    json_object_object_add(ref, "name", json_object_new_string(receiver_name));
    json_object_object_add(ref, "type", json_object_get(pointer));
    json_object_array_add(args, ref);
    json_object *original_args = rust_closure_property(node, "args");
    for (size_t i = 0; i < json_object_array_length(original_args); i++) json_object_array_add(args, json_object_get(json_object_array_get_idx(original_args, i)));
    json_object_object_add(node, "args", args);
    json_object *function = json_object_new_object();
    json_object_object_add(function, "kind", json_object_new_string("variable"));
    json_object_object_add(function, "name", json_object_new_string(helper_name));
    json_object_object_add(function, "rust_direct_callee", json_object_new_boolean(true));
    json_object_object_add(node, "rust_physical_receiver_place", json_object_get(receiver));
    json_object_object_add(node, "callee", function);
    json_object_object_add(node, "rust_physical_receiver_name", json_object_new_string(receiver_name));
    rust_physical_capacity_nodes(model, json_string_property(type, "name"),
        json_string_property(model, "rust_physical_array_name"));
    json_object_put(pointer);
    return true;
}

static bool rust_physical_iteration_names(json_object *model, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_physical_iteration_names(model, json_object_array_get_idx(node, i))) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_boolean_property(node, "rust_physical_iteration") &&
        !json_string_property(node, "rust_physical_iteration_binding")) {
        char name[96];
        if (!rust_allocate_helper_name(model, "__sn_physical_iter", name, sizeof(name))) return false;
        json_object_object_add(node, "rust_physical_iteration_binding", json_object_new_string(name));
    }
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") != 0 && !rust_physical_iteration_names(model, child)) return false;
    return true;
}

static bool rust_lower_physical_receivers(json_object *model)
{
    char array_name[96];
    if (!rust_allocate_helper_name(model, "__SnPhysicalArray", array_name, sizeof(array_name))) return false;
    json_object_object_add(model, "rust_physical_array_name", json_object_new_string(array_name));
    const char *roles[] = {"__SnPhysicalHeader", "__SnPhysicalIterator", "__sn_physical_iteration"};
    const char *keys[] = {"rust_physical_header_name", "rust_physical_iterator_name", "rust_physical_iteration_method"};
    for (size_t i = 0; i < 3; i++) {
        char name[96];
        if (!rust_allocate_helper_name(model, roles[i], name, sizeof(name))) return false;
        json_object_object_add(model, keys[i], json_object_new_string(name));
    }
    json_object *helpers = json_object_new_array();
    if (!rust_physical_receiver_calls(model, model, helpers)) { json_object_put(helpers); return false; }
    if (json_object_array_length(helpers)) {
        char capacity[96];
        if (!rust_allocate_helper_name(model, "__sn_physical_capacity", capacity, sizeof(capacity))) return false;
        json_object_object_add(model, "rust_physical_capacity_function", json_object_new_string(capacity));
        json_object *functions = rust_closure_property(model, "functions");
        for (size_t i = 0; i < json_object_array_length(helpers); i++) json_object_array_add(functions, json_object_get(json_object_array_get_idx(helpers, i)));
    }
    if (!json_object_array_length(helpers)) json_object_object_del(model, "rust_physical_array_name");
    json_object_put(helpers);
    return rust_physical_iteration_names(model, model);
}
