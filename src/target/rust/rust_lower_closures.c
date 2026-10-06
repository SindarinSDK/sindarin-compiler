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

static void rust_closure_name_types(json_object *node, const char *name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_closure_name_types(json_object_array_get_idx(node, i), name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "lambdas") != 0) rust_closure_name_types(value, name);
    }
    if (json_string_property_equals(node, "kind", "function"))
        json_object_object_add(node, "rust_closure_handle_name", json_object_new_string(name));
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
    if (!json_boolean_property(node, "rust_closure_call"))
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
    for (size_t i = 0; i < json_object_array_length(args); i++)
    {
        json_object *arg = json_object_array_get_idx(args, i), *type = NULL;
        json_object_object_get_ex(arg, "type", &type);
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

static void rust_lower_closures(json_object *model)
{
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
    unsigned int argument_id = 0;
    rust_lower_closure_array_arguments(model, model, &argument_id);
    rust_closure_name_types(model, name);
    json_object_object_add(model, "rust_closure_handle_name", json_object_new_string(name));
    json_object_object_add(model, "rust_has_closures", json_object_new_boolean(true));
}
