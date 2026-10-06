/* All lexical/signature decisions are made by closure validation. This pass
 * only installs rendering flags; the shared C model is never changed. */
static void rust_lower_closure_node(json_object *node, bool *uses)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_closure_node(json_object_array_get_idx(node, i), uses);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *comparison = NULL;
    if (json_object_object_get_ex(node, "rust_named_function_comparison", &comparison))
    {
        json_object_object_add(node, "value", json_object_new_boolean(json_object_get_boolean(comparison)));
        json_object_object_add(node, "kind", json_object_new_string("literal"));
        json_object_object_add(node, "value_kind", json_object_new_string("bool"));
        json_object_object_del(node, "left");
        json_object_object_del(node, "right");
    }
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "lambdas") != 0) rust_lower_closure_node(value, uses);
    }
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    if (!json_string_property_equals(type, "kind", "function")) return;
    if (json_boolean_property(node, "rust_direct_callee")) return;
    *uses = true;
    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    if (strcmp(kind, "variable") == 0 &&
        !json_boolean_property(node, "rust_direct_callee") &&
        !json_boolean_property(node, "rust_named_function_value") &&
        !json_boolean_property(node, "is_ref_arg"))
        json_object_object_add(node, "rust_function_read", json_object_new_boolean(true));
    if (strcmp(kind, "assign") == 0)
        json_object_object_add(node, "rust_function_assign", json_object_new_boolean(true));
    if (strcmp(kind, "member") == 0 &&
        !json_boolean_property(node, "is_ref_arg"))
        json_object_object_add(node, "rust_needs_clone", json_object_new_boolean(true));
    if (strcmp(kind, "array_access") == 0 &&
        !json_boolean_property(node, "is_ref_arg"))
        json_object_object_add(node, "rust_function_index", json_object_new_boolean(true));
}

/* Shared model-wide name scan is defined later in rust_lower.c. */
static bool rust_model_contains_string(json_object *node, const char *wanted);

static void rust_closure_name_types(json_object *node, const char *name, const char *scalar_ref)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_closure_name_types(json_object_array_get_idx(node, i), name, scalar_ref);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "lambdas") != 0) rust_closure_name_types(value, name, scalar_ref);
    }
    if (json_string_property_equals(node, "kind", "function"))
        json_object_object_add(node, "rust_closure_handle_name", json_object_new_string(name));
    if (json_boolean_property(node, "rust_closure_scalar_reference"))
        json_object_object_add(node, "rust_closure_scalar_ref_type", json_object_new_string(scalar_ref));
}

static json_object *rust_scalar_reference_target(json_object *model, json_object *node);

static void rust_bind_closure_store_indices(json_object *place)
{
    if (!place || !json_object_is_type(place, json_type_object)) return;
    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "array_access"))
    {
        json_object *index = rust_closure_property(place, "index");
        const char *name = json_string_property(place, "rust_place_raw_index_name");
        if (name)
        {
            json_object *ref = json_object_new_object();
            json_object_object_add(ref, "kind", json_object_new_string("variable"));
            json_object_object_add(ref, "name", json_object_new_string(name));
            json_object_object_add(ref, "type", json_object_get(rust_closure_property(index, "type")));
            json_object_object_add(place, "index", ref);
        }
        parent = rust_closure_property(place, "array");
    }
    else if (json_string_property_equals(place, "kind", "member"))
        parent = rust_closure_property(place, "object");
    rust_bind_closure_store_indices(parent);
}

static void rust_lower_closure_array_arguments(json_object *model, json_object *node,
                                                unsigned int *next_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_closure_array_arguments(model, json_object_array_get_idx(node, i), next_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0)
            rust_lower_closure_array_arguments(model, child, next_id);
    if (json_string_property_equals(node, "kind", "member_assign") &&
        !json_boolean_property(node, "rust_closure_array_member_store"))
    {
        json_object *object = rust_closure_property(node, "object"), *root = object;
        while (json_string_property_equals(root, "kind", "member") ||
               json_string_property_equals(root, "kind", "array_access"))
            root = rust_closure_property(root, json_string_property_equals(root, "kind", "array_access") ? "array" : "object");
        if (json_boolean_property(root, "rust_closure_array_parameter"))
        {
            json_object *body = NULL;
            if (json_object_deep_copy(node, &body, NULL) != 0 || !body) return;
            char guard[96], value[96];
            if (!rust_allocate_helper_name(model, "__sn_closure_member_guard", guard, sizeof(guard)) ||
                !rust_allocate_helper_name(model, "__sn_closure_member_value", value, sizeof(value))) return;
            json_object *store = rust_closure_property(body, "object");
            while (json_string_property_equals(store, "kind", "member") ||
                   json_string_property_equals(store, "kind", "array_access"))
            {
                json_object_object_del(store, "rust_needs_clone");
                store = rust_closure_property(store, json_string_property_equals(store, "kind", "array_access") ? "array" : "object");
            }
            json_object_object_add(store, "rust_closure_array_store_guard", json_object_new_string(guard));
            json_object *indices = json_object_new_array();
            size_t index_id = 0;
            if (!rust_collect_place_indices(model, rust_closure_property(body, "object"), indices, &index_id)) return;
            json_object_object_add(node, "rust_closure_array_member_indices", indices);
            rust_bind_closure_store_indices(rust_closure_property(body, "object"));
            json_object *ref = json_object_new_object();
            json_object_object_add(ref, "kind", json_object_new_string("variable"));
            json_object_object_add(ref, "name", json_object_new_string(value));
            json_object_object_add(ref, "type", json_object_get(rust_closure_property(rust_closure_property(node, "value"), "type")));
            json_object_object_add(body, "value", ref);
            json_object_object_add(node, "rust_closure_array_member_store", json_object_new_boolean(true));
            json_object_object_add(node, "rust_closure_array_member_source", json_object_new_string(json_string_property(root, "name")));
            json_object_object_add(node, "rust_closure_array_member_guard", json_object_new_string(guard));
            json_object_object_add(node, "rust_closure_array_member_value", json_object_new_string(value));
            json_object_object_add(node, "rust_closure_array_member_body", body);
        }
    }
    if (json_boolean_property(node, "rust_closure_private_array_rebind"))
    {
        char value[96], place[96];
        if (!rust_allocate_helper_name(model, "__sn_array_rebind_value", value, sizeof(value)) ||
            !rust_allocate_helper_name(model, "__sn_array_rebind_place", place, sizeof(place))) return;
        json_object_object_add(node, "rust_closure_rebind_value", json_object_new_string(value));
        json_object_object_add(node, "rust_closure_rebind_place", json_object_new_string(place));
    }
    if (json_boolean_property(node, "rust_closure_array_parameter_copy"))
    {
        char owner[96];
        if (!rust_allocate_helper_name(model, "__sn_closure_array_copy", owner, sizeof(owner))) return;
        json_object_object_add(node, "rust_closure_array_copy_owner", json_object_new_string(owner));
    }
    if (json_boolean_property(node, "rust_array_cell_mutation"))
    {
        json_object *callee = rust_closure_property(node, "callee");
        const char *method = json_string_property(callee, "member_name");
        if (method && strcmp(method, "push") != 0 && strcmp(method, "pop") != 0)
        {
            json_object *body = NULL;
            if (json_object_deep_copy(node, &body, NULL) != 0 || !body) return;
            json_object_object_del(body, "rust_array_cell_mutation");
            json_object_object_del(body, "rust_array_cell_nested_mutation");
            json_object *receiver = rust_closure_property(rust_closure_property(body, "callee"), "object");
            json_object *root = receiver;
            while (json_string_property_equals(root, "kind", "array_access"))
            {
                json_object_object_del(root, "rust_needs_clone");
                root = rust_closure_property(root, "array");
            }
            char guard[96];
            if (!rust_allocate_helper_name(model, "__sn_closure_array_mutation_guard", guard, sizeof(guard))) return;
            json_object_object_add(node, "rust_array_cell_other_mutation", json_object_new_boolean(true));
            json_object_object_add(node, "rust_array_cell_other_guard", json_object_new_string(guard));
            json_object_object_add(node, "rust_array_cell_other_parameter", json_object_new_boolean(json_boolean_property(root, "rust_closure_array_parameter")));
            json_object_object_add(root, "rust_array_method_guard", json_object_new_string(guard));
            json_object_object_add(root, "rust_array_method_parameter", json_object_new_boolean(json_boolean_property(root, "rust_closure_array_parameter")));
            json_object *original_args = rust_closure_property(node, "args");
            json_object *args = json_object_new_array();
            json_object_object_add(node, "rust_array_cell_other_args", json_object_get(original_args));
            for (size_t i = 0; i < json_object_array_length(original_args); i++)
            {
                json_object *arg = json_object_array_get_idx(original_args, i);
                char name[96];
                if (!rust_allocate_helper_name(model, "__sn_closure_array_mutation_arg", name, sizeof(name))) return;
                json_object_object_add(arg, "rust_array_cell_other_arg_name", json_object_new_string(name));
                json_object *ref = json_object_new_object();
                json_object_object_add(ref, "kind", json_object_new_string("variable"));
                json_object_object_add(ref, "name", json_object_new_string(name));
                json_object_object_add(ref, "type", json_object_get(rust_closure_property(arg, "type")));
                json_object_array_add(args, ref);
            }
            json_object_object_add(body, "args", args);
            json_object_object_add(node, "rust_array_cell_other_body", body);
        }
        return;
    }
    if (json_string_property_equals(node, "kind", "call") &&
        !json_boolean_property(node, "rust_owned_record_array_mutation"))
    {
        json_object *callee = rust_closure_property(node, "callee");
        json_object *receiver = rust_closure_property(callee, "object");
        json_object *root = receiver;
        bool shared_projection = false;
        while (json_string_property_equals(root, "kind", "member") ||
               json_string_property_equals(root, "kind", "array_access"))
        {
            shared_projection |= json_boolean_property(root, "rust_thread_field");
            root = rust_closure_property(root, json_string_property_equals(root, "kind", "member") ? "object" : "array");
        }
        json_object *receiver_type = rust_closure_property(receiver, "type");
        bool record_method = json_string_property_equals(receiver_type, "kind", "struct") &&
            rust_find_resolved_method(rust_find_struct(model, json_string_property(receiver_type, "name")),
                json_string_property(callee, "member_name"), false);
        if (shared_projection && json_boolean_property(root, "rust_closure_record_value_parameter") &&
            (record_method || rust_closure_mutating_array_method(json_string_property(callee, "member_name"))))
        {
            if (record_method)
            {
                size_t alias_id = 0;
                if (!rust_specialize_receiver_array_alias_call(model, node, &alias_id) ||
                    !rust_specialize_default_array_alias_call(model, rust_closure_property(model, "functions"), node, &alias_id)) return;
                if (json_boolean_property(node, "rust_dynamic_array_alias_call"))
                {
                    rust_lower_closure_array_arguments(model, rust_closure_property(node, "rust_array_alias_branch"), next_id);
                    rust_lower_closure_array_arguments(model, rust_closure_property(node, "rust_array_alias_distinct_branch"), next_id);
                    return;
                }
            }
            json_object *body = NULL;
            if (json_object_deep_copy(node, &body, NULL) != 0 || !body) return;
            json_object *place = rust_closure_property(rust_closure_property(body, "callee"), "object");
            json_object *walk = place;
            while (walk)
            {
                json_object_object_del(walk, "rust_needs_clone");
                if (json_boolean_property(walk, "rust_thread_field"))
                    json_object_object_add(walk, "rust_thread_field_place", json_object_new_boolean(true));
                if (json_boolean_property(walk, "rust_field_array_value"))
                    json_object_object_add(walk, "rust_field_array_place", json_object_new_boolean(true));
                if (json_string_property_equals(walk, "kind", "member")) walk = rust_closure_property(walk, "object");
                else if (json_string_property_equals(walk, "kind", "array_access")) walk = rust_closure_property(walk, "array");
                else break;
            }
            json_object *arguments = rust_closure_property(node, "args"), *refs = json_object_new_array();
            for (size_t i = 0; i < rust_closure_length(arguments); i++)
            {
                json_object *arg = json_object_array_get_idx(arguments, i);
                char name[96];
                if (!rust_allocate_helper_name(model, "__sn_record_array_arg", name, sizeof(name))) return;
                json_object_object_add(arg, "rust_owned_record_array_arg_name", json_object_new_string(name));
                bool borrow_arg = record_method &&
                    (json_boolean_property(arg, "rust_default_array_ref_arg") ||
                     json_boolean_property(arg, "is_ref_arg") || json_boolean_property(arg, "is_borrow_tmp"));
                if (borrow_arg)
                    json_object_object_add(arg, "rust_owned_record_borrow_arg", json_object_new_boolean(true));
                json_object *ref = json_object_new_object();
                json_object_object_add(ref, "kind", json_object_new_string("variable"));
                json_object_object_add(ref, "name", json_object_new_string(name));
                json_object_object_add(ref, "type", json_object_get(rust_closure_property(arg, "type")));
                if (borrow_arg)
                    json_object_object_add(ref, "rust_owned_record_borrow_value", json_object_new_boolean(true));
                json_object_array_add(refs, ref);
            }
            json_object_object_add(body, "args", refs);
            json_object_object_add(node, "rust_owned_record_array_mutation", json_object_new_boolean(true));
            if (record_method)
                json_object_object_add(node, "rust_owned_record_method_mutation", json_object_new_boolean(true));
            json_object_object_add(node, "rust_owned_record_array_args", json_object_get(arguments));
            json_object_object_add(node, "rust_owned_record_array_body", body);
        }
    }
    json_object *scalar_target = json_boolean_property(model, "rust_has_scalar_ref_closures")
        ? rust_scalar_reference_target(model, node) : NULL;
    json_object *scalar_params = rust_closure_property(scalar_target, "params");
    bool scalar_direct = false;
    for (size_t p = 0; p < rust_closure_length(scalar_params); p++)
        scalar_direct |= json_boolean_property(json_object_array_get_idx(scalar_params, p), "rust_closure_scalar_reference");
    bool scalar_native = false;
    if (json_boolean_property(scalar_target, "rust_native_bridge"))
    {
        json_object *call_args = rust_closure_property(node, "args");
        for (size_t i = 0; i < rust_closure_length(call_args); i++)
            scalar_native |= json_boolean_property(json_object_array_get_idx(call_args, i), "rust_closure_scalar_reference");
    }
    if (!json_boolean_property(node, "rust_closure_call") && !scalar_direct && !scalar_native)
    {
        json_object *callee = rust_closure_property(node, "callee");
        json_object *receiver = rust_closure_property(callee, "object");
        json_object *type = rust_closure_property(receiver, "type");
        json_object *root = receiver;
        while (json_string_property_equals(root, "kind", "member") ||
               json_string_property_equals(root, "kind", "array_access"))
            root = rust_closure_property(root,
                json_string_property_equals(root, "kind", "array_access") ? "array" : "object");
        json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
        if (!json_string_property_equals(node, "kind", "call") ||
            !json_string_property_equals(callee, "kind", "member") ||
            !json_boolean_property(root, "rust_closure_array_parameter") ||
            !rust_find_resolved_method(structure, json_string_property(callee, "member_name"), false)) return;
        char guard[96];
        if (!rust_allocate_helper_name(model, "__sn_closure_array_method_guard", guard, sizeof(guard))) return;
        char result[96];
        if (!rust_allocate_helper_name(model, "__sn_closure_array_method_result", result, sizeof(result))) return;
        if (json_boolean_property(type, "pass_self_by_ref"))
        {
            char receiver_name[96];
            if (!rust_allocate_helper_name(model, "__sn_closure_array_reference_receiver", receiver_name, sizeof(receiver_name))) return;
            json_object_object_add(node, "rust_closure_array_reference_receiver",
                                   json_object_new_string(receiver_name));
        }
        json_object_object_add(node, "rust_closure_array_method_result", json_object_new_string(result));
        json_object_object_add(node, "rust_closure_array_method_guard", json_object_new_string(guard));
        json_object_object_add(node, "rust_closure_array_method_source", json_object_new_string(json_string_property(root, "name")));
        json_object *original_args = rust_closure_property(node, "args");
        json_object *args = json_object_new_array();
        json_object_object_add(node, "rust_closure_array_method_args", json_object_get(original_args));
        for (size_t i = 0; i < json_object_array_length(original_args); i++)
        {
            json_object *arg = json_object_array_get_idx(original_args, i);
            char name[96];
            if (!rust_allocate_helper_name(model, "__sn_closure_array_method_arg", name, sizeof(name))) return;
            json_object_object_add(arg, "rust_resolved_arg_name", json_object_new_string(name));
            json_object *ref = json_object_new_object();
            json_object_object_add(ref, "kind", json_object_new_string("variable"));
            json_object_object_add(ref, "name", json_object_new_string(name));
            json_object_object_add(ref, "type", json_object_get(rust_closure_property(arg, "type")));
            if (json_boolean_property(arg, "rust_closure_array_parameter"))
            {
                /* Preserve shared cell identity while evaluating arguments.
                 * Form exclusive projections only after alias dispatch. */
                json_object_object_add(arg, "rust_closure_array_method_cell_arg",
                                       json_object_new_boolean(true));
                json_object_object_add(ref, "rust_closure_array_parameter",
                                       json_object_new_boolean(true));
                json_object_object_add(ref, "rust_default_array_ref_arg",
                                       json_object_new_boolean(true));
                if (json_boolean_property(arg, "rust_native_handle_borrow_arg"))
                    json_object_object_add(ref, "rust_native_handle_borrow_arg",
                                           json_object_new_boolean(true));
                json_object_object_add(ref, "rust_closure_array_method_cell_source",
                                       json_object_new_string(json_string_property(arg, "name")));
            }
            json_object_array_add(args, ref);
        }
        json_object_object_add(node, "args", args);
        return;
    }
    json_object *args = NULL;
    if (!json_object_object_get_ex(node, "args", &args)) return;
    json_object *params = rust_closure_property(rust_closure_property(
        rust_closure_property(node, "callee"), "type"), "param_types");
    if (scalar_direct || scalar_native) params = scalar_params;
    bool scalar_refs = scalar_native;
    for (size_t i = 0; i < rust_closure_length(params); i++)
        scalar_refs |= json_boolean_property(json_object_array_get_idx(params, i), "rust_closure_scalar_reference");
    if (scalar_refs)
    {
        json_object_object_add(model, "rust_has_scalar_ref_closures", json_object_new_boolean(true));
        json_object_object_add(node, "rust_closure_array_call", json_object_new_boolean(true));
        if (scalar_direct || scalar_native)
            json_object_object_add(node, "rust_closure_scalar_direct_call", json_object_new_boolean(true));
        if (scalar_native)
            json_object_object_add(node, "rust_closure_scalar_native_call", json_object_new_boolean(true));
        for (size_t i = 0; i < rust_closure_length(args); i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            json_object *param = i < rust_closure_length(params) ? json_object_array_get_idx(params, i) : NULL;
            bool scalar_param = json_boolean_property(param, "rust_closure_scalar_reference") ||
                (scalar_native && json_string_property_equals(param, "mem_qual", "as_ref") &&
                 rust_closure_scalar_type(rust_closure_property(param, "type")));
            if (scalar_param)
            {
                if (json_boolean_property(arg, "rust_thread_field"))
                    json_object_object_add(arg, "rust_closure_scalar_shared_field", json_object_new_boolean(true));
                const char *existing = NULL;
                for (size_t p = 0; p < i; p++)
                {
                    json_object *previous = json_object_array_get_idx(args, p);
                    if (json_string_property(previous, "rust_closure_scalar_cell_name") &&
                        rust_same_default_array_place(arg, previous))
                        existing = json_string_property(previous, "rust_closure_scalar_cell_name");
                }
                if (!existing)
                {
                    char name[96];
                    if (!rust_allocate_helper_name(model, "__sn_closure_scalar_cell", name, sizeof(name))) return;
                    json_object_object_add(arg, "rust_closure_scalar_cell_name", json_object_new_string(name));
                    json_object_object_add(arg, "rust_closure_scalar_cell_bind", json_object_new_boolean(true));
                }
                else json_object_object_add(arg, "rust_closure_scalar_cell_name", json_object_new_string(existing));
            }
            else if (!json_string_property_equals(rust_closure_property(arg, "type"), "kind", "array") &&
                     !json_string_property(arg, "rust_deferred_arg_name"))
            {
                char name[96];
                if (!rust_allocate_helper_name(model, "__sn_closure_scalar_arg", name, sizeof(name))) return;
                json_object_object_add(arg, "rust_deferred_arg_name", json_object_new_string(name));
            }
        }
    }
    if (scalar_native)
    {
        for (size_t i = 0; i < rust_closure_length(args); i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            if (!json_string_property(arg, "rust_closure_scalar_cell_name")) continue;
            char guard[96], pointer[96];
            if (!rust_allocate_helper_name(model, "__sn_native_scalar_guard", guard, sizeof(guard)) ||
                !rust_allocate_helper_name(model, "__sn_native_scalar_pointer", pointer, sizeof(pointer))) return;
            json_object_object_add(arg, "rust_closure_scalar_native_guard", json_object_new_string(guard));
            json_object_object_add(arg, "rust_closure_scalar_native_pointer", json_object_new_string(pointer));
            json_object *prior = json_object_new_array();
            for (size_t p = 0; p < i; p++)
            {
                json_object *previous = json_object_array_get_idx(args, p);
                if (!json_string_property(previous, "rust_closure_scalar_native_pointer") ||
                    !rust_closure_same_type(rust_closure_property(arg, "type"), rust_closure_property(previous, "type"))) continue;
                json_object *alias = json_object_new_object();
                json_object_object_add(alias, "cell", json_object_get(rust_closure_property(previous, "rust_closure_scalar_cell_name")));
                json_object_object_add(alias, "pointer", json_object_get(rust_closure_property(previous, "rust_closure_scalar_native_pointer")));
                json_object_array_add(prior, alias);
            }
            json_object_object_add(arg, "rust_closure_scalar_native_prior", prior);
        }
    }
    if (scalar_direct || scalar_native) return;
    for (size_t i = 0; i < json_object_array_length(args); i++)
    {
        json_object *arg = json_object_array_get_idx(args, i), *type = NULL;
        json_object_object_get_ex(arg, "type", &type);
        if (json_boolean_property(type, "rust_native_reference_handle"))
        {
            json_object_object_add(arg, "rust_closure_native_handle_arg", json_object_new_boolean(true));
            json_object_object_add(arg, "rust_native_handle_borrow_arg", json_object_new_boolean(true));
            json_object_object_del(arg, "rust_closure_arg_clone");
        }
        if (!json_string_property_equals(type, "kind", "array")) continue;
        json_object_object_add(node, "rust_closure_array_call", json_object_new_boolean(true));
        if (json_boolean_property(arg, "rust_closure_array_parameter")) continue;
        if (json_string_property_equals(arg, "kind", "array_access"))
        {
            json_object *root = NULL, *index = NULL, *groups = NULL, *group = NULL;
            json_object_object_get_ex(arg, "array", &root);
            json_object_object_get_ex(arg, "index", &index);
            if (!json_object_object_get_ex(node, "rust_closure_array_groups", &groups))
            {
                groups = json_object_new_array();
                json_object_object_add(node, "rust_closure_array_groups", groups);
            }
            for (size_t g = 0; g < json_object_array_length(groups); g++)
            {
                json_object *candidate = json_object_array_get_idx(groups, g), *other_root = NULL;
                json_object_object_get_ex(candidate, "root", &other_root);
                if (rust_same_default_array_place(root, other_root)) { group = candidate; break; }
            }
            char name[80];
            if (!group)
            {
                group = json_object_new_object();
                do snprintf(name, sizeof(name), "__sn_closure_array_group_%u", (*next_id)++);
                while (rust_model_contains_string(model, name));
                json_object_object_add(group, "name", json_object_new_string(name));
                json_object_object_add(group, "root", json_object_get(root));
                if (json_boolean_property(root, "rust_closure_array_parameter"))
                {
                    json_object_object_add(group, "shared_root_name",
                                           json_object_new_string(json_string_property(root, "name")));
                    char guard[96];
                    if (!rust_allocate_helper_name(model, "__sn_closure_array_group_guard", guard, sizeof(guard))) return;
                    json_object_object_add(group, "guard_name", json_object_new_string(guard));
                }
                json_object_object_add(group, "indices", json_object_new_array());
                json_object_array_add(groups, group);
            }
            do snprintf(name, sizeof(name), "__sn_closure_array_index_%u", (*next_id)++);
            while (rust_model_contains_string(model, name));
            json_object_object_add(arg, "rust_closure_array_index_name", json_object_new_string(name));
            json_object_object_add(arg, "rust_closure_array_group_name",
                                   json_object_new_string(json_string_property(group, "name")));
            json_object *item = json_object_new_object(), *indices = NULL;
            json_object_object_add(item, "name", json_object_new_string(name));
            json_object_object_get_ex(group, "indices", &indices);
            json_object_array_add(indices, item);
            continue;
        }
        const char *existing = NULL;
        for (size_t j = 0; j < i; j++)
        {
            json_object *earlier = json_object_array_get_idx(args, j);
            if (rust_same_default_array_place(arg, earlier))
            {
                existing = json_string_property(earlier, "rust_closure_array_cell_name");
                if (existing) break;
            }
        }
        char name[80];
        if (!existing)
        {
            do snprintf(name, sizeof(name), "__sn_closure_array_arg_%u", (*next_id)++);
            while (rust_model_contains_string(model, name));
            existing = name;
            json_object_object_add(arg, "rust_closure_array_cell_bind", json_object_new_boolean(true));
            if (json_boolean_property(arg, "rust_array_snapshot_cell"))
            {
                char guard[96];
                if (!rust_allocate_helper_name(model, "__sn_closure_array_arg_guard", guard, sizeof(guard))) return;
                json_object_object_add(arg, "rust_closure_array_guard_name", json_object_new_string(guard));
            }
            if (!json_string_property_equals(arg, "kind", "variable") &&
                !json_string_property_equals(arg, "kind", "member"))
            {
                char owner[96];
                if (!rust_allocate_helper_name(model, "__sn_closure_array_owner", owner, sizeof(owner))) return;
                json_object_object_add(arg, "rust_closure_array_owner_name", json_object_new_string(owner));
            }
        }
        json_object_object_add(arg, "rust_closure_array_cell_name", json_object_new_string(existing));
    }
}

static void rust_lower_ref_previous_names(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < rust_closure_length(node); i++)
            rust_lower_ref_previous_names(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && strcmp(key, "lambdas") != 0)
            rust_lower_ref_previous_names(model, child);
    if (json_boolean_property(node, "rust_ref_old_value_before_rhs"))
    {
        char previous[96];
        if (!rust_allocate_helper_name(model, "__sn_ref_previous", previous, sizeof(previous))) return;
        json_object_object_add(node, "rust_ref_previous_name", json_object_new_string(previous));
    }
}

static void rust_name_rebindable_array_types(json_object *node, const char *name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_name_rebindable_array_types(json_object_array_get_idx(node, i), name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_name_rebindable_array_types(child, name);
    if (json_string_property_equals(node, "kind", "array"))
        json_object_object_add(node, "rust_closure_array_type_name", json_object_new_string(name));
}

static void rust_lower_closures(json_object *model)
{
    rust_lower_ref_previous_names(model, model);
    bool uses = false;
    rust_lower_closure_node(model, &uses);
    if (!uses) return;
    /* Choose the name before installing annotations. No user identifier is
     * reserved, and every nested signature uses the same module-local name. */
    char name[64] = "__SnClosure";
    unsigned int suffix = 0;
    while (rust_model_contains_string(model, name))
        snprintf(name, sizeof(name), "__SnClosure_%u", suffix++);
    char snapshot[64] = "__sn_closure_record_snapshot";
    suffix = 0;
    while (rust_model_contains_string(model, snapshot))
        snprintf(snapshot, sizeof(snapshot), "__sn_closure_record_snapshot_%u", suffix++);
    json_object_object_add(model, "rust_closure_snapshot_method", json_object_new_string(snapshot));
    char scalar_ref[96];
    if (!rust_allocate_helper_name(model, "__SnScalarRef", scalar_ref, sizeof(scalar_ref))) return;
    json_object_object_add(model, "rust_closure_scalar_ref_type", json_object_new_string(scalar_ref));
    const char *scalar_bases[] = {"__SnScalarStorage", "__SnScalarGuard"};
    const char *scalar_keys[] = {"rust_closure_scalar_storage_type", "rust_closure_scalar_guard_type"};
    for (size_t i = 0; i < 2; i++)
    {
        char helper[96];
        if (!rust_allocate_helper_name(model, scalar_bases[i], helper, sizeof(helper))) return;
        json_object_object_add(model, scalar_keys[i], json_object_new_string(helper));
    }
    if (json_boolean_property(model, "rust_closure_rebindable_arrays"))
    {
        char array_type[96];
        if (!rust_allocate_helper_name(model, "__SnClosureArray", array_type, sizeof(array_type))) return;
        json_object_object_add(model, "rust_closure_array_param_type", json_object_new_string(array_type));
        char identity_trait[96], identity_method[96];
        if (!rust_allocate_helper_name(model, "__SnArrayCellIdentity", identity_trait, sizeof(identity_trait)) ||
            !rust_allocate_helper_name(model, "__sn_array_cell_identity", identity_method, sizeof(identity_method))) return;
        json_object_object_add(model, "rust_closure_array_identity_trait", json_object_new_string(identity_trait));
        json_object_object_add(model, "rust_closure_array_identity_method", json_object_new_string(identity_method));
        rust_name_rebindable_array_types(model, array_type);
    }
    unsigned int argument_id = 0;
    rust_lower_closure_array_arguments(model, model, &argument_id);
    rust_closure_name_types(model, name, scalar_ref);
    json_object_object_add(model, "rust_closure_handle_name", json_object_new_string(name));
    json_object_object_add(model, "rust_has_closures", json_object_new_boolean(true));
}

/* A program with borrowed scalar closures uses the same shared place protocol
 * in ordinary callees, so forwarding preserves dynamic aliases. */
static bool rust_has_scalar_reference_lambda(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < rust_closure_length(node); i++)
            if (rust_has_scalar_reference_lambda(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    if (json_string_property_equals(node, "kind", "lambda"))
    {
        json_object *params = rust_closure_property(node, "params");
        for (size_t i = 0; i < rust_closure_length(params); i++)
        {
            json_object *param = json_object_array_get_idx(params, i);
            if (json_string_property_equals(param, "mem_qual", "as_ref") &&
                rust_closure_scalar_type(rust_closure_property(param, "type"))) return true;
        }
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && rust_has_scalar_reference_lambda(child)) return true;
    return false;
}

static void rust_prepare_scalar_reference_callable(json_object *callable)
{
    if (json_boolean_property(callable, "is_native") ||
        json_boolean_property(callable, "rust_native_bridge")) return;
    json_object *params = rust_closure_property(callable, "params");
    for (size_t i = 0; i < rust_closure_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        json_object *type = rust_closure_property(param, "type");
        if (!json_string_property_equals(param, "mem_qual", "as_ref") ||
            !rust_closure_scalar_type(type)) continue;
        json_object_object_add(param, "rust_closure_scalar_reference", json_object_new_boolean(true));
        json_object_object_add(param, "rust_closure_param_share", json_object_new_boolean(true));
        json_object_object_add(param, "rust_shared_cell", json_object_new_boolean(true));
        json_object_object_add(type, "rust_closure_scalar_reference", json_object_new_boolean(true));
    }
}

static void rust_prepare_scalar_reference_callables(json_object *model)
{
    if (!rust_has_scalar_reference_lambda(model)) return;
    json_object_object_add(model, "rust_has_scalar_ref_closures", json_object_new_boolean(true));
    json_object *functions = rust_closure_property(model, "functions");
    for (size_t i = 0; i < rust_closure_length(functions); i++)
        rust_prepare_scalar_reference_callable(json_object_array_get_idx(functions, i));
    json_object *structs = rust_closure_property(model, "structs");
    for (size_t i = 0; i < rust_closure_length(structs); i++)
    {
        json_object *methods = rust_closure_property(json_object_array_get_idx(structs, i), "methods");
        for (size_t m = 0; m < rust_closure_length(methods); m++)
            rust_prepare_scalar_reference_callable(json_object_array_get_idx(methods, m));
    }
}

static json_object *rust_scalar_reference_target(json_object *model, json_object *node)
{
    json_object *callee = rust_closure_property(node, "callee");
    if (json_string_property_equals(callee, "kind", "variable") &&
        json_boolean_property(callee, "rust_direct_callee"))
    {
        json_object *functions = rust_closure_property(model, "functions");
        for (size_t i = 0; i < rust_closure_length(functions); i++)
        {
            json_object *fn = json_object_array_get_idx(functions, i);
            if (json_string_property_equals(fn, "name", json_string_property(callee, "name"))) return fn;
        }
    }
    if (json_string_property_equals(callee, "kind", "member") &&
        !json_boolean_property(node, "is_fn_field_call"))
    {
        json_object *type = rust_closure_property(rust_closure_property(callee, "object"), "type");
        if (json_string_property_equals(type, "kind", "pointer")) type = rust_closure_property(type, "base_type");
        return rust_find_resolved_method(rust_find_struct(model, json_string_property(type, "name")),
            json_string_property(callee, "member_name"), false);
    }
    if (json_string_property_equals(node, "kind", "static_call") ||
        json_string_property_equals(node, "kind", "method_call"))
    {
        const char *name = json_string_property(node, "type_name");
        if (!name) name = json_string_property(rust_closure_property(node, "struct_type"), "name");
        return rust_find_resolved_method(rust_find_struct(model, name), json_string_property(node, "method_name"),
            json_string_property_equals(node, "kind", "static_call") || json_boolean_property(node, "is_static"));
    }
    return NULL;
}
