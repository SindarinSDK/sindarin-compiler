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
            {
                json_object_object_add(node, "rust_string_method",
                                       json_object_new_string(method));
                if (strcmp(method, "split") == 0)
                {
                    json_object *args = NULL;
                    if (json_object_object_get_ex(node, "args", &args) &&
                        json_object_array_length(args) == 2)
                        json_object_object_add(node, "rust_string_split_limited",
                                               json_object_new_boolean(true));
                }
            }
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

static bool rust_borrowed_callable_argument(json_object *arg)
{
    json_object *type = NULL;
    return json_boolean_property(arg, "is_ref_arg") &&
           json_object_object_get_ex(arg, "type", &type) &&
           json_string_property_equals(type, "kind", "function");
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

    bool is_static_call = json_string_property_equals(node, "kind", "static_call");
    if (!is_static_call &&
        !json_string_property_equals(node, "kind", "method_call")) return;

    json_object *args = NULL;
    bool stabilize_args = false;
    if (json_object_object_get_ex(node, "args", &args) &&
        json_object_is_type(args, json_type_array))
    {
        size_t count = json_object_array_length(args);
        /* A callable slot is an ordinary source place.  Creating Rust's &mut
         * for it before later argument values are evaluated can overlap a
         * later read of the same handle.  Preserve the argument order by
         * evaluating the other argument values first, then form this borrow
         * only in the final call expression. */
        for (size_t i = 0; i + 1 < count; i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            if (rust_borrowed_callable_argument(arg))
            {
                json_object_object_add(arg, "rust_defer_callable_borrow",
                                       json_object_new_boolean(true));
                stabilize_args = true;
            }
        }
        for (size_t i = 0; i < count; i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            if ((!json_boolean_property(arg, "is_ref_arg") &&
                 !json_boolean_property(arg, "is_borrow_tmp")) ||
                !json_string_property_equals(arg, "kind", "array_access"))
                continue;

            if (json_boolean_property(arg, "rust_defer_callable_borrow"))
            {
                json_object *arg_prefix = json_object_new_array();
                rust_stabilize_resolved_receiver(
                    model, arg, arg_prefix, next_id, true);
                if (json_object_array_length(arg_prefix) > 0)
                    json_object_object_add(arg, "rust_resolved_arg_prefix",
                                           arg_prefix);
                else
                    json_object_put(arg_prefix);
                continue;
            }

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
                if (json_boolean_property(arg, "rust_defer_callable_borrow"))
                    continue;
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
    if (is_static_call)
    {
        if (stabilize_args)
            json_object_object_add(node, "rust_stabilize_call",
                                   json_object_new_boolean(true));
        return;
    }
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

static void rust_lower_default_array_ref_indices(json_object *model,
                                                 json_object *node,
                                                 size_t *next_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_default_array_ref_indices(
                model, json_object_array_get_idx(node, i), next_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_default_array_ref_indices(model, value, next_id);
    }

    if (!json_boolean_property(node, "rust_default_array_ref_arg")) return;
    json_object *bindings = json_object_new_array();
    if (!bindings) return;
    if (!rust_collect_place_indices(model, node, bindings, next_id))
    {
        json_object_put(bindings);
        return;
    }
    if (json_object_array_length(bindings) == 0)
    {
        json_object_put(bindings);
        return;
    }
    json_object_object_add(node, "rust_place_index_bindings", bindings);
}

/* Rust evaluates call arguments left-to-right, but forming an &mut argument
 * immediately can make a later argument's read of the same array illegal.
 * A variable/member default-array argument has no source-visible computation
 * of its own, so evaluate each later by-value argument into a temporary first
 * and form the array borrow only at the final call.  This retains the original
 * array handle (and therefore callee mutation identity) without unsafe aliases
 * or a semantic clone.  Indexed/produced places require their separate owner
 * and index stabilization and are deliberately not classified as this case. */
static bool rust_deferred_default_array_place(json_object *expr)
{
    if (json_string_property_equals(expr, "kind", "variable")) return true;
    if (json_string_property_equals(expr, "kind", "member"))
    {
        json_object *object = NULL;
        return json_object_object_get_ex(expr, "object", &object) &&
            rust_deferred_default_array_place(object);
    }
    return false;
}

static bool rust_same_default_array_place(json_object *left, json_object *right)
{
    const char *left_kind = json_string_property(left, "kind");
    const char *right_kind = json_string_property(right, "kind");
    if (!left_kind || !right_kind || strcmp(left_kind, right_kind) != 0)
        return false;
    if (strcmp(left_kind, "variable") == 0)
    {
        const char *left_name = json_string_property(left, "name");
        const char *right_name = json_string_property(right, "name");
        return left_name && right_name && strcmp(left_name, right_name) == 0;
    }
    if (strcmp(left_kind, "member") == 0)
    {
        const char *left_name = json_string_property(left, "member_name");
        const char *right_name = json_string_property(right, "member_name");
        json_object *left_object = NULL, *right_object = NULL;
        return left_name && right_name && strcmp(left_name, right_name) == 0 &&
            json_object_object_get_ex(left, "object", &left_object) &&
            json_object_object_get_ex(right, "object", &right_object) &&
            rust_same_default_array_place(left_object, right_object);
    }
    return false;
}

static json_object *rust_default_array_function(json_object *functions,
                                                const char *name)
{
    size_t count = json_object_array_length(functions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (!json_boolean_property(function, "is_native") &&
            json_boolean_property(function, "has_body") &&
            json_string_property_equals(function, "name", name))
            return function;
    }
    return NULL;
}

static void rust_rename_variable_uses(json_object *node, const char *old_name,
                                      const char *new_name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_rename_variable_uses(
                json_object_array_get_idx(node, i), old_name, new_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    if (json_string_property_equals(node, "kind", "variable") &&
        json_string_property_equals(node, "name", old_name))
    {
        json_object_object_del(node, "name");
        json_object_object_add(node, "name", json_object_new_string(new_name));
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_rename_variable_uses(value, old_name, new_name);
    }
}

/* Rust cannot construct two simultaneous &mut Vec references for one source
 * array handle.  For a direct call whose duplicate arguments are the same
 * stable place, emit a private specialization with those formal parameters
 * coalesced.  The specialized body uses one mutable borrow, so sequential
 * mutations through either source formal retain tagged shared-handle identity
 * without an unsafe alias or a semantic clone. */
static bool rust_specialize_default_array_alias_call(json_object *model,
                                                     json_object *functions,
                                                     json_object *call,
                                                     size_t *next_id)
{
    if (!json_string_property_equals(call, "kind", "call") ||
        json_boolean_property(call, "is_closure_call") ||
        json_boolean_property(call, "is_fn_field_call") ||
        json_boolean_property(call, "rust_array_alias_specialized"))
        return true;

    json_object *callee = NULL, *args = NULL;
    if (!json_object_object_get_ex(call, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "variable") ||
        !json_object_object_get_ex(call, "args", &args) ||
        !json_object_is_type(args, json_type_array)) return true;

    const char *callee_name = json_string_property(callee, "name");
    json_object *function = callee_name
        ? rust_default_array_function(functions, callee_name) : NULL;
    json_object *params = NULL;
    if (!function || !json_object_object_get_ex(function, "params", &params) ||
        !json_object_is_type(params, json_type_array)) return true;

    size_t count = json_object_array_length(args);
    if (count < 2 || json_object_array_length(params) != count) return true;
    size_t *canonical = malloc(count * sizeof(*canonical));
    if (!canonical) return false;

    bool has_duplicate = false;
    for (size_t i = 0; i < count; i++)
    {
        canonical[i] = i;
        json_object *arg = json_object_array_get_idx(args, i);
        if (!json_boolean_property(arg, "rust_default_array_ref_arg")) continue;
        json_object *param = json_object_array_get_idx(params, i);
        if (!json_boolean_property(param, "rust_default_array_ref"))
        {
            free(canonical);
            return true;
        }
        if (!rust_deferred_default_array_place(arg))
        {
            free(canonical);
            return true;
        }
        for (size_t j = 0; j < i; j++)
        {
            json_object *earlier = json_object_array_get_idx(args, j);
            if (json_boolean_property(earlier, "rust_default_array_ref_arg") &&
                rust_same_default_array_place(earlier, arg))
            {
                canonical[i] = canonical[j];
                has_duplicate = true;
                break;
            }
        }
    }
    if (!has_duplicate)
    {
        free(canonical);
        return true;
    }

    json_object *specialized = NULL;
    if (json_object_deep_copy(function, &specialized, NULL) != 0 || !specialized)
    {
        free(canonical);
        return false;
    }
    json_object *specialized_params = NULL, *body = NULL;
    if (!json_object_object_get_ex(specialized, "params", &specialized_params) ||
        !json_object_object_get_ex(specialized, "body", &body))
    {
        json_object_put(specialized);
        free(canonical);
        return false;
    }

    for (size_t i = 0; i < count; i++)
    {
        if (canonical[i] == i) continue;
        json_object *from = json_object_array_get_idx(specialized_params, i);
        json_object *to = json_object_array_get_idx(
            specialized_params, canonical[i]);
        const char *from_name = json_string_property(from, "name");
        const char *to_name = json_string_property(to, "name");
        if (!from_name || !to_name)
        {
            json_object_put(specialized);
            free(canonical);
            return false;
        }
        rust_rename_variable_uses(body, from_name, to_name);
    }

    json_object *kept_params = json_object_new_array();
    json_object *kept_args = json_object_new_array();
    if (!kept_params || !kept_args)
    {
        if (kept_params) json_object_put(kept_params);
        if (kept_args) json_object_put(kept_args);
        json_object_put(specialized);
        free(canonical);
        return false;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (canonical[i] != i) continue;
        json_object_array_add(
            kept_params, json_object_get(json_object_array_get_idx(specialized_params, i)));
        json_object_array_add(
            kept_args, json_object_get(json_object_array_get_idx(args, i)));
    }
    json_object_object_del(specialized, "params");
    json_object_object_add(specialized, "params", kept_params);
    json_object_object_del(call, "args");
    json_object_object_add(call, "args", kept_args);

    char specialized_name[96];
    do
    {
        size_t id = (*next_id)++;
        snprintf(specialized_name, sizeof(specialized_name),
                 "__sn_array_alias_call_%zu", id);
    }
    while (rust_call_model_contains_string(model, specialized_name));
    json_object_object_del(specialized, "name");
    json_object_object_add(specialized, "name",
                           json_object_new_string(specialized_name));
    json_object_object_del(callee, "name");
    json_object_object_add(callee, "name", json_object_new_string(specialized_name));
    json_object_object_add(call, "rust_array_alias_specialized",
                           json_object_new_boolean(true));
    json_object_array_add(functions, specialized);
    free(canonical);
    return true;
}

static bool rust_specialize_default_array_alias_calls(json_object *model,
                                                      json_object *functions,
                                                      json_object *node,
                                                      size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_specialize_default_array_alias_calls(
                    model, functions, json_object_array_get_idx(node, i),
                    next_id)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_specialize_default_array_alias_calls(
                model, functions, value, next_id)) return false;
    }
    return rust_specialize_default_array_alias_call(
        model, functions, node, next_id);
}

static void rust_lower_default_array_late_reads(json_object *model,
                                                json_object *node,
                                                size_t *next_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_default_array_late_reads(
                model, json_object_array_get_idx(node, i), next_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_default_array_late_reads(model, value, next_id);
    }

    if (!json_string_property_equals(node, "kind", "call")) return;
    json_object *args = NULL;
    if (!json_object_object_get_ex(node, "args", &args) ||
        !json_object_is_type(args, json_type_array)) return;

    size_t count = json_object_array_length(args);
    size_t first_array_index = count;
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!json_boolean_property(arg, "rust_default_array_ref_arg")) continue;
        if (!rust_deferred_default_array_place(arg)) return;
        if (first_array_index == count) first_array_index = i;
    }
    if (first_array_index == count) return;
    bool has_later_value = false;
    for (size_t i = first_array_index + 1; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!json_boolean_property(arg, "rust_default_array_ref_arg"))
        {
            has_later_value = true;
            break;
        }
    }
    if (!has_later_value) return;

    /* Borrowed siblings need alias-aware place handling rather than a value
     * temporary.  Leave those calls to that representation-specific path. */
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (json_boolean_property(arg, "rust_default_array_ref_arg")) continue;
        if (json_boolean_property(arg, "is_ref_arg") ||
            json_boolean_property(arg, "is_borrow_tmp")) return;
    }

    /* Bind siblings on both sides: leaving an earlier expression in the final
     * call would move it after the pre-evaluated later arguments. */
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (json_boolean_property(arg, "rust_default_array_ref_arg")) continue;
        char arg_name[80];
        do
        {
            size_t id = (*next_id)++;
            snprintf(arg_name, sizeof(arg_name),
                     "__sn_array_call_arg_%zu", id);
        }
        while (rust_call_model_contains_string(model, arg_name));
        json_object_object_add(arg, "rust_deferred_arg_name",
                               json_object_new_string(arg_name));
    }
    json_object_object_add(node, "rust_defer_default_array_borrow",
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

static bool rust_lower_calls(json_object *model)
{
    json_object *functions = NULL;
    if (!json_object_object_get_ex(model, "functions", &functions) ||
        !json_object_is_type(functions, json_type_array)) return false;
    size_t array_alias_id = 0;
    if (!rust_specialize_default_array_alias_calls(
            model, functions, model, &array_alias_id)) return false;
    rust_lower_array_searches(model);
    rust_lower_instance_method_clones(model);
    size_t resolved_call_id = 0;
    rust_lower_resolved_receiver_prefixes(model, model, &resolved_call_id);
    size_t array_arg_id = 0;
    rust_lower_default_array_ref_indices(model, model, &array_arg_id);
    size_t array_late_read_id = 0;
    rust_lower_default_array_late_reads(model, model, &array_late_read_id);
    return true;
}
