/* Scalar/string value records use private C-layout wire types. Referenced
 * records keep the layout in source storage, so C observes the original address,
 * aliases and later mutations. Managed string fields own C allocations. */
static json_object *native_record_child(json_object *node, const char *key)
{
    json_object *child = NULL;
    return node && json_object_object_get_ex(node, key, &child) ? child : NULL;
}

static json_object *native_record_struct(json_object *model, const char *name)
{
    json_object *structs = native_record_child(model, "structs");
    for (size_t i = 0; structs && i < json_object_array_length(structs); i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        const char *candidate = native_string(structure, "name");
        if (name && candidate && strcmp(name, candidate) == 0) return structure;
    }
    return NULL;
}

static bool native_record_type_supported(json_object *model, json_object *type,
                                          json_object *visiting)
{
    if (native_scalar_kind(native_string(type, "kind"), false) ||
        (native_string(type, "kind") && strcmp(native_string(type, "kind"), "string") == 0)) return true;
    if (!native_string(type, "kind") || strcmp(native_string(type, "kind"), "struct") != 0)
        return false;
    const char *name = native_string(type, "name");
    json_object *structure = native_record_struct(model, name);
    json_object *seen = NULL;
    const char *mode = native_string(structure, "mem_mode");
    if (!structure || !name || (mode && strcmp(mode, "val") != 0) ||
        native_bool(structure, "pass_self_by_ref") ||
        native_bool(structure, "is_packed") ||
        native_bool(structure, "is_serializable") ||
        native_bool(structure, "has_user_copy_method") ||
        json_object_object_get_ex(visiting, name, &seen)) return false;
    json_object *fields = native_record_child(structure, "fields");
    if (!fields || !json_object_array_length(fields)) return false;
    json_object_object_add(visiting, name, json_object_new_boolean(true));
    bool supported = true;
    for (size_t i = 0; supported && i < json_object_array_length(fields); i++)
        supported = native_record_type_supported(model, native_record_child(
            json_object_array_get_idx(fields, i), "type"), visiting);
    json_object_object_del(visiting, name);
    return supported;
}

static bool native_record_is_supported(json_object *model, json_object *type)
{
    if (!native_string(type, "kind") || strcmp(native_string(type, "kind"), "struct") != 0)
        return false;
    json_object *visiting = json_object_new_object();
    bool result = visiting && native_record_type_supported(model, type, visiting);
    if (visiting) json_object_put(visiting);
    return result;
}

static bool native_record_register(json_object *model, json_object *type,
                                    json_object *records)
{
    if (!native_string(type, "kind") || strcmp(native_string(type, "kind"), "struct") != 0)
        return true;
    const char *name = native_string(type, "name");
    for (size_t i = 0; i < json_object_array_length(records); i++)
        if (strcmp(native_string(json_object_array_get_idx(records, i), "source_name"), name) == 0)
            return true;
    json_object *structure = native_record_struct(model, name);
    json_object *record = json_object_new_object();
    char stem[96];
    snprintf(stem, sizeof(stem), "__SnNativeRecord_%zu", json_object_array_length(records));
    char *wire = unique_private_name(native_record_child(model, "functions"),
        native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
    if (!record || !wire) { if (record) json_object_put(record); free(wire); return false; }
    json_object *source_name = json_object_new_string(name);
    json_object *wire_name = json_object_new_string(wire);
    json_object *fields_copy = deep_copy(native_record_child(structure, "fields"));
    if (!source_name || !wire_name || !fields_copy)
    {
        if (source_name) json_object_put(source_name);
        if (wire_name) json_object_put(wire_name);
        if (fields_copy) json_object_put(fields_copy);
        json_object_put(record);
        free(wire);
        return false;
    }
    json_object_object_add(record, "source_name", source_name);
    json_object_object_add(record, "wire_name", wire_name);
    json_object_object_add(record, "fields", fields_copy);
    json_object_array_add(records, record);
    free(wire);
    json_object_object_add(structure, "rust_native_value_record", json_object_new_boolean(true));
    json_object *fields = native_record_child(structure, "fields");
    for (size_t i = 0; i < json_object_array_length(fields); i++)
        if (!native_record_register(model, native_record_child(json_object_array_get_idx(fields, i), "type"), records)) return false;
    return true;
}

static void native_record_reference_storage(json_object *model, json_object *type,
                                             json_object *records)
{
    if (!native_string(type, "kind") || strcmp(native_string(type, "kind"), "struct") != 0)
        return;
    const char *name = native_string(type, "name");
    json_object *structure = native_record_struct(model, name);
    if (native_bool(structure, "rust_native_record_storage")) return;
    json_object_object_add(structure, "rust_native_record_storage", json_object_new_boolean(true));
    json_object *fields = native_record_child(structure, "fields");
    for (size_t i = 0; i < json_object_array_length(fields); i++)
    {
        json_object *field = json_object_array_get_idx(fields, i);
        json_object *field_type = native_record_child(field, "type");
        if (native_string(field_type, "kind") && strcmp(native_string(field_type, "kind"), "char") == 0)
            json_object_object_add(field, "rust_native_c_char_storage", json_object_new_boolean(true));
        if (native_string(field_type, "kind") && strcmp(native_string(field_type, "kind"), "string") == 0)
            json_object_object_add(field, "rust_native_c_string_storage", json_object_new_boolean(true));
        native_record_reference_storage(model, field_type, records);
    }
    for (size_t i = 0; i < json_object_array_length(records); i++)
    {
        json_object *record = json_object_array_get_idx(records, i);
        if (strcmp(native_string(record, "source_name"), name) != 0) continue;
        json_object *wire_fields = native_record_child(record, "fields");
        for (size_t f = 0; f < json_object_array_length(fields); f++)
            if (native_bool(json_object_array_get_idx(fields, f), "rust_native_c_char_storage"))
                json_object_object_add(json_object_array_get_idx(wire_fields, f),
                    "rust_native_c_char_storage", json_object_new_boolean(true));
        for (size_t f = 0; f < json_object_array_length(fields); f++)
            if (native_bool(json_object_array_get_idx(fields, f), "rust_native_c_string_storage"))
                json_object_object_add(json_object_array_get_idx(wire_fields, f),
                    "rust_native_c_string_storage", json_object_new_boolean(true));
    }
}

static void native_record_mark_types(json_object *node, json_object *records)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            native_record_mark_types(json_object_array_get_idx(node, i), records);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    const char *kind = native_string(node, "kind"), *name = native_string(node, "name");
    if (kind && name && strcmp(kind, "struct") == 0)
        for (size_t i = 0; i < json_object_array_length(records); i++)
        {
            json_object *record = json_object_array_get_idx(records, i);
            if (strcmp(native_string(record, "source_name"), name) != 0) continue;
            json_object_object_add(node, "rust_native_record_wire", json_object_new_string(native_string(record, "wire_name")));
            if (native_bool(record, "rust_native_record_storage"))
                json_object_object_add(node, "rust_native_record_storage", json_object_new_boolean(true));
            if (native_string(record, "layout_check"))
                json_object_object_add(node, "rust_native_record_layout_check", json_object_new_string(native_string(record, "layout_check")));
            break;
        }
    json_object_object_foreach(node, key, child)
        if (strncmp(key, "rust_", 5) != 0) native_record_mark_types(child, records);
}

static bool native_prepare_records(json_object *model)
{
    json_object *records = json_object_new_array();
    if (!records) return false;
    json_object *functions = native_record_child(model, "functions");
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (!native_bool(function, "is_native")) continue;
        json_object *type = native_record_child(function, "return_type");
        if (native_record_is_supported(model, type) && !native_record_register(model, type, records)) goto fail;
        json_object *params = native_record_child(function, "params");
        for (size_t p = 0; params && p < json_object_array_length(params); p++)
        {
            type = native_record_child(json_object_array_get_idx(params, p), "type");
            if (native_record_is_supported(model, type) && !native_record_register(model, type, records)) goto fail;
        }
    }
    /* Select persistent source layouts only for records exposed by reference.
     * By-value-only records keep the existing source representation. */
    for (size_t i = 0; functions && i < json_object_array_length(functions); i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        if (!native_bool(function, "is_native")) continue;
        json_object *params = native_record_child(function, "params");
        for (size_t p = 0; params && p < json_object_array_length(params); p++)
        {
            json_object *param = json_object_array_get_idx(params, p);
            json_object *type = native_record_child(param, "type");
            if (native_string(param, "mem_qual") && strcmp(native_string(param, "mem_qual"), "as_ref") == 0 &&
                native_record_is_supported(model, type)) native_record_reference_storage(model, type, records);
        }
    }
    /* Managed records always own C-compatible fields, including when a nested
     * record or a by-value native result first introduces the representation. */
    for (size_t i = 0; i < json_object_array_length(records); i++)
    {
        json_object *record = json_object_array_get_idx(records, i);
        json_object *structure = native_record_struct(model, native_string(record, "source_name"));
        if (native_bool(structure, "has_heap_fields"))
        {
            json_object *type = json_object_new_object();
            json_object_object_add(type, "kind", json_object_new_string("struct"));
            json_object_object_add(type, "name", json_object_new_string(native_string(record, "source_name")));
            native_record_reference_storage(model, type, records);
            json_object_put(type);
            char stem[96];
            snprintf(stem, sizeof(stem), "__sn_native_record_layout_%zu", i);
            char *check = unique_private_name(native_record_child(model, "functions"),
                native_record_child(model, "structs"), native_record_child(model, "globals"), stem);
            if (!check) goto fail;
            json_object_object_add(record, "layout_check", json_object_new_string(check));
            free(check);
            json_object_object_add(model, "rust_native_c_string_storage", json_object_new_boolean(true));
        }
        if (native_bool(structure, "rust_native_record_storage"))
            json_object_object_add(record, "rust_native_record_storage", json_object_new_boolean(true));
    }
    if (native_bool(model, "rust_native_c_string_storage"))
    {
        const char *keys[] = {"rust_native_c_string_type", "rust_native_c_string_dup", "rust_native_c_string_free", "rust_native_c_string_arg_trait", "rust_native_c_string_loan_type"};
        const char *stems[] = {"__SnNativeCString", "__sn_native_c_string_dup", "__sn_native_c_string_free", "__SnNativeCStringArg", "__SnNativeCStringLoan"};
        for (size_t i = 0; i < 5; i++)
        {
            char *name = unique_private_name(native_record_child(model, "functions"),
                native_record_child(model, "structs"), native_record_child(model, "globals"), stems[i]);
            if (!name) goto fail;
            json_object_object_add(model, keys[i], json_object_new_string(name));
            free(name);
        }
    }
    if (json_object_array_length(records))
    {
        native_record_mark_types(model, records);
        native_record_mark_types(records, records);
        json_object_object_add(model, "rust_native_records", records);
    }
    else json_object_put(records);
    return true;
fail:
    json_object_put(records);
    return false;
}
