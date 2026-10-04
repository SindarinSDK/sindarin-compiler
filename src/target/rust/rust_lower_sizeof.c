/* A declaration used only by C bodies or unevaluated sizeof operands needs
 * no Rust runtime representation. Scan the partitioned Rust roots first, then
 * close over fields and methods of every runtime-needed declaration. */
static void rust_sizeof_collect_runtime_types(json_object *node,
                                              json_object *needed)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_sizeof_collect_runtime_types(json_object_array_get_idx(node, i), needed);
        return;
    }
    if (!json_object_is_type(node, json_type_object) ||
        json_string_property_equals(node, "kind", "sizeof")) return;
    if (json_string_property_equals(node, "kind", "struct"))
    {
        const char *name = json_string_property(node, "name");
        if (name) json_object_object_add(needed, name, json_object_new_boolean(true));
        return; /* Field metadata is duplicated; visit the declaration instead. */
    }
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0)
            rust_sizeof_collect_runtime_types(value, needed);
    }
}

static void rust_sizeof_collect_layout_types(json_object *node,
                                             json_object *queried)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_sizeof_collect_layout_types(json_object_array_get_idx(node, i), queried);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    if (json_string_property_equals(node, "kind", "sizeof"))
    {
        json_object *type = NULL;
        if (json_object_object_get_ex(node, "target_type", &type))
            rust_sizeof_collect_runtime_types(type, queried);
        return;
    }
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0)
            rust_sizeof_collect_layout_types(value, queried);
    }
}

static void rust_prepare_sizeof_only_declarations(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return;
    json_object *needed = json_object_new_object(), *queried = json_object_new_object();
    json_object_object_foreach(model, key, value)
    {
        if (strcmp(key, "structs") != 0 && strncmp(key, "rust_", 5) != 0)
        {
            rust_sizeof_collect_runtime_types(value, needed);
            rust_sizeof_collect_layout_types(value, queried);
        }
    }
    int prior;
    do
    {
        prior = json_object_object_length(needed);
        for (size_t i = 0; i < json_object_array_length(structs); i++)
        {
            json_object *structure = json_object_array_get_idx(structs, i), *found = NULL;
            const char *name = json_string_property(structure, "name");
            if (name && json_object_object_get_ex(needed, name, &found))
                rust_sizeof_collect_runtime_types(structure, needed);
        }
    } while (prior != json_object_object_length(needed));
    /* A layout-only outer value can contain further layout-only values. */
    do
    {
        prior = json_object_object_length(queried);
        for (size_t i = 0; i < json_object_array_length(structs); i++)
        {
            json_object *structure = json_object_array_get_idx(structs, i), *found = NULL;
            const char *name = json_string_property(structure, "name");
            json_object *fields = NULL;
            if (name && json_object_object_get_ex(queried, name, &found) &&
                json_object_object_get_ex(structure, "fields", &fields))
                rust_sizeof_collect_runtime_types(fields, queried);
        }
    } while (prior != json_object_object_length(queried));
    for (size_t i = 0; i < json_object_array_length(structs); i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i), *found = NULL;
        const char *name = json_string_property(structure, "name");
        const char *mem = json_string_property(structure, "mem_mode");
        if (name && !json_object_object_get_ex(needed, name, &found) &&
            (json_object_object_get_ex(queried, name, &found) ||
             json_boolean_property(structure, "is_native") ||
             json_boolean_property(structure, "is_packed") ||
             (mem && strcmp(mem, "val") != 0)))
            json_object_object_add(structure, "rust_c_layout_only", json_object_new_boolean(true));
    }
    json_object_put(needed);
    json_object_put(queried);
}

/* Private C layout types are never instantiated. Their sole purpose is to ask
 * the target for C sizeof, independent of the representations used by Rust
 * strings, arrays, characters and synchronized/reference-counted structs. */
static json_object *rust_sizeof_wire_type(json_object *model, json_object *type)
{
    const char *kind = json_string_property(type, "kind");
    if (!kind) return NULL;
    const char *wire_kind = kind;
    if (strcmp(kind, "char") == 0) wire_kind = "byte";
    else if (strcmp(kind, "string") == 0 || strcmp(kind, "array") == 0 ||
             strcmp(kind, "pointer") == 0 || strcmp(kind, "opaque") == 0 ||
             strcmp(kind, "function") == 0 || strcmp(kind, "interface") == 0 ||
             (strcmp(kind, "struct") == 0 &&
              json_boolean_property(type, "pass_self_by_ref")))
        wire_kind = "opaque";
    json_object *wire = json_object_new_object();
    json_object_object_add(wire, "kind", json_object_new_string(wire_kind));
    if (strcmp(wire_kind, "struct") != 0) return wire;

    const char *source_name = json_string_property(type, "name");
    json_object *layouts = NULL;
    if (!source_name) goto fail;
    if (!json_object_object_get_ex(model, "rust_sizeof_layouts", &layouts))
    {
        layouts = json_object_new_array();
        json_object_object_add(model, "rust_sizeof_layouts", layouts);
    }
    for (size_t i = 0; i < json_object_array_length(layouts); i++)
    {
        json_object *layout = json_object_array_get_idx(layouts, i);
        if (!json_string_property_equals(layout, "source_name", source_name)) continue;
        if (json_boolean_property(layout, "building")) goto fail;
        json_object_object_add(wire, "name", json_object_new_string(
            json_string_property(layout, "name")));
        return wire;
    }

    char name[96];
    if (!rust_allocate_helper_name(model, "__sn_c_sizeof_layout", name,
                                   sizeof(name))) goto fail;
    json_object *layout = json_object_new_object(), *fields = json_object_new_array();
    json_object_object_add(layout, "source_name", json_object_new_string(source_name));
    json_object_object_add(layout, "name", json_object_new_string(name));
    json_object_object_add(layout, "packed", json_object_new_boolean(
        json_boolean_property(type, "is_packed")));
    json_object_object_add(layout, "fields", fields);
    json_object_object_add(layout, "building", json_object_new_boolean(true));
    json_object_array_add(layouts, layout);
    json_object *source_fields = NULL;
    if (!json_object_object_get_ex(type, "fields", &source_fields)) goto fail;
    for (size_t i = 0; i < json_object_array_length(source_fields); i++)
    {
        json_object *field_type = NULL;
        if (!json_object_object_get_ex(json_object_array_get_idx(source_fields, i),
                                       "type", &field_type)) goto fail;
        json_object *field_wire = rust_sizeof_wire_type(model, field_type);
        if (!field_wire) goto fail;
        json_object *field = json_object_new_object();
        char field_name[48];
        snprintf(field_name, sizeof(field_name), "field_%zu", i);
        json_object_object_add(field, "name", json_object_new_string(field_name));
        json_object_object_add(field, "type", field_wire);
        json_object_array_add(fields, field);
    }
    json_object_object_del(layout, "building");
    json_object_object_add(wire, "name", json_object_new_string(name));
    return wire;

fail:
    json_object_put(wire);
    return NULL;
}

static bool rust_lower_sizeof_layouts(json_object *model, json_object *node)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_lower_sizeof_layouts(model, json_object_array_get_idx(node, i)))
                return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_string_property_equals(node, "kind", "sizeof"))
    {
        /* Do not visit the opaque operand, even if it contains callbacks or
         * unsupported runtime representations. C emits only the resolved type. */
        if (!json_boolean_property(node, "rust_sizeof_c_layout")) return true;
        json_object *type = NULL;
        if (!json_object_object_get_ex(node, "target_type", &type)) return false;
        json_object *wire = rust_sizeof_wire_type(model, type);
        if (!wire) return false;
        json_object_object_add(node, "rust_sizeof_layout_type", wire);
        return true;
    }
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0 &&
            !rust_lower_sizeof_layouts(model, value)) return false;
    }
    return true;
}
