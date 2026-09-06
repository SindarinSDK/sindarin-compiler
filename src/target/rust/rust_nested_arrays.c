/* Ordinary nested sized arrays store handles to their initializer arrays.
 * Keep this projection separate from closure/thread transport: using Arc here
 * retains identity without imposing Send/Sync bounds on source callables. */
static json_object *rust_nested_struct(json_object *model, const char *name)
{
    json_object *structs = NULL;
    json_object_object_get_ex(model, "structs", &structs);
    for (size_t i = 0; name && structs && i < json_object_array_length(structs); i++) {
        json_object *decl = json_object_array_get_idx(structs, i);
        if (json_string_property_equals(decl, "name", name)) return decl;
    }
    return NULL;
}

static json_object *rust_nested_struct_type(json_object *type)
{
    while (json_string_property_equals(type, "kind", "pointer")) {
        json_object *base = NULL;
        if (!json_object_object_get_ex(type, "base_type", &base)) break;
        type = base;
    }
    return type;
}

static json_object *rust_nested_field(json_object *model, json_object *place)
{
    json_object *object = NULL, *type = NULL, *fields = NULL;
    json_object_object_get_ex(place, "object", &object);
    json_object_object_get_ex(object, "type", &type);
    json_object *decl = rust_nested_struct(model, json_string_property(rust_nested_struct_type(type), "name"));
    json_object_object_get_ex(decl, "fields", &fields);
    const char *name = json_string_property(place, "member_name");
    if (!name) name = json_string_property(place, "field_name");
    for (size_t i = 0; name && fields && i < json_object_array_length(fields); i++) {
        json_object *field = json_object_array_get_idx(fields, i);
        if (json_string_property_equals(field, "name", name)) return field;
    }
    return NULL;
}

static void rust_nested_field_projections(json_object *node, json_object *model)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++) rust_nested_field_projections(json_object_array_get_idx(node, i), model);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value) {
        if (strcmp(key, "type")) rust_nested_field_projections(value, model);
    }
    json_object *field = rust_nested_field(model, node);
    if (field && json_boolean_property(field, "rust_nested_field_owner")) {
        if (json_string_property_equals(node, "kind", "member")) {
            json_object *type = NULL;
            json_object_object_get_ex(field, "type", &type);
            json_object_object_add(node, "type", json_object_get(type));
            json_object_object_add(node, "rust_nested_array_read", json_object_new_boolean(true));
        }
        if (json_string_property_equals(node, "kind", "member_assign"))
            json_object_object_add(node, "rust_nested_field_assignment", json_object_new_boolean(true));
    }
    if (json_string_property_equals(node, "kind", "struct_literal")) {
        json_object *decl = rust_nested_struct(model, json_string_property(node, "struct_name"));
        json_object *fields = NULL, *values = NULL;
        json_object_object_get_ex(decl, "fields", &fields);
        json_object_object_get_ex(node, "fields", &values);
        for (size_t i = 0; values && i < json_object_array_length(values); i++) {
            json_object *value = json_object_array_get_idx(values, i);
            for (size_t f = 0; fields && f < json_object_array_length(fields); f++) {
                json_object *field = json_object_array_get_idx(fields, f);
                if (json_boolean_property(field, "rust_nested_field_owner") &&
                    json_string_property_equals(field, "name", json_string_property(value, "name")))
                    json_object_object_add(value, "rust_nested_field_owner", json_object_new_boolean(true));
            }
        }
    }
}

static bool rust_nested_array_elements_have_handles(json_object *type)
{
    json_object *element = NULL;
    if (!json_object_object_get_ex(type, "element_type", &element)) return false;
    return json_boolean_property(element, "rust_nested_array_handle") ||
        rust_nested_array_elements_have_handles(element);
}

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
        json_object *type = NULL;
        json_object_object_get_ex(arg, "type", &type);
        bool transport = rust_nested_array_elements_have_handles(type);
        bool owner = selected && json_boolean_property(json_object_array_get_idx(params, i), "rust_default_array_ref");
        mask[i] = owner ? (transport ? '3' : '1') : (transport ? '2' : '0');
        owns |= mask[i] != '0';
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
    for (size_t i = 0; i < count; i++) if (mask[i] != '0') {
        json_object *param = json_object_array_get_idx(params, i);
        if (mask[i] == '1' || mask[i] == '3') {
            json_object_object_add(param, "rust_nested_array_param", json_object_new_boolean(true));
            json_object_object_add(param, "rust_nested_array_storage", json_object_new_boolean(true));
        }
        if (mask[i] == '2' || mask[i] == '3') {
            json_object *type = NULL, *projected = NULL;
            json_object_object_get_ex(json_object_array_get_idx(args, i), "type", &type);
            json_object_deep_copy(type, &projected, NULL);
            json_object_object_del(projected, "rust_nested_array_handle");
            json_object_object_add(param, "type", projected);
        }
    }
    free(mask);
    json_object_array_add(functions, copy);
    *changed = true;
    return copy;
}

static void rust_nested_mutable_place(json_object *place)
{
    if (!json_string_property_equals(place, "kind", "array_access") ||
        json_boolean_property(place, "rust_nested_array_read")) return;
    json_object_object_add(place, "rust_mutable_array_place", json_object_new_boolean(true));
    json_object *array = NULL;
    json_object_object_get_ex(place, "array", &array);
    rust_nested_mutable_place(array);
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
    if (json_string_property_equals(node, "kind", "lambda")) {
        json_object *captures = NULL;
        json_object_object_get_ex(node, "captures", &captures);
        for (size_t i = 0; captures && i < json_object_array_length(captures); i++) {
            json_object *capture = json_object_array_get_idx(captures, i), *type = NULL;
            RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(capture, "name"));
            if (binding) {
                json_object_object_get_ex(binding->declaration, "type", &type);
                if (type) json_object_object_add(capture, "type", json_object_get(type));
            }
        }
    }
    if (json_string_property_equals(node, "kind", "var_decl")) {
        json_object *init = NULL, *element = NULL, *value = NULL, *type = NULL, *inner = NULL;
        json_object_object_get_ex(node, "initializer", &init);
        json_object_object_get_ex(init, "element_type", &element);
        json_object_object_get_ex(init, "default_value", &value);
        if (json_string_property_equals(init, "kind", "sized_array") &&
            json_string_property_equals(element, "kind", "array")) {
            RustThreadRefBinding *seed = rust_thread_ref_lookup(scope, json_string_property(value, "name"));
            json_object *field = rust_nested_field(model, value);
            if (field) {
                if (!json_boolean_property(field, "rust_nested_field_owner")) *changed = true;
                json_object_object_add(field, "rust_nested_field_owner", json_object_new_boolean(true));
                json_object *field_type = NULL, *object = NULL, *object_type = NULL;
                json_object_object_get_ex(field, "type", &field_type);
                json_object_object_add(field_type, "rust_nested_array_handle", json_object_new_boolean(true));
                json_object_object_get_ex(value, "object", &object);
                json_object_object_get_ex(object, "type", &object_type);
                json_object *decl = rust_nested_struct(model, json_string_property(rust_nested_struct_type(object_type), "name"));
                json_object_object_add(decl, "rust_nested_field_owners", json_object_new_boolean(true));
            }
            bool retained = seed || field || json_boolean_property(value, "rust_nested_array_read");
            json_object_object_add(init, "rust_nested_fresh_initializer", json_object_new_boolean(!retained));
            if (retained) {
                if (seed) {
                if (!json_boolean_property(seed->declaration, "rust_nested_array_storage")) *changed = true;
                json_object_object_add(seed->declaration, "rust_nested_array_storage", json_object_new_boolean(true));
                }
                json_object_object_add(value, "rust_nested_array_owner", json_object_new_boolean(true));
            }
            json_object_object_get_ex(node, "type", &type);
            json_object_object_get_ex(type, "element_type", &inner);
            json_object *source_type = NULL;
            if (seed) json_object_object_get_ex(seed->declaration, "type", &source_type);
            else if (field) json_object_object_get_ex(field, "type", &source_type);
            else json_object_object_get_ex(value, "type", &source_type);
            if (source_type && json_string_property_equals(source_type, "kind", "array")) {
                json_object *projected = NULL;
                json_object_deep_copy(source_type, &projected, NULL);
                json_object_object_add(projected, "rust_nested_array_handle", json_object_new_boolean(true));
                if (!json_object_equal(inner, projected)) *changed = true;
                json_object_object_add(type, "element_type", projected);
                inner = projected;
            }
            if (inner) {
                if (!json_boolean_property(inner, "rust_nested_array_handle")) *changed = true;
                json_object_object_add(inner, "rust_nested_array_handle", json_object_new_boolean(true));
            }
        }
    }
    if (json_string_property_equals(node, "kind", "assign")) {
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(node, "target"));
        if (binding && json_boolean_property(binding->declaration, "rust_nested_array_storage")) {
            json_object_object_add(node, "rust_nested_array_storage", json_object_new_boolean(true));
            if (json_boolean_property(binding->declaration, "rust_nested_global_declaration"))
                json_object_object_add(node, "rust_nested_global_assignment", json_object_new_boolean(true));
        }
    }
    if (json_string_property_equals(node, "kind", "variable")) {
        RustThreadRefBinding *binding = rust_thread_ref_lookup(scope, json_string_property(node, "name"));
        if (binding) {
            json_object *type = NULL;
            json_object_object_get_ex(binding->declaration, "type", &type);
            if (type) json_object_object_add(node, "type", json_object_get(type));
            if (json_boolean_property(binding->declaration, "rust_nested_array_storage")) {
                if (json_boolean_property(binding->declaration, "rust_nested_global_declaration")) {
                    json_object_object_add(node, "rust_nested_global_read", json_object_new_boolean(true));
                    json_object_object_add(node, "rust_nested_array_read", json_object_new_boolean(true));
                    json_object_object_del(node, "rust_cell");
                } else json_object_object_add(node, "rust_cell", json_object_new_boolean(true));
            }
        }
    }
    json_object_object_foreach(node, key, value) {
        if (!strcmp(key, "type")) continue;
        if (!rust_nested_array_walk(value, model, functions, scope, changed)) return false;
    }
    if (json_string_property_equals(node, "kind", "var_decl")) {
        json_object *init = NULL, *type = NULL, *decl_type = NULL;
        json_object_object_get_ex(node, "initializer", &init);
        json_object_object_get_ex(init, "type", &type);
        json_object_object_get_ex(node, "type", &decl_type);
        if (rust_nested_array_elements_have_handles(type) && !rust_nested_array_elements_have_handles(decl_type)) {
            json_object *copy = NULL;
            json_object_deep_copy(type, &copy, NULL);
            json_object_object_del(copy, "rust_nested_array_handle");
            json_object_object_add(node, "type", copy);
            *changed = true;
        }
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
        /* The tagged read only asks the parent for its length for a negative
         * index. In particular, a positive inner read must not evaluate an
         * owner-backed member/index expression again merely to get its len. */
        if (json_boolean_property(array, "rust_nested_array_read"))
            json_object_object_add(node, "rust_nested_parent_read", json_object_new_boolean(true));
        if (json_boolean_property(element, "rust_nested_array_handle")) {
            json_object_object_add(node, "type", json_object_get(element));
            json_object_object_add(node, "rust_nested_array_read", json_object_new_boolean(true));
            json_object_object_del(node, "rust_mutable_array_place");
        }
    }
    if (json_string_property_equals(node, "kind", "index_assign")) {
        json_object *array = NULL;
        json_object_object_get_ex(node, "array", &array);
        rust_nested_mutable_place(array);
        if (json_boolean_property(array, "rust_nested_array_read")) {
            json_object_object_add(array, "rust_nested_array_owner", json_object_new_boolean(true));
            json_object_object_add(node, "rust_nested_index_assign", json_object_new_boolean(true));
        }
    }
    return true;
}

static bool rust_nested_array_callable(json_object *fn, json_object *model,
                                       json_object *functions, bool *changed)
{
    json_object *params = NULL, *body = NULL;
    json_object_object_get_ex(fn, "params", &params);
    json_object_object_get_ex(fn, "body", &body);
    RustThreadRefBinding *scope = NULL;
    json_object *globals = NULL;
    json_object_object_get_ex(model, "globals", &globals);
    for (size_t i = 0; globals && i < json_object_array_length(globals); i++) {
        json_object *global = json_object_array_get_idx(globals, i);
        json_object_object_add(global, "rust_nested_global_declaration", json_object_new_boolean(true));
        RustThreadRefBinding *binding = malloc(sizeof(*binding));
        if (!binding) return false;
        *binding = (RustThreadRefBinding){json_string_property(global, "name"), global, scope}; scope = binding;
    }
    for (size_t i = 0; params && i < json_object_array_length(params); i++) {
        json_object *param = json_object_array_get_idx(params, i);
        RustThreadRefBinding *binding = malloc(sizeof(*binding));
        if (!binding) return false;
        *binding = (RustThreadRefBinding){json_string_property(param, "name"), param, scope}; scope = binding;
    }
    bool ok = rust_nested_array_walk(body, model, functions, scope, changed);
    while (scope) { RustThreadRefBinding *next = scope->next; free(scope); scope = next; }
    return ok;
}

static bool rust_prepare_nested_array_owners(json_object *model)
{
    bool changed;
    do {
        changed = false;
        rust_nested_field_projections(model, model);
        json_object *functions = NULL;
        json_object_object_get_ex(model, "functions", &functions);
        for (size_t f = 0; functions && f < json_object_array_length(functions); f++)
            if (!rust_nested_array_callable(json_object_array_get_idx(functions, f), model, functions, &changed)) return false;
        json_object *structs = NULL;
        json_object_object_get_ex(model, "structs", &structs);
        for (size_t i = 0; structs && i < json_object_array_length(structs); i++) {
            json_object *methods = NULL;
            json_object_object_get_ex(json_object_array_get_idx(structs, i), "methods", &methods);
            for (size_t m = 0; methods && m < json_object_array_length(methods); m++)
                if (!rust_nested_array_callable(json_object_array_get_idx(methods, m), model, functions, &changed)) return false;
        }
    } while (changed);
    return true;
}
