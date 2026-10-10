/* Native manifest contract helpers; included by package_yaml.c. */

static json_object *native_yaml_json(yaml_document_t *document, yaml_node_t *node,
                                    const char *path, unsigned depth)
{
    if (depth > 16) { yaml_config_error(path, "native metadata nesting is too deep"); return NULL; }
    if (node->type == YAML_SCALAR_NODE) {
        if (strlen((const char *)node->data.scalar.value) != node->data.scalar.length) {
            yaml_config_error(path, "native metadata must not contain NUL bytes");
            return NULL;
        }
        return json_object_new_string((const char *)node->data.scalar.value);
    }
    json_object *result = node->type == YAML_MAPPING_NODE ? json_object_new_object()
                        : node->type == YAML_SEQUENCE_NODE ? json_object_new_array() : NULL;
    if (!result) return NULL;
    if (node->type == YAML_MAPPING_NODE) {
        for (yaml_node_pair_t *pair = node->data.mapping.pairs.start;
             pair < node->data.mapping.pairs.top; pair++) {
            yaml_node_t *key = yaml_document_get_node(document, pair->key);
            if (key->type != YAML_SCALAR_NODE ||
                strlen((const char *)key->data.scalar.value) != key->data.scalar.length) {
                yaml_config_error(path, "native metadata keys must be strings without NUL bytes");
                goto error;
            }
            const char *name = (const char *)key->data.scalar.value;
            json_object *previous = NULL;
            if (json_object_object_get_ex(result, name, &previous)) {
                yaml_config_error(path, "duplicate native metadata key");
                goto error;
            }
            json_object *value = native_yaml_json(document,
                yaml_document_get_node(document, pair->value), path, depth + 1);
            if (!value) goto error;
            json_object_object_add(result, name, value);
        }
    } else {
        for (yaml_node_item_t *item = node->data.sequence.items.start;
             item < node->data.sequence.items.top; item++) {
            json_object *value = native_yaml_json(document,
                yaml_document_get_node(document, *item), path, depth + 1);
            if (!value) goto error;
            json_object_array_add(result, value);
        }
    }
    return result;
error:
    json_object_put(result);
    return NULL;
}

static bool native_fields(json_object *object, const char *const *allowed, const char *path)
{
    if (!json_object_is_type(object, json_type_object))
        return yaml_config_error(path, "native contract sections must be mappings");
    json_object_object_foreach(object, key, value) {
        (void)value;
        bool found = false;
        for (int i = 0; allowed[i]; i++)
            if (strcmp(key, allowed[i]) == 0) { found = true; break; }
        if (!found) {
            char message[256];
            snprintf(message, sizeof(message), "unsupported native metadata field '%s'", key);
            return yaml_config_error(path, message);
        }
    }
    return true;
}

static const char *native_text(json_object *object, const char *key)
{
    json_object *value = NULL;
    if (!json_object_object_get_ex(object, key, &value) ||
        !json_object_is_type(value, json_type_string)) return NULL;
    const char *text = json_object_get_string(value);
    return text[0] ? text : NULL;
}

static bool native_list(json_object *object, const char *key, bool required, const char *path)
{
    json_object *value = NULL;
    if (!json_object_object_get_ex(object, key, &value))
        return !required || yaml_config_error(path, "required native input list is missing");
    if (!json_object_is_type(value, json_type_array) ||
        (required && json_object_array_length(value) == 0))
        return yaml_config_error(path, "native inputs must be string sequences");
    for (size_t i = 0; i < json_object_array_length(value); i++) {
        json_object *item = json_object_array_get_idx(value, i);
        if (!json_object_is_type(item, json_type_string) || !json_object_get_string(item)[0])
            return yaml_config_error(path, "native input paths/names must be nonempty strings");
    }
    return true;
}

static bool native_ownership_name(const char *name)
{
    return name && (strcmp(name, "value") == 0 || strcmp(name, "borrowed") == 0 ||
                    strcmp(name, "owned") == 0);
}

static bool validate_native_plan(json_object *plan, const char *path)
{
    static const char *const root_fields[] = {"abi", "declarations", "builds", "bindings", "assembly", NULL};
    static const char *const build_fields[] = {"name", "language", "sources", "entry", "module",
                                             "include_dirs", "libraries", NULL};
    static const char *const binding_fields[] = {"declaration", "build", "symbol", "function", "convention",
                                               "ownership", "failure", NULL};
    static const char *const ownership_fields[] = {"parameters", "result", "borrowed_from", NULL};
    if (!native_fields(plan, root_fields, path)) return false;
    const char *abi = native_text(plan, "abi");
    if (!abi || (strcmp(abi, "1.0") != 0 && strcmp(abi, "1.1") != 0 && strcmp(abi, "1.5") != 0))
        return yaml_config_error(path, "native abi must be 1.0, 1.1 or 1.5");
    json_object *assembly = NULL;
    if (json_object_object_get_ex(plan, "assembly", &assembly)) {
        static const char *const fields[] = {"path", "sha256", NULL};
        if (!native_fields(assembly, fields, path) || !native_text(assembly, "path"))
            return yaml_config_error(path, "native assembly requires a path and descriptor sha256");
        const char *digest = native_text(assembly, "sha256");
        if (!digest || strlen(digest) != 64)
            return yaml_config_error(path, "native assembly sha256 must contain 64 lowercase hexadecimal digits");
        for (int i = 0; i < 64; i++)
            if (!((digest[i] >= '0' && digest[i] <= '9') || (digest[i] >= 'a' && digest[i] <= 'f')))
                return yaml_config_error(path, "native assembly sha256 must contain 64 lowercase hexadecimal digits");
    }
    if (!native_list(plan, "declarations", true, path)) return false;
    json_object *builds = NULL, *bindings = NULL;
    if (!json_object_object_get_ex(plan, "builds", &builds) ||
        !json_object_is_type(builds, json_type_array) || !json_object_array_length(builds))
        return yaml_config_error(path, "native builds must be a nonempty sequence");
    for (size_t i = 0; i < json_object_array_length(builds); i++) {
        json_object *build = json_object_array_get_idx(builds, i);
        if (!native_fields(build, build_fields, path)) return false;
        const char *name = native_text(build, "name"), *language = native_text(build, "language");
        if (!name || !language || (strcmp(language, "C") != 0 &&
            strcmp(language, "RS") != 0 && strcmp(language, "GO") != 0 && strcmp(language, "SN") != 0))
            return yaml_config_error(path, "native build requires name and language C, RS, GO or SN");
        for (size_t j = 0; j < i; j++)
            if (strcmp(name, native_text(json_object_array_get_idx(builds, j), "name")) == 0)
                return yaml_config_error(path, "duplicate native build name");
        if (!native_list(build, "sources", true, path) ||
            !native_list(build, "include_dirs", false, path) ||
            !native_list(build, "libraries", false, path)) return false;
        json_object *entry = NULL, *module = NULL;
        bool has_entry = json_object_object_get_ex(build, "entry", &entry);
        bool has_module = json_object_object_get_ex(build, "module", &module);
        if (strcmp(language, "RS") == 0 || strcmp(language, "SN") == 0) {
            const char *root = native_text(build, "entry");
            if (!root || has_module)
                return yaml_config_error(path, "RS/SN build requires entry and cannot declare a Go module");
            json_object *sources = NULL;
            json_object_object_get_ex(build, "sources", &sources);
            bool found = false;
            for (size_t j = 0; j < json_object_array_length(sources); j++)
                if (strcmp(root, json_object_get_string(json_object_array_get_idx(sources, j))) == 0)
                    found = true;
            if (!found) return yaml_config_error(path, "RS/SN entry must appear in sources");
        } else if (strcmp(language, "GO") == 0) {
            if (!native_text(build, "module") || has_entry)
                return yaml_config_error(path, "GO build requires module and cannot declare an RS entry");
        } else if (has_entry || has_module)
            return yaml_config_error(path, "C build cannot declare an RS entry or Go module");
    }
    if (!json_object_object_get_ex(plan, "bindings", &bindings) ||
        !json_object_is_type(bindings, json_type_array) || !json_object_array_length(bindings))
        return yaml_config_error(path, "native bindings must be a nonempty sequence");
    for (size_t i = 0; i < json_object_array_length(bindings); i++) {
        json_object *binding = json_object_array_get_idx(bindings, i);
        if (!native_fields(binding, binding_fields, path)) return false;
        const char *declaration = native_text(binding, "declaration");
        const char *build_name = native_text(binding, "build");
        const char *symbol = native_text(binding, "symbol");
        const char *convention = native_text(binding, "convention");
        const char *failure = native_text(binding, "failure");
        json_object *function = NULL;
        if (json_object_object_get_ex(binding, "function", &function) && !native_text(binding, "function"))
            return yaml_config_error(path, "native function must be a nonempty backing function name");
        if (!declaration || !build_name || !symbol || !convention || strcmp(convention, "C") != 0 ||
            !failure || (strcmp(failure, "abort") != 0 && strcmp(failure, "status") != 0))
            return yaml_config_error(path, "native binding requires declaration/build/symbol, convention C and failure abort/status");
        bool found = false;
        for (size_t j = 0; j < json_object_array_length(builds); j++)
            if (strcmp(build_name, native_text(json_object_array_get_idx(builds, j), "name")) == 0)
                found = true;
        if (!found) return yaml_config_error(path, "native binding refers to an unknown build");
        for (size_t j = 0; j < i; j++)
            if (strcmp(declaration, native_text(json_object_array_get_idx(bindings, j), "declaration")) == 0)
                return yaml_config_error(path, "duplicate native binding declaration");
        json_object *ownership = NULL, *parameters = NULL;
        if (!json_object_object_get_ex(binding, "ownership", &ownership))
            return yaml_config_error(path, "native binding ownership is required");
        if (!native_fields(ownership, ownership_fields, path)) return false;
        const char *result = native_text(ownership, "result");
        if (!native_ownership_name(result) ||
            !json_object_object_get_ex(ownership, "parameters", &parameters) ||
            !json_object_is_type(parameters, json_type_object))
            return yaml_config_error(path, "native ownership requires parameters mapping and result value/borrowed/owned");
        json_object_object_foreach(parameters, key, value) {
            if (!key[0] || !json_object_is_type(value, json_type_string) ||
                !native_ownership_name(json_object_get_string(value)))
                return yaml_config_error(path, "native parameter ownership must be value, borrowed or owned");
        }
        json_object *owner = NULL;
        bool has_owner = json_object_object_get_ex(ownership, "borrowed_from", &owner);
        if (strcmp(result, "borrowed") == 0) {
            const char *source = native_text(ownership, "borrowed_from");
            json_object *parameter = NULL;
            if (!source || !json_object_object_get_ex(parameters, source, &parameter) ||
                strcmp(json_object_get_string(parameter), "borrowed") != 0)
                return yaml_config_error(path, "borrowed result requires borrowed_from naming a borrowed parameter");
        } else if (has_owner)
            return yaml_config_error(path, "borrowed_from is only valid for borrowed results");
    }
    return true;
}
