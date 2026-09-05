/* Included by rust_validate.c. Owns ordinary and resolved call validation. */

static bool rust_array_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "push") == 0 || strcmp(name, "pop") == 0 ||
           strcmp(name, "insert") == 0 || strcmp(name, "remove") == 0 ||
           strcmp(name, "reverse") == 0 || strcmp(name, "clear") == 0 ||
           strcmp(name, "clone") == 0 || strcmp(name, "contains") == 0 ||
           strcmp(name, "indexOf") == 0 || strcmp(name, "concat") == 0 ||
           strcmp(name, "join") == 0 || strcmp(name, "toString") == 0;
}

static bool rust_array_text_element_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (!kind) return false;
    if (strcmp(kind, "array") == 0)
    {
        json_object *element_type = NULL;
        return json_object_object_get_ex(type, "element_type", &element_type) &&
               rust_array_text_element_supported(element_type);
    }
    return strcmp(kind, "int") == 0 || strcmp(kind, "long") == 0 ||
           strcmp(kind, "uint") == 0 || strcmp(kind, "double") == 0 ||
           strcmp(kind, "bool") == 0 ||
           strcmp(kind, "char") == 0 || strcmp(kind, "byte") == 0 ||
           strcmp(kind, "string") == 0;
}

static bool rust_string_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "contains") == 0 || strcmp(name, "startsWith") == 0 ||
           strcmp(name, "endsWith") == 0 || strcmp(name, "trim") == 0 ||
           strcmp(name, "toUpper") == 0 || strcmp(name, "toLower") == 0 ||
           strcmp(name, "substring") == 0 || strcmp(name, "replace") == 0 ||
           strcmp(name, "charAt") == 0 || strcmp(name, "indexOf") == 0;
}

static bool rust_primitive_conversion_member(const char *type_kind, const char *name)
{
    if (!type_kind || !name) return false;
    if (strcmp(type_kind, "int") == 0)
        return strcmp(name, "toDouble") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toUint") == 0 || strcmp(name, "toByte") == 0 ||
               strcmp(name, "toChar") == 0;
    if (strcmp(type_kind, "long") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toDouble") == 0;
    if (strcmp(type_kind, "double") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0;
    if (strcmp(type_kind, "uint") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toDouble") == 0;
    if (strcmp(type_kind, "byte") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toChar") == 0;
    if (strcmp(type_kind, "bool") == 0)
        return strcmp(name, "toInt") == 0;
    if (strcmp(type_kind, "char") == 0)
        return strcmp(name, "toInt") == 0;
    if (strcmp(type_kind, "string") == 0)
        return strcmp(name, "toInt") == 0 || strcmp(name, "toLong") == 0 ||
               strcmp(name, "toDouble") == 0;
    return false;
}

static bool rust_primitive_integer_conversion_supported(const char *type_kind,
                                                        const char *name)
{
    return (strcmp(type_kind, "int") == 0 &&
            (strcmp(name, "toDouble") == 0 || strcmp(name, "toLong") == 0 ||
             strcmp(name, "toUint") == 0 ||
             strcmp(name, "toByte") == 0)) ||
           (strcmp(type_kind, "long") == 0 &&
            (strcmp(name, "toInt") == 0 || strcmp(name, "toDouble") == 0)) ||
           (strcmp(type_kind, "uint") == 0 && strcmp(name, "toDouble") == 0) ||
           (strcmp(type_kind, "byte") == 0 && strcmp(name, "toInt") == 0) ||
           (strcmp(type_kind, "bool") == 0 && strcmp(name, "toInt") == 0);
}

static bool rust_array_search_type_supported(const char *kind)
{
    return rust_integer_type(kind) ||
           (kind && (strcmp(kind, "float") == 0 || strcmp(kind, "double") == 0)) ||
           (kind && (strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
                     strcmp(kind, "string") == 0));
}

static bool rust_mutation_place_is_self_rooted(json_object *place)
{
    if (!json_object_is_type(place, json_type_object)) return false;
    if (json_string_property_equals(place, "kind", "variable"))
        return json_string_property_equals(place, "name", "self");

    json_object *parent = NULL;
    if (json_string_property_equals(place, "kind", "member") &&
        json_object_object_get_ex(place, "object", &parent))
        return rust_mutation_place_is_self_rooted(parent);
    if (json_string_property_equals(place, "kind", "array_access") &&
        json_object_object_get_ex(place, "array", &parent))
        return rust_mutation_place_is_self_rooted(parent);
    return false;
}

static bool rust_is_mutating_array_call(json_object *node)
{
    if (!json_string_property_equals(node, "kind", "call")) return false;
    json_object *callee = NULL, *object = NULL, *type = NULL;
    if (!json_object_object_get_ex(node, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &type) ||
        !json_string_property_equals(type, "kind", "array")) return false;
    const char *method = json_string_property(callee, "member_name");
    return method && (strcmp(method, "push") == 0 ||
                      strcmp(method, "pop") == 0 ||
                      strcmp(method, "insert") == 0 ||
                      strcmp(method, "remove") == 0 ||
                      strcmp(method, "reverse") == 0 ||
                      strcmp(method, "clear") == 0);
}

static bool rust_instance_method_node_supported(json_object *node,
                                                bool allow_owned_self)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!rust_instance_method_node_supported(
                    json_object_array_get_idx(node, i), allow_owned_self)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;

    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, "variable") == 0)
        return allow_owned_self || !json_string_property_equals(node, "name", "self");
    if (kind && strcmp(kind, "member_assign") == 0)
    {
        json_object *object = NULL, *value = NULL;
        if (!json_object_object_get_ex(node, "object", &object) ||
            !json_object_object_get_ex(node, "value", &value) ||
            !rust_instance_method_node_supported(value, allow_owned_self)) return false;
        if (json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self")) return true;
        return rust_instance_method_node_supported(object, allow_owned_self);
    }
    if (kind && strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        if (!json_object_object_get_ex(node, "object", &object)) return false;
        if (json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self")) return true;
        return rust_instance_method_node_supported(object, allow_owned_self);
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (!rust_instance_method_node_supported(value, allow_owned_self)) return false;
    }
    return true;
}

static bool rust_method_has_direct_mutation(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_method_has_direct_mutation(
                    json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    json_object *place = NULL;
    if ((json_string_property_equals(node, "kind", "compound_assign") &&
         json_object_object_get_ex(node, "target", &place)) ||
        ((json_string_property_equals(node, "kind", "increment") ||
          json_string_property_equals(node, "kind", "decrement")) &&
         json_object_object_get_ex(node, "operand", &place)) ||
        (json_string_property_equals(node, "kind", "member_assign") &&
         json_object_object_get_ex(node, "object", &place)) ||
        (json_string_property_equals(node, "kind", "index_assign") &&
         json_object_object_get_ex(node, "array", &place)))
    {
        if (rust_mutation_place_is_self_rooted(place)) return true;
    }
    if (rust_is_mutating_array_call(node))
    {
        json_object *callee = NULL, *object = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_object_object_get_ex(callee, "object", &object) &&
            rust_mutation_place_is_self_rooted(object)) return true;
    }
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_method_has_direct_mutation(value)) return true;
    }
    return false;
}

static bool rust_method_calls_mutating_self(json_object *node,
                                            json_object *methods)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_method_calls_mutating_self(
                    json_object_array_get_idx(node, i), methods)) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    if (json_string_property_equals(node, "kind", "call"))
    {
        json_object *callee = NULL, *object = NULL;
        if (json_object_object_get_ex(node, "callee", &callee) &&
            json_string_property_equals(callee, "kind", "member") &&
            json_object_object_get_ex(callee, "object", &object) &&
            json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", "self"))
        {
            const char *called_name = json_string_property(callee, "member_name");
            size_t method_count = json_object_array_length(methods);
            for (size_t i = 0; called_name && i < method_count; i++)
            {
                json_object *called = json_object_array_get_idx(methods, i);
                const char *name = json_string_property(called, "name");
                if (name && strcmp(name, called_name) == 0 &&
                    json_boolean_property(called, "rust_mutating")) return true;
            }
        }
    }

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_method_calls_mutating_self(value, methods)) return true;
    }
    return false;
}

static bool rust_validate_struct_methods(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return true;

    size_t struct_count = json_object_array_length(structs);
    for (size_t i = 0; i < struct_count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        const char *struct_name = json_string_property(structure, "name");
        json_object *methods = NULL;
        if (!json_object_object_get_ex(structure, "methods", &methods)) continue;

        size_t method_count = json_object_array_length(methods);
        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            json_object *body = NULL;
            if (!json_boolean_property(method, "is_static") &&
                json_object_object_get_ex(method, "body", &body) &&
                rust_method_has_direct_mutation(body))
                json_object_object_add(method, "rust_mutating",
                                       json_object_new_boolean(true));
        }
        bool changed;
        do
        {
            changed = false;
            for (size_t m = 0; m < method_count; m++)
            {
                json_object *method = json_object_array_get_idx(methods, m);
                json_object *body = NULL;
                if (!json_boolean_property(method, "is_static") &&
                    !json_boolean_property(method, "rust_mutating") &&
                    json_object_object_get_ex(method, "body", &body) &&
                    rust_method_calls_mutating_self(body, methods))
                {
                    json_object_object_add(method, "rust_mutating",
                                           json_object_new_boolean(true));
                    changed = true;
                }
            }
        }
        while (changed);

        for (size_t m = 0; m < method_count; m++)
        {
            json_object *method = json_object_array_get_idx(methods, m);
            const char *method_name = json_string_property(method, "name");
            json_object *return_type = NULL, *params = NULL, *body = NULL;
            bool is_static = json_boolean_property(method, "is_static");
            if (json_boolean_property(method, "is_native"))
            {
                fprintf(stderr,
                        "Error: Rust target supports only non-native methods on plain value struct '%s'\n",
                        struct_name ? struct_name : "<anonymous>");
                return false;
            }
            if (!json_object_object_get_ex(method, "return_type", &return_type) ||
                !rust_type_supported(return_type))
            {
                fprintf(stderr,
                        "Error: Rust target does not support return type of static method '%s.%s'\n",
                        struct_name ? struct_name : "<anonymous>",
                        method_name ? method_name : "<anonymous>");
                return false;
            }
            if (json_object_object_get_ex(method, "params", &params))
            {
                size_t param_count = json_object_array_length(params);
                for (size_t p = 0; p < param_count; p++)
                {
                    json_object *param = json_object_array_get_idx(params, p);
                    json_object *param_type = NULL;
                    const char *mem_qual = json_string_property(param, "mem_qual");
                    const char *sync_mod = json_string_property(param, "sync_mod");
                    bool has_param_type =
                        json_object_object_get_ex(param, "type", &param_type);
                    bool mem_qual_supported =
                        !mem_qual || strcmp(mem_qual, "default") == 0 ||
                        (has_param_type &&
                         strcmp(mem_qual, "as_ref") == 0 &&
                         rust_scalar_ref_parameter_type_supported(param_type)) ||
                        (is_static && has_param_type &&
                         strcmp(mem_qual, "as_ref") == 0 &&
                         rust_heap_free_named_struct_type(param_type)) ||
                        (is_static && has_param_type &&
                         strcmp(mem_qual, "as_val") == 0 &&
                         rust_heap_free_named_struct_type(param_type)) ||
                        (!is_static &&
                         !json_boolean_property(method, "rust_mutating") &&
                         has_param_type && strcmp(mem_qual, "as_val") == 0 &&
                         rust_heap_free_named_struct_type(param_type));
                    if (!has_param_type ||
                        !rust_type_supported(param_type) ||
                        !mem_qual_supported ||
                        (sync_mod && strcmp(sync_mod, "none") != 0))
                    {
                        fprintf(stderr,
                                "Error: Rust target does not support a parameter of %smethod '%s.%s'\n",
                                is_static ? "static " : "",
                                struct_name ? struct_name : "<anonymous>",
                                method_name ? method_name : "<anonymous>");
                        return false;
                    }
                }
            }
            json_object_object_get_ex(method, "body", &body);
            if (!rust_validate_statements(body) ||
                (!is_static && !rust_instance_method_node_supported(
                    body, !json_boolean_property(structure, "has_heap_fields"))))
            {
                if (!rust_validation_reported_error)
                    fprintf(stderr,
                            "Error: Rust target encountered an unsupported construct in method '%s.%s'\n",
                            struct_name ? struct_name : "<anonymous>",
                            method_name ? method_name : "<anonymous>");
                return false;
            }
        }
    }
    return true;
}

static bool rust_validate_static_call(json_object *expr)
{
    json_object *args = NULL;
    json_object_object_get_ex(expr, "args", &args);
    return rust_validate_expr_array(args);
}

static bool rust_validate_call(json_object *expr)
{
    RustValidationResult closure = rust_validate_closure_call(expr);
    if (closure != RUST_VALIDATION_UNHANDLED)
        return closure == RUST_VALIDATION_SUPPORTED;
    json_object *callee = NULL, *args = NULL, *callee_kind = NULL;
    if (!json_object_object_get_ex(expr, "callee", &callee) ||
        !json_object_object_get_ex(callee, "kind", &callee_kind))
        return false;
    const char *callee_kind_name = json_object_get_string(callee_kind);
    if (!callee_kind_name) return false;
    if (strcmp(callee_kind_name, "member") == 0)
    {
        json_object *object = NULL, *object_type = NULL;
        const char *object_type_kind = NULL;
        if (!json_object_object_get_ex(callee, "object", &object) ||
            !json_object_object_get_ex(object, "type", &object_type) ||
            !(object_type_kind = json_string_property(object_type, "kind"))) return false;
        const char *method = json_string_property(callee, "member_name");
        if (rust_primitive_conversion_member(object_type_kind, method))
        {
            json_object_object_get_ex(expr, "args", &args);
            if (!args || json_object_array_length(args) != 0) return false;
            if (!rust_validate_expr(object)) return false;
            if (!rust_primitive_integer_conversion_supported(object_type_kind, method))
            {
                fprintf(stderr,
                        "Error: Rust target does not support primitive conversion %s.%s() yet\n",
                        object_type_kind, method);
                return false;
            }
            json_object_object_add(expr, "rust_primitive_conversion",
                                   json_object_new_boolean(true));
            return true;
        }
        if (strcmp(object_type_kind, "array") == 0)
        {
            if (!rust_array_method_supported(method))
            {
                fprintf(stderr, "Error: Rust target does not support array method '%s' yet\n",
                        method ? method : "<unknown>");
                return false;
            }
            if ((strcmp(method, "contains") == 0 || strcmp(method, "indexOf") == 0))
            {
                json_object *element_type = NULL, *arg = NULL, *arg_type = NULL;
                const char *element_kind = NULL, *arg_kind = NULL;
                if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                    !(element_kind = json_string_property(element_type, "kind")) ||
                    !rust_array_search_type_supported(element_kind))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support array method '%s' for %s elements yet\n",
                            method, element_kind ? element_kind : "<unknown>");
                    return false;
                }
                if (strcmp(element_kind, "float") == 0 &&
                    (!json_object_object_get_ex(expr, "args", &args) ||
                     json_object_array_length(args) != 1 ||
                     !(arg = json_object_array_get_idx(args, 0)) ||
                     !json_object_object_get_ex(arg, "type", &arg_type) ||
                     !(arg_kind = json_string_property(arg_type, "kind")) ||
                     strcmp(arg_kind, "float") != 0))
                {
                    fprintf(stderr,
                            "Error: Rust target requires array method '%s' on float[] to receive an exact float argument; got %s\n",
                            method, arg_kind ? arg_kind : "<unknown>");
                    return false;
                }
            }
            if (strcmp(method, "concat") == 0)
            {
                json_object *element_type = NULL;
                const char *element_kind = NULL;
                if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                    !(element_kind = json_string_property(element_type, "kind")) ||
                    !rust_array_concat_type_supported(element_type))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support array method 'concat' for %s elements yet\n",
                            element_kind ? element_kind : "<unknown>");
                    return false;
                }
            }
            if (strcmp(method, "join") == 0)
            {
                json_object *element_type = NULL;
                const char *element_kind = NULL;
                if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                    !(element_kind = json_string_property(element_type, "kind")) ||
                    !rust_array_text_element_supported(element_type))
                {
                    fprintf(stderr,
                            "Error: Rust target does not support array method 'join' for %s elements yet\n",
                            element_kind ? element_kind : "<unknown>");
                    return false;
                }
            }
            if (strcmp(method, "toString") == 0)
            {
                json_object *element_type = NULL;
                if (!json_object_object_get_ex(object_type, "element_type", &element_type) ||
                    !json_string_property_equals(element_type, "kind", "byte"))
                {
                    fprintf(stderr,
                            "Error: Rust target supports array method 'toString' only for byte elements\n");
                    return false;
                }
            }
            if (!rust_validate_expr(object))
                return false;
        }
        else if (strcmp(object_type_kind, "string") == 0)
        {
            if (!rust_string_method_supported(method))
            {
                fprintf(stderr, "Error: Rust target does not support string method '%s' yet\n",
                        method ? method : "<unknown>");
                return false;
            }
            if (!rust_validate_expr(object)) return false;
        }
        else if (strcmp(object_type_kind, "struct") == 0)
        {
            if (!rust_validate_expr(object)) return false;
        }
        else if (strcmp(object_type_kind, "pointer") == 0)
        {
            json_object *base_type = NULL;
            if (!json_object_object_get_ex(object_type, "base_type", &base_type) ||
                !json_string_property_equals(base_type, "kind", "struct") ||
                !rust_validate_expr(object)) return false;
        }
        else return false;
    }
    else if (strcmp(callee_kind_name, "variable") != 0) return false;
    json_object_object_get_ex(expr, "args", &args);
    return rust_validate_expr_array(args);
}

/* These model kinds previously reached the expression fallback. */
static bool rust_validate_resolved_call(json_object *expr)
{
    (void)expr;
    return false;
}
