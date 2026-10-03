static void rust_lower_float_conversions(json_object *model, json_object *node,
                                         json_object *return_type);

/* This private expression view follows the actual C expression widths. Source
 * integer literals render as LL, whereas checked arithmetic has a typed helper
 * boundary. A resolved int32 result alone does not describe an unchecked RHS. */
static const char *rust_numeric_c_kind(json_object *node)
{
    json_object *type = NULL;
    json_object_object_get_ex(node, "type", &type);
    const char *kind = json_string_property(type, "kind");
    if (rust_numeric_floating_kind(kind)) return rust_float_array_c_kind(node);
    if (!rust_fixed_integral_kind(kind)) return NULL;
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "int"))
    {
        const char *value = json_string_property(node, "value");
        /* Positive LL tokens beyond INT64_MAX use the compiler's unsigned
         * extension; encoded uint64 values with a minus sign remain signed LL. */
        return value && value[0] != '-' && strtoull(value, NULL, 10) > INT64_MAX ? "uint" : "int";
    }
    if (json_string_property_equals(node, "kind", "unary"))
    {
        json_object *operand = NULL;
        json_object_object_get_ex(node, "operand", &operand);
        const char *inner = rust_numeric_c_kind(operand);
        return inner && strcmp(inner, "byte") == 0 ? "int32" : inner;
    }
    if (json_string_property_equals(node, "kind", "binary") &&
        !json_string_property(node, "rust_checked_method"))
    {
        json_object *left = NULL, *right = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        const char *left_kind = rust_numeric_c_kind(left), *right_kind = rust_numeric_c_kind(right);
        if (!left_kind || !right_kind) return kind;
        const char *common = rust_integral_promotion_type(left_kind, right_kind, json_string_property(node, "op"));
        if (common && strcmp(common, "i64") == 0) return "int";
        if (common && strcmp(common, "u64") == 0) return "uint";
        if (common && strcmp(common, "u32") == 0) return "uint32";
        if (common && strcmp(common, "i32") == 0) return "int32";
    }
    return kind;
}

static void rust_numeric_c_values(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_numeric_c_values(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_numeric_c_values(child);
    }
    const char *kind = rust_numeric_c_kind(node);
    if (kind)
        json_object_object_add(node, "rust_c_numeric_expression_kind", json_object_new_string(kind));
    if (kind && json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "int"))
        json_object_object_add(node, "rust_c_numeric_literal_type", json_object_new_string(rust_numeric_type_name(kind)));
    if (json_string_property_equals(node, "kind", "binary"))
    {
        json_object *left = NULL, *right = NULL, *type = NULL;
        json_object_object_get_ex(node, "left", &left);
        json_object_object_get_ex(node, "right", &right);
        json_object_object_get_ex(node, "type", &type);
        const char *left_kind = rust_numeric_c_kind(left), *right_kind = rust_numeric_c_kind(right);
        if (!rust_fixed_integral_kind(left_kind) || !rust_fixed_integral_kind(right_kind)) return;
        bool checked = json_string_property_equals(node, "arithmetic_mode", "checked") &&
                       json_string_property(node, "rust_checked_method");
        const char *common = checked ? rust_numeric_type_name(json_string_property(type, "kind")) :
            rust_integral_promotion_type(left_kind, right_kind, json_string_property(node, "op"));
        if (common)
        {
            json_object_object_add(node, "rust_mixed_integral_binary_type", json_object_new_string(common));
            json_object_object_add(node, "rust_mixed_integral_binary_wrapping", json_object_new_boolean(!checked));
        }
    }
}

/* C's chain flattener hoists owned array receivers before the enclosing
 * statement. Reuse that owner for read/store/negative-length projections. */
static bool rust_numeric_place_owners(json_object *model, json_object *place,
                                      json_object *bindings)
{
    if (!place) return true;
    json_object *parent = NULL;
    bool indexed = json_string_property_equals(place, "kind", "array_access");
    const char *key = indexed ? "array" : "object";
    if (!indexed && !json_string_property_equals(place, "kind", "member") &&
        !json_string_property_equals(place, "kind", "member_access")) return true;
    json_object_object_get_ex(place, key, &parent);
    if (!rust_numeric_place_owners(model, parent, bindings)) return false;
    bool stable = json_string_property_equals(parent, "kind", "variable") ||
                  json_string_property_equals(parent, "kind", "array_access") ||
                  ((json_string_property_equals(parent, "kind", "member") ||
                    json_string_property_equals(parent, "kind", "member_access")) &&
                   !json_boolean_property(parent, "needs_struct_tmp_lift"));
    json_object *type = NULL;
    json_object_object_get_ex(parent, "type", &type);
    if (indexed && !stable && json_string_property_equals(type, "kind", "array"))
    {
        char name[96];
        if (!rust_allocate_helper_name(model, "__sn_numeric_owner", name, sizeof(name))) return false;
        json_object *binding = json_object_new_object(), *ref = json_object_new_object();
        json_object_object_add(binding, "name", json_object_new_string(name));
        json_object_object_add(binding, "type", json_object_get(type));
        json_object_object_add(binding, "initializer", json_object_get(parent));
        json_object_array_add(bindings, binding);
        json_object_object_add(ref, "kind", json_object_new_string("variable"));
        json_object_object_add(ref, "name", json_object_new_string(name));
        json_object_object_add(ref, "type", json_object_get(type));
        json_object_object_add(place, key, ref);
    }
    return true;
}

/* Resolved indices use C's negative-index length reads. Keep the final indexed
 * expression a place, so temporary owners live through the enclosing borrow,
 * rather than returning a reference from an inner temporary-owner block. */
static void rust_numeric_place_projection(json_object *place, bool store)
{
    if (!place) return;
    json_object_object_del(place, "rust_needs_clone");
    if (json_string_property_equals(place, "kind", "variable"))
    {
        if (store && json_boolean_property(place, "rust_array_snapshot_cell"))
            json_object_object_add(place, "rust_numeric_place_array_cell", json_object_new_boolean(true));
        if (store && (json_boolean_property(place, "rust_cell") ||
                      json_boolean_property(place, "rust_thread_array_read")))
            json_object_object_add(place, "rust_numeric_place_mutex", json_object_new_boolean(true));
        return;
    }
    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "array_access"))
    {
        json_object_object_add(place, "rust_cleanup_length_read", json_object_new_boolean(true));
        json_object_object_get_ex(place, "array", &parent);
    }
    else if (json_string_property_equals(place, "kind", "member") ||
             json_string_property_equals(place, "kind", "member_access"))
        json_object_object_get_ex(place, "object", &parent);
    if (parent) rust_numeric_place_projection(parent, store);
}

static bool rust_lower_numeric_places_walk(json_object *model, json_object *node,
                                           size_t *next_id, json_object *params, bool in_lambda)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_lower_numeric_places_walk(model, json_object_array_get_idx(node, i), next_id, params, in_lambda))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object *own_params = NULL;
    if (json_object_object_get_ex(node, "params", &own_params)) params = own_params;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_lower_numeric_places_walk(model, value, next_id, params,
            in_lambda || json_string_property_equals(node, "kind", "lambda"))) return false;
    }
    json_object *literal_type = NULL;
    json_object_object_get_ex(node, "type", &literal_type);
    const char *literal_value = json_string_property(node, "value");
    if (json_string_property_equals(node, "kind", "literal") &&
        json_string_property_equals(node, "value_kind", "int") &&
        json_string_property_equals(literal_type, "kind", "uint") &&
        literal_value && literal_value[0] == '-')
        json_object_object_add(node, "rust_unsigned_encoded_literal", json_object_new_boolean(true));
    if (!json_boolean_property(node, "rust_numeric_computed_mutation")) return true;

    bool compound = json_string_property_equals(node, "kind", "compound_assign");
    json_object *place = NULL, *type = NULL, *store = NULL, *store_indices = json_object_new_array();
    json_object_object_get_ex(node, compound ? "target" : "operand", &place);
    json_object_object_get_ex(place, "type", &type);
    json_object *owners = json_object_new_array();
    /* C collects lambda definitions before chain flattening. Its source
     * lambda is scanned eagerly at construction, while the separately emitted
     * callable body retains the original receiver expressions. */
    json_object *owner_place = place;
    if (in_lambda)
    {
        owner_place = NULL;
        json_object_deep_copy(place, &owner_place, NULL);
    }
    if (!rust_numeric_place_owners(model, owner_place, owners)) return false;
    if (in_lambda) json_object_put(owner_place);
    json_object_object_add(node, "rust_numeric_owner_bindings", owners);
    json_object *base = place;
    while (json_string_property_equals(base, "kind", "member") ||
           json_string_property_equals(base, "kind", "member_access") ||
           json_string_property_equals(base, "kind", "array_access"))
    {
        json_object *parent = NULL;
        json_object_object_get_ex(base, json_string_property_equals(base, "kind", "array_access") ? "array" : "object", &parent);
        base = parent;
    }
    if (params && json_string_property_equals(base, "kind", "variable"))
        for (size_t i = 0; i < json_object_array_length(params); i++)
        {
            json_object *param = json_object_array_get_idx(params, i), *param_type = NULL;
            json_object_object_get_ex(param, "type", &param_type);
            if (json_string_property_equals(param, "name", json_string_property(base, "name")) &&
                json_string_property_equals(param_type, "kind", "struct") &&
                !json_string_property_equals(param, "mem_qual", "as_ref"))
                json_object_object_add(param, "rust_by_value_mutated", json_object_new_boolean(true));
        }
    json_object_deep_copy(place, &store, NULL);
    if (!rust_collect_place_indices_mode(model, store, store_indices, next_id, true)) return false;
    rust_numeric_place_projection(store, true);
    json_object_object_add(node, "rust_numeric_store", store);
    json_object_object_add(node, "rust_numeric_store_indices", store_indices);
    json_object_object_add(node, "rust_numeric_storage_type", json_object_get(type));
    json_object_object_add(node, "rust_numeric_place_compound", json_object_new_boolean(compound));

    const char *left_kind = json_string_property(type, "kind");
    const char *common = rust_numeric_type_name(left_kind);
    if (compound)
    {
        json_object *read = NULL, *read_indices = json_object_new_array(), *rhs = NULL, *rhs_type = NULL, *value = NULL;
        json_object_deep_copy(place, &read, NULL);
        if (!rust_collect_place_indices_mode(model, read, read_indices, next_id, true)) return false;
        rust_numeric_place_projection(read, false);
        json_object_object_add(node, "rust_numeric_read", read);
        json_object_object_add(node, "rust_numeric_read_indices", read_indices);
        json_object_object_get_ex(node, "value", &value);
        json_object_deep_copy(value, &rhs, NULL);
        rust_float_array_c_values(rhs);
        rust_numeric_c_values(rhs);
        rust_lower_float_conversions(model, rhs, NULL);
        json_object_object_get_ex(rhs, "type", &rhs_type);
        const char *right_kind = rust_numeric_c_kind(rhs);
        if (!right_kind) right_kind = rust_integral_c_operand_kind(rhs, json_string_property(rhs_type, "kind"));
        const char *op = json_string_property(node, "op");
        if (rust_numeric_floating_kind(left_kind) || rust_numeric_floating_kind(right_kind))
            common = strcmp(left_kind, "double") == 0 || strcmp(right_kind, "double") == 0 ? "f64" : "f32";
        else common = rust_integral_promotion_type(left_kind, right_kind, op);
        json_object_object_add(node, "rust_numeric_place_rhs", rhs);
    }
    else if (strcmp(left_kind, "byte") == 0) common = "i32";
    json_object_object_add(node, "rust_numeric_place_common_type", json_object_new_string(common));
    json_object_object_add(node, "rust_numeric_place_floating", json_object_new_boolean(common[0] == 'f'));
    const char *bases[] = {"__sn_numeric_old", "__sn_numeric_rhs", "__sn_numeric_next", "__sn_numeric_place"};
    const char *keys[] = {"rust_numeric_old_name", "rust_numeric_rhs_name", "rust_numeric_next_name", "rust_numeric_place_name"};
    for (size_t i = 0; i < 4; i++)
    {
        char name[96];
        if (!rust_allocate_helper_name(model, bases[i], name, sizeof(name))) return false;
        json_object_object_add(node, keys[i], json_object_new_string(name));
    }
    return true;
}

/* Attach the bindings to the statement that C's chain pass would prepend
 * them to. Lambda source bodies are scanned at construction. Private projections are
 * copies of source expressions and must not cause duplicate declarations. */
static void rust_numeric_owner_statements(json_object *node, json_object *statement,
                                           bool in_lambda)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_numeric_owner_statements(json_object_array_get_idx(node, i), statement, in_lambda);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    const char *kind = json_string_property(node, "kind");
    const char *statements[] = {"using", "lock", "return", "var_decl", "expr", "if", "while", "for",
                               "for_each", "for_each_iter", "block", "break", "continue"};
    for (size_t i = 0; !in_lambda && kind && i < sizeof(statements) / sizeof(statements[0]); i++)
        if (strcmp(kind, statements[i]) == 0) { statement = node; break; }
    if (kind && strcmp(kind, "lambda") == 0) in_lambda = true;
    json_object *bindings = NULL;
    if (json_boolean_property(node, "rust_numeric_computed_mutation") && statement &&
        json_object_object_get_ex(node, "rust_numeric_owner_bindings", &bindings))
    {
        json_object *combined = NULL;
        if (!json_object_object_get_ex(statement, "rust_numeric_owner_bindings", &combined))
        {
            combined = json_object_new_array();
            json_object_object_add(statement, "rust_numeric_owner_bindings", combined);
        }
        for (size_t i = 0; i < json_object_array_length(bindings); i++)
        {
            json_object *binding = json_object_array_get_idx(bindings, i), *init = NULL;
            json_object_object_get_ex(binding, "initializer", &init);
            rust_numeric_owner_statements(init, statement, in_lambda);
            json_object_array_add(combined, json_object_get(binding));
        }
    }
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_numeric_owner_statements(child, statement, in_lambda);
    }
}

static bool rust_lower_numeric_places(json_object *model, json_object *node,
                                      size_t *next_id)
{
    if (!rust_lower_numeric_places_walk(model, node, next_id, NULL, false)) return false;
    rust_numeric_owner_statements(node, NULL, false);
    return true;
}
