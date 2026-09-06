/* Ordinary nested sized arrays store handles to their initializer arrays.
 * Keep this projection separate from closure/thread transport: using Arc here
 * retains identity without imposing Send/Sync bounds on source callables. */
static json_object *rust_nested_array_specialization(json_object *model,
    json_object *functions, json_object *fn, json_object *args,
    RustThreadRefBinding *scope, bool *changed)
{
    const char *base = json_string_property(fn, "rust_nested_array_base");
    if (!base) base = json_string_property(fn, "name");
    json_object *original = fn;
    for (size_t i = 0; i < json_object_array_length(functions); i++) {
        json_object *candidate = json_object_array_get_idx(functions, i);
        if (json_string_property_equals(candidate, "name", base)) { original = candidate; break; }
    }
    json_object *params = NULL;
    json_object_object_get_ex(original, "params", &params);
    size_t count = params ? json_object_array_length(params) : 0;
    char *mask = calloc(count + 1, 1);
    if (!mask) return NULL;
    bool owns = false;
    for (size_t i = 0; i < count; i++) {
        json_object *arg = args && i < json_object_array_length(args) ? json_object_array_get_idx(args, i) : NULL;
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(arg, "name"));
        bool selected = (binding && json_boolean_property(binding->declaration, "rust_nested_array_storage")) ||
            json_boolean_property(arg, "rust_nested_array_read");
        mask[i] = selected && json_boolean_property(json_object_array_get_idx(params, i), "rust_default_array_ref") ? '1' : '0';
        owns |= mask[i] == '1';
    }
    if (!owns) { free(mask); return original; }
    for (size_t i = 0; i < json_object_array_length(functions); i++) {
        json_object *candidate = json_object_array_get_idx(functions, i);
        if (json_string_property_equals(candidate, "rust_nested_array_base", base) &&
            json_string_property_equals(candidate, "rust_nested_array_mask", mask)) {
            free(mask); return candidate;
        }
    }
    json_object *copy = NULL;
    json_object_deep_copy(original, &copy, NULL);
    if (!copy) { free(mask); return NULL; }
    char name[80]; size_t id = 0;
    do { snprintf(name, sizeof(name), "__sn_nested_array_call_%zu", id++); }
    while (rust_model_contains_string(model, name));
    json_object_object_add(copy, "name", json_object_new_string(name));
    json_object_object_add(copy, "rust_nested_array_base", json_object_new_string(base));
    json_object_object_add(copy, "rust_nested_array_mask", json_object_new_string(mask));
    json_object_object_get_ex(copy, "params", &params);
    for (size_t i = 0; i < count; i++) if (mask[i] == '1') {
        json_object *param = json_object_array_get_idx(params, i);
        json_object_object_add(param, "rust_nested_array_param", json_object_new_boolean(true));
        json_object_object_add(param, "rust_nested_array_storage", json_object_new_boolean(true));
    }
    free(mask);
    json_object_array_add(functions, copy);
    *changed = true;
    return copy;
}

static bool rust_nested_array_walk(json_object *node, json_object *model, json_object *functions, RustThreadRefBinding *scope,
                                   bool *changed)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array)) {
        RustThreadRefBinding *locals = scope;
        bool ok = true;
        for (size_t i = 0; ok && i < json_object_array_length(node); i++) {
            json_object *child = json_object_array_get_idx(node, i);
            ok = rust_nested_array_walk(child, model, functions, locals, changed);
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
    if (json_string_property_equals(node, "kind", "var_decl")) {
        json_object *init = NULL, *element = NULL, *value = NULL, *type = NULL, *inner = NULL;
        json_object_object_get_ex(node, "initializer", &init);
        json_object_object_get_ex(init, "element_type", &element);
        json_object_object_get_ex(init, "default_value", &value);
        if (json_string_property_equals(init, "kind", "sized_array") &&
            json_string_property_equals(element, "kind", "array") &&
            json_string_property_equals(value, "kind", "variable")) {
            RustThreadRefBinding *seed = rust_thread_ref_lookup(scope, json_string_property(value, "name"));
            if (seed) {
                if (!json_boolean_property(seed->declaration, "rust_nested_array_storage")) *changed = true;
                json_object_object_add(seed->declaration, "rust_nested_array_storage", json_object_new_boolean(true));
                json_object_object_add(value, "rust_nested_array_owner", json_object_new_boolean(true));
                json_object_object_get_ex(node, "type", &type);
                json_object_object_get_ex(type, "element_type", &inner);
                if (inner) json_object_object_add(inner, "rust_nested_array_handle", json_object_new_boolean(true));
            }
        }
    }
    if (json_string_property_equals(node, "kind", "assign")) {
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(node, "target"));
        if (binding && json_boolean_property(binding->declaration, "rust_nested_array_storage"))
            json_object_object_add(node, "rust_nested_array_storage", json_object_new_boolean(true));
    }
    if (json_string_property_equals(node, "kind", "variable")) {
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(node, "name"));
        if (binding) {
            json_object *type = NULL;
            json_object_object_get_ex(binding->declaration, "type", &type);
            if (type) json_object_object_add(node, "type", json_object_get(type));
            if (json_boolean_property(binding->declaration, "rust_nested_array_storage"))
                json_object_object_add(node, "rust_cell", json_object_new_boolean(true));
        }
    }
    json_object_object_foreach(node, key, value) {
        if (!strcmp(key, "type")) continue;
        if (!rust_nested_array_walk(value, model, functions, scope, changed)) return false;
    }
    if (json_string_property_equals(node, "kind", "call")) {
        json_object *callee = NULL, *args = NULL;
        json_object_object_get_ex(node, "callee", &callee);
        json_object_object_get_ex(node, "args", &args);
        for (size_t f = 0; json_string_property(callee, "name") && functions && f < json_object_array_length(functions); f++) {
            json_object *fn = json_object_array_get_idx(functions, f), *params = NULL;
            if (!json_string_property_equals(fn, "name", json_string_property(callee, "name"))) continue;
            fn = rust_nested_array_specialization(model, functions, fn, args, scope, changed);
            if (!fn) return false;
            json_object_object_add(callee, "name", json_object_new_string(json_string_property(fn, "name")));
            json_object_object_get_ex(fn, "params", &params);
            for (size_t i = 0; params && args && i < json_object_array_length(params) && i < json_object_array_length(args); i++) {
                json_object *param = json_object_array_get_idx(params, i), *arg = json_object_array_get_idx(args, i);
                if (json_boolean_property(param, "rust_nested_array_param")) {
                    json_object_object_add(arg, "rust_nested_array_owner", json_object_new_boolean(true));
                    json_object_object_del(arg, "rust_default_array_ref_arg");
                    json_object_object_del(arg, "is_ref_arg");
                }
            }
            break;
        }
    }
    if (json_string_property_equals(node, "kind", "array_access")) {
        json_object *array = NULL, *type = NULL, *element = NULL;
        json_object_object_get_ex(node, "array", &array);
        json_object_object_get_ex(array, "type", &type);
        json_object_object_get_ex(type, "element_type", &element);
        if (json_boolean_property(element, "rust_nested_array_handle")) {
            json_object_object_add(node, "type", json_object_get(element));
            json_object_object_add(node, "rust_nested_array_read", json_object_new_boolean(true));
        }
    }
    if (json_string_property_equals(node, "kind", "index_assign")) {
        json_object *array = NULL;
        json_object_object_get_ex(node, "array", &array);
        if (json_boolean_property(array, "rust_nested_array_read")) {
            json_object_object_add(array, "rust_nested_array_owner", json_object_new_boolean(true));
            json_object_object_add(node, "rust_nested_index_assign", json_object_new_boolean(true));
        }
    }
    return true;
}

static bool rust_prepare_nested_array_owners(json_object *model)
{
    bool changed;
    do {
        changed = false;
        json_object *functions = NULL;
        json_object_object_get_ex(model, "functions", &functions);
        for (size_t f = 0; functions && f < json_object_array_length(functions); f++) {
            json_object *fn = json_object_array_get_idx(functions, f), *params = NULL, *body = NULL;
            json_object_object_get_ex(fn, "params", &params);
            json_object_object_get_ex(fn, "body", &body);
            RustThreadRefBinding *scope = NULL;
            for (size_t i = 0; params && i < json_object_array_length(params); i++) {
                json_object *param = json_object_array_get_idx(params, i);
                RustThreadRefBinding *binding = malloc(sizeof(*binding));
                if (!binding) return false;
                *binding = (RustThreadRefBinding){json_string_property(param, "name"), param, scope}; scope = binding;
            }
            bool ok = rust_nested_array_walk(body, model, functions, scope, &changed);
            while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
            if (!ok) return false;
        }
    } while (changed);
    return true;
}
