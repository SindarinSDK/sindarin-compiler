/* C performs vararg promotion using its actual source types and target ABI.
 * Rust calls an ordinary, fully typed native bridge for each resolved shape. */
static json_object *native_variadic_declaration(json_object *functions,
                                               json_object *callee)
{
    if (!native_string(callee, "kind") ||
        strcmp(native_string(callee, "kind"), "variable") != 0) return NULL;
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (native_bool(function, "is_native") &&
            native_bool(function, "is_variadic") &&
            function_matches_callable(function, native_string(callee, "name")))
            return function;
    }
    return NULL;
}

static json_object *native_variadic_parameter(json_object *fixed, json_object *arg,
                                              size_t index)
{
    json_object *parameter = fixed ? deep_copy(fixed) : json_object_new_object();
    if (!parameter) return NULL;
    char name[64];
    snprintf(name, sizeof(name), "__sn_va_arg_%zu", index);
    json_object_object_add(parameter, "name", json_object_new_string(name));
    /* An as-val argument is transferred to the real callee. The adapter must
     * not also run the source function's parameter cleanup after forwarding. */
    if (fixed) json_object_object_del(parameter, "needs_struct_cleanup");
    if (!fixed)
    {
        json_object *type = NULL;
        json_object_object_get_ex(arg, "type", &type);
        json_object *copy = deep_copy(type);
        if (!copy) { json_object_put(parameter); return NULL; }
        json_object_object_add(parameter, "type", copy);
        json_object_object_add(parameter, "mem_qual", json_object_new_string("default"));
        json_object_object_add(parameter, "sync_mod", json_object_new_string("none"));
    }
    return parameter;
}

static json_object *native_variadic_bridge(json_object *function, json_object *params,
                                           const char *name, const char *source_file)
{
    json_object *bridge = json_object_new_object();
    json_object *return_type = NULL;
    json_object_object_get_ex(function, "return_type", &return_type);
    json_object_object_add(bridge, "name", json_object_new_string(name));
    json_object_object_add(bridge, "source_file", json_object_new_string(source_file));
    json_object_object_add(bridge, "return_type", deep_copy(return_type));
    json_object_object_add(bridge, "modifier", json_object_new_string("default"));
    json_object_object_add(bridge, "is_native", json_object_new_boolean(true));
    json_object_object_add(bridge, "has_body", json_object_new_boolean(true));
    json_object_object_add(bridge, "is_variadic", json_object_new_boolean(false));
    json_object_object_add(bridge, "has_arena_param", json_object_new_boolean(false));
    json_object_object_add(bridge, "params", json_object_get(params));

    json_object *callee_type = json_object_new_object();
    json_object_object_add(callee_type, "kind", json_object_new_string("function"));
    json_object_object_add(callee_type, "return_type", deep_copy(return_type));
    json_object_object_add(callee_type, "is_native", json_object_new_boolean(true));
    json_object_object_add(callee_type, "is_variadic", json_object_new_boolean(true));
    json_object *fixed = NULL, *fixed_types = json_object_new_array();
    json_object_object_get_ex(function, "params", &fixed);
    for (size_t i = 0; fixed && i < json_object_array_length(fixed); i++)
    {
        json_object *type = NULL;
        json_object_object_get_ex(json_object_array_get_idx(fixed, i), "type", &type);
        json_object_array_add(fixed_types, deep_copy(type));
    }
    json_object_object_add(callee_type, "param_types", fixed_types);
    json_object *callee = json_object_new_object();
    json_object_object_add(callee, "kind", json_object_new_string("variable"));
    json_object_object_add(callee, "name", json_object_new_string(native_string(function, "name")));
    json_object_object_add(callee, "type", callee_type);
    const char *alias = native_string(function, "c_alias");
    if (alias)
    {
        json_object_object_add(callee, "has_c_alias", json_object_new_boolean(true));
        json_object_object_add(callee, "c_alias", json_object_new_string(alias));
    }
    json_object *args = json_object_new_array();
    for (size_t i = 0; i < json_object_array_length(params); i++)
    {
        json_object *parameter = json_object_array_get_idx(params, i), *type = NULL;
        json_object_object_get_ex(parameter, "type", &type);
        json_object *arg = json_object_new_object();
        json_object_object_add(arg, "kind", json_object_new_string("variable"));
        json_object_object_add(arg, "name", json_object_new_string(native_string(parameter, "name")));
        /* A fixed as-ref bridge parameter is already a C pointer. Passing the
         * parameter itself preserves its address and any duplicate alias. */
        if (native_string(parameter, "mem_qual") &&
            strcmp(native_string(parameter, "mem_qual"), "as_ref") == 0)
        {
            json_object *pointer = json_object_new_object();
            json_object_object_add(pointer, "kind", json_object_new_string("pointer"));
            json_object_object_add(pointer, "base_type", deep_copy(type));
            json_object_object_add(arg, "type", pointer);
        }
        else json_object_object_add(arg, "type", deep_copy(type));
        json_object_array_add(args, arg);
    }
    json_object *call = json_object_new_object();
    json_object_object_add(call, "kind", json_object_new_string("call"));
    json_object_object_add(call, "type", deep_copy(return_type));
    json_object_object_add(call, "callee", callee);
    json_object_object_add(call, "args", args);
    json_object_object_add(call, "is_closure_call", json_object_new_boolean(false));
    json_object *statement = json_object_new_object();
    bool is_void = native_string(return_type, "kind") &&
                   strcmp(native_string(return_type, "kind"), "void") == 0;
    json_object_object_add(statement, "kind", json_object_new_string(is_void ? "expr" : "return"));
    json_object_object_add(statement, is_void ? "expr" : "value", call);
    json_object *body = json_object_new_array();
    json_object_array_add(body, statement);
    json_object_object_add(bridge, "body", body);
    return bridge;
}

static bool native_variadic_lower_node(json_object *node, json_object *model,
                                       json_object *private_model, json_object *shapes,
                                       const char *source_file)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        /* Generated bridges are appended only after walking the source roots. */
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (!native_variadic_lower_node(json_object_array_get_idx(node, i),
                    model, private_model, shapes, source_file)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    /* Native bodies belong to the original C projection. Their variadic calls
     * require neither Rust adapters nor changes to the C source graph. */
    if (native_bool(node, "is_native") &&
        (native_bool(node, "has_body") ||
         (native_string(node, "kind") && strcmp(native_string(node, "kind"), "lambda") == 0))) return true;
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0 &&
            !native_variadic_lower_node(value, model, private_model, shapes, source_file)) return false;
    }
    if (!native_string(node, "kind") || strcmp(native_string(node, "kind"), "call") != 0 ||
        native_bool(node, "is_closure_call")) return true;
    json_object *callee = NULL, *type = NULL, *args = NULL, *functions = NULL;
    json_object_object_get_ex(node, "callee", &callee);
    json_object_object_get_ex(callee, "type", &type);
    if (!native_bool(type, "is_native") || !native_bool(type, "is_variadic")) return true;
    json_object_object_get_ex(model, "functions", &functions);
    json_object *function = native_variadic_declaration(functions, callee);
    if (!function)
    {
        fprintf(stderr, "Error: Rust target cannot resolve a native variadic declaration\n");
        return false;
    }
    json_object_object_get_ex(node, "args", &args);
    json_object *fixed = NULL;
    json_object_object_get_ex(function, "params", &fixed);
    size_t count = args ? json_object_array_length(args) : 0;
    size_t fixed_count = fixed ? json_object_array_length(fixed) : 0;
    if (count < fixed_count) return false;
    json_object *params = json_object_new_array();
    for (size_t i = 0; i < count; i++)
    {
        json_object *parameter = native_variadic_parameter(
            i < fixed_count ? json_object_array_get_idx(fixed, i) : NULL,
            json_object_array_get_idx(args, i), i);
        if (!parameter) { json_object_put(params); return false; }
        json_object_array_add(params, parameter);
    }
    json_object *bridge = NULL;
    for (size_t i = 0; i < json_object_array_length(shapes); i++)
    {
        json_object *shape = json_object_array_get_idx(shapes, i), *prior_params = NULL, *prior_bridge = NULL;
        const char *target = native_string(shape, "target");
        json_object_object_get_ex(shape, "params", &prior_params);
        json_object_object_get_ex(shape, "bridge", &prior_bridge);
        if (target && strcmp(target, native_string(function, "name")) == 0 &&
            json_object_equal(prior_params, params)) { bridge = prior_bridge; break; }
    }
    if (!bridge)
    {
        json_object *structs = NULL, *globals = NULL;
        json_object_object_get_ex(model, "structs", &structs);
        json_object_object_get_ex(model, "globals", &globals);
        char *name = unique_private_name(native_string(model, "package_native_namespace"), functions, structs, globals, "__sn_native_variadic");
        if (!name) { json_object_put(params); return false; }
        bridge = native_variadic_bridge(function, params, name, source_file);
        free(name);
        json_object *shape = json_object_new_object();
        json_object_object_add(shape, "target", json_object_new_string(native_string(function, "name")));
        json_object_object_add(shape, "params", json_object_get(params));
        json_object_object_add(shape, "bridge", json_object_get(bridge));
        json_object_array_add(shapes, shape);
        /* Publish the name for hygiene, but never traverse this new C body. */
        json_object_array_add(functions, bridge);
        json_object *c_functions = NULL;
        json_object_object_get_ex(private_model, "functions", &c_functions);
        json_object_array_add(c_functions, deep_copy(bridge));
    }
    json_object_put(params);
    json_object *new_callee = json_object_new_object();
    json_object_object_add(new_callee, "kind", json_object_new_string("variable"));
    json_object_object_add(new_callee, "name", json_object_new_string(native_string(bridge, "name")));
    json_object *new_type = json_object_new_object(), *return_type = NULL, *param_types = json_object_new_array(), *bridge_params = NULL;
    json_object_object_get_ex(bridge, "return_type", &return_type);
    json_object_object_get_ex(bridge, "params", &bridge_params);
    json_object_object_add(new_type, "kind", json_object_new_string("function"));
    json_object_object_add(new_type, "return_type", deep_copy(return_type));
    json_object_object_add(new_type, "is_native", json_object_new_boolean(true));
    json_object_object_add(new_type, "is_variadic", json_object_new_boolean(false));
    for (size_t i = 0; i < json_object_array_length(bridge_params); i++)
    {
        json_object *param_type = NULL;
        json_object_object_get_ex(json_object_array_get_idx(bridge_params, i), "type", &param_type);
        json_object_array_add(param_types, deep_copy(param_type));
    }
    json_object_object_add(new_type, "param_types", param_types);
    json_object_object_add(new_callee, "type", new_type);
    json_object_object_add(node, "callee", new_callee);
    return true;
}

static bool native_lower_variadic_calls(json_object *model, json_object *private_model,
                                        const char *source_file)
{
    json_object *shapes = json_object_new_array();
    bool ok = native_variadic_lower_node(model, model, private_model, shapes, source_file);
    json_object_put(shapes);
    if (!ok) return false;
    json_object *functions = NULL, *remaining = json_object_new_array();
    json_object_object_get_ex(model, "functions", &functions);
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (native_bool(function, "is_native") && native_bool(function, "is_variadic") &&
            !rust_roots_reference_callable(model, function)) continue;
        json_object_array_add(remaining, json_object_get(function));
    }
    json_object_object_add(model, "functions", remaining);
    return true;
}
