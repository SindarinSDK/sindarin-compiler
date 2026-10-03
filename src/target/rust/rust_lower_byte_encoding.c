/* Included by rust_lower.c. Byte encoding reads the C array's logical length
 * from its actual element object representation, without numeric narrowing. */
static void rust_annotate_byte_encoding(json_object *node, const char *module, bool *used)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_annotate_byte_encoding(json_object_array_get_idx(node, i), module, used);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_annotate_byte_encoding(value, module, used);
    }
    json_object *callee = NULL, *object = NULL, *type = NULL, *element = NULL;
    if (!json_string_property_equals(node, "kind", "call") ||
        !json_object_object_get_ex(node, "callee", &callee) ||
        !json_string_property_equals(callee, "kind", "member") ||
        !json_object_object_get_ex(callee, "object", &object) ||
        !json_object_object_get_ex(object, "type", &type) ||
        !json_string_property_equals(type, "kind", "array") ||
        !json_object_object_get_ex(type, "element_type", &element) ||
        !json_string_property_equals(element, "kind", "byte")) return;
    const char *name = json_string_property(callee, "member_name"), *function = NULL;
    if (name && strcmp(name, "toHex") == 0) function = "hex";
    else if (name && strcmp(name, "toBase64") == 0) function = "base64";
    else if (name && strcmp(name, "toStringLatin1") == 0) function = "latin1";
    else if (name && strcmp(name, "toString") == 0) function = "string";
    if (!function) return;
    json_object_object_add(node, "rust_byte_encoding_module", json_object_new_string(module));
    json_object_object_add(node, "rust_byte_encoding_function", json_object_new_string(function));
    *used = true;
}

/* A readonly encoder parameter can borrow the original concrete element type.
 * Prove both its uses and absence of language callbacks/mutable array methods;
 * the general aliasing/mutation ABI still requires an owning runtime handle.
 * The proof never changes admission of other functions. */
static bool rust_byte_encoding_readonly(json_object *node, const char *name, bool *used)
{
    if (!node) return true;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            if (!rust_byte_encoding_readonly(json_object_array_get_idx(node, i), name, used)) return false;
        return true;
    }
    if (!json_object_is_type(node, json_type_object)) return true;
    if (json_string_property_equals(node, "kind", "variable") &&
        json_string_property_equals(node, "name", name)) return false;
    if (json_string_property_equals(node, "kind", "lambda") ||
        json_string_property_equals(node, "kind", "method_call") ||
        json_string_property_equals(node, "kind", "static_call")) return false;
    json_object *callee = NULL, *object = NULL;
    if (json_string_property_equals(node, "kind", "call"))
    {
        json_object_object_get_ex(node, "callee", &callee);
        json_object_object_get_ex(callee, "object", &object);
        bool encoding = json_string_property(node, "rust_byte_encoding_function") != NULL;
        if (encoding && json_string_property_equals(object, "kind", "variable") &&
            json_string_property_equals(object, "name", name))
        {
            *used = true;
            return true;
        }
        /* These scalar/string runtime methods cannot invoke language code. */
        if (!encoding && !json_string_property(node, "rust_character_method") &&
            !json_string_property(node, "rust_string_method")) return false;
    }
    if (json_string_property_equals(node, "kind", "member") &&
        json_string_property_equals(node, "member_name", "length") &&
        json_object_object_get_ex(node, "object", &object) &&
        json_string_property_equals(object, "kind", "variable") &&
        json_string_property_equals(object, "name", name)) return true;
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0 && !rust_byte_encoding_readonly(value, name, used)) return false;
    }
    return true;
}

static void rust_byte_encoding_parameters(json_object *node, const char *module)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        for (size_t i = 0; i < json_object_array_length(node); i++)
            rust_byte_encoding_parameters(json_object_array_get_idx(node, i), module);
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;
    json_object_object_foreach(node, key, value)
    {
        if (strncmp(key, "rust_", 5) != 0) rust_byte_encoding_parameters(value, module);
    }
    json_object *params = NULL, *body = NULL;
    if (json_boolean_property(node, "is_native") ||
        !json_object_object_get_ex(node, "params", &params) ||
        !json_object_object_get_ex(node, "body", &body)) return;
    for (size_t i = 0; i < json_object_array_length(params); i++)
    {
        json_object *param = json_object_array_get_idx(params, i), *type = NULL, *element = NULL;
        const char *name = json_string_property(param, "name");
        if (!name || !json_boolean_property(param, "rust_default_array_ref") ||
            !json_object_object_get_ex(param, "type", &type) ||
            !json_string_property_equals(type, "kind", "array") ||
            !json_object_object_get_ex(type, "element_type", &element) ||
            !json_string_property_equals(element, "kind", "byte")) continue;
        bool used = false;
        if (!rust_byte_encoding_readonly(body, name, &used) || !used) continue;
        json_object_object_add(param, "rust_byte_encoding_array_param", json_object_new_boolean(true));
        json_object_object_add(param, "rust_byte_encoding_module", json_object_new_string(module));
    }
}

static bool rust_lower_byte_encoding(json_object *model)
{
    char module[96];
    if (!rust_allocate_helper_name(model, "__sn_byte_encoding", module, sizeof(module))) return false;
    bool used = false;
    rust_annotate_byte_encoding(model, module, &used);
    if (used)
    {
        rust_byte_encoding_parameters(model, module);
        json_object_object_add(model, "rust_uses_byte_encoding", json_object_new_boolean(true));
        json_object_object_add(model, "rust_byte_encoding_module", json_object_new_string(module));
    }
    return true;
}
