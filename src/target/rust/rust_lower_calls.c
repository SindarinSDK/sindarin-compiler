/* Included by rust_lower.c. Call annotations retain their original passes. */

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
         * the call site. C's model does not need an acquire annotation for all
         * default parameters, so record the Rust move/clone decision here. */
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
                    json_object *arg_type = NULL;
                    const char *arg_kind = json_string_property(arg, "kind");
                    if (json_object_object_get_ex(arg, "type", &arg_type) &&
                        json_string_property_equals(arg_type, "kind", "string") &&
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
                json_object *arg_type = NULL;
                const char *arg_kind = json_string_property(arg, "kind");
                if (json_object_object_get_ex(arg, "type", &arg_type) &&
                    json_string_property_equals(arg_type, "kind", "string") &&
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
            if (rust_owned_value_type(node))
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
    if (kind && strcmp(kind, "call") == 0)
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
        rust_owned_value_type(node))
        json_object_object_add(node, "rust_needs_clone",
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
}
