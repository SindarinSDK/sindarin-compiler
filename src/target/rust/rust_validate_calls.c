/* Included by rust_validate.c. Owns ordinary and resolved call validation. */

static bool rust_array_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "push") == 0 || strcmp(name, "pop") == 0 ||
           strcmp(name, "insert") == 0 || strcmp(name, "remove") == 0 ||
           strcmp(name, "reverse") == 0 || strcmp(name, "clear") == 0 ||
           strcmp(name, "clone") == 0 || strcmp(name, "contains") == 0 ||
           strcmp(name, "indexOf") == 0 || strcmp(name, "concat") == 0 ||
           strcmp(name, "join") == 0;
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
                        (json_boolean_property(method, "is_operator") &&
                         has_param_type && strcmp(mem_qual, "as_ref") == 0 &&
                         json_string_property_equals(param_type, "kind", "struct")) ||
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
                    strcmp(element_kind, "string") != 0)
                {
                    fprintf(stderr,
                            "Error: Rust target does not support array method 'join' for %s elements yet\n",
                            element_kind ? element_kind : "<unknown>");
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

static bool rust_report_resolved_call_error(const char *message)
{
    rust_validation_reported_error = true;
    fprintf(stderr, "Error: Rust target %s\n", message);
    return false;
}

static bool rust_resolved_types_equal(json_object *left, json_object *right)
{
    const char *left_kind = json_string_property(left, "kind");
    const char *right_kind = json_string_property(right, "kind");
    if (!left_kind || !right_kind || strcmp(left_kind, right_kind) != 0)
        return false;

    if (strcmp(left_kind, "struct") == 0)
    {
        const char *left_name = json_string_property(left, "name");
        const char *right_name = json_string_property(right, "name");
        return left_name && right_name && strcmp(left_name, right_name) == 0;
    }
    if (strcmp(left_kind, "array") == 0)
    {
        json_object *left_element = NULL, *right_element = NULL;
        return json_object_object_get_ex(left, "element_type", &left_element) &&
            json_object_object_get_ex(right, "element_type", &right_element) &&
            rust_resolved_types_equal(left_element, right_element);
    }
    return true;
}

static json_object *rust_find_resolved_method(json_object *structure,
                                              const char *name,
                                              bool is_static)
{
    json_object *methods = NULL;
    if (!structure || !name ||
        !json_object_object_get_ex(structure, "methods", &methods) ||
        !json_object_is_type(methods, json_type_array)) return NULL;

    size_t count = json_object_array_length(methods);
    for (size_t i = 0; i < count; i++)
    {
        json_object *method = json_object_array_get_idx(methods, i);
        if (json_string_property_equals(method, "name", name) &&
            json_boolean_property(method, "is_static") == is_static)
            return method;
    }
    return NULL;
}

static bool rust_resolved_value_type_supported(json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (!kind || strcmp(kind, "pointer") == 0 ||
        strcmp(kind, "function") == 0 || strcmp(kind, "interface") == 0 ||
        strcmp(kind, "nil") == 0) return false;
    if (strcmp(kind, "array") == 0)
    {
        json_object *element = NULL;
        return json_object_object_get_ex(type, "element_type", &element) &&
            rust_resolved_value_type_supported(element);
    }
    if (strcmp(kind, "struct") == 0)
    {
        json_object *structure = rust_find_struct(
            rust_validation_model, json_string_property(type, "name"));
        const char *mem_mode = json_string_property(structure, "mem_mode");
        return structure && !json_boolean_property(structure, "is_native") &&
            !json_boolean_property(structure, "is_packed") &&
            !json_boolean_property(structure, "pass_self_by_ref") &&
            (!mem_mode || strcmp(mem_mode, "val") == 0);
    }
    return rust_type_supported(type);
}

static bool rust_resolved_stable_place(json_object *expr)
{
    const char *kind = json_string_property(expr, "kind");
    if (!kind) return false;
    if (strcmp(kind, "variable") == 0) return true;
    if (strcmp(kind, "member") == 0)
    {
        json_object *object = NULL;
        return json_object_object_get_ex(expr, "object", &object) &&
            rust_resolved_stable_place(object);
    }
    if (strcmp(kind, "array_access") == 0)
    {
        json_object *array = NULL, *index = NULL;
        const char *index_kind = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
            json_object_object_get_ex(expr, "index", &index) &&
            rust_resolved_stable_place(array) &&
            (index_kind = json_string_property(index, "kind")) &&
            (strcmp(index_kind, "literal") == 0 ||
             rust_resolved_stable_place(index));
    }
    return false;
}

static bool rust_resolved_clone_source(json_object *arg)
{
    json_object *type = NULL;
    const char *kind = json_string_property(arg, "kind");
    const char *type_kind = NULL;
    if (!kind || (strcmp(kind, "variable") != 0 &&
                  strcmp(kind, "member") != 0 &&
                  strcmp(kind, "array_access") != 0) ||
        !json_object_object_get_ex(arg, "type", &type) ||
        !(type_kind = json_string_property(type, "kind"))) return false;
    return strcmp(type_kind, "string") == 0 ||
        strcmp(type_kind, "array") == 0 || strcmp(type_kind, "struct") == 0;
}

static bool rust_validate_method_call(json_object *expr)
{
    json_object *is_static_obj = NULL, *struct_type = NULL, *result_type = NULL;
    json_object *args = NULL, *structure = NULL, *method = NULL;
    json_object *params = NULL, *method_return = NULL, *object = NULL;
    const char *method_name = json_string_property(expr, "method_name");

    if (!json_object_object_get_ex(expr, "is_static", &is_static_obj) ||
        !json_object_is_type(is_static_obj, json_type_boolean) ||
        !method_name || !json_object_object_get_ex(expr, "struct_type", &struct_type) ||
        !json_object_is_type(struct_type, json_type_object) ||
        !json_string_property_equals(struct_type, "kind", "struct") ||
        !json_object_object_get_ex(expr, "type", &result_type) ||
        !json_object_is_type(result_type, json_type_object) ||
        !json_object_object_get_ex(expr, "args", &args) ||
        !json_object_is_type(args, json_type_array))
        return rust_report_resolved_call_error(
            "encountered malformed resolved method_call model");

    bool is_static = json_object_get_boolean(is_static_obj);
    const char *struct_name = json_string_property(struct_type, "name");
    structure = rust_find_struct(rust_validation_model, struct_name);
    if (!structure)
        return rust_report_resolved_call_error(
            "encountered resolved method_call with an unknown struct receiver");
    if (json_boolean_property(structure, "is_native") ||
        json_boolean_property(struct_type, "is_native"))
        return rust_report_resolved_call_error(
            "does not support native resolved method_call receivers");
    if (json_boolean_property(structure, "pass_self_by_ref") ||
        json_boolean_property(struct_type, "pass_self_by_ref") ||
        !json_string_property_equals(structure, "mem_mode", "val"))
        return rust_report_resolved_call_error(
            "does not support reference-struct resolved method_call receivers");
    if (json_boolean_property(structure, "is_packed"))
        return rust_report_resolved_call_error(
            "does not support packed resolved method_call receivers");

    method = rust_find_resolved_method(structure, method_name, is_static);
    if (!method || json_boolean_property(method, "is_native") ||
        !json_object_object_get_ex(method, "params", &params) ||
        !json_object_is_type(params, json_type_array) ||
        !json_object_object_get_ex(method, "return_type", &method_return) ||
        !rust_resolved_types_equal(result_type, method_return) ||
        json_object_array_length(args) != json_object_array_length(params))
        return rust_report_resolved_call_error(
            "encountered incomplete or inconsistent resolved method_call metadata");

    if (json_boolean_property(expr, "source_arg_before_object") &&
        (is_static || json_object_array_length(args) != 1))
        return rust_report_resolved_call_error(
            "encountered inconsistent resolved method_call source-order metadata");

    if (json_boolean_property(expr, "source_arg_before_object"))
    {
        json_object *receiver_is_place = NULL;
        if (!json_object_object_get_ex(expr, "source_receiver_is_place",
                                       &receiver_is_place) ||
            !json_object_is_type(receiver_is_place, json_type_boolean))
            return rust_report_resolved_call_error(
                "encountered incomplete resolved method_call source-order metadata");
    }

    if (!rust_resolved_value_type_supported(result_type))
    {
        if (json_string_property_equals(result_type, "kind", "function"))
            return rust_report_resolved_call_error(
                "does not support closure-dependent resolved method_call results yet");
        return rust_report_resolved_call_error(
            "does not support this resolved method_call result representation");
    }

    if (is_static)
    {
        if (json_object_object_get_ex(expr, "object", &object))
            return rust_report_resolved_call_error(
                "encountered a static resolved method_call with an instance receiver");
    }
    else
    {
        json_object *object_type = NULL;
        if (!json_object_object_get_ex(expr, "object", &object) ||
            !json_object_is_type(object, json_type_object) ||
            !json_object_object_get_ex(object, "type", &object_type))
            return rust_report_resolved_call_error(
                "encountered an instance resolved method_call without a receiver");
        if (json_string_property_equals(object_type, "kind", "pointer"))
            return rust_report_resolved_call_error(
                "does not support pointer resolved method_call receivers");
        if (!rust_resolved_types_equal(object_type, struct_type))
            return rust_report_resolved_call_error(
                "encountered inconsistent resolved method_call receiver metadata");

    }

    size_t arg_count = json_object_array_length(args);
    for (size_t i = 0; i < arg_count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        json_object *param = json_object_array_get_idx(params, i);
        json_object *arg_type = NULL, *param_type = NULL;
        const char *mem_qual = json_string_property(param, "mem_qual");
        if (!json_object_is_type(arg, json_type_object) ||
            !json_object_object_get_ex(arg, "type", &arg_type) ||
            !json_object_object_get_ex(param, "type", &param_type) ||
            !rust_resolved_types_equal(arg_type, param_type))
            return rust_report_resolved_call_error(
                "encountered incomplete or inconsistent resolved method_call argument metadata");
        if (!rust_resolved_value_type_supported(arg_type))
        {
            if (json_string_property_equals(arg_type, "kind", "function"))
                return rust_report_resolved_call_error(
                    "does not support closure-dependent resolved method_call arguments yet");
            return rust_report_resolved_call_error(
                "does not support this resolved method_call argument representation");
        }

        bool passes_by_ref = mem_qual && strcmp(mem_qual, "as_ref") == 0;
        bool is_ref_arg = json_boolean_property(arg, "is_ref_arg");
        bool is_borrow_tmp = json_boolean_property(arg, "is_borrow_tmp");
        if (is_ref_arg == is_borrow_tmp && is_ref_arg)
            return rust_report_resolved_call_error(
                "encountered conflicting resolved method_call borrow metadata");
        if (passes_by_ref != (is_ref_arg || is_borrow_tmp))
            return rust_report_resolved_call_error(
                "encountered inconsistent resolved method_call borrow metadata");
        if (passes_by_ref && !json_boolean_property(arg, "is_borrow_tmp") &&
            !rust_resolved_stable_place(arg))
            return rust_report_resolved_call_error(
                "requires resolved as-ref method arguments to be stable mutable places");

        if (!passes_by_ref &&
            (json_boolean_property(arg, "is_copy_arg") ||
             json_boolean_property(arg, "source_is_borrow") ||
             rust_resolved_clone_source(arg)))
            json_object_object_add(arg, "rust_resolved_clone",
                                   json_object_new_boolean(true));

    }

    bool source_args_first =
        json_boolean_property(expr, "source_arg_before_object");
    if (!is_static && !source_args_first && !rust_validate_expr(object))
    {
        if (!rust_validation_reported_error)
            return rust_report_resolved_call_error(
                "encountered an unsupported resolved method_call receiver expression");
        return false;
    }
    for (size_t i = 0; i < arg_count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!rust_validate_expr(arg))
        {
            if (!rust_validation_reported_error)
                return rust_report_resolved_call_error(
                    "encountered an unsupported resolved method_call argument expression");
            return false;
        }
    }
    if (!is_static && source_args_first && !rust_validate_expr(object))
    {
        if (!rust_validation_reported_error)
            return rust_report_resolved_call_error(
                "encountered an unsupported resolved method_call receiver expression");
        return false;
    }
    return true;
}

static bool rust_validate_borrow_inferred_call(json_object *expr)
{
    json_object *type = NULL, *inner_call = NULL, *checks = NULL;
    const char *result_name = json_string_property(expr, "result_type_name");
    if (!result_name || !json_object_object_get_ex(expr, "type", &type) ||
        !json_object_is_type(type, json_type_object) ||
        !json_string_property_equals(type, "kind", "struct") ||
        !json_string_property_equals(type, "name", result_name) ||
        !json_object_object_get_ex(expr, "inner_call", &inner_call) ||
        !json_object_is_type(inner_call, json_type_object) ||
        !json_object_object_get_ex(expr, "borrow_check_args", &checks) ||
        !json_object_is_type(checks, json_type_array) ||
        json_object_array_length(checks) == 0)
        return rust_report_resolved_call_error(
            "encountered malformed borrow_inferred_call model");

    /* The shared model currently creates this wrapper exclusively for native
     * calls whose reference-struct result may alias a reference-struct input.
     * Neither representation is in this slice's Rust envelope; accepting the
     * inner call as an ordinary owned call would lose the retain decision. */
    return rust_report_resolved_call_error(
        "does not support native reference-struct borrow_inferred_call ownership yet");
}

static bool rust_validate_resolved_call(json_object *expr)
{
    if (json_string_property_equals(expr, "kind", "method_call"))
        return rust_validate_method_call(expr);
    if (json_string_property_equals(expr, "kind", "borrow_inferred_call"))
        return rust_validate_borrow_inferred_call(expr);
    return rust_report_resolved_call_error(
        "encountered malformed resolved call model");
}
