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

/* Select private resolved-call stabilization names against every string in
 * the model so a source helper-like identifier cannot be captured. */
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
                                             size_t *next_id,
                                             bool stabilize_indices)
{
    if (json_object_is_type(expr, json_type_array))
    {
        size_t count = json_object_array_length(expr);
        for (size_t i = 0; i < count; i++)
            rust_stabilize_resolved_receiver(
                model, json_object_array_get_idx(expr, i), prefix, next_id,
                stabilize_indices);
        return;
    }
    if (!json_object_is_type(expr, json_type_object)) return;

    if (json_string_property_equals(expr, "kind", "array_access"))
    {
        json_object *array = NULL, *index = NULL;
        bool has_array = json_object_object_get_ex(expr, "array", &array);
        if (has_array && rust_call_stable_place(array))
            rust_stabilize_resolved_receiver(
                model, array, prefix, next_id, stabilize_indices);
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
        {
            rust_stabilize_resolved_receiver(
                model, index, prefix, next_id, stabilize_indices);

            if (!stabilize_indices) return;

            char index_name[80];
            do
            {
                size_t id = (*next_id)++;
                snprintf(index_name, sizeof(index_name),
                         "__sn_resolved_place_index_%zu", id);
            }
            while (rust_call_model_contains_string(model, index_name));

            json_object *resolved_array = NULL;
            if (json_object_object_get_ex(expr, "array", &resolved_array))
            {
                json_object *index_decl = json_object_new_object();
                json_object_object_add(index_decl, "rust_resolved_index_decl",
                                       json_object_new_boolean(true));
                json_object_object_add(index_decl, "name",
                                       json_object_new_string(index_name));
                json_object_object_add(index_decl, "array",
                                       json_object_get(resolved_array));
                json_object_object_add(index_decl, "index",
                                       json_object_get(index));
                json_object_array_add(prefix, index_decl);
                json_object_object_add(expr, "rust_resolved_index_name",
                                       json_object_new_string(index_name));
            }
        }
        return;
    }

    if (json_string_property_equals(expr, "kind", "member"))
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(expr, "object", &object))
            rust_stabilize_resolved_receiver(
                model, object, prefix, next_id, stabilize_indices);
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
    bool stabilize_args = false;
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

            json_object *array = NULL;
            if (json_object_object_get_ex(arg, "array", &array))
            {
                json_object *arg_prefix = json_object_new_array();
                rust_stabilize_resolved_receiver(
                    model, array, arg_prefix, next_id, true);
                if (json_object_array_length(arg_prefix) > 0)
                    json_object_object_add(arg, "rust_resolved_arg_prefix",
                                           arg_prefix);
                else
                    json_object_put(arg_prefix);
            }

            char array_name[80], index_name[80];
            do
            {
                size_t id = (*next_id)++;
                snprintf(array_name, sizeof(array_name),
                         "__sn_resolved_array_%zu", id);
            }
            while (rust_call_model_contains_string(model, array_name));
            do
            {
                size_t id = (*next_id)++;
                snprintf(index_name, sizeof(index_name),
                         "__sn_resolved_index_%zu", id);
            }
            while (rust_call_model_contains_string(model, index_name));
            json_object_object_add(arg, "rust_ref_array_name",
                                   json_object_new_string(array_name));
            json_object_object_add(arg, "rust_ref_index_name",
                                   json_object_new_string(index_name));
            stabilize_args = true;
        }

        /* An array producer borrowed for an as-ref argument must outlive the
         * call.  Bind every argument in order so lifting that producer does
         * not reorder it across sibling arguments. */
        if (stabilize_args)
        {
            for (size_t i = 0; i < count; i++)
            {
                json_object *arg = json_object_array_get_idx(args, i);
                char arg_name[80];
                do
                {
                    size_t id = (*next_id)++;
                    snprintf(arg_name, sizeof(arg_name),
                             "__sn_resolved_arg_%zu", id);
                }
                while (rust_call_model_contains_string(model, arg_name));
                json_object_object_add(arg, "rust_resolved_arg_name",
                                       json_object_new_string(arg_name));
            }
            json_object_object_add(node, "rust_stabilize_args",
                                   json_object_new_boolean(true));
        }
    }

    json_object *object = NULL;
    if (json_object_object_get_ex(node, "object", &object))
    {
        json_object *prefix = json_object_new_array();
        rust_stabilize_resolved_receiver(
            model, object, prefix, next_id,
            json_boolean_property(node, "rust_receiver_mutating"));
        if (json_object_array_length(prefix) > 0)
        {
            json_object_object_add(node, "rust_receiver_prefix", prefix);
        }
        else
            json_object_put(prefix);
    }

    json_object *receiver_prefix = NULL;
    bool has_receiver_prefix = json_object_object_get_ex(
        node, "rust_receiver_prefix", &receiver_prefix);
    if ((stabilize_args || has_receiver_prefix) &&
        !json_boolean_property(node, "is_static"))
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
        json_object_object_add(node, "rust_stabilize_call",
                               json_object_new_boolean(true));
    }
    else if (stabilize_args)
        json_object_object_add(node, "rust_stabilize_call",
                               json_object_new_boolean(true));
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

static void rust_lower_calls(json_object *model)
{
    rust_lower_array_searches(model);
    rust_lower_instance_method_clones(model);
    size_t resolved_call_id = 0;
    rust_lower_resolved_receiver_prefixes(model, model, &resolved_call_id);
}
