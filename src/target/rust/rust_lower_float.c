/* Run after call/capture and scalar-reference lowering. Annotating the existing expression
 * preserves its source type and all ownership/borrow metadata; only rendering
 * converts its value at a C-compatible floating storage or call boundary. */
static void rust_float_convert_value(json_object *value, json_object *target_type)
{
    json_object *source_type = NULL;
    if (!value || !json_object_object_get_ex(value, "type", &source_type)) return;
    const char *from = json_string_property(source_type, "kind");
    const char *c_kind = json_string_property(value, "rust_c_float_expression_kind");
    if (c_kind) from = c_kind;
    const char *to = json_string_property(target_type, "kind");
    const char *numeric_c_kind = json_string_property(value, "rust_c_numeric_expression_kind");
    /* Ordinary integer expressions also convert at storage boundaries. In
     * particular, byte + int retains its promoted width until assignment. */
    if (!numeric_c_kind && rust_fixed_integral_kind(from) &&
        rust_fixed_integral_kind(to)) numeric_c_kind = from;
    if (numeric_c_kind && rust_numeric_type_name(numeric_c_kind) && rust_numeric_type_name(to) &&
        strcmp(rust_numeric_type_name(numeric_c_kind), rust_numeric_type_name(to)) != 0)
    {
        if (json_string_property_equals(value, "kind", "literal") &&
            json_string_property_equals(value, "value_kind", "int"))
            json_object_object_add(value, "rust_c_numeric_literal_type",
                json_object_new_string(rust_numeric_type_name(numeric_c_kind)));
        json_object_object_add(value, "rust_c_numeric_conversion_type",
                               json_object_new_string(rust_numeric_type_name(to)));
        return;
    }
    /* Optimization can remove a floating identity such as i * 1.0 while
     * retaining the callee's floating parameter type. C converts the surviving
     * integer at that boundary; Rust needs the conversion written explicitly.
     * Char promotion retains its separate platform-aware lowering. */
    if (rust_numeric_integral_kind(from) && strcmp(from, "char") != 0 &&
        rust_numeric_floating_kind(to))
    {
        json_object_object_add(value, "rust_float_conversion_type",
                               json_object_new_string(rust_numeric_type_name(to)));
        return;
    }
    if (!rust_numeric_floating_kind(from) || !rust_numeric_floating_kind(to) ||
        strcmp(from, to) == 0) return;
    json_object_object_add(value, "rust_float_conversion_type",
                           json_object_new_string(rust_numeric_type_name(to)));
}

static void rust_float_convert_args(json_object *args, json_object *params,
                                    bool parameter_objects)
{
    if (!args || !params) return;
    size_t count = json_object_array_length(args);
    size_t param_count = json_object_array_length(params);
    for (size_t i = 0; i < count && i < param_count; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (json_boolean_property(arg, "is_ref_arg") ||
            json_boolean_property(arg, "is_borrow_tmp")) continue;
        json_object *type = json_object_array_get_idx(params, i);
        if (parameter_objects) json_object_object_get_ex(type, "type", &type);
        rust_float_convert_value(arg, type);
    }
}

static void rust_lower_float_conversions(json_object *model, json_object *node,
                                         json_object *return_type)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_lower_float_conversions(model, json_object_array_get_idx(node, i), return_type);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *body = NULL, *own_return = NULL;
    bool has_body = json_object_object_get_ex(node, "body", &body) ||
                    json_object_object_get_ex(node, "body_stmts", &body);
    if (has_body && json_object_object_get_ex(node, "return_type", &own_return))
        return_type = own_return;
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        rust_lower_float_conversions(model, child, return_type);
    }
    if (has_body && own_return) rust_float_convert_value(body, own_return);

    const char *kind = json_string_property(node, "kind");
    json_object *type = NULL, *value = NULL;
    json_object_object_get_ex(node, "type", &type);
    if (kind && strcmp(kind, "assign") == 0 &&
        (rust_numeric_type_name(json_string_property(type, "kind")) ||
         json_string_property_equals(type, "kind", "bool")))
        json_object_object_add(node, "rust_scalar_assignment_value",
                               json_object_new_boolean(true));
    if (kind && strcmp(kind, "member_assign") == 0 &&
        (rust_numeric_type_name(json_string_property(type, "kind")) ||
         json_string_property_equals(type, "kind", "bool")))
        json_object_object_add(node, "rust_field_assignment_value", json_object_new_boolean(true));
    /* A discarded statement needs no final read of its stored scalar. Nested
     * assignments retain their values for the enclosing expression. */
    if (kind && strcmp(kind, "expr") == 0)
    {
        json_object *expression = NULL;
        json_object_object_get_ex(node, "expr", &expression);
        if (json_string_property_equals(expression, "kind", "assign"))
            json_object_object_del(expression, "rust_scalar_assignment_value");
        if (json_string_property_equals(expression, "kind", "member_assign"))
            json_object_object_del(expression, "rust_field_assignment_value");
    }
    if (kind && strcmp(kind, "return") == 0)
    {
        json_object_object_get_ex(node, "value", &value);
        rust_float_convert_value(value, return_type);
    }
    else if (kind && strcmp(kind, "compound_assign") == 0)
    {
        json_object *target = NULL, *target_type = NULL, *value_type = NULL;
        json_object_object_get_ex(node, "target", &target);
        json_object_object_get_ex(target, "type", &target_type);
        json_object_object_get_ex(node, "value", &value);
        json_object_object_get_ex(value, "type", &value_type);
        const char *target_kind = json_string_property(target_type, "kind");
        const char *value_kind = json_string_property(value, "rust_c_float_expression_kind");
        if (!value_kind) value_kind = json_string_property(value_type, "kind");
        if (rust_numeric_floating_kind(target_kind) && rust_numeric_floating_kind(value_kind) &&
            strcmp(target_kind, value_kind) != 0)
        {
            /* C computes the mixed expression in double, then converts back
             * to the place's width. Do not round the RHS before arithmetic. */
            json_object_object_add(node, "rust_float_compound_type", json_object_new_string("f64"));
            json_object_object_add(node, "rust_float_compound_storage_type",
                json_object_new_string(rust_numeric_type_name(json_string_property(target_type, "kind"))));
        }
    }
    else if (kind && (strcmp(kind, "assign") == 0 ||
                     strcmp(kind, "member_assign") == 0 ||
                     strcmp(kind, "index_assign") == 0))
    {
        json_object_object_get_ex(node, "value", &value);
        rust_float_convert_value(value, type);
    }
    else if (kind && strcmp(kind, "array_literal") == 0)
    {
        json_object *elements = NULL, *element_type = NULL;
        json_object_object_get_ex(node, "elements", &elements);
        json_object_object_get_ex(type, "element_type", &element_type);
        if (elements)
            for (size_t i = 0; i < json_object_array_length(elements); i++)
                rust_float_convert_value(json_object_array_get_idx(elements, i), element_type);
    }
    else if (kind && strcmp(kind, "struct_literal") == 0)
    {
        json_object *fields = NULL, *declared_fields = NULL;
        json_object_object_get_ex(node, "fields", &fields);
        json_object_object_get_ex(type, "fields", &declared_fields);
        if (fields && declared_fields)
            for (size_t i = 0; i < json_object_array_length(fields); i++)
            {
                json_object *field = json_object_array_get_idx(fields, i);
                const char *name = json_string_property(field, "name");
                for (size_t j = 0; j < json_object_array_length(declared_fields); j++)
                {
                    json_object *declared = json_object_array_get_idx(declared_fields, j);
                    if (!name || !json_string_property_equals(declared, "name", name)) continue;
                    json_object_object_get_ex(field, "value", &value);
                    json_object_object_get_ex(declared, "type", &type);
                    rust_float_convert_value(value, type);
                    break;
                }
            }
    }
    else if (kind && strcmp(kind, "call") == 0)
    {
        if (json_boolean_property(node, "rust_float_array_search") ||
            json_boolean_property(node, "rust_float_array_storage")) return;
        json_object *callee = NULL, *callee_type = NULL, *args = NULL, *params = NULL;
        json_object_object_get_ex(node, "callee", &callee);
        json_object_object_get_ex(callee, "type", &callee_type);
        json_object_object_get_ex(callee_type, "param_types", &params);
        json_object_object_get_ex(node, "args", &args);
        json_object *object = NULL, *object_type = NULL;
        json_object_object_get_ex(callee, "object", &object);
        json_object_object_get_ex(object, "type", &object_type);
        if (json_string_property_equals(callee, "kind", "member") &&
            json_string_property_equals(callee, "member_name", "insert") &&
            json_string_property_equals(object_type, "kind", "array") &&
            json_object_array_length(args) == 2 && json_object_array_length(params) == 2)
        {
            /* Array lowering puts the index before the element. The shared
             * callable type retains the source element/index parameter order. */
            rust_float_convert_value(json_object_array_get_idx(args, 0),
                                     json_object_array_get_idx(params, 1));
            rust_float_convert_value(json_object_array_get_idx(args, 1),
                                     json_object_array_get_idx(params, 0));
        }
        else rust_float_convert_args(args, params, false);
    }
    else if (kind && strcmp(kind, "var_decl") == 0)
    {
        json_object_object_get_ex(node, "initializer", &value);
        if (json_string_property(value, "rust_c_float_expression_kind") ||
            json_string_property(value, "rust_c_numeric_expression_kind"))
            rust_float_convert_value(value, type);
    }
    else if (kind && (strcmp(kind, "static_call") == 0 || strcmp(kind, "method_call") == 0))
    {
        json_object *structure_type = NULL, *args = NULL, *params = NULL;
        json_object_object_get_ex(node, "struct_type", &structure_type);
        const char *name = json_string_property(structure_type, "name");
        if (!name) name = json_string_property(node, "type_name");
        json_object *structure = rust_find_struct(model, name);
        bool is_static = strcmp(kind, "static_call") == 0 || json_boolean_property(node, "is_static");
        json_object *method = rust_find_resolved_method(
            structure, json_string_property(node, "method_name"), is_static);
        json_object_object_get_ex(method, "params", &params);
        json_object_object_get_ex(node, "args", &args);
        rust_float_convert_args(args, params, true);
    }
    /* Struct declaration default fields have no expression kind. */
    if (!kind && type && json_object_object_get_ex(node, "default_value", &value))
        rust_float_convert_value(value, type);
}
