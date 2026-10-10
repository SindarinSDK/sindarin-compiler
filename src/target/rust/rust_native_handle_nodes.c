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
        const char *alias = json_string_property(callee, "c_alias");
        for (size_t i = 0; name && functions && i < json_object_array_length(functions); i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            if (json_string_property_equals(function, "name", name) ||
                (alias && json_boolean_property(function, "rust_native_bridge") &&
                 json_string_property_equals(function, "c_alias", alias))) return function;
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
    if (!rust_nullable_child(model, "rust_native_handles") &&
        !rust_nullable_child(model, "rust_native_handle_array_support")) return;
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
        if ((rust_native_handle_type(rust_nullable_child(param, "type")) ||
             (json_boolean_property(node, "rust_native_handle_method") &&
              json_boolean_property(rust_nullable_child(param, "type"), "rust_native_handle_array"))) &&
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
             ((json_boolean_property(function, "rust_native_bridge") || json_boolean_property(function, "rust_native_handle_method")) && json_boolean_property(rust_nullable_child(param, "type"), "rust_native_handle_array"))) &&
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

static void rust_native_handle_compounds(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_native_handle_compounds(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_native_handle_compounds(model, child);
    if (!json_string_property_equals(node, "kind", "compound_assign")) return;
    json_object *target = rust_nullable_child(node, "target");
    json_object *object = rust_nullable_child(target, "object");
    json_object *type = rust_nullable_child(object, "type");
    if (!rust_native_handle_type(type) || !json_string_property(target, "rust_native_handle_get")) return;
    json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
    json_object *fields = rust_nullable_child(structure, "fields");
    const char *setter = NULL;
    for (size_t i = 0; fields && i < json_object_array_length(fields); i++) {
        json_object *field = json_object_array_get_idx(fields, i);
        if (json_string_property_equals(field, "name", json_string_property(target, "member_name")))
            setter = json_string_property(field, "rust_native_handle_set");
    }
    if (!setter) return;
    char owner[96], value[96], rhs[96];
    if (!rust_allocate_helper_name(model, "__sn_native_field_owner", owner, sizeof(owner)) ||
        !rust_allocate_helper_name(model, "__sn_native_field_value", value, sizeof(value)) ||
        !rust_allocate_helper_name(model, "__sn_native_field_rhs", rhs, sizeof(rhs))) return;
    json_object *body = NULL, *getter = NULL;
    if (json_object_deep_copy(node, &body, NULL) || json_object_deep_copy(target, &getter, NULL)) {
        if (body) json_object_put(body);
        if (getter) json_object_put(getter);
        return;
    }
    json_object *local = json_object_new_object();
    json_object_object_add(local, "kind", json_object_new_string("variable"));
    json_object_object_add(local, "name", json_object_new_string(owner));
    json_object_object_add(local, "type", json_object_get(type));
    json_object_object_add(getter, "object", local);
    local = json_object_new_object();
    json_object_object_add(local, "kind", json_object_new_string("variable"));
    json_object_object_add(local, "name", json_object_new_string(value));
    json_object_object_add(local, "type", json_object_get(rust_nullable_child(target, "type")));
    json_object_object_add(body, "target", local);
    local = json_object_new_object();
    json_object_object_add(local, "kind", json_object_new_string("variable"));
    json_object_object_add(local, "name", json_object_new_string(rhs));
    json_object_object_add(local, "type", json_object_get(rust_nullable_child(rust_nullable_child(node, "value"), "type")));
    json_object_object_add(body, "value", local);
    json_object_object_add(node, "rust_native_compound_body", body);
    json_object_object_add(node, "rust_native_compound_get", getter);
    json_object_object_add(node, "rust_native_compound_set", json_object_new_string(setter));
    json_object_object_add(node, "rust_native_compound_owner", json_object_new_string(owner));
    json_object_object_add(node, "rust_native_compound_value", json_object_new_string(value));
    json_object_object_add(node, "rust_native_compound_rhs", json_object_new_string(rhs));
}

/* A nested native handle/header read borrows the original closure array.
 * Resolve indices before the guard and extract only the canonical pointer;
 * copying a Vec of native headers would add observable C element retains. */
static void rust_native_closure_array_place(json_object *model, json_object *node)
{
    if (!json_string_property_equals(node, "kind", "array_access")) return;
    json_object *parent = NULL;
    const char *key = NULL;
    json_object *root = rust_array_join_place_root(node, &parent, &key);
    if (!json_boolean_property(root, "rust_closure_array_parameter")) return;
    json_object *place = NULL;
    if (json_object_deep_copy(node, &place, NULL) != 0 || !place) return;
    char guard[96];
    if (!rust_allocate_helper_name(model, "__sn_native_closure_array_guard", guard, sizeof(guard)))
    { json_object_put(place); return; }
    json_object *actual_root = rust_array_join_place_root(place, &parent, &key);
    json_object_object_add(actual_root, "rust_closure_array_store_guard", json_object_new_string(guard));
    json_object *cursor = place;
    while (cursor)
    {
        json_object_object_del(cursor, "rust_needs_clone");
        json_object_object_del(cursor, "rust_native_handle_owned_read");
        json_object_object_del(cursor, "rust_native_handle_borrow_snapshot");
        if (json_string_property_equals(cursor, "kind", "array_access"))
            cursor = rust_nullable_child(cursor, "array");
        else if (json_string_property_equals(cursor, "kind", "member"))
            cursor = rust_nullable_child(cursor, "object");
        else break;
    }
    size_t next_index = 0;
    if (!rust_assign_array_join_place_index_names(model, place, &next_index))
    { json_object_put(place); return; }
    json_object_object_add(node, "rust_native_closure_array_place", place);
    json_object_object_add(node, "rust_native_closure_array_guard", json_object_new_string(guard));
    json_object_object_add(node, "rust_native_closure_array_source", json_object_new_string(json_string_property(root, "name")));
}

static void rust_lower_native_handle_reads(json_object *model, json_object *node, bool borrow)
{
    if (!rust_nullable_child(model, "rust_native_handles") &&
        !rust_nullable_child(model, "rust_native_handle_array_support")) return;
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
    if (kind && (!strcmp(kind, "array_access") || !strcmp(kind, "index_assign")) &&
        json_boolean_property(rust_nullable_child(rust_nullable_child(node, "array"), "type"), "rust_native_array_codec"))
    {
        const char *roles[] = {"owner", "index", "value", NULL};
        for (int i = 0; roles[i]; i++) {
            char key[96], stem[96], name[96];
            snprintf(key, sizeof(key), "rust_native_array_codec_%s", roles[i]);
            if (rust_nullable_child(node, key)) continue;
            snprintf(stem, sizeof(stem), "__sn_native_array_codec_%s", roles[i]);
            if (!rust_allocate_helper_name(model, stem, name, sizeof(name))) return;
            json_object_object_add(node, key, json_object_new_string(name));
        }
    }

    if (json_string_property_equals(node, "kind", "index_assign") &&
        json_string_property_equals(rust_nullable_child(node, "array"), "kind", "array_access") &&
        json_boolean_property(rust_nullable_child(rust_nullable_child(node, "array"), "type"), "rust_native_handle_array")) {
        json_object_object_add(node, "rust_native_array_nested_store", json_object_new_boolean(true));
        const char *roles[] = {"owner", "index", "value", NULL};
        for (int i = 0; roles[i]; i++) {
            char key[96], stem[96], name[96];
            snprintf(key, sizeof(key), "rust_native_array_codec_%s", roles[i]);
            if (rust_nullable_child(node, key)) continue;
            snprintf(stem, sizeof(stem), "__sn_native_array_codec_%s", roles[i]);
            if (!rust_allocate_helper_name(model, stem, name, sizeof(name))) return;
            json_object_object_add(node, key, json_object_new_string(name));
        }
    }
    if (json_string_property_equals(node, "kind", "array_access") && borrow &&
        json_boolean_property(rust_nullable_child(node, "type"), "rust_native_handle_array") &&
        json_boolean_property(rust_nullable_child(rust_nullable_child(node, "array"), "type"), "rust_native_array_codec") &&
        !json_boolean_property(node, "rust_native_array_return_owner"))
        json_object_object_add(node, "rust_native_array_inner_borrow", json_object_new_boolean(true));
    if ((handle || json_boolean_property(rust_nullable_child(node, "type"), "rust_native_handle_array")) && read)
    {
        json_object_object_del(node, "rust_needs_clone");
        json_object_object_del(node, "rust_resolved_clone");
        bool field_read = json_boolean_property(node, "rust_thread_field");
        bool owner_read = json_boolean_property(node, "rust_thread_ref_owner");
        bool cell_read = json_string_property_equals(node, "kind", "variable") &&
            (json_boolean_property(node, "rust_cell") || json_boolean_property(node, "rust_array_snapshot_cell"));
        json_object_object_add(node, "rust_native_handle_owned_read", json_object_new_boolean(json_boolean_property(node, "rust_native_array_return_owner") || (!borrow && !field_read && !cell_read && !owner_read)));
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
    if (read && (handle || json_boolean_property(rust_nullable_child(node, "type"), "rust_native_handle_array")))
        rust_native_closure_array_place(model, node);
    /* Compare array string bytes after evaluating the other operand, as C's
     * string comparison helper does. A plain array binding supplies the view's
     * checked lifetime; cell and temporary owners need separate protocols. */
    if (json_string_property_equals(node, "kind", "binary") &&
        (json_string_property_equals(node, "op", "eq") || json_string_property_equals(node, "op", "neq"))) {
        const char *sides[] = {"left", "right", NULL};
        for (int i = 0; sides[i]; i++) {
            json_object *value = rust_nullable_child(node, sides[i]);
            json_object *array = rust_nullable_child(value, "array");
            if (json_string_property_equals(value, "kind", "array_access") &&
                json_string_property_equals(rust_nullable_child(value, "type"), "kind", "string") &&
                json_boolean_property(rust_nullable_child(array, "type"), "rust_native_array_codec") &&
                ((json_string_property_equals(array, "kind", "variable") &&
                  (!json_boolean_property(array, "rust_cell") || json_boolean_property(array, "rust_closure_array_parameter") || json_boolean_property(array, "rust_array_snapshot_cell"))) ||
                 (json_string_property_equals(array, "kind", "member") &&
                  json_string_property_equals(rust_nullable_child(array, "object"), "kind", "variable") &&
                  !json_boolean_property(array, "rust_thread_field")) ||
                 json_string_property_equals(array, "kind", "call") || json_string_property_equals(array, "kind", "method_call") || json_string_property_equals(array, "kind", "static_call"))) {
                json_object_object_add(value, "rust_native_array_string_live_view", json_object_new_boolean(true));
                if (json_string_property_equals(array, "kind", "call") || json_string_property_equals(array, "kind", "method_call") || json_string_property_equals(array, "kind", "static_call"))
                    json_object_object_add(value, "rust_native_array_string_owned_view", json_object_new_boolean(true));
                if (json_boolean_property(array, "rust_closure_array_parameter") || json_boolean_property(array, "rust_array_snapshot_cell"))
                    json_object_object_add(value, "rust_native_array_string_cell_view", json_object_new_boolean(true));
            }
        }
    }

    json_object *native_function = rust_native_handle_callable(model, node);
    if (json_boolean_property(native_function, "is_native") &&
        json_boolean_property(model, "rust_native_array_string_args")) {
        json_object *params = rust_nullable_child(native_function, "params");
        json_object *args = rust_nullable_child(node, "args");
        for (size_t i = 0; args && params && i < json_object_array_length(args) && i < json_object_array_length(params); i++) {
            json_object *param = json_object_array_get_idx(params, i);
            json_object *value = json_object_array_get_idx(args, i);
            json_object *array = rust_nullable_child(value, "array");
            if (json_string_property_equals(rust_nullable_child(param, "type"), "kind", "string") &&
                json_string_property_equals(param, "mem_qual", "default") &&
                json_string_property_equals(value, "kind", "array_access") &&
                json_boolean_property(rust_nullable_child(array, "type"), "rust_native_array_codec") &&
                ((json_string_property_equals(array, "kind", "variable") &&
                  (!json_boolean_property(array, "rust_cell") || json_boolean_property(array, "rust_closure_array_parameter") || json_boolean_property(array, "rust_array_snapshot_cell"))) ||
                 (json_string_property_equals(array, "kind", "member") &&
                  json_string_property_equals(rust_nullable_child(array, "object"), "kind", "variable") &&
                  !json_boolean_property(array, "rust_thread_field")) ||
                 json_string_property_equals(array, "kind", "call") || json_string_property_equals(array, "kind", "method_call") || json_string_property_equals(array, "kind", "static_call"))) {
                json_object_object_add(value, "rust_native_array_string_live_view", json_object_new_boolean(true));
                if (json_string_property_equals(array, "kind", "call") || json_string_property_equals(array, "kind", "method_call") || json_string_property_equals(array, "kind", "static_call"))
                    json_object_object_add(value, "rust_native_array_string_owned_view", json_object_new_boolean(true));
                if (json_boolean_property(array, "rust_closure_array_parameter") || json_boolean_property(array, "rust_array_snapshot_cell"))
                    json_object_object_add(value, "rust_native_array_string_cell_view", json_object_new_boolean(true));
            }
        }
    }
    json_object *left_type = rust_nullable_child(rust_nullable_child(node, "left"), "type");
    json_object *right_type = rust_nullable_child(rust_nullable_child(node, "right"), "type");
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0 && strcmp(key, "rust_closure_array_method_args") != 0) continue;
        bool child_borrow = false;
        if ((strcmp(key, "object") == 0 || strcmp(key, "receiver") == 0) ||
            (strcmp(key, "operand") == 0 && json_string_property_equals(node, "kind", "sizeof")))
            child_borrow = true;
        if ((strcmp(key, "left") == 0 || strcmp(key, "right") == 0) &&
            (rust_native_handle_type(left_type) || rust_native_handle_type(right_type) ||
             json_boolean_property(left_type, "rust_native_handle_array") ||
             json_boolean_property(right_type, "rust_native_handle_array"))) child_borrow = true;
        /* Native-array concat reads both original C headers. Cloning its
         * right operand would introduce observable extra element retains. */
        if (strcmp(key, "args") == 0 &&
            json_string_property_equals(rust_nullable_child(node, "callee"), "member_name", "concat") &&
            json_boolean_property(rust_nullable_child(rust_nullable_child(rust_nullable_child(node, "callee"), "object"), "type"), "rust_native_handle_array"))
            child_borrow = true;
        /* A borrowed member/index projection must borrow its owner too. */
        if (strcmp(key, "array") == 0 && handle && read) child_borrow = true;
        if (strcmp(key, "array") == 0 && kind && (!strcmp(kind, "array_access") || !strcmp(kind, "index_assign")) &&
            json_boolean_property(rust_nullable_child(child, "type"), "rust_native_handle_array")) child_borrow = true;
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
                                                   json_object *inserts, size_t *next_id, bool indexed_only)
{
    if (!node || !json_object_is_type(node, json_type_object)) return;
    const char *kind = json_string_property(node, "kind");
    if (!kind) return;
    if (strcmp(kind, "borrow_inferred_call") == 0)
    {
        rust_native_handle_extract_expression(model, rust_nullable_child(node, "inner_call"), inserts, next_id, indexed_only);
        return;
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0 || strcmp(key, "borrow_check_args") == 0) continue;
        if (json_object_is_type(child, json_type_array))
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
                rust_native_handle_extract_expression(model, json_object_array_get_idx(child, i), inserts, next_id, indexed_only);
        }
        else rust_native_handle_extract_expression(model, child, inserts, next_id, indexed_only);
    }
    /* A declaration's indexed temporary keeps its C header until the
     * enclosing scope ends, even when only its string element is passed on. */
    if (!strcmp(kind, "array_access")) {
        json_object *value = rust_nullable_child(node, "array");
        json_object *type = rust_nullable_child(value, "type");
        const char *value_kind = json_string_property(value, "kind");
        if (json_boolean_property(type, "rust_native_handle_array") && value_kind &&
            (!strcmp(value_kind, "call") || !strcmp(value_kind, "method_call") || !strcmp(value_kind, "static_call"))) {
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
            json_object_object_add(node, "array", read);
        }
    }
    bool call = strcmp(kind, "call") == 0 || strcmp(kind, "method_call") == 0 || strcmp(kind, "static_call") == 0;
    if (!call || indexed_only) return;
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
        const char *borrow_flags[] = {"rust_closure_native_handle_arg", "rust_native_handle_borrow_arg", NULL};
        for (size_t f = 0; borrow_flags[f]; f++)
            if (json_boolean_property(value, borrow_flags[f]))
                json_object_object_add(read, borrow_flags[f], json_object_new_boolean(true));
        if (i == 0) json_object_object_add(receiver_parent, "object", read);
        else json_object_array_put_idx(args, i - 1, read);
    }
}

static void rust_lower_native_handle_temporaries(json_object *model, json_object *node, size_t *next_id)
{
    if (!rust_nullable_child(model, "rust_native_handles") &&
        !rust_nullable_child(model, "rust_native_handle_array_support")) return;
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
                rust_native_handle_extract_expression(model, rust_nullable_child(statement, "initializer"), inserts, next_id, false);
            if (json_string_property_equals(statement, "kind", "expr"))
                rust_native_handle_extract_expression(model, rust_nullable_child(statement, "expr"), inserts, next_id, true);
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
