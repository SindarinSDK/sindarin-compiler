/* Copy-only reference-record arguments use the existing physical place
 * protocol. Helpers keep raw views within the call instead of constructing
 * simultaneous exclusive Rust references to aliases of the same C place. */
static bool rust_reference_record_parameter(json_object *model, json_object *parameter)
{
    json_object *type = rust_closure_property(parameter, "type");
    json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
    return json_string_property_equals(parameter, "mem_qual", "as_ref") &&
        !json_boolean_property(parameter, "rust_native_handle_borrow_param") &&
        !json_boolean_property(parameter, "rust_thread_aggregate_param") &&
        !json_boolean_property(type, "rust_native_reference_handle") &&
        !json_boolean_property(type, "rust_native_record_storage") &&
        !json_boolean_property(structure, "rust_thread_fields") &&
        rust_heap_free_value_record_parameter_type(model, type);
}

static void rust_reference_record_body(json_object *node, json_object *parameters)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_reference_record_body(json_object_array_get_idx(node, i), parameters);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "compound_assign") ||
        json_string_property_equals(node, "kind", "increment") ||
        json_string_property_equals(node, "kind", "decrement")) {
        json_object *place = rust_closure_property(node,
            json_string_property_equals(node, "kind", "compound_assign") ? "target" : "operand");
        json_object *root = rust_assignment_place_root(place);
        if (json_boolean_property(root, "rust_reference_record_parameter_use") ||
            json_boolean_property(root, "rust_reference_record_self"))
            for (size_t i = 0; i < json_object_array_length(parameters); i++) {
                json_object *parameter = json_object_array_get_idx(parameters, i);
                if (json_boolean_property(parameter, "rust_reference_record_pointer") &&
                    json_string_property_equals(parameter, "name", json_string_property(root, "name")))
                    json_object_object_add(node, "rust_reference_record_raw_store", json_object_new_boolean(true));
            }
    }
    if ((json_boolean_property(node, "rust_reference_record_parameter_use") &&
         !json_boolean_property(node, "is_ref_arg")) ||
        json_boolean_property(node, "rust_reference_record_parameter_assign")) {
        const char *name = json_string_property(node,
            json_boolean_property(node, "rust_reference_record_parameter_assign") ? "target" : "name");
        for (size_t i = 0; name && i < json_object_array_length(parameters); i++) {
            json_object *parameter = json_object_array_get_idx(parameters, i);
            if (json_boolean_property(parameter, "rust_reference_record_pointer") &&
                json_string_property_equals(parameter, "name", name))
                json_object_object_add(node,
                    json_boolean_property(node, "rust_reference_record_parameter_assign") ? "rust_deref_target" : "rust_deref",
                    json_object_new_boolean(true));
        }
    }
    /* Executable read/store projections are copies, and must be rewritten
     * with the body. Type graphs and helper caches are not executable. */
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") && strcmp(key, "rust_reference_record_helper"))
            rust_reference_record_body(child, parameters);
}

static bool rust_reference_record_calls(json_object *model, json_object *node,
    json_object *functions, json_object *helpers, bool physical_context);

static void rust_reference_record_self_marks(json_object *node, const char *name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_reference_record_self_marks(json_object_array_get_idx(node, i), name);
    } else if (json_object_is_type(node, json_type_object)) {
        if (json_string_property_equals(node, "kind", "variable") &&
            json_string_property_equals(node, "name", name))
            json_object_object_add(node, "rust_reference_record_self", json_object_new_boolean(true));
        json_object_object_foreach(node, key, child)
            if (strcmp(key, "type")) rust_reference_record_self_marks(child, name);
    }
}

static json_object *rust_reference_record_address(json_object *place, json_object *type)
{
    json_object *argument = json_object_new_object(), *pointer = json_object_new_object();
    json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
    json_object_object_add(pointer, "base_type", json_object_get(type));
    json_object_object_add(argument, "type", pointer);
    json_object_object_add(argument, "kind", json_object_new_string("rust_reference_record_address"));
    json_object_object_add(argument, "place", json_object_get(place));
    return argument;
}

static bool rust_reference_record_stable_place(json_object *place)
{
    return json_string_property_equals(place, "kind", "variable") ||
        (json_string_property_equals(place, "kind", "member") &&
         rust_reference_record_stable_place(rust_closure_property(place, "object")));
}

static const char *rust_reference_record_helper(json_object *model,
    json_object *function, json_object *record_type, json_object *functions, json_object *helpers)
{
    const char *cached = json_string_property(function, "rust_reference_record_helper");
    if (cached) return cached;
    char name[96];
    if (!rust_allocate_helper_name(model, "__sn_reference_record", name, sizeof(name))) return NULL;
    json_object *helper = NULL;
    if (json_object_deep_copy(function, &helper, NULL) || !helper) return NULL;
    json_object_object_add(function, "rust_reference_record_helper", json_object_new_string(name));
    json_object_object_add(helper, "name", json_object_new_string(name));
    json_object_object_add(helper, "rust_reference_record_helper", json_object_new_string(name));
    json_object_object_add(helper, "rust_physical_receiver_body", json_object_new_boolean(true));
    json_object *params = rust_closure_property(helper, "params");
    if (record_type && !json_boolean_property(function, "is_static")) {
        char self_name[96];
        if (!rust_allocate_helper_name(model, "__sn_reference_self", self_name, sizeof(self_name))) return NULL;
        json_object *pointer = json_object_new_object(), *self = json_object_new_object();
        json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
        json_object_object_add(pointer, "base_type", json_object_get(record_type));
        json_object_object_add(self, "name", json_object_new_string(self_name));
        json_object_object_add(self, "type", pointer);
        json_object_object_add(self, "mem_qual", json_object_new_string("default"));
        json_object_object_add(self, "rust_reference_record_pointer", json_object_new_boolean(true));
        json_object *all = json_object_new_array();
        json_object_array_add(all, self);
        for (size_t p = 0; p < json_object_array_length(params); p++)
            json_object_array_add(all, json_object_get(json_object_array_get_idx(params, p)));
        json_object_object_add(helper, "params", all);
        params = all;
        rust_physical_self_nodes(rust_closure_property(helper, "body"), self_name, pointer);
        rust_reference_record_self_marks(rust_closure_property(helper, "body"), self_name);
    }
    for (size_t i = 0; i < json_object_array_length(params); i++) {
        json_object *param = json_object_array_get_idx(params, i);
        if (!rust_reference_record_parameter(model, param)) continue;
        json_object *pointer = json_object_new_object();
        json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
        json_object_object_add(pointer, "base_type", json_object_get(rust_closure_property(param, "type")));
        json_object_object_add(param, "type", pointer);
        json_object_object_add(param, "mem_qual", json_object_new_string("default"));
        json_object_object_add(param, "rust_reference_record_pointer", json_object_new_boolean(true));
    }
    /* Register before recursion, matching physical method helper handling. */
    json_object_array_add(helpers, helper);
    json_object *body = rust_closure_property(helper, "body");
    rust_reference_record_body(body, params);
    if (!rust_reference_record_calls(model, body, functions, helpers, true)) return NULL;
    return json_string_property(function, "rust_reference_record_helper");
}

static bool rust_reference_record_calls(json_object *model, json_object *node,
    json_object *functions, json_object *helpers, bool physical_context)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_reference_record_calls(model, json_object_array_get_idx(node, i), functions, helpers, physical_context)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") && strcmp(key, "rust_reference_record_helper") &&
            !rust_reference_record_calls(model, child, functions, helpers, physical_context)) return false;
    bool call = json_string_property_equals(node, "kind", "call");
    bool method_call = json_string_property_equals(node, "kind", "method_call") ||
        json_string_property_equals(node, "kind", "static_call");
    if ((!call && !method_call) || json_boolean_property(node, "is_closure_call")) return true;
    json_object *callee = rust_closure_property(node, "callee");
    json_object *receiver = rust_closure_property(callee, "object");
    if (method_call) receiver = rust_closure_property(node, "object");
    json_object *record_type = method_call ? rust_closure_property(node, "struct_type") : rust_closure_property(receiver, "type");
    if (json_string_property_equals(record_type, "kind", "pointer"))
        record_type = rust_closure_property(record_type, "base_type");
    bool is_static = json_string_property_equals(node, "kind", "static_call") ||
        json_boolean_property(node, "is_static");
    json_object *fn = NULL;
    if (method_call || (call && json_string_property_equals(callee, "kind", "member"))) {
        const char *record = json_string_property(record_type, "name");
        if (is_static) {
            record = json_string_property(node, "type_name");
            if (!record) record = json_string_property(rust_closure_property(node, "struct_type"), "name");
        }
        json_object *structure = rust_find_struct(model, record);
        const char *method = json_string_property(method_call ? node : callee,
            method_call ? "method_name" : "member_name");
        fn = rust_find_resolved_method(structure, method, is_static);
        if (!fn || json_boolean_property(structure, "is_native") ||
            json_boolean_property(structure, "has_heap_fields") ||
            json_boolean_property(structure, "pass_self_by_ref") ||
            (!is_static && !rust_reference_record_stable_place(receiver))) return true;
        if (!record_type) record_type = rust_closure_property(node, "struct_type");
    } else {
        if (!json_string_property_equals(callee, "kind", "variable") ||
            !json_boolean_property(callee, "rust_direct_callee")) return true;
        const char *name = json_string_property(callee, "name");
        for (size_t i = 0; name && i < json_object_array_length(functions); i++) {
            json_object *candidate = json_object_array_get_idx(functions, i);
            if (json_string_property_equals(candidate, "name", name)) { fn = candidate; break; }
        }
        record_type = NULL;
    }
    if (!fn || json_boolean_property(fn, "is_native") || json_boolean_property(fn, "rust_native_bridge")) return true;
    json_object *params = rust_closure_property(fn, "params");
    bool eligible = record_type && physical_context && !is_static;
    for (size_t p = 0; (physical_context || json_boolean_property(fn, "rust_reference_record_compounds")) && p < json_object_array_length(params); p++)
        eligible |= rust_reference_record_parameter(model, json_object_array_get_idx(params, p));
    if (!eligible) return true;
    const char *helper = rust_reference_record_helper(model, fn, record_type, functions, helpers);
    if (!helper) return false;
    json_object *old_args = rust_closure_property(node, "args"), *args = json_object_new_array();
    if (record_type && !is_static)
        json_object_array_add(args, rust_reference_record_address(receiver, record_type));
    for (size_t p = 0; p < json_object_array_length(old_args); p++) {
        json_object *place = json_object_array_get_idx(old_args, p);
        json_object *param = p < json_object_array_length(params) ? json_object_array_get_idx(params, p) : NULL;
        json_object_array_add(args, param && rust_reference_record_parameter(model, param) ?
            rust_reference_record_address(place, rust_closure_property(param, "type")) : json_object_get(place));
    }
    json_object *function = json_object_new_object();
    json_object_object_add(function, "kind", json_object_new_string("variable"));
    json_object_object_add(function, "name", json_object_new_string(helper));
    json_object_object_add(function, "rust_direct_callee", json_object_new_boolean(true));
    json_object_object_add(node, "callee", function);
    json_object_object_add(node, "args", args);
    json_object_object_add(node, "kind", json_object_new_string("call"));
    return true;
}

static bool rust_lower_reference_record_parameters(json_object *model)
{
    json_object *functions = rust_closure_property(model, "functions");
    json_object *helpers = json_object_new_array();
    /* Cache slots exist before model traversal, so recursive helper creation
     * replaces a value without resizing an ancestor being iterated. */
    for (size_t i = 0; i < json_object_array_length(functions); i++)
        json_object_object_add(json_object_array_get_idx(functions, i), "rust_reference_record_helper", NULL);
    json_object *structures = rust_closure_property(model, "structs");
    for (size_t i = 0; i < json_object_array_length(structures); i++) {
        json_object *methods = rust_closure_property(json_object_array_get_idx(structures, i), "methods");
        for (size_t m = 0; m < json_object_array_length(methods); m++)
            json_object_object_add(json_object_array_get_idx(methods, m), "rust_reference_record_helper", NULL);
    }
    if (!rust_reference_record_calls(model, model, functions, helpers, false)) {
        json_object_put(helpers);return false;
    }
    for (size_t i = 0; i < json_object_array_length(helpers); i++)
        json_object_array_add(functions, json_object_get(json_object_array_get_idx(helpers, i)));
    json_object_put(helpers);
    return true;
}
