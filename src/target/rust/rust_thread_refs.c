/* Scalar reference parameters called by threads use shared owners, never an
 * exclusive Rust borrow moved across spawn. This also preserves aliased args. */
typedef struct RustThreadRefBinding {
    const char *name;
    json_object *declaration;
    struct RustThreadRefBinding *next;
} RustThreadRefBinding;

static void rust_thread_ref_find_targets(json_object *node, json_object *functions)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_thread_ref_find_targets(json_object_array_get_idx(node, i), functions);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "thread_spawn")) {
        json_object *call = NULL, *callee = NULL;
        json_object_object_get_ex(node, "call", &call);
        if (call && !json_boolean_property(call, "is_closure_call"))
            json_object_object_get_ex(call, "callee", &callee);
        const char *name = json_string_property(callee, "name");
        for (size_t i = 0; name && i < json_object_array_length(functions); i++) {
            json_object *fn = json_object_array_get_idx(functions, i), *params = NULL;
            if (!json_string_property_equals(fn, "name", name)) continue;
            json_object_object_get_ex(fn, "params", &params);
            for (size_t j = 0; params && j < json_object_array_length(params); j++) {
                json_object *param = json_object_array_get_idx(params, j), *type = NULL;
                json_object_object_get_ex(param, "type", &type);
                if (json_string_property_equals(param, "mem_qual", "as_ref") &&
                    rust_scalar_ref_parameter_type_supported(type)) {
                    json_object_object_add(param, "rust_thread_ref_param", json_object_new_boolean(true));
                    json_object_object_add(param, "rust_shared_cell", json_object_new_boolean(true));
                }
            }
        }
    }
    json_object_object_foreach(node, key, value) {
        (void)key; rust_thread_ref_find_targets(value, functions);
    }
}

static bool rust_thread_ref_walk(json_object *node, json_object *functions,
                                 RustThreadRefBinding *scope, bool *changed)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        RustThreadRefBinding *locals = scope;
        bool ok = true;
        for (size_t i = 0; ok && i < json_object_array_length(node); i++) {
            json_object *child = json_object_array_get_idx(node, i);
            ok = rust_thread_ref_walk(child, functions, locals, changed);
            if (json_string_property_equals(child, "kind", "var_decl")) {
                RustThreadRefBinding *binding = malloc(sizeof(*binding));
                if (!binding) { ok = false; break; }
                *binding = (RustThreadRefBinding){json_string_property(child, "name"), child, locals};
                locals = binding;
            }
        }
        while (locals != scope) { RustThreadRefBinding *next = locals->next; free(locals); locals = next; }
        return ok;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_string_property_equals(node, "kind", "call") &&
        !json_boolean_property(node, "is_closure_call")) {
        json_object *callee = NULL, *args = NULL;
        json_object_object_get_ex(node, "callee", &callee);
        json_object_object_get_ex(node, "args", &args);
        const char *name = json_string_property(callee, "name");
        for (size_t f = 0; name && f < json_object_array_length(functions); f++) {
            json_object *fn = json_object_array_get_idx(functions, f), *params = NULL;
            if (!json_string_property_equals(fn, "name", name)) continue;
            json_object_object_get_ex(fn, "params", &params);
            for (size_t i = 0; args && params && i < json_object_array_length(params) && i < json_object_array_length(args); i++) {
                json_object *param = json_object_array_get_idx(params, i);
                json_object *arg = json_object_array_get_idx(args, i);
                const char *arg_name = json_string_property(arg, "name");
                RustThreadRefBinding *binding = scope;
                while (binding && (!arg_name || strcmp(arg_name, binding->name))) binding = binding->next;
                json_object *param_type = NULL;
                json_object_object_get_ex(param, "type", &param_type);
                if (binding && json_boolean_property(binding->declaration, "rust_shared_cell") &&
                    json_string_property_equals(param, "mem_qual", "as_ref") &&
                    rust_scalar_ref_parameter_type_supported(param_type) &&
                    !json_boolean_property(param, "rust_thread_ref_param")) {
                    json_object_object_add(param, "rust_thread_ref_param", json_object_new_boolean(true));
                    json_object_object_add(param, "rust_shared_cell", json_object_new_boolean(true));
                    *changed = true;
                }
                if (!json_boolean_property(param, "rust_thread_ref_param")) continue;
                if (!binding || !json_string_property_equals(arg, "kind", "variable")) {
                    fprintf(stderr, "Error: Rust thread reference lowering still requires an owned scalar variable place; aggregate/global reference projections remain unimplemented\n");
                    return false;
                }
                json_object *decl = binding->declaration;
                json_object_object_add(decl, "rust_thread_ref_storage", json_object_new_boolean(true));
                if (!json_boolean_property(decl, "rust_shared_cell")) {
                    json_object_object_add(decl, "rust_shared_cell", json_object_new_boolean(true));
                    *changed = true;
                }
                if (!json_string_property_equals(decl, "kind", "var_decl")) {
                    const char *flag = json_string_property_equals(decl, "mem_qual", "as_ref") ? "rust_thread_ref_param" : "rust_thread_ref_local";
                    if (!json_boolean_property(decl, flag)) {
                        json_object_object_add(decl, flag, json_object_new_boolean(true)); *changed = true;
                    }
                }
                json_object_object_add(arg, "rust_thread_ref_owner", json_object_new_boolean(true));
            }
        }
    }
    json_object_object_foreach(node, key, value) {
        (void)key; if (!rust_thread_ref_walk(value, functions, scope, changed)) return false;
    }
    return true;
}

static bool rust_prepare_thread_references(json_object *model)
{
    if (array_is_empty(model, "threads")) return true;
    json_object *functions = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    rust_thread_ref_find_targets(model, functions);
    bool changed;
    do {
        changed = false;
        for (size_t i = 0; functions && i < json_object_array_length(functions); i++) {
            json_object *fn = json_object_array_get_idx(functions, i), *params = NULL, *body = NULL;
            json_object_object_get_ex(fn, "params", &params);
            json_object_object_get_ex(fn, "body", &body);
            RustThreadRefBinding *scope = NULL;
            for (size_t p = 0; params && p < json_object_array_length(params); p++) {
                json_object *param = json_object_array_get_idx(params, p);
                RustThreadRefBinding *binding = malloc(sizeof(*binding));
                if (!binding) return false;
                *binding = (RustThreadRefBinding){json_string_property(param, "name"), param, scope}; scope = binding;
            }
            bool ok = rust_thread_ref_walk(body, functions, scope, &changed);
            while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
            if (!ok) return false;
        }
    } while (changed);
    return true;
}
