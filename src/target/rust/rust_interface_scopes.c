/* C scope allocations preserve zero-sized source storage. A Rust callback owns
 * execution and cleanup while the selected C compiler owns these stack slots.
 * Do not infer empty-object identity from an OS name or an artificial byte. */
static bool rust_interface_scope_collect(json_object *node, json_object *bindings,
                                          bool *has_empty)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_interface_scope_collect(json_object_array_get_idx(node, i), bindings, has_empty))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object) ||
        json_string_property_equals(node, "kind", "lambda") ||
        json_string_property_equals(node, "kind", "sizeof")) return true;
    json_object *type = rust_nullable_child(node, "type");
    if (json_string_property_equals(node, "kind", "var_decl") &&
        json_boolean_property(type, "rust_interface_owned_metadata") &&
        !json_boolean_property(type, "pass_self_by_ref"))
    {
        json_object_array_add(bindings, json_object_get(node));
        *has_empty |= json_boolean_property(type, "rust_interface_empty");
    }
    json_object_object_foreach(node, key, child)
        if (strcmp(key, "type") != 0 && strncmp(key, "rust_", 5) != 0 &&
            !rust_interface_scope_collect(child, bindings, has_empty)) return false;
    return true;
}

static bool rust_interface_scope_callable(json_object *model, json_object *node,
                                           json_object *scopes)
{
    if (rust_nullable_child(node, "rust_interface_scope")) return true;
    json_object *body = rust_nullable_child(node, "body");
    if (!body) body = rust_nullable_child(node, "body_stmts");
    if (!body || json_boolean_property(node, "is_native")) return true;
    json_object *bindings = json_object_new_array();
    bool empty = false;
    json_object *params = rust_nullable_child(node, "params");
    for (size_t p = 0; params && p < json_object_array_length(params); p++)
    {
        json_object *param = json_object_array_get_idx(params, p);
        json_object *type = rust_nullable_child(param, "type");
        if (!json_boolean_property(type, "rust_interface_empty") ||
            json_string_property_equals(param, "mem_qual", "as_ref")) continue;
        json_object_object_add(param, "rust_interface_scope_parameter", json_object_new_boolean(true));
        json_object_object_add(param, "rust_by_value_mutated", json_object_new_boolean(true));
        json_object_array_add(bindings, json_object_get(param));
        empty = true;
    }
    /* C closure trampolines copy value captures into invocation-local
     * variables. Their identities belong to that call, not the heap capture
     * stored when the closure was created. */
    json_object *captures = rust_nullable_child(node, "captures");
    for (size_t c = 0; captures && c < json_object_array_length(captures); c++)
    {
        json_object *capture = json_object_array_get_idx(captures, c);
        json_object *type = rust_nullable_child(capture, "type");
        if (!json_boolean_property(type, "rust_interface_owned_metadata") ||
            json_boolean_property(type, "pass_self_by_ref") ||
            json_boolean_property(capture, "is_ref") ||
            json_boolean_property(capture, "c_borrowed_ref_capture")) continue;
        json_object_object_add(capture, "rust_interface_scope_capture", json_object_new_boolean(true));
        json_object_array_add(bindings, json_object_get(capture));
        empty |= json_boolean_property(type, "rust_interface_empty");
    }
    if (!rust_interface_scope_collect(body, bindings, &empty))
    { json_object_put(bindings); return false; }
    if (!empty) { json_object_put(bindings); return true; }

    const char *keys[] = {"name", "rust_name", "addresses_name"};
    const char *stems[] = {"__sn_interface_c_scope", "__sn_interface_run_scope", "__sn_interface_scope_addresses"};
    json_object *scope = json_object_new_object(), *slots = json_object_new_array();
    json_object *parameters = json_object_new_array();
    json_object *capture_slots = json_object_new_array();
    for (size_t i = 0; i < 3; i++)
    {
        char name[100];
        if (!rust_allocate_helper_name(model, stems[i], name, sizeof(name)))
        { json_object_put(bindings); json_object_put(scope); json_object_put(slots); json_object_put(parameters); json_object_put(capture_slots); return false; }
        json_object_object_add(scope, keys[i], json_object_new_string(name));
    }
    json_object_object_add(scope, "bindings", slots);
    json_object_object_add(scope, "parameters", parameters);
    json_object_object_add(scope, "captures", capture_slots);
    json_object_object_add(scope, "count", json_object_new_int64(json_object_array_length(bindings)));
    for (size_t i = 0; i < json_object_array_length(bindings); i++)
    {
        json_object *binding = json_object_array_get_idx(bindings, i);
        json_object *type = rust_nullable_child(binding, "type");
        json_object *slot = json_object_new_object();
        json_object_object_add(slot, "index", json_object_new_int64(i));
        json_object_object_add(slot, "layout_type", json_object_get(rust_nullable_child(type, "rust_interface_layout_type")));
        if (json_boolean_property(binding, "rust_interface_scope_parameter"))
        {
            json_object_object_add(slot, "parameter", json_object_new_boolean(true));
            json_object_object_add(slot, "source_name", json_object_get(rust_nullable_child(binding, "name")));
            json_object *prior = json_object_new_array();
            for (size_t p = 0; p < json_object_array_length(parameters); p++)
            {
                json_object *entry = json_object_new_object();
                json_object_object_add(entry, "index", json_object_get(rust_nullable_child(
                    json_object_array_get_idx(parameters, p), "index")));
                json_object_array_add(prior, entry);
            }
            json_object_object_add(slot, "prior_parameters", prior);
            json_object_array_add(parameters, json_object_get(slot));
        }
        if (json_boolean_property(binding, "rust_interface_scope_capture"))
        {
            json_object_object_add(slot, "source_name", json_object_get(rust_nullable_child(binding, "name")));
            json_object_object_add(slot, "existing_snapshot", json_object_new_boolean(
                json_boolean_property(binding, "rust_mutable_owned_snapshot")));
            json_object_array_add(capture_slots, json_object_get(slot));
        }
        json_object_array_add(slots, slot);
        json_object_object_add(binding, "rust_interface_scope_index", json_object_new_int64(i));
        json_object_object_add(binding, "rust_interface_scope_addresses", json_object_get(rust_nullable_child(scope, "addresses_name")));
    }
    json_object_object_add(node, "rust_interface_scope", json_object_get(scope));
    json_object_array_add(scopes, scope);
    json_object_put(bindings);
    return true;
}

static bool rust_interface_scope_lambdas(json_object *model, json_object *node,
                                         json_object *scopes)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_interface_scope_lambdas(model, json_object_array_get_idx(node, i), scopes)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_string_property_equals(node, "kind", "lambda") &&
        !rust_interface_scope_callable(model, node, scopes)) return false;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && strcmp(key, "type") != 0 &&
            strcmp(key, "lambdas") != 0 && !rust_interface_scope_lambdas(model, child, scopes)) return false;
    return true;
}

static bool rust_interface_preserve_assignment_origins(json_object *model, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_interface_preserve_assignment_origins(model, json_object_array_get_idx(node, i))) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_string_property_equals(node, "kind", "assign") &&
        json_boolean_property(rust_nullable_child(node, "type"), "rust_interface_borrow_origin") &&
        !json_boolean_property(node, "rust_record_reference_assign"))
    {
        const char *keys[] = {"rust_interface_store_value", "rust_interface_store_target", "rust_interface_store_origin"};
        for (size_t i = 0; i < 3; i++)
        {
            char name[100];
            if (!rust_allocate_helper_name(model, keys[i], name, sizeof(name))) return false;
            json_object_object_add(node, keys[i], json_object_new_string(name));
        }
        json_object_object_add(node, "rust_interface_preserve_slot", json_object_new_boolean(true));
    }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0 && !rust_interface_preserve_assignment_origins(model, child)) return false;
    return true;
}

static bool rust_interface_scope_layout(json_object *layouts, json_object *layout,
                                         json_object *ordered, json_object *seen)
{
    const char *name = json_string_property(layout, "name");
    if (!name) return false;
    if (rust_nullable_child(seen, name)) return true;
    json_object_object_add(seen, name, json_object_new_boolean(true));
    json_object *fields = rust_nullable_child(layout, "fields");
    for (size_t f = 0; fields && f < json_object_array_length(fields); f++)
    {
        json_object *type = rust_nullable_child(json_object_array_get_idx(fields, f), "type");
        if (!json_string_property_equals(type, "kind", "struct")) continue;
        bool found = false;
        for (size_t i = 0; i < json_object_array_length(layouts); i++)
        {
            json_object *child = json_object_array_get_idx(layouts, i);
            if (!json_string_property_equals(child, "name", json_string_property(type, "name"))) continue;
            if (!rust_interface_scope_layout(layouts, child, ordered, seen)) return false;
            found = true; break;
        }
        if (!found) return false;
    }
    json_object_array_add(ordered, json_object_get(layout));
    return true;
}

static bool rust_prepare_interface_scopes(json_object *model, RustNativePlan *plan)
{
    if (!json_boolean_property(model, "rust_has_interface_arguments")) return true;
    json_object *scopes = json_object_new_array();
    json_object_object_add(model, "rust_interface_scopes", scopes);
    json_object *functions = rust_nullable_child(model, "functions");
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
        if (!rust_interface_scope_callable(model, json_object_array_get_idx(functions, i), scopes)) return false;
    json_object *structures = rust_nullable_child(model, "structs");
    for (size_t s = 0; structures && s < json_object_array_length(structures); s++)
    {
        json_object *methods = rust_nullable_child(json_object_array_get_idx(structures, s), "methods");
        for (size_t m = 0; methods && m < json_object_array_length(methods); m++)
            if (!rust_interface_scope_callable(model, json_object_array_get_idx(methods, m), scopes)) return false;
    }
    if (!rust_interface_scope_lambdas(model, model, scopes)) return false;
    if (!json_object_array_length(scopes)) return true;

    /* An origin survives readonly sharing but is cleared by an owning clone.
     * A declaration installs its C destination slot after initialization. */
    for (size_t s = 0; s < json_object_array_length(structures); s++)
    {
        json_object *structure = json_object_array_get_idx(structures, s);
        if (json_boolean_property(structure, "rust_interface_owned_metadata") &&
            !json_boolean_property(structure, "pass_self_by_ref"))
            json_object_object_add(structure, "rust_interface_borrow_origin", json_object_new_boolean(true));
    }
    rust_interface_annotate_nodes(model, model);
    if (!rust_interface_preserve_assignment_origins(model, model)) return false;
    json_object *ordered = json_object_new_array(), *seen = json_object_new_object();
    json_object *layouts = rust_nullable_child(model, "rust_sizeof_layouts");
    bool ok = true;
    for (size_t l = 0; ok && layouts && l < json_object_array_length(layouts); l++)
        ok = rust_interface_scope_layout(layouts, json_object_array_get_idx(layouts, l), ordered, seen);
    json_object_put(seen);
    json_object *support = json_object_new_object();
    json_object_object_add(support, "layouts", ordered);
    json_object_object_add(support, "scopes", json_object_get(scopes));
    ok = ok && rust_native_plan_set_interface_support(plan, support);
    json_object_put(support);
    return ok;
}
