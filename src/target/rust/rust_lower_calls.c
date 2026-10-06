/* Included by rust_lower.c. Call annotations retain their original passes. */

static bool rust_owned_value_type(json_object *node);

static bool rust_call_argument_needs_acquire(json_object *node)
{
    json_object *type = NULL;
    if (!json_object_object_get_ex(node, "type", &type)) return false;
    /* Reference structs use sharing Clone implementations. Acquiring the
     * handle keeps a borrowed C argument from consuming its caller's lvalue. */
    return json_string_property_equals(type, "kind", "string") ||
           (json_string_property_equals(type, "kind", "struct") &&
            json_boolean_property(type, "pass_self_by_ref"));
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

        /* C borrows owned strings and reference-struct handles at calls.
         * Rust receives owned values, so acquire an lvalue before passing it.
         * Reference Clone shares identity rather than copying field values. */
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
                        rust_call_argument_needs_acquire(arg) &&
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
                    rust_call_argument_needs_acquire(arg) &&
                    arg_kind && (strcmp(arg_kind, "variable") == 0 ||
                                 strcmp(arg_kind, "member") == 0 ||
                                 strcmp(arg_kind, "array_access") == 0))
                    json_object_object_add(arg, "rust_needs_clone",
                                           json_object_new_boolean(true));
            }
        }
    }
}

/* Generic array access renders its receiver once for the value and once for
 * its length.  A splitLines receiver can be an owned string temporary with
 * observable effects, so bind the produced array before evaluating the index.
 * This is deliberately limited to the newly admitted string method. */
static void rust_lower_split_lines_accesses(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_split_lines_accesses(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_split_lines_accesses(value);
    }

    json_object *array = NULL;
    if (json_string_property_equals(node, "kind", "array_access") &&
        json_object_object_get_ex(node, "array", &array) &&
        json_string_property_equals(array, "kind", "call") &&
        json_string_property_equals(array, "rust_string_method", "splitLines"))
    {
        json_object_object_add(node, "rust_bind_array_once",
                               json_object_new_boolean(true));
        json_object *type = NULL;
        if (json_object_object_get_ex(node, "type", &type) &&
            json_string_property_equals(type, "kind", "string"))
            json_object_object_add(node, "rust_needs_clone",
                                   json_object_new_boolean(true));
    }
}

/* Floating array methods preserve the actual C argument object bytes. */
#include "rust_lower_float_array.c"

static void rust_lower_float_array_calls(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_float_array_calls(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, child)
    {
        (void)key;
        rust_lower_float_array_calls(child);
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
    if (!method || !element_kind || (strcmp(element_kind, "float") != 0 &&
                                    strcmp(element_kind, "double") != 0)) return;
    bool search = strcmp(method, "contains") == 0 || strcmp(method, "indexOf") == 0;
    bool push = strcmp(method, "push") == 0;
    bool insert = strcmp(method, "insert") == 0;
    if (!search && !push && !insert) return;
    json_object *args = NULL, *value = NULL;
    if (!json_object_object_get_ex(node, "args", &args) ||
        json_object_array_length(args) != (insert ? 2 : 1)) return;
    size_t index = insert ? 1 : 0;
    json_object_deep_copy(json_object_array_get_idx(args, index), &value, NULL);
    rust_float_array_c_values(value);
    if (search)
        json_object_object_add(node, "rust_float_array_search", json_object_new_boolean(true));
    else
    {
        json_object_object_add(node, "rust_float_array_storage", json_object_new_boolean(true));
        json_object_object_add(value, "rust_float_array_storage_type",
                               json_object_new_string(strcmp(element_kind, "float") == 0 ? "f32" : "f64"));
        if (insert) json_object_object_add(node, "rust_float_array_insert", json_object_new_boolean(true));
        if (push && json_string_property_equals(object, "kind", "variable"))
            json_object_object_add(node, "rust_float_array_push", json_object_new_boolean(true));
    }
    json_object_array_put_idx(args, index, value);
    if (json_string_property_equals(object, "kind", "variable"))
        json_object_object_add(node, "rust_float_search_defer_receiver", json_object_new_boolean(true));
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

static void rust_rename_binding_uses(json_object *node, int64_t binding_id,
                                     const char *new_name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_rename_binding_uses(
                json_object_array_get_idx(node, i), binding_id, new_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object *node_binding = NULL, *name = NULL;
    if (json_object_object_get_ex(node, "rust_binding_id", &node_binding) &&
        json_object_get_int64(node_binding) == binding_id &&
        json_object_object_get_ex(node, "name", &name))
    {
        json_object_object_del(node, "name");
        json_object_object_add(node, "name", json_object_new_string(new_name));
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_rename_binding_uses(value, binding_id, new_name);
    }
}

static void rust_mark_binding_receiver_field(json_object *node,
                                             int64_t binding_id,
                                             const char *field_name)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_mark_binding_receiver_field(
                json_object_array_get_idx(node, i), binding_id, field_name);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object *node_binding = NULL;
    if (json_string_property_equals(node, "kind", "variable") &&
        json_object_object_get_ex(node, "rust_binding_id", &node_binding) &&
        json_object_get_int64(node_binding) == binding_id)
        json_object_object_add(node, "rust_receiver_alias_field",
                               json_object_new_string(field_name));

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_mark_binding_receiver_field(value, binding_id, field_name);
    }
}

static json_object *rust_receiver_structure(json_object *model,
                                            json_object *receiver)
{
    json_object *type = NULL, *base_type = NULL;
    if (!json_object_object_get_ex(receiver, "type", &type)) return NULL;
    if (json_string_property_equals(type, "kind", "pointer"))
    {
        if (!json_object_object_get_ex(type, "base_type", &base_type)) return NULL;
        type = base_type;
    }
    if (!json_string_property_equals(type, "kind", "struct")) return NULL;
    return rust_find_struct(model, json_string_property(type, "name"));
}

static const char *rust_receiver_array_alias_field(json_object *receiver,
                                                   json_object *arg)
{
    if (rust_direct_receiver_array_alias(receiver, arg))
        return json_string_property(arg, "member_name");
    if (json_string_property_equals(receiver, "kind", "variable") &&
        json_string_property_equals(receiver, "name", "self"))
        return json_string_property(arg, "rust_receiver_alias_field");
    return NULL;
}

static json_object *rust_receiver_array_alias_specialization(
    json_object *methods, const char *origin,
    const char *const *alias_fields, size_t count)
{
    size_t method_count = json_object_array_length(methods);
    for (size_t i = 0; i < method_count; i++)
    {
        json_object *method = json_object_array_get_idx(methods, i);
        json_object *pattern = NULL;
        if (!json_string_property_equals(
                method, "rust_receiver_array_alias_origin", origin) ||
            !json_object_object_get_ex(
                method, "rust_receiver_array_alias_pattern", &pattern) ||
            !json_object_is_type(pattern, json_type_array) ||
            json_object_array_length(pattern) != count) continue;
        bool matches = true;
        for (size_t p = 0; p < count; p++)
        {
            const char *stored = json_object_get_string(
                json_object_array_get_idx(pattern, p));
            const char *expected = alias_fields[p] ? alias_fields[p] : "";
            if (!stored || strcmp(stored, expected) != 0)
            {
                matches = false;
                break;
            }
        }
        if (matches) return method;
    }
    return NULL;
}

static bool rust_remove_receiver_alias_items(
    json_object *owner, const char *key, json_object *items,
    const char *const *alias_fields, size_t count)
{
    if (json_object_array_length(items) != count) return false;
    json_object *kept = json_object_new_array();
    if (!kept) return false;
    for (size_t i = 0; i < count; i++)
        if (!alias_fields[i])
            json_object_array_add(
                kept, json_object_get(json_object_array_get_idx(items, i)));
    json_object_object_del(owner, key);
    json_object_object_add(owner, key, kept);
    return true;
}

static bool rust_specialize_receiver_array_alias_calls(json_object *model,
                                                       json_object *node,
                                                       size_t *next_id);

/* A stable `bag.method(bag.field, ...)` call cannot be represented by
 * simultaneous `&mut bag` and field borrows. Clone the method privately,
 * replace each aliased formal's resolved uses with its `self.field`, and omit
 * those arguments. Marked uses propagate the same pattern through method
 * forwarders; registering the complete pattern before visiting the clone keeps
 * recursive and mutually recursive forwarding finite. */
static bool rust_specialize_receiver_array_alias_call(json_object *model,
                                                      json_object *call,
                                                      size_t *next_id)
{
    if (!json_string_property_equals(call, "kind", "call") ||
        json_boolean_property(call, "rust_receiver_array_alias_specialized"))
        return true;

    json_object *callee = NULL, *receiver = NULL, *args = NULL;
    if (!json_object_object_get_ex(call, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &receiver) ||
        !json_string_property_equals(receiver, "kind", "variable") ||
        !json_object_object_get_ex(call, "args", &args) ||
        !json_object_is_type(args, json_type_array)) return true;

    size_t arg_count = json_object_array_length(args);
    if (arg_count == 0) return true;
    const char **alias_fields = calloc(arg_count, sizeof(*alias_fields));
    if (!alias_fields) return false;
    size_t alias_count = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!json_boolean_property(arg, "rust_default_array_ref_arg")) continue;
        alias_fields[i] = rust_receiver_array_alias_field(receiver, arg);
        if (alias_fields[i]) alias_count++;
    }
    if (alias_count == 0)
    {
        free(alias_fields);
        return true;
    }

    json_object *structure = rust_receiver_structure(model, receiver);
    json_object *methods = NULL;
    const char *origin = json_string_property(callee, "member_name");
    if (!structure || !origin ||
        !json_object_object_get_ex(structure, "methods", &methods) ||
        !json_object_is_type(methods, json_type_array))
    {
        free(alias_fields);
        return false;
    }
    json_object *method = rust_find_resolved_method(structure, origin, false);
    json_object *params = NULL, *body = NULL;
    if (!method || json_boolean_property(method, "is_native") ||
        !json_object_object_get_ex(method, "params", &params) ||
        !json_object_is_type(params, json_type_array) ||
        json_object_array_length(params) != arg_count ||
        !json_object_object_get_ex(method, "body", &body))
    {
        free(alias_fields);
        return false;
    }

    /* An as-val array has its own entry copy and must never be rewritten as
     * the receiver field, even when the source argument names that field. */
    for (size_t i = 0; i < arg_count; i++)
    {
        if (alias_fields[i] && json_boolean_property(
                json_object_array_get_idx(params, i), "needs_array_copy"))
        {
            alias_fields[i] = NULL;
            alias_count--;
        }
    }
    if (alias_count == 0)
    {
        free(alias_fields);
        return true;
    }

    json_object *specialized = rust_receiver_array_alias_specialization(
        methods, origin, alias_fields, arg_count);
    const char *specialized_name = specialized
        ? json_string_property(specialized, "name") : NULL;
    if (!specialized)
    {
        if (json_object_deep_copy(method, &specialized, NULL) != 0 ||
            !specialized)
        {
            free(alias_fields);
            return false;
        }
        json_object *specialized_params = NULL, *specialized_body = NULL;
        if (!json_object_object_get_ex(
                specialized, "params", &specialized_params) ||
            !json_object_object_get_ex(
                specialized, "body", &specialized_body))
        {
            json_object_put(specialized);
            free(alias_fields);
            return false;
        }
        for (size_t i = 0; i < arg_count; i++)
        {
            if (!alias_fields[i]) continue;
            json_object *removed_param = json_object_array_get_idx(
                specialized_params, i);
            json_object *binding = NULL;
            if (!json_object_object_get_ex(
                    removed_param, "rust_binding_id", &binding))
            {
                json_object_put(specialized);
                free(alias_fields);
                return false;
            }
            rust_mark_binding_receiver_field(
                specialized_body, json_object_get_int64(binding),
                alias_fields[i]);
        }
        if (!rust_remove_receiver_alias_items(
                specialized, "params", specialized_params,
                alias_fields, arg_count))
        {
            json_object_put(specialized);
            free(alias_fields);
            return false;
        }

        char name[96];
        do
        {
            size_t id = (*next_id)++;
            snprintf(name, sizeof(name),
                     "__sn_receiver_array_alias_%zu", id);
        }
        while (rust_call_model_contains_string(model, name));
        json_object_object_del(specialized, "name");
        json_object_object_add(specialized, "name", json_object_new_string(name));
        json_object_object_add(specialized, "rust_receiver_array_alias_origin",
                               json_object_new_string(origin));
        json_object *pattern = json_object_new_array();
        if (!pattern)
        {
            json_object_put(specialized);
            free(alias_fields);
            return false;
        }
        for (size_t i = 0; i < arg_count; i++)
            json_object_array_add(
                pattern, json_object_new_string(
                    alias_fields[i] ? alias_fields[i] : ""));
        json_object_object_add(specialized,
                               "rust_receiver_array_alias_pattern", pattern);
        json_object_array_add(methods, specialized);
        specialized_name = json_string_property(specialized, "name");
        if (!rust_specialize_receiver_array_alias_calls(
                model, specialized_body, next_id))
        {
            free(alias_fields);
            return false;
        }
    }
    if (!specialized_name ||
        !rust_remove_receiver_alias_items(
            call, "args", args, alias_fields, arg_count))
    {
        free(alias_fields);
        return false;
    }

    json_object *callee_type = NULL, *param_types = NULL;
    if (json_object_object_get_ex(callee, "type", &callee_type) &&
        json_object_object_get_ex(callee_type, "param_types", &param_types) &&
        json_object_is_type(param_types, json_type_array) &&
        json_object_array_length(param_types) == arg_count &&
        !rust_remove_receiver_alias_items(
            callee_type, "param_types", param_types,
            alias_fields, arg_count))
    {
        free(alias_fields);
        return false;
    }
    json_object_object_del(callee, "member_name");
    json_object_object_add(callee, "member_name",
                           json_object_new_string(specialized_name));
    json_object_object_add(call, "rust_receiver_array_alias_specialized",
                           json_object_new_boolean(true));
    free(alias_fields);
    return true;
}

static bool rust_specialize_receiver_array_alias_calls(json_object *model,
                                                       json_object *node,
                                                       size_t *next_id)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_specialize_receiver_array_alias_calls(
                    model, json_object_array_get_idx(node, i), next_id))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_specialize_receiver_array_alias_calls(
                model, value, next_id)) return false;
    }
    return rust_specialize_receiver_array_alias_call(model, node, next_id);
}

static bool rust_array_alias_pattern_matches(json_object *function,
                                             const char *origin,
                                             const size_t *canonical,
                                             size_t count)
{
    if (!json_string_property_equals(
            function, "rust_array_alias_origin", origin)) return false;
    json_object *pattern = NULL;
    if (!json_object_object_get_ex(
            function, "rust_array_alias_pattern", &pattern) ||
        !json_object_is_type(pattern, json_type_array) ||
        json_object_array_length(pattern) != count) return false;
    for (size_t i = 0; i < count; i++)
        if ((size_t)json_object_get_int64(
                json_object_array_get_idx(pattern, i)) != canonical[i])
            return false;
    return true;
}

static json_object *rust_array_alias_specialization(json_object *functions,
                                                     const char *origin,
                                                     const size_t *canonical,
                                                     size_t count)
{
    size_t function_count = json_object_array_length(functions);
    for (size_t i = 0; i < function_count; i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (rust_array_alias_pattern_matches(
                function, origin, canonical, count)) return function;
    }
    return NULL;
}

static bool rust_coalesce_array_alias_args(json_object *call,
                                           json_object *args,
                                           const size_t *canonical,
                                           size_t count)
{
    json_object *kept_args = json_object_new_array();
    if (!kept_args) return false;
    for (size_t i = 0; i < count; i++)
        if (canonical[i] == i)
            json_object_array_add(
                kept_args, json_object_get(json_object_array_get_idx(args, i)));
    json_object_object_del(call, "args");
    json_object_object_add(call, "args", kept_args);
    return true;
}

static bool rust_specialize_default_array_alias_calls(json_object *model,
                                                      json_object *functions,
                                                      json_object *node,
                                                      size_t *next_id);

static bool rust_specialize_default_array_alias_call(json_object *model,
                                                     json_object *functions,
                                                     json_object *call,
                                                     size_t *next_id);

/* Classify each shared formal against the earlier representatives. Each leaf
 * contains one actual identity partition, which the static alias lowering
 * coalesces before it constructs any mutable references. */
static bool rust_dispatch_closure_array_aliases(json_object *model,
                                                json_object *functions,
                                                json_object *call,
                                                size_t *canonical,
                                                size_t count,
                                                size_t position,
                                                size_t candidate,
                                                size_t *next_id)
{
    json_object *args = rust_closure_property(call, "args");
    while (position < count)
    {
        json_object *arg = json_object_array_get_idx(args, position);
        if (canonical[position] == position &&
            json_boolean_property(arg, "rust_closure_array_parameter") &&
            json_boolean_property(arg, "rust_default_array_ref_arg")) break;
        position++;
        candidate = 0;
    }
    if (position == count)
    {
        /* Keep the source-order arguments until the complete partition is
         * known. The existing specialization retains owned as-val copies. */
        for (size_t i = 0; i < count; i++)
            if (canonical[i] != i)
                json_object_array_put_idx(args, i, json_object_get(
                    json_object_array_get_idx(args, canonical[i])));
        json_object_object_add(call, "rust_array_alias_partition_leaf",
                               json_object_new_boolean(true));
        return rust_specialize_default_array_alias_call(
            model, functions, call, next_id);
    }

    json_object *right = json_object_array_get_idx(args, position);
    json_object *right_type = rust_closure_property(right, "type");
    while (candidate < position)
    {
        json_object *left = json_object_array_get_idx(args, candidate);
        if (canonical[candidate] == candidate &&
            json_boolean_property(left, "rust_default_array_ref_arg") &&
            json_boolean_property(left, "rust_closure_array_parameter") &&
            json_object_equal(rust_closure_property(left, "type"), right_type)) break;
        candidate++;
    }
    if (candidate == position)
        return rust_dispatch_closure_array_aliases(
            model, functions, call, canonical, count, position + 1, 0, next_id);

    json_object *same = NULL, *distinct = NULL;
    if (json_object_deep_copy(call, &same, NULL) != 0 || !same ||
        json_object_deep_copy(call, &distinct, NULL) != 0 || !distinct)
    {
        if (same) json_object_put(same);
        if (distinct) json_object_put(distinct);
        return false;
    }
    canonical[position] = candidate;
    bool ok = rust_dispatch_closure_array_aliases(
        model, functions, same, canonical, count, position + 1, 0, next_id);
    canonical[position] = position;
    if (ok) ok = rust_dispatch_closure_array_aliases(
        model, functions, distinct, canonical, count, position, candidate + 1, next_id);
    if (!ok)
    {
        json_object_put(same);
        json_object_put(distinct);
        return false;
    }
    json_object *left = json_object_array_get_idx(args, candidate);
    const char *left_name = json_string_property(left, "rust_closure_array_method_cell_source");
    const char *right_name = json_string_property(right, "rust_closure_array_method_cell_source");
    if (!left_name) left_name = json_string_property(left, "name");
    if (!right_name) right_name = json_string_property(right, "name");
    json_object_object_add(call, "rust_array_alias_left", json_object_new_string(left_name));
    json_object_object_add(call, "rust_array_alias_right", json_object_new_string(right_name));
    json_object_object_add(call, "rust_array_alias_branch", same);
    json_object_object_add(call, "rust_array_alias_distinct_branch", distinct);
    json_object_object_add(call, "rust_dynamic_array_alias_call", json_object_new_boolean(true));
    return true;
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
    if ((!json_string_property_equals(call, "kind", "call") &&
         !json_string_property_equals(call, "kind", "method_call") &&
         !json_string_property_equals(call, "kind", "static_call")) ||
        json_boolean_property(call, "is_closure_call") ||
        json_boolean_property(call, "is_fn_field_call") ||
        json_boolean_property(call, "rust_array_alias_specialized") ||
        json_boolean_property(call, "rust_dynamic_array_alias_call"))
        return true;

    json_object *callee = NULL, *args = NULL, *structure = NULL;
    json_object *definitions = functions, *name_owner = NULL;
    const char *name_key = "name";
    if (!json_object_object_get_ex(call, "args", &args) ||
        !json_object_is_type(args, json_type_array)) return true;
    if (json_string_property_equals(call, "kind", "call")) {
        if (!json_object_object_get_ex(call, "callee", &callee)) return true;
        name_owner = callee;
        if (json_string_property_equals(callee, "kind", "member")) {
            json_object *receiver = NULL;
            json_object_object_get_ex(callee, "object", &receiver);
            structure = rust_receiver_structure(model, receiver);
            name_key = "member_name";
        } else if (!json_string_property_equals(callee, "kind", "variable")) return true;
    } else {
        name_owner = call;
        name_key = "method_name";
        if (json_string_property_equals(call, "kind", "static_call"))
            structure = rust_find_struct(model, json_string_property(call, "type_name"));
        else {
            json_object *type = NULL;
            json_object_object_get_ex(call, "struct_type", &type);
            structure = rust_find_struct(model, json_string_property(type, "name"));
        }
    }
    if (strcmp(name_key, "name") != 0 &&
        (!structure || !json_object_object_get_ex(structure, "methods", &definitions)))
        return true;
    const char *callee_name = json_string_property(name_owner, name_key);
    json_object *function = structure
        ? rust_find_resolved_method(structure, callee_name,
              json_string_property_equals(call, "kind", "static_call") ||
              json_boolean_property(call, "is_static"))
        : (callee_name ? rust_default_array_function(definitions, callee_name) : NULL);
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
    if (!json_boolean_property(call, "rust_array_alias_partition_leaf"))
    {
        size_t shared_count = 0;
        for (size_t i = 0; i < count; i++)
        {
            json_object *arg = json_object_array_get_idx(args, i);
            if (canonical[i] == i &&
                json_boolean_property(arg, "rust_default_array_ref_arg") &&
                json_boolean_property(arg, "rust_closure_array_parameter")) shared_count++;
        }
        if (shared_count > 1)
        {
            bool ok = rust_dispatch_closure_array_aliases(
                model, functions, call, canonical, count, 0, 0, next_id);
            free(canonical);
            return ok;
        }
    }
    if (!has_duplicate)
    {
        free(canonical);
        return true;
    }

    /* A recursive or mutually recursive body can rediscover the same alias
     * partition. Reuse the specialization already registered for that
     * function/pattern so recursive lowering terminates without reintroducing
     * overlapping mutable references. */
    json_object *existing = rust_array_alias_specialization(
        definitions, callee_name, canonical, count);
    if (existing)
    {
        const char *existing_name = json_string_property(existing, "name");
        if (!existing_name ||
            !rust_coalesce_array_alias_args(call, args, canonical, count))
        {
            free(canonical);
            return false;
        }
        json_object_object_del(name_owner, name_key);
        json_object_object_add(name_owner, name_key,
                               json_object_new_string(existing_name));
        json_object_object_add(call, "rust_array_alias_specialized",
                               json_object_new_boolean(true));
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

    json_object *copy_inputs = json_object_new_array(), *copies = json_object_new_array();
    /* Coalesce incoming handles, not the owned as-val locals. Keep the
     * original borrowed input alive under a private name while making each
     * independent copy in source parameter order. */
    for (size_t i = 0; i < count; i++)
    {
        json_object *param = json_object_array_get_idx(specialized_params, i);
        if (canonical[i] != i || !json_boolean_property(param, "needs_array_copy")) continue;
        char input_name[96];
        do {
            snprintf(input_name, sizeof(input_name), "__sn_array_copy_input_%zu", (*next_id)++);
        } while (rust_call_model_contains_string(model, input_name));
        json_object *input = json_object_new_object();
        json_object_object_add(input, "name", json_object_new_string(input_name));
        json_object_object_add(input, "source", json_object_get(rust_closure_property(param, "name")));
        json_object_array_add(copy_inputs, input);
        json_object_object_add(param, "rust_array_copy_source", json_object_new_string(input_name));
    }
    for (size_t i = 0; i < count; i++)
    {
        json_object *param = json_object_array_get_idx(specialized_params, i);
        if (!json_boolean_property(param, "needs_array_copy")) continue;
        json_object *source = json_object_array_get_idx(specialized_params, canonical[i]);
        const char *source_name = json_string_property(source, "rust_array_copy_source");
        if (!source_name) source_name = json_string_property(source, "name");
        json_object_object_add(param, "rust_array_copy_source", json_object_new_string(source_name));
        json_object_array_add(copies, json_object_get(param));
    }
    json_object_object_add(specialized, "rust_array_copy_inputs", copy_inputs);
    if (json_object_array_length(copies))
        json_object_object_add(specialized, "rust_array_alias_copies", copies);
    else json_object_put(copies);

    for (size_t i = 0; i < count; i++)
    {
        if (canonical[i] == i) continue;
        json_object *from = json_object_array_get_idx(specialized_params, i);
        if (json_boolean_property(from, "needs_array_copy")) continue;
        json_object *to = json_object_array_get_idx(
            specialized_params, canonical[i]);
        const char *from_name = json_string_property(from, "name");
        const char *to_name = json_string_property(to, "rust_array_copy_source");
        if (!to_name) to_name = json_string_property(to, "name");
        json_object *from_binding = NULL;
        if (!from_name || !to_name ||
            !json_object_object_get_ex(
                from, "rust_binding_id", &from_binding))
        {
            json_object_put(specialized);
            free(canonical);
            return false;
        }
        rust_rename_binding_uses(
            body, json_object_get_int64(from_binding), to_name);
    }

    json_object *kept_params = json_object_new_array();
    if (!kept_params)
    {
        json_object_put(specialized);
        free(canonical);
        return false;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (canonical[i] != i) continue;
        json_object_array_add(
            kept_params, json_object_get(json_object_array_get_idx(specialized_params, i)));
    }
    json_object_object_del(specialized, "params");
    json_object_object_add(specialized, "params", kept_params);
    if (!rust_coalesce_array_alias_args(call, args, canonical, count))
    {
        json_object_put(specialized);
        free(canonical);
        return false;
    }

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
    json_object_object_add(specialized, "rust_array_alias_origin",
                           json_object_new_string(callee_name));
    json_object *pattern = json_object_new_array();
    if (!pattern)
    {
        json_object_put(specialized);
        free(canonical);
        return false;
    }
    for (size_t i = 0; i < count; i++)
        json_object_array_add(pattern, json_object_new_int64((int64_t)canonical[i]));
    json_object_object_add(specialized, "rust_array_alias_pattern", pattern);
    json_object_object_del(name_owner, name_key);
    json_object_object_add(name_owner, name_key, json_object_new_string(specialized_name));
    json_object_object_add(call, "rust_array_alias_specialized",
                           json_object_new_boolean(true));
    json_object_array_add(definitions, specialized);
    bool ok = rust_specialize_default_array_alias_calls(
        model, functions, body, next_id);
    free(canonical);
    return ok;
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

    bool direct_call = json_string_property_equals(node, "kind", "call");
    bool method_call = json_string_property_equals(node, "kind", "method_call");
    bool static_call = json_string_property_equals(node, "kind", "static_call");
    if (!direct_call && !method_call && !static_call) return;
    if (!direct_call) {
        json_object *structure_type = NULL, *params = NULL;
        json_object_object_get_ex(node, "struct_type", &structure_type);
        const char *structure_name = static_call ? json_string_property(node, "type_name")
            : json_string_property(structure_type, "name");
        json_object *method = rust_find_resolved_method(
            rust_find_struct(model, structure_name), json_string_property(node, "method_name"),
            static_call || json_boolean_property(node, "is_static"));
        json_object_object_get_ex(method, "params", &params);
        bool has_copy = false;
        for (size_t i = 0; params && i < json_object_array_length(params); i++)
            has_copy |= json_boolean_property(json_object_array_get_idx(params, i), "needs_array_copy");
        /* Shared closure cells also need argument effects to finish before
         * any mutable projection, including the all-distinct partition. */
        json_object *method_args = rust_closure_property(node, "args");
        bool has_shared_array = false;
        for (size_t i = 0; method_args && i < json_object_array_length(method_args); i++)
            has_shared_array |= json_boolean_property(
                json_object_array_get_idx(method_args, i), "rust_closure_array_parameter");
        if (!has_copy && !has_shared_array) return;
    }
    json_object *args = NULL;
    if (!json_object_object_get_ex(node, "args", &args) ||
        !json_object_is_type(args, json_type_array)) return;

    size_t count = json_object_array_length(args);
    size_t first_array_index = count;
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!(json_boolean_property(arg, "rust_default_array_ref_arg") ||
            json_boolean_property(arg, "rust_thread_array_copy_borrow"))) continue;
        if (!rust_deferred_default_array_place(arg)) return;
        if (first_array_index == count) first_array_index = i;
    }
    if (first_array_index == count) return;
    bool has_later_value = false;
    for (size_t i = first_array_index + 1; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!(json_boolean_property(arg, "rust_default_array_ref_arg") ||
            json_boolean_property(arg, "rust_thread_array_copy_borrow")))
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
        if ((json_boolean_property(arg, "rust_default_array_ref_arg") ||
            json_boolean_property(arg, "rust_thread_array_copy_borrow"))) continue;
        if (json_boolean_property(arg, "is_ref_arg") ||
            json_boolean_property(arg, "is_borrow_tmp")) return;
    }

    /* Bind siblings on both sides: leaving an earlier expression in the final
     * call would move it after the pre-evaluated later arguments. */
    for (size_t i = 0; i < count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if ((json_boolean_property(arg, "rust_default_array_ref_arg") ||
            json_boolean_property(arg, "rust_thread_array_copy_borrow"))) continue;
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

/* C returns and initializes independent copies of borrowed string elements.
 * Rust must acquire an element before its array owner leaves scope. Bind computed
 * owners once, without moving a borrowed field or nested array place. */
static void rust_lower_indexed_string_acquires(json_object *node,
                                               json_object *model,
                                               size_t *binding_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_indexed_string_acquires(json_object_array_get_idx(node, i),
                                               model, binding_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child) {
        (void)key;
        rust_lower_indexed_string_acquires(child, model, binding_id);
    }
    json_object *value = NULL, *type = NULL, *array = NULL;
    bool initializer = json_string_property_equals(node, "kind", "var_decl");
    if (initializer) json_object_object_get_ex(node, "initializer", &value);
    else if (json_string_property_equals(node, "kind", "return"))
        json_object_object_get_ex(node, "value", &value);
    else return;
    if (!json_string_property_equals(value, "kind", "array_access") ||
        !json_object_object_get_ex(value, "type", &type) ||
        !json_string_property_equals(type, "kind", "string") ||
        json_boolean_property(value, "rust_resolved_clone")) return;
    bool bind = json_object_object_get_ex(value, "array", &array) &&
        !json_string_property_equals(array, "kind", "variable") &&
        !json_boolean_property(value, "rust_bind_array_once");
    /* Ordinary initializers already acquire in var_decl. A bound computed
     * owner acquires its element inside the block, before the owner expires. */
    if (initializer && !bind) return;
    json_object_object_add(value, "rust_needs_clone", json_object_new_boolean(true));
    if (!bind) return;
    if (initializer)
        json_object_object_add(node, "rust_indexed_initializer_owned", json_object_new_boolean(true));
    char array_name[128], index_name[128];
    do {
        snprintf(array_name, sizeof(array_name), "__sn_returned_string_%zu_array", *binding_id);
        snprintf(index_name, sizeof(index_name), "__sn_returned_string_%zu_index", *binding_id);
        (*binding_id)++;
    } while (rust_call_model_contains_string(model, array_name) ||
             rust_call_model_contains_string(model, index_name));
    json_object_object_add(value, "rust_bound_string_array_name", json_object_new_string(array_name));
    json_object_object_add(value, "rust_bound_string_index_name", json_object_new_string(index_name));
}

/* Reading a reference record acquires its identity, never its contents.
 * This applies equally to array/field reads, return edges and assignments to
 * globals. Store projections remove clone annotations before borrowing. */
static void rust_lower_reference_record_reads(json_object *node, json_object *model)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_reference_record_reads(json_object_array_get_idx(node, i), model);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_lower_reference_record_reads(child, model);
    }
    json_object *type = NULL;
    const char *kind = json_string_property(node, "kind");
    if (kind && (strcmp(kind, "variable") == 0 || strcmp(kind, "member") == 0 ||
                 strcmp(kind, "array_access") == 0) &&
        json_object_object_get_ex(node, "type", &type) &&
        json_string_property_equals(type, "kind", "struct") &&
        json_boolean_property(rust_find_struct(model, json_string_property(type, "name")),
                              "rust_thread_reference_identity") &&
        !json_boolean_property(node, "is_ref_arg") &&
        !json_boolean_property(node, "rust_resolved_clone"))
        json_object_object_add(node, "rust_needs_clone", json_object_new_boolean(true));
}

/* Normalize nested array mutation indices before taking the mutable element
 * borrow. Otherwise Rust overlaps the indexing length read with that borrow. */
static void rust_lower_indexed_array_mutations(json_object *model, json_object *node, size_t *next_id)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_indexed_array_mutations(model, json_object_array_get_idx(node, i), next_id);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) rust_lower_indexed_array_mutations(model, child, next_id);
    if (!json_string_property_equals(node, "kind", "call") ||
        json_boolean_property(node, "rust_array_cell_mutation") ||
        json_boolean_property(node, "rust_field_array_mutation")) return;
    json_object *callee = NULL, *receiver = NULL, *type = NULL;
    json_object_object_get_ex(node, "callee", &callee);
    json_object_object_get_ex(callee, "object", &receiver);
    json_object_object_get_ex(receiver, "type", &type);
    if (!json_string_property_equals(callee, "kind", "member") ||
        !json_string_property_equals(receiver, "kind", "array_access") ||
        !json_string_property_equals(type, "kind", "array")) return;
    const char *method = json_string_property(callee, "member_name");
    if (!rust_closure_mutating_array_method(method)) return;
    /* The C push macro evaluates its value before its array place. Evaluate
     * and acquire that value once, before normalizing nested receiver indices. */
    json_object *args = NULL;
    if (strcmp(method, "push") == 0 && json_object_object_get_ex(node, "args", &args) &&
        json_object_array_length(args) == 1)
    {
        char name[96];
        do {
            snprintf(name, sizeof(name), "__sn_indexed_push_value_%zu", (*next_id)++);
        } while (rust_call_model_contains_string(model, name));
        json_object *value = json_object_array_get_idx(args, 0);
        json_object_object_add(node, "rust_indexed_array_push_value", json_object_get(value));
        json_object_object_add(node, "rust_indexed_array_push_value_name", json_object_new_string(name));
        json_object *variable = json_object_new_object(), *value_type = NULL;
        json_object_object_get_ex(value, "type", &value_type);
        json_object_object_add(variable, "kind", json_object_new_string("variable"));
        json_object_object_add(variable, "name", json_object_new_string(name));
        if (value_type) json_object_object_add(variable, "type", json_object_get(value_type));
        json_object_array_put_idx(args, 0, variable);
    }
    json_object *prefix = json_object_new_array();
    rust_stabilize_resolved_receiver(model, receiver, prefix, next_id, true);
    if (json_object_array_length(prefix))
        json_object_object_add(node, "rust_indexed_array_mutation_prefix", prefix);
    else json_object_put(prefix);
}

static bool rust_lower_calls(json_object *model)
{
    rust_lower_reference_record_reads(model, model);
    rust_lower_split_lines_accesses(model);
    json_object *functions = NULL;
    if (!json_object_object_get_ex(model, "functions", &functions) ||
        !json_object_is_type(functions, json_type_array)) return false;
    size_t receiver_array_alias_id = 0;
    if (!rust_specialize_receiver_array_alias_calls(
            model, model, &receiver_array_alias_id)) return false;
    size_t array_alias_id = 0;
    if (!rust_specialize_default_array_alias_calls(
            model, functions, model, &array_alias_id)) return false;
    rust_lower_float_array_calls(model);
    rust_lower_instance_method_clones(model);
    size_t resolved_call_id = 0;
    rust_lower_resolved_receiver_prefixes(model, model, &resolved_call_id);
    size_t array_arg_id = 0;
    rust_lower_default_array_ref_indices(model, model, &array_arg_id);
    size_t array_late_read_id = 0;
    rust_lower_default_array_late_reads(model, model, &array_late_read_id);
    size_t indexed_array_mutation_id = 0;
    rust_lower_indexed_array_mutations(model, model, &indexed_array_mutation_id);
    size_t returned_string_id = 0;
    rust_lower_indexed_string_acquires(model, model, &returned_string_id);
    return true;
}
