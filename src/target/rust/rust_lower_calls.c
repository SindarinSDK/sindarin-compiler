/* Included by rust_lower.c. Call annotations retain their original passes. */

static bool rust_owned_value_type(json_object *node);

static bool rust_owned_string_call_argument(json_object *node)
{
    json_object *type = NULL;
    if (!json_object_object_get_ex(node, "type", &type)) return false;
    return json_string_property_equals(type, "kind", "string");
}

static void rust_lower_call_strings(json_object *node, const char *kind)
{
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL, *object = NULL, *type = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_string_property_equals(callee, "kind", "member") &&
            json_object_object_get_ex(callee, "object", &object) &&
            json_object_object_get_ex(object, "type", &type) &&
            json_string_property_equals(type, "kind", "string"))
        {
            const char *method = json_string_property(callee, "member_name");
            if (method)
                json_object_object_add(node, "rust_string_method",
                                       json_object_new_string(method));
        }

        /* Sindarin passes owned strings by value without consuming an lvalue at
         * the call site. C's string ABI does not need an acquire annotation for
         * every default parameter, so record Rust's move/clone decision here. */
        bool copies_owned_args = false;
        if (json_object_object_get_ex(node, "callee", &callee))
        {
            copies_owned_args = json_string_property_equals(callee, "kind", "variable");
            if (!copies_owned_args &&
                json_string_property_equals(callee, "kind", "member") &&
                json_object_object_get_ex(callee, "object", &object) &&
                json_object_object_get_ex(object, "type", &type))
            {
                copies_owned_args = json_string_property_equals(type, "kind", "struct");
                if (json_string_property_equals(type, "kind", "pointer"))
                {
                    json_object *base_type = NULL;
                    copies_owned_args =
                        json_object_object_get_ex(type, "base_type", &base_type) &&
                        json_string_property_equals(base_type, "kind", "struct");
                }
            }
        }
        if (copies_owned_args)
        {
            json_object *args = NULL;
            if (json_object_object_get_ex(node, "args", &args))
            {
                size_t count = json_object_array_length(args);
                for (size_t i = 0; i < count; i++)
                {
                    json_object *arg = json_object_array_get_idx(args, i);
                    const char *arg_kind = json_string_property(arg, "kind");
                    if (!json_boolean_property(arg, "is_ref_arg") &&
                        !json_boolean_property(arg, "is_copy_arg") &&
                        rust_owned_string_call_argument(arg) &&
                        arg_kind && (strcmp(arg_kind, "variable") == 0 ||
                                     strcmp(arg_kind, "member") == 0 ||
                                     strcmp(arg_kind, "array_access") == 0))
                        json_object_object_add(arg, "rust_needs_clone",
                                               json_object_new_boolean(true));
                }
            }
        }
    }
    else if (strcmp(kind, "static_call") == 0)
    {
        json_object *args = NULL;
        if (json_object_object_get_ex(node, "args", &args))
        {
            size_t count = json_object_array_length(args);
            for (size_t i = 0; i < count; i++)
            {
                json_object *arg = json_object_array_get_idx(args, i);
                const char *arg_kind = json_string_property(arg, "kind");
                if (!json_boolean_property(arg, "is_ref_arg") &&
                    !json_boolean_property(arg, "is_copy_arg") &&
                    rust_owned_string_call_argument(arg) &&
                    arg_kind && (strcmp(arg_kind, "variable") == 0 ||
                                 strcmp(arg_kind, "member") == 0 ||
                                 strcmp(arg_kind, "array_access") == 0))
                    json_object_object_add(arg, "rust_needs_clone",
                                           json_object_new_boolean(true));
            }
        }
    }
}

/* C array search compares non-string elements byte-for-byte. Mark floating
 * searches so Rust preserves C behavior for signed zero and NaN payloads. */
static void rust_lower_array_searches(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_array_searches(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_array_searches(value);
    }

    if (!json_string_property_equals(node, "kind", "call")) return;
    json_object *callee = NULL, *object = NULL, *array_type = NULL, *element_type = NULL;
    if (!json_object_object_get_ex(node, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &array_type) ||
        !json_string_property_equals(array_type, "kind", "array") ||
        !json_object_object_get_ex(array_type, "element_type", &element_type)) return;

    const char *method = json_string_property(callee, "member_name");
    const char *element_kind = json_string_property(element_type, "kind");
    if (!method || (strcmp(method, "contains") != 0 && strcmp(method, "indexOf") != 0) ||
        !element_kind || (strcmp(element_kind, "float") != 0 &&
                          strcmp(element_kind, "double") != 0)) return;

    json_object_object_add(node, "rust_float_array_search", json_object_new_boolean(true));
    json_object_object_add(node, "rust_float_array_search_type",
                           json_object_new_string(strcmp(element_kind, "float") == 0 ? "f32" : "f64"));
}

static bool rust_owned_value_type(json_object *node)
{
    json_object *type = NULL;
    if (!json_object_object_get_ex(node, "type", &type)) return false;
    const char *kind = json_string_property(type, "kind");
    return kind && (strcmp(kind, "string") == 0 ||
                    strcmp(kind, "array") == 0 ||
                    strcmp(kind, "struct") == 0);
}

static void rust_mark_instance_method_clones(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_instance_method_clones(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    /* Borrowed match patterns are rendered only behind as_str(); neither the
     * pattern nor a variable-rooted owner chain must be cloned or moved. */
    if (json_boolean_property(node, "rust_string_pattern_borrowed")) return;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0 &&
        json_string_property_equals(node, "name", "self"))
    {
        if (!json_boolean_property(node, "rust_resolved_clone"))
            json_object_object_add(node, "rust_needs_clone",
                                   json_object_new_boolean(true));
        return;
    }
    if (kind && strcmp(kind, "member_assign") == 0)
    {
        json_object *value = NULL;
        if (json_object_object_get_ex(node, "value", &value))
            rust_mark_instance_method_clones(value);
        return;
    }
    if (kind && strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(node, "object", &object) &&
            json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self"))
        {
            if (rust_owned_value_type(node) &&
                !json_boolean_property(node, "rust_resolved_clone"))
                json_object_object_add(node, "rust_needs_clone",
                                       json_object_new_boolean(true));
            return;
        }
    }
    if (kind && strcmp(kind, "index_assign") == 0)
    {
        json_object *index = NULL, *value = NULL;
        if (json_object_object_get_ex(node, "index", &index))
            rust_mark_instance_method_clones(index);
        if (!json_boolean_property(node, "source_is_borrow") &&
            json_object_object_get_ex(node, "value", &value))
            rust_mark_instance_method_clones(value);
        return;
    }
    if (kind && (strcmp(kind, "call") == 0 ||
                 strcmp(kind, "method_call") == 0))
    {
        json_object *args = NULL;
        if (json_object_object_get_ex(node, "args", &args))
            rust_mark_instance_method_clones(args);
        return;
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_mark_instance_method_clones(value);
    }

    if (kind && (strcmp(kind, "member") == 0 ||
                 strcmp(kind, "array_access") == 0) &&
        rust_owned_value_type(node) &&
        !json_boolean_property(node, "rust_resolved_clone"))
        json_object_object_add(node, "rust_needs_clone",
                               json_object_new_boolean(true));
}

/* Select private receiver-stabilization names against every string in the
 * model so a source helper-like identifier cannot be captured. */
static bool rust_call_model_contains_string(json_object *node,
                                            const char *wanted)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_string))
        return strcmp(json_object_get_string(node), wanted) == 0;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_call_model_contains_string(
                    json_object_array_get_idx(node, i), wanted)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_call_model_contains_string(value, wanted)) return true;
    }
    return false;
}

static void rust_stabilize_resolved_receiver(json_object *model,
                                             json_object *expr,
                                             json_object *prefix,
                                             size_t *next_id)
{
    if (json_object_is_type(expr, json_type_array))
    {
        size_t count = json_object_array_length(expr);
        for (size_t i = 0; i < count; i++)
            rust_stabilize_resolved_receiver(
                model, json_object_array_get_idx(expr, i), prefix, next_id);
        return;
    }
    if (!json_object_is_type(expr, json_type_object)) return;

    if (json_string_property_equals(expr, "kind", "array_access"))
    {
        json_object *array = NULL, *index = NULL;
        bool has_array = json_object_object_get_ex(expr, "array", &array);
        if (has_array && rust_call_stable_place(array))
            rust_stabilize_resolved_receiver(model, array, prefix, next_id);
        else if (has_array)
        {
            /* The array operand precedes its index in source evaluation order,
             * so append its owning temporary before stabilizing the index. */
            json_object *array_type = NULL;
            if (json_object_object_get_ex(array, "type", &array_type))
            {
                char owner_name[80];
                do
                {
                    size_t id = (*next_id)++;
                    snprintf(owner_name, sizeof(owner_name),
                             "__sn_resolved_owner_%zu", id);
                }
                while (rust_call_model_contains_string(model, owner_name));

                json_object *var_decl = json_object_new_object();
                json_object_object_add(var_decl, "kind",
                                       json_object_new_string("var_decl"));
                json_object_object_add(var_decl, "name",
                                       json_object_new_string(owner_name));
                json_object_object_add(var_decl, "type",
                                       json_object_get(array_type));
                json_object_object_add(var_decl, "initializer",
                                       json_object_get(array));
                json_object_array_add(prefix, var_decl);

                json_object *var_ref = json_object_new_object();
                json_object_object_add(var_ref, "kind",
                                       json_object_new_string("variable"));
                json_object_object_add(var_ref, "name",
                                       json_object_new_string(owner_name));
                json_object_object_add(var_ref, "type",
                                       json_object_get(array_type));
                json_object_object_del(expr, "array");
                json_object_object_add(expr, "array", var_ref);
            }
        }
        if (json_object_object_get_ex(expr, "index", &index))
            rust_stabilize_resolved_receiver(model, index, prefix, next_id);
        return;
    }

    if (json_string_property_equals(expr, "kind", "member"))
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(expr, "object", &object))
            rust_stabilize_resolved_receiver(model, object, prefix, next_id);
    }
}

static void rust_lower_resolved_receiver_prefixes(json_object *model,
                                                  json_object *node,
                                                  size_t *next_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_resolved_receiver_prefixes(
                model, json_object_array_get_idx(node, i), next_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_resolved_receiver_prefixes(model, value, next_id);
    }

    if (!json_string_property_equals(node, "kind", "method_call")) return;

    json_object *args = NULL;
    if (json_object_object_get_ex(node, "args", &args) &&
        json_object_is_type(args, json_type_array))
    {
        size_t count = json_object_array_length(args);
        for (size_t i = 0; i < count; i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            if ((!json_boolean_property(arg, "is_ref_arg") &&
                 !json_boolean_property(arg, "is_borrow_tmp")) ||
                !json_string_property_equals(arg, "kind", "array_access"))
                continue;

            char index_name[80];
            do
            {
                size_t id = (*next_id)++;
                snprintf(index_name, sizeof(index_name),
                         "__sn_resolved_index_%zu", id);
            }
            while (rust_call_model_contains_string(model, index_name));
            json_object_object_add(arg, "rust_ref_index_name",
                                   json_object_new_string(index_name));
        }
    }

    json_object *object = NULL;
    if (json_object_object_get_ex(node, "object", &object))
    {
        json_object *prefix = json_object_new_array();
        rust_stabilize_resolved_receiver(
            model, object, prefix, next_id);
        if (json_object_array_length(prefix) > 0)
        {
            char receiver_name[80];
            do
            {
                size_t id = (*next_id)++;
                snprintf(receiver_name, sizeof(receiver_name),
                         "__sn_resolved_receiver_%zu", id);
            }
            while (rust_call_model_contains_string(model, receiver_name));
            json_object_object_add(node, "rust_receiver_name",
                                   json_object_new_string(receiver_name));
            json_object_object_add(node, "rust_receiver_prefix", prefix);
        }
        else
            json_object_put(prefix);
    }
}

static void rust_lower_instance_method_clones(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return;
    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        json_object *methods = NULL;
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;
        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            json_object *body = NULL;
            if (!json_boolean_property(method, "is_static") &&
                json_object_object_get_ex(method, "body", &body))
                rust_mark_instance_method_clones(body);
        }
    }
}

/* Rust's &mut ABI cannot express a tagged-valid call that passes both a
 * value-struct place and one of its subobjects by reference.  Select only
 * callees that actually receive overlapping stable places, then use raw place
 * pointers for their already-supported scalar / heap-free value-struct
 * as-ref parameters.  Raw pointers preserve the source storage identity
 * without constructing overlapping live Rust references. */
static bool rust_raw_place_param_type(json_object *model, json_object *type)
{
    if (rust_scalar_ref_parameter_type_supported(type)) return true;
    if (!json_string_property_equals(type, "kind", "struct")) return false;
    json_object *structure = rust_find_struct(model, json_string_property(type, "name"));
    return structure && !json_boolean_property(structure, "is_native") &&
        !json_boolean_property(structure, "is_packed") &&
        !json_boolean_property(structure, "pass_self_by_ref") &&
        !json_boolean_property(structure, "has_heap_fields");
}

static bool rust_place_contains(json_object *parent_place, json_object *place)
{
    if (rust_resolved_places_alias(parent_place, place)) return true;
    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "member") &&
        json_object_object_get_ex(place, "object", &parent))
        return rust_place_contains(parent_place, parent);
    if (json_string_property_equals(place, "kind", "array_access") &&
        json_object_object_get_ex(place, "array", &parent))
        return rust_place_contains(parent_place, parent);
    return false;
}

static bool rust_places_overlap(json_object *left, json_object *right)
{
    return rust_place_contains(left, right) || rust_place_contains(right, left);
}

static bool rust_raw_place_stable(json_object *place)
{
    if (json_string_property_equals(place, "kind", "variable")) return true;
    if (json_string_property_equals(place, "kind", "member"))
    {
        json_object *object = NULL;
        return json_object_object_get_ex(place, "object", &object) &&
            rust_raw_place_stable(object);
    }
    return false;
}

static json_object *rust_find_function(json_object *model, const char *name)
{
    json_object *functions = NULL;
    if (!name || !json_object_object_get_ex(model, "functions", &functions)) return NULL;
    size_t count = json_object_array_length(functions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (json_string_property_equals(function, "name", name) &&
            !json_boolean_property(function, "is_native")) return function;
    }
    return NULL;
}

static json_object *rust_raw_receiver_struct_type(json_object *type)
{
    if (json_string_property_equals(type, "kind", "struct")) return type;
    json_object *base_type = NULL;
    if (json_string_property_equals(type, "kind", "pointer") &&
        json_object_object_get_ex(type, "base_type", &base_type) &&
        json_string_property_equals(base_type, "kind", "struct"))
        return base_type;
    return NULL;
}

static json_object *rust_raw_place_call_target(json_object *model, json_object *node)
{
    const char *kind = json_string_property(node, "kind");
    if (!kind) return NULL;
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL;
        if (json_boolean_property(node, "rust_closure_call") ||
            !json_object_object_get_ex(node, "callee", &callee)) return NULL;
        if (json_string_property_equals(callee, "kind", "variable"))
            return rust_find_function(model, json_string_property(callee, "name"));
        if (json_string_property_equals(callee, "kind", "member"))
        {
            json_object *object = NULL, *type = NULL;
            if (!json_object_object_get_ex(callee, "object", &object) ||
                !json_object_object_get_ex(object, "type", &type)) return NULL;
            type = rust_raw_receiver_struct_type(type);
            if (!type) return NULL;
            json_object *structure = rust_find_struct(model,
                json_string_property(type, "name"));
            return rust_find_resolved_method(structure,
                json_string_property(callee, "member_name"), false);
        }
        return NULL;
    }
    if (strcmp(kind, "static_call") == 0)
    {
        json_object *structure = rust_find_struct(model,
            json_string_property(node, "type_name"));
        return rust_find_resolved_method(structure,
            json_string_property(node, "method_name"), true);
    }
    if (strcmp(kind, "method_call") == 0)
    {
        json_object *struct_type = NULL;
        if (!json_object_object_get_ex(node, "struct_type", &struct_type)) return NULL;
        json_object *structure = rust_find_struct(model,
            json_string_property(struct_type, "name"));
        return rust_find_resolved_method(structure,
            json_string_property(node, "method_name"),
            json_boolean_property(node, "is_static"));
    }
    return NULL;
}

static json_object *rust_raw_place_call_receiver(json_object *node)
{
    if (json_string_property_equals(node, "kind", "call"))
    {
        json_object *callee = NULL, *object = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_string_property_equals(callee, "kind", "member") &&
            json_object_object_get_ex(callee, "object", &object))
            return object;
    }
    if (json_string_property_equals(node, "kind", "method_call") &&
        !json_boolean_property(node, "is_static"))
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(node, "object", &object)) return object;
    }
    return NULL;
}

static bool rust_raw_definition_place(json_object *definition, json_object *place)
{
    json_object *root = place;
    while (root && (json_string_property_equals(root, "kind", "member") ||
                    json_string_property_equals(root, "kind", "array_access")))
    {
        json_object *next = NULL;
        const char *key = json_string_property_equals(root, "kind", "member") ?
            "object" : "array";
        if (!json_object_object_get_ex(root, key, &next)) return false;
        root = next;
    }
    if (!json_string_property_equals(root, "kind", "variable")) return false;
    if (json_string_property_equals(root, "name", "self"))
        return json_boolean_property(definition, "rust_raw_receiver_abi");

    json_object *params = NULL;
    if (!json_object_object_get_ex(definition, "params", &params)) return false;
    size_t count = json_object_array_length(params);
    for (size_t i = 0; i < count; i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        if (json_boolean_property(param, "rust_raw_ref_param") &&
            json_string_property_equals(param, "name",
                                        json_string_property(root, "name")))
            return true;
    }
    return false;
}

static bool rust_call_has_overlapping_ref_places(json_object *model,
                                                 json_object *node,
                                                 json_object *target,
                                                 json_object *caller,
                                                 bool *receiver_overlap)
{
    json_object *args = NULL, *params = NULL;
    if (!json_object_object_get_ex(node, "args", &args) ||
        !json_object_object_get_ex(target, "params", &params)) return false;
    size_t count = json_object_array_length(args);
    if (count != json_object_array_length(params)) return false;
    json_object *receiver = rust_raw_place_call_receiver(node);
    if (receiver && rust_raw_place_stable(receiver))
    {
        for (size_t i = 0; i < count; i++)
        {
            json_object *param = json_object_array_get_idx(params, i);
            json_object *type = NULL;
            json_object *arg = json_object_array_get_idx(args, i);
            if (!json_string_property_equals(param, "mem_qual", "as_ref") ||
                !json_object_object_get_ex(param, "type", &type) ||
                !rust_raw_place_param_type(model, type) ||
                !rust_raw_place_stable(arg)) continue;
            if (rust_places_overlap(receiver, arg) ||
                (caller && rust_raw_definition_place(caller, receiver) &&
                 rust_raw_definition_place(caller, arg)))
            {
                if (receiver_overlap) *receiver_overlap = true;
                return true;
            }
        }
    }
    for (size_t i = 0; i < count; i++)
    {
        json_object *left_param = json_object_array_get_idx(params, i);
        json_object *left_type = NULL;
        if (!json_string_property_equals(left_param, "mem_qual", "as_ref") ||
            !json_object_object_get_ex(left_param, "type", &left_type) ||
            !rust_raw_place_param_type(model, left_type)) continue;
        json_object *left = json_object_array_get_idx(args, i);
        if (!rust_raw_place_stable(left)) continue;
        for (size_t j = i + 1; j < count; j++)
        {
            json_object *right_param = json_object_array_get_idx(params, j);
            json_object *right_type = NULL;
            if (!json_string_property_equals(right_param, "mem_qual", "as_ref") ||
                !json_object_object_get_ex(right_param, "type", &right_type) ||
                !rust_raw_place_param_type(model, right_type)) continue;
            json_object *right = json_object_array_get_idx(args, j);
            if (rust_raw_place_stable(right) &&
                (rust_places_overlap(left, right) ||
                 (caller && rust_raw_definition_place(caller, left) &&
                  rust_raw_definition_place(caller, right))))
                return true;
        }
    }
    return false;
}

static void rust_select_raw_place_callees(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_select_raw_place_callees(model,
                json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") != 0 && strcmp(key, "struct_type") != 0)
            rust_select_raw_place_callees(model, value);
    }
    json_object *target = rust_raw_place_call_target(model, node);
    bool receiver_overlap = false;
    if (target && rust_call_has_overlapping_ref_places(
            model, node, target, NULL, &receiver_overlap))
    {
        json_object_object_add(target, "rust_raw_place_abi",
                               json_object_new_boolean(true));
        if (receiver_overlap)
            json_object_object_add(target, "rust_raw_receiver_abi",
                                   json_object_new_boolean(true));
    }
}

static bool rust_select_forwarded_raw_calls(json_object *model,
                                            json_object *caller,
                                            json_object *node)
{
    if (!node) return false;
    bool changed = false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            changed |= rust_select_forwarded_raw_calls(
                model, caller, json_object_array_get_idx(node, i));
        return changed;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") != 0 && strcmp(key, "struct_type") != 0)
            changed |= rust_select_forwarded_raw_calls(model, caller, value);
    }

    json_object *target = rust_raw_place_call_target(model, node);
    bool receiver_overlap = false;
    if (!target || !rust_call_has_overlapping_ref_places(
            model, node, target, caller, &receiver_overlap)) return changed;
    if (!json_boolean_property(target, "rust_raw_place_abi")) changed = true;
    json_object_object_add(target, "rust_raw_place_abi",
                           json_object_new_boolean(true));
    if (receiver_overlap)
    {
        if (!json_boolean_property(target, "rust_raw_receiver_abi")) changed = true;
        json_object_object_add(target, "rust_raw_receiver_abi",
                               json_object_new_boolean(true));
    }
    return changed;
}

static void rust_mark_raw_ref_uses(json_object *node,
                                   const char *param_name,
                                   const char *replacement_name)
{
    if (!node || !param_name) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_raw_ref_uses(json_object_array_get_idx(node, i),
                                   param_name, replacement_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    if (json_string_property_equals(node, "kind", "variable") &&
        json_string_property_equals(node, "name", param_name) &&
        (replacement_name || json_boolean_property(node, "is_captured")))
    {
        json_object_object_add(node, "rust_deref", json_object_new_boolean(true));
        if (replacement_name)
            json_object_object_add(node, "rust_raw_name",
                                   json_object_new_string(replacement_name));
    }
    if (json_string_property_equals(node, "kind", "assign") &&
        json_string_property_equals(node, "target", param_name) &&
        (replacement_name || json_boolean_property(node, "is_captured")))
    {
        json_object_object_add(node, "rust_deref_target",
                               json_object_new_boolean(true));
        if (replacement_name)
            json_object_object_add(node, "rust_raw_target_name",
                                   json_object_new_string(replacement_name));
    }
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") != 0 && strcmp(key, "struct_type") != 0)
            rust_mark_raw_ref_uses(value, param_name, replacement_name);
    }
}

static void rust_mark_raw_place_definitions(json_object *model,
                                            json_object *definitions,
                                            size_t *next_id)
{
    if (!json_object_is_type(definitions, json_type_array)) return;
    size_t count = json_object_array_length(definitions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *definition = json_object_array_get_idx(definitions, i);
        if (!json_boolean_property(definition, "rust_raw_place_abi")) continue;
        json_object *params = NULL, *body = NULL;
        if (!json_object_object_get_ex(definition, "params", &params) ||
            !json_object_object_get_ex(definition, "body", &body)) continue;
        if (json_boolean_property(definition, "rust_raw_receiver_abi") &&
            !json_boolean_property(definition, "is_static"))
        {
            const char *self_name = json_string_property(
                definition, "rust_raw_self_name");
            char generated_name[80];
            if (!self_name)
            {
                do
                {
                    size_t id = (*next_id)++;
                    snprintf(generated_name, sizeof(generated_name),
                             "__sn_raw_self_%zu", id);
                }
                while (rust_call_model_contains_string(model, generated_name));
                json_object_object_add(definition, "rust_raw_self_name",
                                       json_object_new_string(generated_name));
                self_name = json_string_property(definition,
                                                 "rust_raw_self_name");
            }
            rust_mark_raw_ref_uses(body, "self", self_name);
        }
        size_t param_count = json_object_array_length(params);
        for (size_t p = 0; p < param_count; p++)
        {
            json_object *param = json_object_array_get_idx(params, p);
            json_object *type = NULL;
            const char *name = json_string_property(param, "name");
            if (!name || !json_string_property_equals(param, "mem_qual", "as_ref") ||
                !json_object_object_get_ex(param, "type", &type) ||
                !rust_raw_place_param_type(model, type)) continue;
            json_object_object_add(param, "rust_raw_ref_param",
                                   json_object_new_boolean(true));
            rust_mark_raw_ref_uses(body, name, NULL);
        }
    }
}

static bool rust_select_definition_array_calls(json_object *model,
                                               json_object *definitions)
{
    if (!json_object_is_type(definitions, json_type_array)) return false;
    bool changed = false;
    size_t count = json_object_array_length(definitions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *definition = json_object_array_get_idx(definitions, i);
        json_object *body = NULL;
        if (json_boolean_property(definition, "rust_raw_place_abi") &&
            json_object_object_get_ex(definition, "body", &body))
            changed |= rust_select_forwarded_raw_calls(
                model, definition, body);
    }
    return changed;
}

static void rust_mark_raw_place_calls(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_raw_place_calls(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") != 0 && strcmp(key, "struct_type") != 0)
            rust_mark_raw_place_calls(model, value);
    }
    json_object *target = rust_raw_place_call_target(model, node);
    if (!target || !json_boolean_property(target, "rust_raw_place_abi")) return;
    json_object *args = NULL, *params = NULL;
    if (!json_object_object_get_ex(node, "args", &args) ||
        !json_object_object_get_ex(target, "params", &params)) return;
    size_t count = json_object_array_length(args);
    for (size_t i = 0; i < count && i < json_object_array_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        json_object *arg = json_object_array_get_idx(args, i);
        if (json_boolean_property(param, "rust_raw_ref_param") &&
            json_boolean_property(arg, "is_ref_arg"))
            json_object_object_add(arg, "rust_raw_ref_arg",
                                   json_object_new_boolean(true));
    }
    if (json_boolean_property(target, "rust_raw_receiver_abi"))
    {
        json_object_object_add(node, "rust_raw_receiver_call",
                               json_object_new_boolean(true));
        if (json_string_property_equals(node, "kind", "call"))
        {
            json_object *callee = NULL, *object = NULL, *type = NULL;
            if (json_object_object_get_ex(node, "callee", &callee) &&
                json_object_object_get_ex(callee, "object", &object) &&
                json_object_object_get_ex(object, "type", &type))
            {
                type = rust_raw_receiver_struct_type(type);
                const char *name = type ? json_string_property(type, "name") : NULL;
                if (name)
                    json_object_object_add(node, "rust_raw_receiver_type",
                                           json_object_new_string(name));
            }
        }
    }
    json_object_object_add(node, "rust_raw_ref_call",
                           json_object_new_boolean(true));
}

/* A raw-selected wrapper can still call an ordinary as-ref callee.  Keep that
 * edge ordinary, but reborrow the wrapper's raw place for only the duration of
 * the call.  The generic captured-as-ref rendering adds a dereference intended
 * for &mut-backed captures; applying it to an already dereferenced raw place
 * would produce &mut *(*(ptr)). */
static void rust_mark_raw_reborrow_calls(json_object *model,
                                         json_object *caller,
                                         json_object *node)
{
    if (!node || !caller) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_raw_reborrow_calls(
                model, caller, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strcmp(key, "type") != 0 && strcmp(key, "struct_type") != 0)
            rust_mark_raw_reborrow_calls(model, caller, value);
    }

    json_object *target = rust_raw_place_call_target(model, node);
    json_object *args = NULL, *params = NULL;
    if (!target || !json_object_object_get_ex(node, "args", &args) ||
        !json_object_object_get_ex(target, "params", &params)) return;
    size_t count = json_object_array_length(args);
    for (size_t i = 0; i < count && i < json_object_array_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        json_object *arg = json_object_array_get_idx(args, i);
        if (json_string_property_equals(param, "mem_qual", "as_ref") &&
            !json_boolean_property(param, "rust_raw_ref_param") &&
            json_boolean_property(arg, "is_ref_arg") &&
            rust_raw_definition_place(caller, arg))
            json_object_object_add(arg, "rust_raw_reborrow_arg",
                                   json_object_new_boolean(true));
    }
}

static void rust_mark_definition_raw_reborrows(json_object *model,
                                               json_object *definitions)
{
    if (!json_object_is_type(definitions, json_type_array)) return;
    size_t count = json_object_array_length(definitions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *definition = json_object_array_get_idx(definitions, i);
        json_object *body = NULL;
        if (json_boolean_property(definition, "rust_raw_place_abi") &&
            json_object_object_get_ex(definition, "body", &body))
            rust_mark_raw_reborrow_calls(model, definition, body);
    }
}

static void rust_lower_overlapping_ref_places(json_object *model)
{
    rust_select_raw_place_callees(model, model);
    json_object *functions = NULL, *structs = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    json_object_object_get_ex(model, "structs", &structs);
    size_t raw_self_id = 0;
    bool changed;
    do
    {
        changed = false;
        rust_mark_raw_place_definitions(model, functions, &raw_self_id);
        changed |= rust_select_definition_array_calls(model, functions);
        if (json_object_is_type(structs, json_type_array))
        {
            size_t count = json_object_array_length(structs);
            for (size_t i = 0; i < count; i++)
            {
                json_object *methods = NULL;
                if (!json_object_object_get_ex(
                        json_object_array_get_idx(structs, i),
                        "methods", &methods)) continue;
                rust_mark_raw_place_definitions(model, methods, &raw_self_id);
                changed |= rust_select_definition_array_calls(model, methods);
            }
        }
    } while (changed);
    rust_mark_raw_place_definitions(model, functions, &raw_self_id);
    if (json_object_is_type(structs, json_type_array))
    {
        size_t count = json_object_array_length(structs);
        for (size_t i = 0; i < count; i++)
        {
            json_object *methods = NULL;
            if (json_object_object_get_ex(json_object_array_get_idx(structs, i),
                                          "methods", &methods))
                rust_mark_raw_place_definitions(model, methods, &raw_self_id);
        }
    }
    rust_mark_raw_place_calls(model, model);
    rust_mark_definition_raw_reborrows(model, functions);
    if (json_object_is_type(structs, json_type_array))
    {
        size_t count = json_object_array_length(structs);
        for (size_t i = 0; i < count; i++)
        {
            json_object *methods = NULL;
            if (json_object_object_get_ex(json_object_array_get_idx(structs, i),
                                          "methods", &methods))
                rust_mark_definition_raw_reborrows(model, methods);
        }
    }
}

static void rust_lower_calls(json_object *model)
{
    rust_lower_array_searches(model);
    rust_lower_instance_method_clones(model);
    size_t resolved_call_id = 0;
    rust_lower_resolved_receiver_prefixes(model, model, &resolved_call_id);
    rust_lower_overlapping_ref_places(model);
}
