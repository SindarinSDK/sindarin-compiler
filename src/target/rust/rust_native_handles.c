/* Canonical native as-ref allocations stay owned by C. Rust handles never
 * substitute value-record wire copies for their refcounted C payloads. */
static bool native_handle_type(json_object *type)
{
    return native_bool(type, "rust_native_reference_handle");
}

static void native_handle_mark_types(json_object *node, json_object *handles)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            native_handle_mark_types(json_object_array_get_idx(node, i), handles);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    const char *name = native_string(node, "name");
    if (name && native_string(node, "kind") &&
        strcmp(native_string(node, "kind"), "struct") == 0)
        for (size_t i = 0; i < json_object_array_length(handles); i++) {
            json_object *handle = json_object_array_get_idx(handles, i);
            if (strcmp(name, native_string(handle, "name")) != 0) continue;
            if (native_bool(handle, "rust_native_serial_handle")) {
                json_object_object_add(node, "rust_native_serial_handle", json_object_new_boolean(true));
                json_object_object_add(node, "rust_native_serial_invalidate", json_object_get(native_record_child(handle, "rust_native_serial_invalidate")));
                json_object_object_add(node, "rust_native_serial_hold_parent", json_object_get(native_record_child(handle, "rust_native_serial_hold_parent")));
            }
            json_object_object_add(node, "rust_native_reference_handle", json_object_new_boolean(true));
            const char *keys[] = {"rust_native_handle_field", "rust_native_handle_adopt",
                "rust_native_handle_retain", "rust_native_handle_release", "rust_native_handle_refs", "rust_native_handle_borrow", NULL};
            for (int k = 0; keys[k]; k++)
                json_object_object_add(node, keys[k], json_object_new_string(native_string(handle, keys[k])));
            break;
        }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) native_handle_mark_types(child, handles);
}

static json_object *native_handle_expression(const char *name, json_object *type)
{
    json_object *value = json_object_new_object();
    json_object_object_add(value, "kind", json_object_new_string("variable"));
    json_object_object_add(value, "name", json_object_new_string(name));
    json_object_object_add(value, "type", json_object_get(type));
    return value;
}

/* Native methods share the existing native-function ABI planning. The C
 * projection retains the original declaration; only the Rust projection gains
 * an explicit borrowed receiver and a wrapper body. */
static bool native_handle_prepare_methods(json_object *model, json_object *handle)
{
    json_object *methods = native_record_child(handle, "methods");
    json_object *functions = native_record_child(model, "functions");
    json_object *type = json_object_new_object();
    json_object_object_add(type, "kind", json_object_new_string("struct"));
    json_object_object_add(type, "name", json_object_new_string(native_string(handle, "name")));
    json_object_object_add(type, "pass_self_by_ref", json_object_new_boolean(true));
    json_object_object_add(type, "is_native", json_object_new_boolean(true));
    for (size_t i = 0; methods && i < json_object_array_length(methods); i++) {
        json_object *method = json_object_array_get_idx(methods, i);
        if (!native_bool(method, "is_native")) continue;
        char stem[128];
        snprintf(stem, sizeof(stem), "__sn_native_handle_%s_method_%zu", native_string(handle, "name"), i);
        char *name = unique_private_name(native_string(model, "package_native_namespace"), functions, native_record_child(model, "structs"),
                                         native_record_child(model, "globals"), stem);
        if (!name) { json_object_put(type); return false; }
        json_object *function = json_object_new_object();
        json_object_object_add(function, "name", json_object_new_string(name));
        json_object_object_add(function, "is_native", json_object_new_boolean(true));
        json_object_object_add(function, "has_body", json_object_new_boolean(false));
        if (native_bool(handle, "rust_native_static_namespace"))
            json_object_object_add(function, "rust_native_namespace_callable", json_object_new_boolean(true));
        if (native_bool(method, "rust_native_serial_end"))
            json_object_object_add(function, "rust_native_serial_end", json_object_new_boolean(true));
        if (native_bool(method, "rust_native_serial_child"))
            json_object_object_add(function, "rust_native_serial_child", json_object_new_boolean(true));
        json_object_object_add(function, "return_type", json_object_get(native_record_child(method, "return_type")));
        const char *alias = native_string(method, "c_alias");
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "__sn__%s_%s", native_string(handle, "name"), native_string(method, "name"));
        json_object_object_add(function, "c_alias", json_object_new_string(alias ? alias : fallback));
        json_object *params = json_object_new_array(), *args = json_object_new_array();
        if (!native_bool(method, "is_static")) {
            json_object *self = json_object_new_object();
            char *receiver = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(method, "params"), NULL, NULL, "__receiver");
            if (!receiver) { free(name); json_object_put(function); json_object_put(params); json_object_put(args); json_object_put(type); return false; }
            json_object_object_add(self, "name", json_object_new_string(receiver));
            free(receiver);
            json_object_object_add(self, "type", json_object_get(type));
            json_object_object_add(self, "mem_qual", json_object_new_string("default"));
            json_object_object_add(self, "sync_mod", json_object_new_string("none"));
            json_object_array_add(params, self);
            json_object_array_add(args, native_handle_expression("self", type));
        }
        json_object *original = native_record_child(method, "params");
        for (size_t p = 0; original && p < json_object_array_length(original); p++) {
            json_object *param = json_object_array_get_idx(original, p);
            json_object_array_add(params, deep_copy(param));
            json_object_array_add(args, native_handle_expression(native_string(param, "name"), native_record_child(param, "type")));
        }
        json_object_object_add(function, "params", params);
        json_object_object_add(function, "body", json_object_new_array());
        json_object_array_add(functions, function);
        json_object *call_type = json_object_new_object();
        json_object_object_add(call_type, "kind", json_object_new_string("function"));
        json_object_object_add(call_type, "is_native", json_object_new_boolean(true));
        json_object_object_add(call_type, "return_type", json_object_get(native_record_child(method, "return_type")));
        json_object *ptypes = json_object_new_array();
        for (size_t p = 0; p < json_object_array_length(params); p++)
            json_object_array_add(ptypes, json_object_get(native_record_child(json_object_array_get_idx(params, p), "type")));
        json_object_object_add(call_type, "param_types", ptypes);
        json_object *call = json_object_new_object();
        json_object_object_add(call, "kind", json_object_new_string("call"));
        json_object_object_add(call, "type", json_object_get(native_record_child(method, "return_type")));
        json_object_object_add(call, "callee", native_handle_expression(name, call_type));
        json_object_object_add(call, "args", args);
        json_object_put(call_type);
        json_object *stmt = json_object_new_object();
        bool is_void = strcmp(native_string(native_record_child(method, "return_type"), "kind"), "void") == 0;
        json_object_object_add(stmt, "kind", json_object_new_string(is_void ? "expr" : "return"));
        json_object_object_add(stmt, is_void ? "expr" : "value", call);
        json_object *body = json_object_new_array(); json_object_array_add(body, stmt);
        json_object_object_add(method, "body", body);
        json_object_object_add(method, "is_native", json_object_new_boolean(false));
        json_object_object_add(method, "rust_native_handle_method", json_object_new_boolean(true));
        free(name);
    }
    json_object_put(type);
    return true;
}

#include "rust_native_handle_arrays.c"

static bool native_prepare_handles(json_object *model, RustNativePlan *plan)
{
    json_object *handles = json_object_new_array();
    json_object *structures = native_record_child(model, "structs");
    for (size_t i = 0; structures && i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i);
        if (native_bool(structure, "rust_native_static_namespace")) {
            if (!native_handle_prepare_methods(model, structure)) { json_object_put(handles); return false; }
            continue;
        }
        if (!native_bool(structure, "is_native") || !native_bool(structure, "pass_self_by_ref") ||
            native_bool(structure, "is_packed") || native_bool(structure, "is_serializable") ||
            native_bool(structure, "has_user_copy_method")) continue;
        const char *keys[] = {"rust_native_handle_field", "rust_native_handle_adopt", "rust_native_handle_retain",
            "rust_native_handle_release", "rust_native_handle_refs", NULL};
        const char *roles[] = {"pointer", "adopt", "retain", "release", "refs"};
        for (int k = 0; keys[k]; k++) {
            char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_handle_%zu_%s", i, roles[k]);
            char *name = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(model, "functions"), structures,
                native_record_child(model, "globals"), stem);
            if (!name) { json_object_put(handles); return false; }
            json_object_object_add(structure, keys[k], json_object_new_string(name)); free(name);
        }
        char borrow_stem[160];
        snprintf(borrow_stem, sizeof(borrow_stem), "__sn_native_handle_%zu_borrow", i);
        char *borrow_name = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(structure, "methods"),
            native_record_child(structure, "fields"), native_record_child(model, "functions"), borrow_stem);
        if (!borrow_name) { json_object_put(handles); return false; }
        json_object_object_add(structure, "rust_native_handle_borrow", json_object_new_string(borrow_name));
        free(borrow_name);
        json_object *fields = native_record_child(structure, "fields");
        for (size_t f = 0; fields && f < json_object_array_length(fields); f++) {
            json_object *field = json_object_array_get_idx(fields, f);
            json_object *field_type = native_record_child(field, "type");
            bool string_field = native_string(field_type, "kind") &&
                                strcmp(native_string(field_type, "kind"), "string") == 0;
            if (!native_scalar_type(field_type, false) && !string_field) continue;
            if (string_field)
                json_object_object_add(model, "rust_nullable_strings", json_object_new_boolean(true));
            char stem[160]; snprintf(stem, sizeof(stem), "__sn_native_handle_%zu_get_%zu", i, f);
            char *getter = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(model, "functions"), structures,
                native_record_child(model, "globals"), stem);
            if (!getter) { json_object_put(handles); return false; }
            json_object_object_add(field, "rust_native_handle_get", json_object_new_string(getter)); free(getter);
            snprintf(stem, sizeof(stem), "__sn_native_handle_%zu_set_%zu", i, f);
            char *setter = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(model, "functions"), structures,
                native_record_child(model, "globals"), stem);
            if (!setter) { json_object_put(handles); return false; }
            json_object_object_add(field, "rust_native_handle_set", json_object_new_string(setter)); free(setter);
        }
        json_object_object_add(structure, "rust_native_reference_handle", json_object_new_boolean(true));
        json_object_array_add(handles, json_object_get(structure));
        if (!native_handle_prepare_methods(model, structure)) { json_object_put(handles); return false; }
    }
    native_handle_mark_types(model, handles);
    if (!native_prepare_handle_arrays(model, handles)) { json_object_put(handles); return false; }
    plan->handles = json_object_get(handles);
    json_object *array_support = native_record_child(model, "rust_native_handle_array_support");
    if (array_support) plan->array_support = json_object_get(array_support);
    if (json_object_array_length(handles)) json_object_object_add(model, "rust_native_handles", handles);
    else json_object_put(handles);
    return true;
}

/* Native handles can move only after the actual C owner implementation supplies
 * atomic credits. Annotate the private C projection, never the default C model.
 * Every generated retain/release and array callback then uses the same atomic
 * field. C emission asserts its type and ABI before Rust may claim Send. */
static bool native_prepare_handle_atomic_owners(json_object *private_model,
                                                 json_object *handles)
{
    for (size_t i = 0; handles && i < json_object_array_length(handles); i++)
    {
        json_object *handle = json_object_array_get_idx(handles, i);
        if (native_bool(handle, "rust_native_serial_handle")) continue;
        json_object *structure = native_record_struct(private_model, native_string(handle, "name"));
        if (!structure || !native_bool(structure, "is_native") ||
            !native_bool(structure, "pass_self_by_ref")) return false;
        if (!native_record_child(structure, "native_record"))
            json_object_object_add(structure, "rust_native_handle_atomic_refs", json_object_new_boolean(true));
        json_object_object_add(handle, "rust_native_handle_send", json_object_new_boolean(true));
    }
    return true;
}
