/* An array field is an owner, not a temporary Vec snapshot. Specialize
 * ordinary default-array calls to carry that owner through forwarding and
 * recursion. Each expression borrows it only after evaluating callbacks. */
static bool rust_field_array_owner(json_object *node)
{
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    return json_string_property_equals(type, "kind", "array") &&
        (json_boolean_property(node, "rust_thread_field") ||
         json_boolean_property(node, "rust_field_array_value"));
}

static void rust_field_array_binding_uses(json_object *node, int64_t binding_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_field_array_binding_uses(json_object_array_get_idx(node, i), binding_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *id = NULL;
    if (json_string_property_equals(node, "kind", "variable") &&
        json_object_object_get_ex(node, "rust_binding_id", &id) &&
        json_object_get_int64(id) == binding_id)
    {
        json_object_object_add(node, "rust_field_array_value", json_object_new_boolean(true));
        json_object_object_del(node, "rust_needs_clone");
    }
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        rust_field_array_binding_uses(child, binding_id);
    }
}

static bool rust_field_array_calls_walk(json_object *model, json_object *functions, json_object *node);

static bool rust_field_array_call(json_object *model, json_object *functions, json_object *call)
{
    const char *kind = json_string_property(call, "kind");
    if (!kind || (strcmp(kind, "call") != 0 && strcmp(kind, "static_call") != 0 && strcmp(kind, "method_call") != 0) ||
        json_boolean_property(call, "is_closure_call") ||
        json_boolean_property(call, "is_fn_field_call") ||
        json_boolean_property(call, "rust_field_array_specialized")) return true;
    json_object *callee = NULL, *args = NULL;
    json_object_object_get_ex(call, "callee", &callee);
    json_object_object_get_ex(call, "args", &args);
    if (!args) return true;
    const char *origin = NULL, *rewrite_key = NULL;
    json_object *rewrite = NULL, *declarations = functions, *function = NULL;
    if (strcmp(kind, "call") == 0 && json_string_property_equals(callee, "kind", "variable"))
    {
        origin = json_string_property(callee, "name");
        function = rust_default_array_function(functions, origin);
        rewrite = callee; rewrite_key = "name";
    }
    else
    {
        json_object *structure_type = NULL, *object = NULL;
        const char *structure_name = NULL;
        bool is_static = strcmp(kind, "static_call") == 0 || json_boolean_property(call, "is_static");
        if (strcmp(kind, "call") == 0)
        {
            if (!json_string_property_equals(callee, "kind", "member")) return true;
            json_object_object_get_ex(callee, "object", &object);
            json_object_object_get_ex(object, "type", &structure_type);
            origin = json_string_property(callee, "member_name");
            rewrite = callee; rewrite_key = "member_name";
        }
        else
        {
            json_object_object_get_ex(call, "struct_type", &structure_type);
            structure_name = json_string_property(call, "type_name");
            origin = json_string_property(call, "method_name");
            rewrite = call; rewrite_key = "method_name";
        }
        if (!structure_name) structure_name = json_string_property(structure_type, "name");
        json_object *structure = rust_find_struct(model, structure_name);
        function = rust_find_resolved_method(structure, origin, is_static);
        if (!structure || !function || json_boolean_property(function, "is_native")) return true;
        json_object_object_get_ex(structure, "methods", &declarations);
    }
    json_object *params = NULL;
    json_object_object_get_ex(function, "params", &params);
    size_t count = json_object_array_length(args);
    if (!function || !params || json_object_array_length(params) != count) return true;
    json_object *pattern = json_object_new_array();
    bool selected = false;
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        json_object *param = json_object_array_get_idx(params, i);
        bool field = rust_field_array_owner(arg) && json_boolean_property(param, "rust_default_array_ref") &&
            !json_boolean_property(param, "needs_array_copy");
        json_object_array_add(pattern, json_object_new_boolean(field));
        selected |= field;
    }
    if (!selected) { json_object_put(pattern); return true; }
    json_object *specialized = NULL;
    for (size_t i = 0; i < json_object_array_length(declarations); i++)
    {
        json_object *candidate = json_object_array_get_idx(declarations, i), *existing_pattern = NULL;
        if (json_string_property_equals(candidate, "rust_field_array_origin", origin) &&
            json_object_object_get_ex(candidate, "rust_field_array_pattern", &existing_pattern) &&
            json_object_equal(existing_pattern, pattern)) { specialized = candidate; break; }
    }
    bool created = !specialized;
    if (created)
    {
        if (json_object_deep_copy(function, &specialized, NULL) != 0 || !specialized)
        { json_object_put(pattern); return false; }
        json_object *specialized_params = NULL, *body = NULL;
        json_object_object_get_ex(specialized, "params", &specialized_params);
        json_object_object_get_ex(specialized, "body", &body);
        for (size_t i = 0; i < count; i++)
        {
            if (!json_object_get_boolean(json_object_array_get_idx(pattern, i))) continue;
            json_object *param = json_object_array_get_idx(specialized_params, i), *id = NULL;
            if (!json_object_object_get_ex(param, "rust_binding_id", &id))
            { json_object_put(pattern); json_object_put(specialized); return false; }
            json_object_object_add(param, "rust_field_array_param", json_object_new_boolean(true));
            rust_field_array_binding_uses(body, json_object_get_int64(id));
        }
        char name[96];
        if (!rust_allocate_helper_name(model, "__sn_field_array_call", name, sizeof(name)))
        { json_object_put(pattern); json_object_put(specialized); return false; }
        json_object_object_add(specialized, "name", json_object_new_string(name));
        json_object_object_add(specialized, "rust_field_array_origin", json_object_new_string(origin));
        json_object_object_add(specialized, "rust_field_array_pattern", json_object_get(pattern));
        /* Register before following recursive calls. */
        json_object_array_add(declarations, specialized);
    }
    for (size_t i = 0; i < count; i++)
    {
        if (!json_object_get_boolean(json_object_array_get_idx(pattern, i))) continue;
        json_object *arg = json_object_array_get_idx(args, i);
        json_object_object_add(arg, "rust_field_array_arg", json_object_new_boolean(true));
        json_object_object_del(arg, "rust_default_array_ref_arg");
        json_object_object_del(arg, "is_ref_arg");
        json_object_object_del(arg, "is_borrow_tmp");
        json_object_object_del(arg, "rust_needs_clone");
    }
    json_object_object_add(rewrite, rewrite_key, json_object_new_string(json_string_property(specialized, "name")));
    json_object_object_add(call, "rust_field_array_specialized", json_object_new_boolean(true));
    json_object_put(pattern);
    json_object *body = NULL;
    json_object_object_get_ex(specialized, "body", &body);
    return !created || rust_field_array_calls_walk(model, functions, body);
}

static bool rust_field_array_calls_walk(json_object *model, json_object *functions, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_field_array_calls_walk(model, functions, json_object_array_get_idx(node, i))) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0 && !rust_field_array_calls_walk(model, functions, child)) return false;
    }
    return rust_field_array_call(model, functions, node);
}

static bool rust_specialize_field_array_calls(json_object *model)
{
    json_object *functions = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    return rust_field_array_calls_walk(model, functions, model);
}
