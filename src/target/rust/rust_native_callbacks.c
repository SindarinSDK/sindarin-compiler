/* Function values crossing C need its real closure prefix and retain/release
 * protocol. Signature-specific Rust thunks keep the callable environment alive. */
static bool native_callback_type_supported(json_object *type)
{
    if (!native_string(type, "kind") || strcmp(native_string(type, "kind"), "function")) return false;
    json_object *result = NULL, *params = NULL, *quals = NULL;
    json_object_object_get_ex(type, "return_type", &result);
    json_object_object_get_ex(type, "param_types", &params);
    json_object_object_get_ex(type, "param_mem_quals", &quals);
    if (!native_scalar_type(result, true) && !native_string(result, "rust_native_record_wire") && !native_bool(result, "rust_native_handle_array") && (!native_string(result, "kind") || strcmp(native_string(result, "kind"), "string"))) return false;
    for (size_t i = 0; params && i < json_object_array_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i);
        if (!native_scalar_type(param, false) && !native_string(param, "rust_native_record_wire") && !native_bool(param, "rust_native_handle_array") && (!native_string(param, "kind") || strcmp(native_string(param, "kind"), "string"))) return false;
        const char *qual = quals ? json_object_get_string(json_object_array_get_idx(quals, i)) : NULL;
        if (qual && strcmp(qual, "default")) return false;
    }
    return true;
}

static bool native_prepare_callbacks(json_object *model, RustNativePlan *plan)
{
    json_object *functions = NULL, *structs = NULL, *globals = NULL;
    json_object_object_get_ex(model, "functions", &functions);
    json_object_object_get_ex(model, "structs", &structs);
    json_object_object_get_ex(model, "globals", &globals);
    json_object *callbacks = json_object_new_array();
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
    {
        json_object *function = json_object_array_get_idx(functions, i), *params = NULL;
        if (!native_bool(function, "is_native")) continue;
        json_object_object_get_ex(function, "params", &params);
        for (size_t p = 0; p <= (params ? json_object_array_length(params) : 0); p++)
        {
            bool result_slot = !params || p == json_object_array_length(params);
            json_object *param = result_slot ? function : json_object_array_get_idx(params, p), *type = NULL;
            json_object_object_get_ex(param, result_slot ? "return_type" : "type", &type);
            if (!native_callback_type_supported(type)) continue;
            json_object *callback_result = NULL, *callback_params = NULL;
            json_object_object_get_ex(type, "return_type", &callback_result);
            json_object_object_get_ex(type, "param_types", &callback_params);
            bool strings = native_string(callback_result, "kind") && !strcmp(native_string(callback_result, "kind"), "string");
            for (size_t a = 0; callback_params && a < json_object_array_length(callback_params); a++) {
                json_object *argument = json_object_array_get_idx(callback_params, a);
                strings |= native_string(argument, "kind") && !strcmp(native_string(argument, "kind"), "string");
            }
            json_object *result_element = NULL;
            json_object_object_get_ex(callback_result, "element_type", &result_element);
            strings |= native_string(result_element, "kind") && !strcmp(native_string(result_element, "kind"), "string");
            for (size_t a = 0; callback_params && a < json_object_array_length(callback_params); a++) {
                json_object *element = NULL;
                json_object_object_get_ex(json_object_array_get_idx(callback_params, a), "element_type", &element);
                strings |= native_string(element, "kind") && !strcmp(native_string(element, "kind"), "string");
            }
            if (strings) json_object_object_add(model, "rust_native_callback_strings", json_object_new_boolean(true));
            bool existing = false;
            for (size_t c = 0; c < json_object_array_length(callbacks); c++)
            {
                json_object *registered = json_object_array_get_idx(callbacks, c), *registered_type = NULL;
                json_object_object_get_ex(registered, "type", &registered_type);
                if (!json_object_equal(registered_type, type)) continue;
                if (result_slot) json_object_object_add(registered, "has_native_result", json_object_new_boolean(true));
                json_object_object_add(param, result_slot ? "rust_native_callback_result" : "rust_native_callback_name", json_object_new_string(native_string(registered, "name")));
                json_object_object_add(function, "rust_native_callback_bridge", json_object_new_boolean(true));
                existing = true; break;
            }
            if (existing) continue;
            char stem[96]; snprintf(stem, sizeof(stem), "__SnNativeCallback_%zu", json_object_array_length(callbacks));
            char *name = unique_private_name(functions, structs, globals, stem);
            if (!name) { json_object_put(callbacks); return false; }
            json_object *callback = json_object_new_object();
            json_object_object_add(callback, "name", json_object_new_string(name));
            json_object_object_add(callback, "type", json_object_get(type));
            if (result_slot) json_object_object_add(callback, "has_native_result", json_object_new_boolean(true));
            json_object_array_add(callbacks, callback);
            json_object_object_add(param, result_slot ? "rust_native_callback_result" : "rust_native_callback_name", json_object_new_string(name));
            json_object_object_add(function, "rust_native_callback_bridge", json_object_new_boolean(true));
            free(name);
        }
    }
    if (!json_object_array_length(callbacks)) { json_object_put(callbacks); return true; }
    const char *keys[] = {"rust_native_callback_alloc", "rust_native_callback_free", "rust_native_callback_header", "rust_native_reentry_cell", "rust_native_reentry_guard", "rust_native_reentry_lease", "rust_native_char_scope", "rust_native_char_wires", "rust_native_char_read", "rust_native_char_write", "rust_native_scalar_cell", "rust_native_scalar_trait", "rust_native_char_persistent", "rust_native_char_load", "rust_native_char_store", "rust_native_field_read_trait", "rust_native_char_guard", "rust_native_callable_owner", "rust_native_callback_retain", "rust_native_callback_release", "rust_native_callback_credit_lock", "rust_native_callback_credit_unlock", "rust_native_callback_string_dup"};
    const char *stems[] = {"__sn_callback_alloc", "__sn_callback_free", "__SnCallbackHeader", "__SnReentryCell", "__SnReentryGuard", "__SnNativeLease", "__SnCharWireScope", "__SN_CHAR_WIRES", "__sn_char_wire_read", "__sn_char_wire_write", "__SnNativeScalarCell", "__SnNativeScalarValue", "__SN_PERSISTENT_CHARS", "__sn_char_place_load", "__sn_char_place_store", "__SnNativeFieldRead", "__SnNativeCharGuard", "__SnOptionalCallable", "__sn_callback_retain", "__sn_callback_release", "__sn_callback_credit_lock", "__sn_callback_credit_unlock", "__sn_callback_string_dup"};
    for (size_t i = 0; i < 23; i++)
    {
        char *name = unique_private_name(functions, structs, globals, stems[i]);
        if (!name) { json_object_put(callbacks); return false; }
        json_object_object_add(model, keys[i], json_object_new_string(name)); free(name);
    }
    json_object_object_add(model, "rust_native_callbacks", callbacks);
    plan->callback_support = json_object_new_object();
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        json_object *value = NULL; json_object_object_get_ex(model, keys[i], &value);
        json_object_object_add(plan->callback_support, keys[i], json_object_get(value));
    }
    return true;
}
