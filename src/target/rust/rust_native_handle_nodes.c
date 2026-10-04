/* Included by rust_target.c. Native handles borrow canonical C allocations at
 * default call edges and acquire only when an expression supplies an owner. */
static bool rust_native_handle_type(json_object *type)
{
    return json_boolean_property(type, "rust_native_reference_handle");
}

static json_object *rust_native_handle_callable(json_object *model, json_object *node)
{
    json_object *callee = rust_nullable_child(node, "callee");
    if (json_string_property_equals(node, "kind", "call") &&
        json_string_property_equals(callee, "kind", "variable"))
    {
        json_object *functions = rust_nullable_child(model, "functions");
        const char *name = json_string_property(callee, "name");
        for (size_t i = 0; name && functions && i < json_object_array_length(functions); i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (json_string_property_equals(function, "name", name)) return function;
        }
    }
    const char *name = json_string_property(node, "type_name");
    if (!name) name = json_string_property(rust_nullable_child(node, "struct_type"), "name");
    if (!name) name = json_string_property(rust_nullable_child(rust_nullable_child(callee, "object"), "type"), "name");
    const char *method = json_string_property(node, "method_name");
    if (!method) method = json_string_property(callee, "member_name");
    return rust_find_resolved_method(rust_find_struct(model, name), method,
        json_string_property_equals(node, "kind", "static_call") || json_boolean_property(node, "is_static"));
}

static void rust_prepare_native_handle_nodes(json_object *model, json_object *node)
{
    if (!rust_nullable_child(model, "rust_native_handles")) return;
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_prepare_native_handle_nodes(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *params = rust_nullable_child(node, "params");
    for (size_t p = 0; params && p < json_object_array_length(params); p++)
    {
        json_object *param = json_object_array_get_idx(params, p);
        if (rust_native_handle_type(rust_nullable_child(param, "type")) &&
            json_string_property_equals(param, "mem_qual", "default"))
            json_object_object_add(param, "rust_native_handle_borrow_param", json_object_new_boolean(true));
    }
    json_object *function = rust_native_handle_callable(model, node);
    params = rust_nullable_child(function, "params");
    json_object *args = rust_nullable_child(node, "args");
    for (size_t p = 0; args && params && p < json_object_array_length(args) && p < json_object_array_length(params); p++)
    {
        json_object *param = json_object_array_get_idx(params, p);
        json_object *arg = json_object_array_get_idx(args, p);
        json_object *expected_type = rust_nullable_child(param, "type");
        json_object *elements = rust_nullable_child(arg, "elements");
        if (json_boolean_property(expected_type, "rust_native_handle_array") &&
            json_string_property_equals(arg, "kind", "array_literal") &&
            elements && json_object_array_length(elements) == 0 &&
            json_string_property_equals(rust_nullable_child(rust_nullable_child(arg, "type"), "element_type"), "kind", "nil"))
        {
            /* C allocates inferred empty call literals as untyped long-long
             * arrays, with no element callbacks. Keep that actual header while
             * projecting the callee's native array type into Rust. */
            json_object_object_add(arg, "type", json_object_get(expected_type));
            json_object_object_add(arg, "rust_native_handle_untyped_array", json_object_new_boolean(true));
        }
        if ((rust_native_handle_type(rust_nullable_child(param, "type")) ||
             (json_boolean_property(function, "rust_native_bridge") && json_boolean_property(rust_nullable_child(param, "type"), "rust_native_handle_array"))) &&
            json_string_property_equals(param, "mem_qual", "default"))
            json_object_object_add(json_object_array_get_idx(args, p), "rust_native_handle_borrow_arg", json_object_new_boolean(true));
    }
    json_object *param_types = rust_nullable_child(rust_nullable_child(rust_nullable_child(node, "callee"), "type"), "param_types");
    for (size_t p = 0; args && param_types && p < json_object_array_length(args) && p < json_object_array_length(param_types); p++)
    {
        json_object *arg = json_object_array_get_idx(args, p);
        json_object *expected_type = json_object_array_get_idx(param_types, p);
        json_object *elements = rust_nullable_child(arg, "elements");
        if (json_boolean_property(expected_type, "rust_native_handle_array") &&
            json_string_property_equals(arg, "kind", "array_literal") &&
            elements && json_object_array_length(elements) == 0 &&
            json_string_property_equals(rust_nullable_child(rust_nullable_child(arg, "type"), "element_type"), "kind", "nil"))
        {
            json_object_object_add(arg, "type", json_object_get(expected_type));
            json_object_object_add(arg, "rust_native_handle_untyped_array", json_object_new_boolean(true));
        }
    }
    if (json_string_property_equals(node, "kind", "member") ||
        json_string_property_equals(node, "kind", "member_assign"))
    {
        json_object *object_type = rust_nullable_child(rust_nullable_child(node, "object"), "type");
        if (rust_native_handle_type(object_type))
        {
            json_object *structure = rust_find_struct(model, json_string_property(object_type, "name"));
            json_object *fields = rust_nullable_child(structure, "fields");
            const char *member = json_string_property(node, "member_name");
            bool store = json_string_property_equals(node, "kind", "member_assign");
            if (store) member = json_string_property(node, "field_name");
            for (size_t f = 0; member && fields && f < json_object_array_length(fields); f++)
            {
                json_object *field = json_object_array_get_idx(fields, f);
                const char *getter = json_string_property(field, store ? "rust_native_handle_set" : "rust_native_handle_get");
                if (getter && json_string_property_equals(field, "name", member))
                    json_object_object_add(node, store ? "rust_native_handle_set" : "rust_native_handle_get", json_object_new_string(getter));
            }
        }
    }
    if (json_string_property_equals(node, "kind", "borrow_inferred_call"))
    {
        json_object *inner = rust_nullable_child(node, "inner_call");
        json_object *native = rust_native_handle_callable(model, inner);
        if (rust_native_handle_type(rust_nullable_child(node, "type")) &&
            (json_boolean_property(native, "rust_native_handle_bridge") ||
             json_boolean_property(native, "rust_native_handle_method")) &&
            rust_native_handle_type(rust_nullable_child(native, "return_type")))
            json_object_object_add(node, "rust_native_handle_owned_result", json_object_new_boolean(true));
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_prepare_native_handle_nodes(model, child);
}

static void rust_lower_native_handle_reads(json_object *model, json_object *node, bool borrow)
{
    if (!rust_nullable_child(model, "rust_native_handles")) return;
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_native_handle_reads(model, json_object_array_get_idx(node, i), borrow);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    bool handle = rust_native_handle_type(rust_nullable_child(node, "type"));
    if (json_boolean_property(node, "rust_native_handle_borrow_arg")) borrow = true;
    if (json_boolean_property(node, "rust_default_array_ref_arg") &&
        json_boolean_property(rust_nullable_child(node, "type"), "rust_native_handle_array")) borrow = true;
    const char *kind = json_string_property(node, "kind");
    bool read = kind && (strcmp(kind, "variable") == 0 || strcmp(kind, "member") == 0 || strcmp(kind, "array_access") == 0);
    if ((handle || json_boolean_property(rust_nullable_child(node, "type"), "rust_native_handle_array")) && read)
    {
        json_object_object_del(node, "rust_needs_clone");
        json_object_object_del(node, "rust_resolved_clone");
        bool field_read = json_boolean_property(node, "rust_thread_field");
        bool owner_read = json_boolean_property(node, "rust_thread_ref_owner");
        bool cell_read = json_string_property_equals(node, "kind", "variable") && json_boolean_property(node, "rust_cell");
        json_object_object_add(node, "rust_native_handle_owned_read", json_object_new_boolean(!borrow && !field_read && !cell_read && !owner_read));
        if (handle && borrow && !field_read && !cell_read)
            json_object_object_add(node, "rust_native_handle_borrow_snapshot", json_object_new_boolean(true));
        if (field_read || cell_read)
            json_object_object_add(node, "rust_native_handle_read_acquired", json_object_new_boolean(!borrow));
        if (cell_read && borrow)
            json_object_object_add(node, "rust_native_handle_cell_borrow_view", json_object_new_boolean(true));
        if (field_read && borrow)
        {
            json_object_object_add(node, "rust_native_handle_borrow_view", json_object_new_boolean(true));
            json_object_object_add(model, "rust_uses_thread_field_map_read", json_object_new_boolean(true));
        }
    }
    if (json_string_property_equals(node, "kind", "for_each") &&
        json_boolean_property(rust_nullable_child(rust_nullable_child(node, "iterable"), "type"), "rust_native_handle_array"))
    {
        char owner[160], length[160], index[160];
        size_t id = 0;
        do {
            snprintf(owner, sizeof(owner), "__sn_native_array_iter_%zu_owner", id);
            snprintf(length, sizeof(length), "__sn_native_array_iter_%zu_length", id);
            snprintf(index, sizeof(index), "__sn_native_array_iter_%zu_index", id++);
        } while (rust_call_model_contains_string(model, owner) || rust_call_model_contains_string(model, length) || rust_call_model_contains_string(model, index));
        json_object_object_add(node, "rust_native_array_iter_owner", json_object_new_string(owner));
        json_object_object_add(node, "rust_native_array_iter_length", json_object_new_string(length));
        json_object_object_add(node, "rust_native_array_iter_index", json_object_new_string(index));
    }
    json_object *left_type = rust_nullable_child(rust_nullable_child(node, "left"), "type");
    json_object *right_type = rust_nullable_child(rust_nullable_child(node, "right"), "type");
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0) continue;
        bool child_borrow = false;
        if ((strcmp(key, "object") == 0 || strcmp(key, "receiver") == 0) ||
            (strcmp(key, "operand") == 0 && json_string_property_equals(node, "kind", "sizeof")))
            child_borrow = true;
        if ((strcmp(key, "left") == 0 || strcmp(key, "right") == 0) &&
            (rust_native_handle_type(left_type) || rust_native_handle_type(right_type))) child_borrow = true;
        /* Native-array concat reads both original C headers. Cloning its
         * right operand would introduce observable extra element retains. */
        if (strcmp(key, "args") == 0 &&
            json_string_property_equals(rust_nullable_child(node, "callee"), "member_name", "concat") &&
            json_boolean_property(rust_nullable_child(rust_nullable_child(rust_nullable_child(node, "callee"), "object"), "type"), "rust_native_handle_array"))
            child_borrow = true;
        /* A borrowed member/index projection must borrow its owner too. */
        if (strcmp(key, "array") == 0 && handle && read) child_borrow = true;
        if (strcmp(key, "iterable") == 0 && json_string_property_equals(node, "kind", "for_each") &&
            json_boolean_property(rust_nullable_child(child, "type"), "rust_native_handle_array") &&
            !json_boolean_property(node, "needs_iterable_cleanup")) child_borrow = true;
        rust_lower_native_handle_reads(model, child, child_borrow);
    }
}

/* C lifts owning call receivers/arguments of declarations into the enclosing
 * scope. Keep the same credits alive in Rust; statement-only calls still use
 * their existing statement scope. This is observable through native refcounts. */
static void rust_native_handle_extract_expression(json_object *model, json_object *node,
                                                   json_object *inserts, size_t *next_id)
{
    if (!node || !json_object_is_type(node, json_type_object)) return;
    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    if (strcmp(kind, "borrow_inferred_call") == 0)
    {
        rust_native_handle_extract_expression(model, rust_nullable_child(node, "inner_call"), inserts, next_id);
        return;
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0 || strcmp(key, "borrow_check_args") == 0) continue;
        if (json_object_is_type(child, json_type_array))
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
                rust_native_handle_extract_expression(model, json_object_array_get_idx(child, i), inserts, next_id);
        }
        else rust_native_handle_extract_expression(model, child, inserts, next_id);
    }
    bool call = strcmp(kind, "call") == 0 || strcmp(kind, "method_call") == 0 || strcmp(kind, "static_call") == 0;
    if (!call) return;
    json_object *callee = rust_nullable_child(node, "callee");
    json_object *receiver_parent = strcmp(kind, "method_call") == 0 ? node : callee;
    json_object *args = rust_nullable_child(node, "args");
    for (size_t i = 0; i <= (args ? json_object_array_length(args) : 0); i++)
    {
        json_object *value = i == 0 ? rust_nullable_child(receiver_parent, "object") : json_object_array_get_idx(args, i - 1);
        json_object *type = rust_nullable_child(value, "type");
        const char *value_kind = json_string_property(value, "kind");
        if ((!rust_native_handle_type(type) && !json_boolean_property(type, "rust_native_handle_array")) || !value_kind ||
            (strcmp(value_kind, "call") != 0 && strcmp(value_kind, "method_call") != 0 &&
             strcmp(value_kind, "static_call") != 0 && strcmp(value_kind, "borrow_inferred_call") != 0)) continue;
        char name[160];
        do { snprintf(name, sizeof(name), "__sn_native_owned_temporary_%zu", (*next_id)++); }
        while (rust_call_model_contains_string(model, name));
        json_object *declaration = json_object_new_object();
        json_object_object_add(declaration, "kind", json_object_new_string("var_decl"));
        json_object_object_add(declaration, "name", json_object_new_string(name));
        json_object_object_add(declaration, "type", json_object_get(type));
        json_object_object_add(declaration, "initializer", json_object_get(value));
        json_object_object_add(declaration, "mem_qual", json_object_new_string("default"));
        json_object_object_add(declaration, "sync_mod", json_object_new_string("none"));
        json_object_array_add(inserts, declaration);
        json_object *read = json_object_new_object();
        json_object_object_add(read, "kind", json_object_new_string("variable"));
        json_object_object_add(read, "name", json_object_new_string(name));
        json_object_object_add(read, "type", json_object_get(type));
        if (i == 0) json_object_object_add(receiver_parent, "object", read);
        else json_object_array_put_idx(args, i - 1, read);
    }
}

static void rust_lower_native_handle_temporaries(json_object *model, json_object *node, size_t *next_id)
{
    if (!rust_nullable_child(model, "rust_native_handles")) return;
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        json_object *ordered = json_object_new_array();
        for (size_t i = 0; i < json_object_array_length(node); i++)
        {
            json_object *statement = json_object_array_get_idx(node, i);
            rust_lower_native_handle_temporaries(model, statement, next_id);
            json_object *inserts = json_object_new_array();
            if (json_string_property_equals(statement, "kind", "var_decl"))
                rust_native_handle_extract_expression(model, rust_nullable_child(statement, "initializer"), inserts, next_id);
            for (size_t t = 0; t < json_object_array_length(inserts); t++)
                json_object_array_add(ordered, json_object_get(json_object_array_get_idx(inserts, t)));
            json_object_array_add(ordered, json_object_get(statement));
            json_object_put(inserts);
        }
        while (json_object_array_length(node)) json_object_array_del_idx(node, 0, 1);
        for (size_t i = 0; i < json_object_array_length(ordered); i++)
            json_object_array_add(node, json_object_get(json_object_array_get_idx(ordered, i)));
        json_object_put(ordered);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && strcmp(key, "borrow_check_args") != 0)
            rust_lower_native_handle_temporaries(model, child, next_id);
}
