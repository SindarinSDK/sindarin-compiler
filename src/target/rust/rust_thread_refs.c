/* Scalar reference parameters called by threads use shared owners, never an
 * exclusive Rust borrow moved across spawn. This also preserves aliased args. */
typedef struct RustThreadRefBinding {
    const char *name;
    json_object *declaration;
    struct RustThreadRefBinding *next;
} RustThreadRefBinding;

/* Reuse the lexical owner graph for default arrays escaping through threads.
 * Array owners retain buffer identity; scalar owners retain variable storage. */
static bool rust_thread_array_parameter(json_object *param)
{
    json_object *type = NULL;
    json_object_object_get_ex(param, "type", &type);
    return json_string_property_equals(type, "kind", "array") &&
        json_boolean_property(param, "rust_default_array_ref");
}

static void rust_thread_array_promote(json_object *decl, bool *changed)
{
    if (!json_boolean_property(decl, "rust_thread_array_storage")) *changed = true;
    json_object_object_add(decl, "rust_thread_array_storage", json_object_new_boolean(true));
    json_object_object_add(decl, "rust_shared_cell", json_object_new_boolean(true));
    if (!json_string_property_equals(decl, "kind", "var_decl") &&
        !json_boolean_property(decl, "rust_thread_global_binding")) {
        json_object_object_add(decl, "rust_thread_ref_param", json_object_new_boolean(true));
        json_object_object_add(decl, "rust_thread_array_param", json_object_new_boolean(true));
    }
}

static RustThreadRefBinding *rust_thread_ref_lookup(RustThreadRefBinding *scope, const char *name)
{
    while (scope && (!name || strcmp(name, scope->name))) scope = scope->next;
    return scope;
}

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
                if (rust_thread_array_parameter(param)) {
                    bool changed = false;
                    rust_thread_array_promote(param, &changed);
                }
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

/* Resolve methods by their owning type, so array owners crossing a method
 * boundary participate in the same fixed-point transport graph as free calls. */
static json_object *rust_thread_ref_target(json_object *model, json_object *functions,
                                           json_object *call)
{
    json_object *callee = NULL, *type = NULL, *object = NULL;
    const char *name = NULL, *structure_name = NULL;
    bool is_static = json_string_property_equals(call, "kind", "static_call") ||
        json_boolean_property(call, "is_static");
    if (json_string_property_equals(call, "kind", "call")) {
        json_object_object_get_ex(call, "callee", &callee);
        if (json_string_property_equals(callee, "kind", "variable"))
            return rust_default_array_function(functions, json_string_property(callee, "name"));
        if (!json_string_property_equals(callee, "kind", "member")) return NULL;
        json_object_object_get_ex(callee, "object", &object);
        json_object_object_get_ex(object, "type", &type);
        name = json_string_property(callee, "member_name");
    } else {
        json_object_object_get_ex(call, "struct_type", &type);
        structure_name = json_string_property(call, "type_name");
        name = json_string_property(call, "method_name");
    }
    if (!structure_name) structure_name = json_string_property(type, "name");
    json_object *target = rust_find_resolved_method(rust_find_struct(model, structure_name), name, is_static);
    return target && !json_boolean_property(target, "is_native") ? target : NULL;
}

static bool rust_thread_ref_walk(json_object *node, json_object *functions,
                                 json_object *model, RustThreadRefBinding *scope, bool *changed)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        RustThreadRefBinding *locals = scope;
        bool ok = true;
        for (size_t i = 0; ok && i < json_object_array_length(node); i++) {
            json_object *child = json_object_array_get_idx(node, i);
            ok = rust_thread_ref_walk(child, functions, model, locals, changed);
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
    /* Tagged local array initialization/assignment copies the value. Only
     * parameter transport shares the caller's selected array owner. */
    if (json_string_property_equals(node, "kind", "assign")) {
        RustThreadRefBinding *target = rust_thread_ref_lookup(scope, json_string_property(node, "target"));
        if (target && json_boolean_property(target->declaration, "rust_thread_array_storage"))
            json_object_object_add(node, json_boolean_property(target->declaration, "rust_thread_global_binding")
                ? "rust_thread_array_global" : "rust_thread_array_storage", json_object_new_boolean(true));
    }
    if (json_string_property_equals(node, "kind", "variable")) {
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(node, "name"));
        if (binding && json_boolean_property(binding->declaration, "rust_thread_array_storage")) {
            json_object_object_add(node, "rust_thread_array_read", json_object_new_boolean(true));
            if (json_boolean_property(binding->declaration, "rust_thread_global_binding"))
                json_object_object_add(node, "rust_thread_array_global", json_object_new_boolean(true));
            json_object_object_del(node, "rust_deref");
        }
    }
    if ((json_string_property_equals(node, "kind", "call") ||
         json_string_property_equals(node, "kind", "method_call") ||
         json_string_property_equals(node, "kind", "static_call")) &&
        !json_boolean_property(node, "is_closure_call")) {
        json_object *args = NULL;
        json_object_object_get_ex(node, "args", &args);
        json_object *fn = rust_thread_ref_target(model, functions, node), *params = NULL;
        if (fn) {
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
                if (binding && json_boolean_property(binding->declaration, "rust_thread_array_storage") &&
                    rust_thread_array_parameter(param)) rust_thread_array_promote(param, changed);
                if (json_boolean_property(param, "rust_thread_array_param")) {
                    if (binding && json_string_property_equals(arg, "kind", "variable")) {
                        rust_thread_array_promote(binding->declaration, changed);
                        json_object_object_add(arg, "rust_thread_ref_owner", json_object_new_boolean(true));
                    } else {
                        json_object_object_add(arg, "rust_thread_array_temporary", json_object_new_boolean(true));
                        /* A copied input from a field/index still belongs to
                         * its source owner. Build the temporary from a value
                         * snapshot, never move the source's array out. */
                        if (json_boolean_property(param, "needs_array_copy") &&
                            (json_string_property_equals(arg, "kind", "member") ||
                             json_string_property_equals(arg, "kind", "array_access")))
                        {
                            json_object_object_add(arg, "rust_needs_clone", json_object_new_boolean(true));
                            json_object_object_add(arg, "rust_thread_array_copy_borrow", json_object_new_boolean(true));
                        }
                    }
                    json_object_object_del(arg, "rust_default_array_ref_arg");
                    json_object_object_del(arg, "rust_thread_default_array_arg");
                    json_object_object_del(arg, "is_ref_arg");
                    continue;
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
        (void)key; if (!rust_thread_ref_walk(value, functions, model, scope, changed)) return false;
    }
    return true;
}

/* Repeated scalar reference arguments need the same owner contract even when
 * the call is synchronous. The existing graph propagates that contract through
 * forwarding functions and chooses Rc<Cell<T>> outside threaded programs. */
static bool rust_find_scalar_reference_aliases(json_object *node,
                                              json_object *functions)
{
    if (!node) return false;
    bool found = false;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            found |= rust_find_scalar_reference_aliases(
                json_object_array_get_idx(node, i), functions);
        return found;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    if (json_string_property_equals(node, "kind", "call") &&
        !json_boolean_property(node, "is_closure_call"))
    {
        json_object *callee = NULL, *args = NULL;
        json_object_object_get_ex(node, "callee", &callee);
        json_object_object_get_ex(node, "args", &args);
        const char *name = json_string_property(callee, "name");
        for (size_t f = 0; name && f < json_object_array_length(functions); f++)
        {
            json_object *fn = json_object_array_get_idx(functions, f), *params = NULL;
            if (!json_string_property_equals(fn, "name", name) ||
                json_boolean_property(fn, "is_native") ||
                json_boolean_property(fn, "rust_native_bridge")) continue;
            json_object_object_get_ex(fn, "params", &params);
            size_t count = json_object_array_length(args);
            if (count > json_object_array_length(params)) count = json_object_array_length(params);
            for (size_t i = 0; i < count; i++)
            {
                json_object *arg = json_object_array_get_idx(args, i);
                json_object *param = json_object_array_get_idx(params, i), *type = NULL;
                json_object_object_get_ex(param, "type", &type);
                if (!json_string_property_equals(arg, "kind", "variable") ||
                    !json_string_property_equals(param, "mem_qual", "as_ref") ||
                    !rust_scalar_ref_parameter_type_supported(type)) continue;
                const char *arg_name = json_string_property(arg, "name");
                for (size_t j = i + 1; arg_name && j < count; j++)
                {
                    json_object *other = json_object_array_get_idx(args, j);
                    json_object *other_param = json_object_array_get_idx(params, j), *other_type = NULL;
                    json_object_object_get_ex(other_param, "type", &other_type);
                    if (!json_string_property_equals(other, "kind", "variable") ||
                        !json_string_property_equals(other, "name", arg_name) ||
                        !json_string_property_equals(other_param, "mem_qual", "as_ref") ||
                        !rust_scalar_ref_parameter_type_supported(other_type)) continue;
                    json_object_object_add(param, "rust_thread_ref_param", json_object_new_boolean(true));
                    json_object_object_add(param, "rust_shared_cell", json_object_new_boolean(true));
                    json_object_object_add(other_param, "rust_thread_ref_param", json_object_new_boolean(true));
                    json_object_object_add(other_param, "rust_shared_cell", json_object_new_boolean(true));
                    found = true;
                }
            }
        }
    }
    json_object_object_foreach(node, key, child)
    {
        (void)key;
        found |= rust_find_scalar_reference_aliases(child, functions);
    }
    return found;
}

static bool rust_prepare_thread_references(json_object *model)
{
    json_object *functions = NULL, *globals = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    json_object_object_get_ex(model, "globals", &globals);
    bool aliases = rust_find_scalar_reference_aliases(model, functions);
    if (array_is_empty(model, "threads") && !aliases) return true;
    rust_thread_ref_find_targets(model, functions);
    json_object *callables = json_object_new_array(), *structures = NULL;
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
        json_object_array_add(callables, json_object_get(json_object_array_get_idx(functions, i)));
    json_object_object_get_ex(model, "structs", &structures);
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++) {
        json_object *methods = NULL;
        json_object_object_get_ex(json_object_array_get_idx(structures, i), "methods", &methods);
        for (size_t j = 0; methods && j < json_object_array_length(methods); j++)
            json_object_array_add(callables, json_object_get(json_object_array_get_idx(methods, j)));
    }
    bool changed;
    do {
        changed = false;
        for (size_t i = 0; i < json_object_array_length(callables); i++) {
            json_object *fn = json_object_array_get_idx(callables, i), *params = NULL, *body = NULL;
            json_object_object_get_ex(fn, "params", &params);
            json_object_object_get_ex(fn, "body", &body);
            RustThreadRefBinding *scope = NULL;
            /* Globals are outer lexical bindings. Parameters and subsequent
             * locals shadow them, including namespaced global declarations. */
            for (size_t g = 0; globals && g < json_object_array_length(globals); g++) {
                json_object *global = json_object_array_get_idx(globals, g);
                json_object_object_add(global, "rust_thread_global_binding", json_object_new_boolean(true));
                RustThreadRefBinding *binding = malloc(sizeof(*binding));
                if (!binding) {
                    while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
                    json_object_put(callables);
                    return false;
                }
                *binding = (RustThreadRefBinding){json_string_property(global, "name"), global, scope};
                scope = binding;
            }
            /* Initializers are evaluated in global scope, never in the
             * parameter/local scope of the function being traversed. */
            for (size_t g = 0; globals && g < json_object_array_length(globals); g++) {
                json_object *initializer = NULL;
                json_object_object_get_ex(json_object_array_get_idx(globals, g), "initializer", &initializer);
                if (!rust_thread_ref_walk(initializer, functions, model, scope, &changed)) {
                    while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
                    json_object_put(callables);
                    return false;
                }
            }
            for (size_t p = 0; params && p < json_object_array_length(params); p++) {
                json_object *param = json_object_array_get_idx(params, p);
                RustThreadRefBinding *binding = malloc(sizeof(*binding));
                if (!binding) {
                    while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
                    json_object_put(callables);
                    return false;
                }
                *binding = (RustThreadRefBinding){json_string_property(param, "name"), param, scope}; scope = binding;
            }
            bool ok = rust_thread_ref_walk(body, functions, model, scope, &changed);
            while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
            if (!ok) { json_object_put(callables); return false; }
        }
    } while (changed);
    json_object_put(callables);
    return true;
}
