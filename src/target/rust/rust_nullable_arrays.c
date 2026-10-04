/* Rust-private null state for language arrays; C wire layout is unchanged. */
static bool rust_nullable_array_type(json_object *type)
{
    return json_string_property_equals(type, "kind", "array");
}

static bool rust_prepare_nullable_array_nodes(json_object *node,
                                               json_object *expected,
                                               json_object *return_type,
                                               json_object *model)
{
    if (!node) return false;
    bool nullable = false;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            nullable |= rust_prepare_nullable_array_nodes(
                json_object_array_get_idx(node, i), expected, return_type, model);
        return nullable;
    }
    if (!json_object_is_type(node, json_type_object) ||
        json_string_property_equals(node, "kind", "sizeof")) return false;
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "nil") &&
        rust_nullable_array_type(expected))
    {
        json_object_object_add(node, "type", json_object_get(expected));
        json_object_object_add(node, "rust_array_nil", json_object_new_boolean(true));
        json_object_object_add(node, "is_arr_temp", json_object_new_boolean(true));
        return true;
    }
    json_object *type = rust_nullable_child(node, "type");
    json_object *local_return = rust_nullable_child(node, "return_type");
    if (local_return) return_type = local_return;
    if (json_string_property_equals(node, "kind", "var_decl") &&
        rust_nullable_array_type(type) &&
        !rust_nullable_child(node, "initializer")) nullable = true;
    if (json_boolean_property(node, "is_native"))
    {
        nullable |= rust_nullable_array_type(local_return);
        json_object *params = rust_nullable_child(node, "params");
        for (size_t i = 0; params && i < json_object_array_length(params); i++)
            nullable |= rust_nullable_array_type(rust_nullable_child(
                json_object_array_get_idx(params, i), "type"));
    }
    json_object *left = rust_nullable_child(node, "left");
    json_object *right = rust_nullable_child(node, "right");
    json_object *callee_type = rust_nullable_child(
        rust_nullable_child(node, "callee"), "type");
    json_object *call_params = rust_nullable_child(callee_type, "param_types");
    if (json_string_property_equals(node, "kind", "static_call"))
    {
        json_object *structure = rust_find_struct(model, json_string_property(node, "type_name"));
        json_object *method = rust_find_resolved_method(
            structure, json_string_property(node, "method_name"), true);
        call_params = rust_nullable_child(method, "params");
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) == 0) continue;
        json_object *context = NULL;
        if (strcmp(key, "initializer") == 0 || strcmp(key, "default_value") == 0)
            context = type;
        else if (strcmp(key, "value") == 0)
            context = json_string_property_equals(node, "kind", "return")
                ? return_type : type;
        else if (strcmp(key, "left") == 0)
            context = rust_nullable_child(right, "type");
        else if (strcmp(key, "right") == 0)
            context = rust_nullable_child(left, "type");
        else if (strcmp(key, "elements") == 0)
            context = rust_nullable_child(type, "element_type");
        if (strcmp(key, "fields") == 0 &&
            json_string_property_equals(node, "kind", "struct_literal"))
        {
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *field = json_object_array_get_idx(child, i);
                nullable |= rust_prepare_nullable_array_nodes(
                    rust_nullable_child(field, "value"),
                    rust_nullable_field_type(type, json_string_property(field, "name")),
                    return_type, model);
            }
        }
        else if (strcmp(key, "args") == 0 && call_params)
        {
            json_object *params = call_params;
            for (size_t i = 0; i < json_object_array_length(child); i++)
            {
                json_object *param = params && i < json_object_array_length(params)
                    ? json_object_array_get_idx(params, i) : NULL;
                json_object *param_type = rust_nullable_child(param, "type");
                nullable |= rust_prepare_nullable_array_nodes(
                    json_object_array_get_idx(child, i), param_type ? param_type : param,
                    return_type, model);
                json_object *argument = json_object_array_get_idx(child, i);
                if (json_boolean_property(argument, "rust_array_nil"))
                {
                    const char *qualifier = json_string_property(param, "mem_qual");
                    json_object *qualifiers = rust_nullable_child(callee_type, "param_mem_quals");
                    if (!qualifier && qualifiers && i < json_object_array_length(qualifiers))
                        qualifier = json_object_get_string(json_object_array_get_idx(qualifiers, i));
                    if (!qualifier || strcmp(qualifier, "as_val") != 0)
                        json_object_object_add(argument, "is_borrow_tmp", json_object_new_boolean(true));
                }
            }
        }
        else nullable |= rust_prepare_nullable_array_nodes(child, context, return_type, model);
    }
    return nullable;
}

static void rust_mark_nullable_array_types(json_object *node, const char *name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_mark_nullable_array_types(json_object_array_get_idx(node, i), name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (rust_nullable_array_type(node))
        json_object_object_add(node, "rust_nullable_array_name", json_object_new_string(name));
    json_object *type = rust_nullable_child(node, "type");
    if (json_string_property_equals(node, "kind", "binary") &&
        (json_string_property_equals(node, "op", "eq") || json_string_property_equals(node, "op", "neq")))
    {
        json_object *left = rust_nullable_child(node, "left");
        json_object *right = rust_nullable_child(node, "right");
        if (json_boolean_property(left, "rust_array_nil") || json_boolean_property(right, "rust_array_nil"))
        {
            json_object_object_add(node, "rust_array_nil_comparison", json_object_new_boolean(true));
            json_object_object_add(node, "rust_float_array_equality", json_object_new_boolean(true));
            json_object_object_add(node, "rust_array_nil_operand", json_object_get(
                json_boolean_property(left, "rust_array_nil") ? right : left));
        }
    }
    const char *kind = json_string_property(node, "kind");
    bool vector_result = kind && (strcmp(kind, "array_literal") == 0 ||
        strcmp(kind, "array") == 0 || strcmp(kind, "sized_array") == 0 ||
        strcmp(kind, "range") == 0 || strcmp(kind, "array_slice") == 0);
    if (rust_nullable_array_type(type) && json_string_property_equals(node, "kind", "call"))
    {
        json_object *callee = rust_nullable_child(node, "callee");
        json_object *object = rust_nullable_child(callee, "object");
        vector_result |= json_string_property_equals(rust_nullable_child(object, "type"), "kind", "string");
    }
    if (vector_result && rust_nullable_array_type(type))
        json_object_object_add(node, "rust_nullable_array_value", json_object_new_boolean(true));
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_mark_nullable_array_types(child, name);
    }
}

static void rust_prepare_nullable_arrays(json_object *model)
{
    if (!rust_prepare_nullable_array_nodes(model, NULL, NULL, model)) return;
    char name[128];
    size_t id = 0;
    do { snprintf(name, sizeof(name), "__sn_nullable_array_%zu", id++); }
    while (rust_model_contains_string(model, name));
    rust_mark_nullable_array_types(model, name);
    json_object_object_add(model, "rust_nullable_arrays", json_object_new_boolean(true));
    json_object_object_add(model, "rust_nullable_array_name", json_object_new_string(name));
}

/* Callback-free nested owners, including constant negative indices.  Keep
 * this broader classification private to stores; call admission is unchanged. */
static bool rust_nested_array_stable_place(json_object *node)
{
    if (json_string_property_equals(node, "kind", "variable")) return true;
    if (json_string_property_equals(node, "kind", "member"))
        return rust_nested_array_stable_place(rust_nullable_child(node, "object"));
    if (!json_string_property_equals(node, "kind", "array_access")) return false;
    json_object *index = rust_nullable_child(node, "index");
    bool constant_negative = json_string_property_equals(index, "kind", "unary") &&
        json_string_property_equals(index, "op", "negate") &&
        json_string_property_equals(rust_nullable_child(index, "operand"), "kind", "literal");
    return rust_nested_array_stable_place(rust_nullable_child(node, "array")) &&
        (json_string_property_equals(index, "kind", "literal") ||
         rust_call_stable_place(index) || constant_negative);
}

/* Resolve nested stable indices before taking the store's mutable borrow. */
static bool rust_lower_nullable_array_stores(json_object *model, json_object *node,
                                              size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_lower_nullable_array_stores(model, json_object_array_get_idx(node, i), next_id))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0 &&
            !rust_lower_nullable_array_stores(model, child, next_id)) return false;
    }
    json_object *array = rust_nullable_child(node, "array");
    if (!json_string_property_equals(node, "kind", "index_assign") ||
        !json_string_property_equals(array, "kind", "array_access") ||
        !rust_nested_array_stable_place(array) || json_boolean_property(node, "rust_capture_index_assign"))
        return true;
    json_object *store = json_object_new_object();
    json_object_object_add(store, "kind", json_object_new_string("array_access"));
    json_object *copy = NULL;
    json_object_deep_copy(array, &copy, NULL);
    json_object_object_add(store, "array", copy);
    copy = NULL;
    json_object_deep_copy(rust_nullable_child(node, "index"), &copy, NULL);
    json_object_object_add(store, "index", copy);
    json_object_object_add(store, "type", json_object_get(rust_nullable_child(node, "type")));
    json_object *indices = json_object_new_array();
    if (!rust_collect_place_indices_mode(model, store, indices, next_id, true))
    { json_object_put(store); json_object_put(indices); return false; }
    rust_cleanup_length_reads(store);
    json_object_object_add(node, "rust_nullable_array_store", store);
    json_object_object_add(node, "rust_nullable_array_indices", indices);
    char name[128];
    if (!rust_allocate_helper_name(model, "__sn_nullable_store_value", name, sizeof(name))) return false;
    json_object_object_add(node, "rust_nullable_array_store_value", json_object_new_string(name));
    return true;
}
