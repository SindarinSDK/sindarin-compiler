/* Built-in serialization objects have their C vtables in sn_serial.h, rather
 * than source struct declarations. Preserve those opaque allocations. */
typedef struct {
    const char *name;
    const char *result;
    const char *first;
    const char *second;
} NativeSerialMethod;

static const NativeSerialMethod native_encoder_methods[] = {
    {"writeStr", "void", "string", "string"},
    {"writeInt", "void", "string", "int"},
    {"writeDouble", "void", "string", "double"},
    {"writeBool", "void", "string", "bool"},
    {"writeNull", "void", "string", NULL},
    {"beginObject", "Encoder", "string", NULL},
    {"beginArray", "Encoder", "string", NULL},
    {"end", "void", NULL, NULL},
    {"appendStr", "void", "string", NULL},
    {"appendInt", "void", "int", NULL},
    {"appendDouble", "void", "double", NULL},
    {"appendBool", "void", "bool", NULL},
    {"appendObject", "Encoder", NULL, NULL},
    {"result", "string", NULL, NULL},
};

static const NativeSerialMethod native_decoder_methods[] = {
    {"readStr", "string", "string", NULL},
    {"readInt", "int", "string", NULL},
    {"readDouble", "double", "string", NULL},
    {"readBool", "bool", "string", NULL},
    {"hasKey", "bool", "string", NULL},
    {"readObject", "Decoder", "string", NULL},
    {"readArray", "Decoder", "string", NULL},
    {"length", "int", NULL, NULL},
    {"at", "Decoder", "int", NULL},
    {"atStr", "string", "int", NULL},
    {"atInt", "int", "int", NULL},
    {"atDouble", "double", "int", NULL},
    {"atBool", "bool", "int", NULL},
};

static json_object *native_serial_type(const char *name)
{
    if (strcmp(name, "Encoder") != 0 && strcmp(name, "Decoder") != 0)
        return native_primitive_type(name);
    json_object *type = json_object_new_object();
    json_object_object_add(type, "kind", json_object_new_string("struct"));
    json_object_object_add(type, "name", json_object_new_string(name));
    json_object_object_add(type, "is_native", json_object_new_boolean(true));
    json_object_object_add(type, "pass_self_by_ref", json_object_new_boolean(true));
    return type;
}

static bool native_serial_has_type(json_object *node, const char *name)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (native_serial_has_type(json_object_array_get_idx(node, i), name)) return true;
    } else if (json_object_is_type(node, json_type_object)) {
        if (native_string(node, "kind") && strcmp(native_string(node, "kind"), "struct") == 0 &&
            native_string(node, "name") && strcmp(native_string(node, "name"), name) == 0 &&
            native_bool(node, "is_native") && native_bool(node, "pass_self_by_ref")) return true;
        json_object_object_foreach(node, key, value)
            if (strncmp(key, "rust_", 5) != 0 && native_serial_has_type(value, name)) return true;
    }
    return false;
}

static void native_serial_rust_calls(json_object *model, json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array)) {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            native_serial_rust_calls(model, json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object *callee = native_record_child(node, "callee");
    json_object *receiver = native_record_child(callee, "object");
    const char *name = native_string(native_record_child(receiver, "type"), "name");
    if (!name) name = native_string(native_record_child(node, "struct_type"), "name");
    if (!name) name = native_string(node, "type_name");
    json_object *structure = native_record_struct(model, name);
    if (native_bool(structure, "rust_serializable")) {
        const char *method_name = native_string(callee, "member_name");
        if (!method_name) method_name = native_string(node, "method_name");
        json_object *methods = native_record_child(structure, "methods");
        for (size_t m = 0; method_name && m < json_object_array_length(methods); m++) {
            json_object *method = json_object_array_get_idx(methods, m);
            if (!native_bool(method, "is_serializable_method") ||
                strcmp(method_name, native_string(method, "name")) != 0) continue;
            json_object *type = native_record_child(callee, "type");
            if (type) json_object_object_add(type, "is_native", json_object_new_boolean(false));
            if (callee) {
                json_object_object_del(callee, "c_alias");
                json_object_object_del(callee, "has_c_alias");
            }
            json_object_object_add(node, "is_native", json_object_new_boolean(false));
            json_object *args = native_record_child(node, "args");
            json_object *params = native_record_child(method, "params");
            for (size_t p = 0; args && params && p < json_object_array_length(args) && p < json_object_array_length(params); p++) {
                json_object *param = json_object_array_get_idx(params, p);
                if (native_bool(param, "rust_default_array_ref"))
                    json_object_object_add(json_object_array_get_idx(args, p), "rust_default_array_ref_arg", json_object_new_boolean(true));
            }
            break;
        }
    }
    json_object_object_foreach(node, key, value)
        if (strncmp(key, "rust_", 5) != 0) native_serial_rust_calls(model, value);
}

static bool native_prepare_serial_types(json_object *model, json_object *private_model,
                                        const char *source_file)
{
    const char *names[] = {"Encoder", "Decoder"};
    json_object *structures = native_record_child(model, "structs");
    for (size_t k = 0; k < 2; k++) {
        if (!native_serial_has_type(model, names[k])) continue;
        json_object *structure = json_object_new_object();
        json_object_object_add(structure, "name", json_object_new_string(names[k]));
        json_object_object_add(structure, "is_native", json_object_new_boolean(true));
        json_object_object_add(structure, "pass_self_by_ref", json_object_new_boolean(true));
        json_object_object_add(structure, "mem_mode", json_object_new_string("ref"));
        json_object_object_add(structure, "fields", json_object_new_array());
        json_object_object_add(structure, "rust_native_serial_handle", json_object_new_boolean(true));
        const char *roles[] = {"owner", "registry", "invalidate", "hold_parent", NULL};
        for (size_t r = 0; roles[r]; r++) {
            char stem[128];
            snprintf(stem, sizeof(stem), "__sn_serial_%s_%s", names[k], roles[r]);
            char *name = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(model, "functions"), structures,
                native_record_child(model, "globals"), stem);
            if (!name) { json_object_put(structure); return false; }
            char key[96]; snprintf(key, sizeof(key), "rust_native_serial_%s", roles[r]);
            json_object_object_add(structure, key, json_object_new_string(name));
            free(name);
        }
        json_object *methods = json_object_new_array();
        const NativeSerialMethod *specs = k == 0 ? native_encoder_methods : native_decoder_methods;
        size_t count = k == 0 ? sizeof(native_encoder_methods) / sizeof(*specs) : sizeof(native_decoder_methods) / sizeof(*specs);
        for (size_t m = 0; m < count; m++) {
            const NativeSerialMethod *spec = &specs[m];
            json_object *method = json_object_new_object(), *params = json_object_new_array();
            json_object_object_add(method, "name", json_object_new_string(spec->name));
            json_object_object_add(method, "is_native", json_object_new_boolean(true));
            json_object_object_add(method, "is_static", json_object_new_boolean(false));
            json_object_object_add(method, "return_type", native_serial_type(spec->result));
            if (strcmp(spec->result, names[k]) == 0)
                json_object_object_add(method, "rust_native_serial_child", json_object_new_boolean(true));
            const char *types[] = {spec->first, spec->second};
            for (size_t p = 0; p < 2 && types[p]; p++) {
                json_object *param = json_object_new_object();
                json_object_object_add(param, "name", json_object_new_string(p == 0 ? "first" : "second"));
                json_object_object_add(param, "type", native_serial_type(types[p]));
                json_object_object_add(param, "mem_qual", json_object_new_string("default"));
                json_object_object_add(param, "sync_mod", json_object_new_string("none"));
                json_object_array_add(params, param);
            }
            json_object_object_add(method, "params", params);
            char stem[160]; snprintf(stem, sizeof(stem), "__sn_serial_%s_%s", names[k], spec->name);
            char *bridge_name = unique_private_name(native_string(model, "package_native_namespace"), native_record_child(private_model, "functions"), structures,
                native_record_child(model, "globals"), stem);
            if (!bridge_name) { json_object_put(method); json_object_put(methods); json_object_put(structure); return false; }
            json_object_object_add(method, "c_alias", json_object_new_string(bridge_name));
            if (k == 0 && strcmp(spec->name, "end") == 0)
                json_object_object_add(method, "rust_native_serial_end", json_object_new_boolean(true));
            json_object *function = json_object_new_object();
            json_object_object_add(function, "name", json_object_new_string(bridge_name));
            free(bridge_name);
            json_object_object_add(function, "is_native", json_object_new_boolean(true));
            json_object_object_add(function, "has_body", json_object_new_boolean(true));
            json_object_object_add(function, "source_file", json_object_new_string(source_file));
            json_object_object_add(function, "return_type", native_serial_type(spec->result));
            json_object *all_params = json_object_new_array(), *self = json_object_new_object();
            json_object_object_add(self, "name", json_object_new_string("receiver"));
            json_object_object_add(self, "type", native_serial_type(names[k]));
            json_object_object_add(self, "mem_qual", json_object_new_string("default"));
            json_object_object_add(self, "sync_mod", json_object_new_string("none"));
            json_object_array_add(all_params, self);
            for (size_t p = 0; p < json_object_array_length(params); p++)
                json_object_array_add(all_params, deep_copy(json_object_array_get_idx(params, p)));
            json_object_object_add(function, "params", all_params);
            json_object *args = json_object_new_array(), *types_array = json_object_new_array();
            for (size_t p = 0; p < json_object_array_length(all_params); p++) {
                json_object *param = json_object_array_get_idx(all_params, p);
                json_object *type = native_record_child(param, "type");
                json_object_array_add(args, native_handle_expression(native_string(param, "name"), type));
                json_object_array_add(types_array, json_object_get(type));
            }
            json_object *callee_type = native_primitive_type("function");
            json_object_object_add(callee_type, "is_native", json_object_new_boolean(true));
            json_object_object_add(callee_type, "return_type", native_serial_type(spec->result));
            json_object_object_add(callee_type, "param_types", types_array);
            char symbol[160]; snprintf(symbol, sizeof(symbol), "__sn__%s_%s", names[k], spec->name);
            json_object *call = json_object_new_object();
            json_object_object_add(call, "kind", json_object_new_string("call"));
            json_object_object_add(call, "type", native_serial_type(spec->result));
            json_object_object_add(call, "callee", native_handle_expression(symbol, callee_type));
            json_object_put(callee_type);
            json_object_object_add(call, "args", args);
            json_object *statement = json_object_new_object(), *body = json_object_new_array();
            bool is_void = strcmp(spec->result, "void") == 0;
            json_object_object_add(statement, "kind", json_object_new_string(is_void ? "expr" : "return"));
            json_object_object_add(statement, is_void ? "expr" : "value", call);
            json_object_array_add(body, statement);
            json_object_object_add(function, "body", body);
            json_object_array_add(native_record_child(private_model, "functions"), function);
            json_object_array_add(methods, method);
        }
        json_object_object_add(structure, "methods", methods);
        json_object_array_add(structures, structure);
    }
    for (size_t i = 0; i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i);
        if (!native_bool(structure, "is_serializable")) continue;
        json_object *methods = native_record_child(structure, "methods");
        json_object *fields = native_record_child(structure, "fields");
        for (size_t f = 0; f < json_object_array_length(fields); f++) {
            json_object *field = json_object_array_get_idx(fields, f);
            json_object *key = native_record_child(field, "c_alias");
            if (!key) key = native_record_child(field, "name");
            json_object_object_add(field, "rust_serial_key", json_object_get(key));
            char stem[96];
            snprintf(stem, sizeof(stem), "__sn_serial_value_%zu", f);
            char *local = unique_private_name(native_string(model, "package_native_namespace"), fields, methods,
                native_record_child(model, "globals"), stem);
            if (!local) return false;
            json_object_object_add(field, "rust_serial_field_value", json_object_new_string(local));
            free(local);
        }
        char *helper = unique_private_name(native_string(model, "package_native_namespace"), methods, fields,
            native_record_child(model, "functions"), "__sn_serial_encode_impl");
        if (!helper) return false;
        json_object_object_add(structure, "rust_serializable", json_object_new_boolean(true));
        json_object_object_add(structure, "is_serializable", json_object_new_boolean(false));
        json_object_object_add(structure, "rust_serial_encode_impl", json_object_new_string(helper));
        size_t count = json_object_array_length(methods);
        json_object *encode = NULL;
        for (size_t m = 0; m < count; m++) {
            json_object *method = json_object_array_get_idx(methods, m);
            if (!native_bool(method, "is_serializable_method")) continue;
            json_object_object_add(method, "is_native", json_object_new_boolean(false));
            json_object_object_add(method, "rust_serial_fields", json_object_get(fields));
            json_object_object_add(method, "rust_serial_type_name", json_object_get(native_record_child(structure, "name")));
            json_object_object_add(method, "rust_serial_encode_impl", json_object_new_string(helper));
            json_object *params = native_record_child(method, "params");
            for (size_t p = 0; p < json_object_array_length(params); p++) {
                json_object *param = json_object_array_get_idx(params, p);
                if (native_string(native_record_child(param, "type"), "kind") &&
                    strcmp(native_string(native_record_child(param, "type"), "kind"), "array") == 0 &&
                    native_string(param, "mem_qual") && strcmp(native_string(param, "mem_qual"), "default") == 0)
                    json_object_object_add(param, "rust_default_array_ref", json_object_new_boolean(true));
            }
            if (strcmp(native_string(method, "name"), "encode") == 0) encode = method;
        }
        if (encode) {
            json_object *internal = deep_copy(encode);
            json_object_object_add(internal, "rust_serial_fields", json_object_get(fields));
            json_object_object_add(internal, "name", json_object_new_string(helper));
            json_object_object_add(internal, "return_type", native_primitive_type("void"));
            json_object_object_add(internal, "rust_serial_internal_encode", json_object_new_boolean(true));
            json_object_array_add(methods, internal);
        }
        free(helper);
    }
    /* The nested serializer uses each nested source type's private helper. */
    for (size_t i = 0; i < json_object_array_length(structures); i++) {
        json_object *structure = json_object_array_get_idx(structures, i);
        if (!native_bool(structure, "rust_serializable")) continue;
        json_object *fields = native_record_child(structure, "fields");
        for (size_t f = 0; f < json_object_array_length(fields); f++) {
            json_object *field = json_object_array_get_idx(fields, f);
            const char *name = native_string(field, "serial_struct_name");
            if (!name) name = native_string(field, "serial_elem_struct_name");
            json_object *nested = native_record_struct(model, name);
            if (nested && native_record_child(nested, "rust_serial_encode_impl"))
                json_object_object_add(field, "rust_serial_encode_impl", json_object_get(native_record_child(nested, "rust_serial_encode_impl")));
        }
    }
    native_serial_rust_calls(model, model);
    return true;
}
