/* Semantic YAML edits preserve native contracts and unrecognized extensions. */
typedef struct {
    unsigned char *data;
    size_t length, capacity;
} PackageYamlBuffer;

static int yaml_edit_output(void *context, unsigned char *bytes, size_t size)
{
    PackageYamlBuffer *buffer = context;
    if (!size) return 1;
    if (size > SIZE_MAX - buffer->length) return 0;
    size_t needed = buffer->length + size;
    if (needed > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 1024;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2) { capacity = needed; break; }
            capacity *= 2;
        }
        void *data = realloc(buffer->data, capacity);
        if (!data) return 0;
        buffer->data = data;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->length, bytes, size);
    buffer->length += size;
    return 1;
}

static bool yaml_edit_pair(yaml_document_t *document, int mapping,
                           const char *key, const char *value)
{
    if (!value[0]) return true;
    int key_id = yaml_document_add_scalar(document, (yaml_char_t *)YAML_STR_TAG,
                    (yaml_char_t *)key, (int)strlen(key), YAML_PLAIN_SCALAR_STYLE);
    int value_id = yaml_document_add_scalar(document, (yaml_char_t *)YAML_STR_TAG,
                    (yaml_char_t *)value, (int)strlen(value), YAML_ANY_SCALAR_STYLE);
    return key_id && value_id && yaml_document_append_mapping_pair(document, mapping, key_id, value_id);
}

static bool yaml_edit_dependency(yaml_document_t *document, const PackageDependency *dep)
{
    yaml_node_t *root = yaml_document_get_root_node(document);
    int root_id = 1;
    if (!root) {
        root_id = yaml_document_add_mapping(document, (yaml_char_t *)YAML_MAP_TAG, YAML_BLOCK_MAPPING_STYLE);
        if (!root_id) return false;
        root = yaml_document_get_node(document, root_id);
    }
    int dependencies = 0, previous_mapping = 0;
    size_t replace_index = 0;
    for (yaml_node_pair_t *pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(document, pair->key);
        if (strcmp((const char *)key->data.scalar.value, "dependencies") == 0)
            dependencies = pair->value;
    }
    bool existing_dependencies = dependencies != 0;
    if (!dependencies) {
        int key = yaml_document_add_scalar(document, (yaml_char_t *)YAML_STR_TAG,
                      (yaml_char_t *)"dependencies", -1, YAML_PLAIN_SCALAR_STYLE);
        dependencies = yaml_document_add_sequence(document, (yaml_char_t *)YAML_SEQ_TAG, YAML_BLOCK_SEQUENCE_STYLE);
        if (!key || !dependencies || !yaml_document_append_mapping_pair(document, root_id, key, dependencies))
            return false;
    } else {
        yaml_node_t *sequence = yaml_document_get_node(document, dependencies);
        for (yaml_node_item_t *item = sequence->data.sequence.items.start;
             item < sequence->data.sequence.items.top && !previous_mapping; item++) {
            yaml_node_t *mapping = yaml_document_get_node(document, *item);
            for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start;
                 pair < mapping->data.mapping.pairs.top; pair++) {
                yaml_node_t *key = yaml_document_get_node(document, pair->key);
                yaml_node_t *value = yaml_document_get_node(document, pair->value);
                if (strcmp((const char *)key->data.scalar.value, "name") == 0 &&
                    strcmp((const char *)value->data.scalar.value, dep->name) == 0) {
                    previous_mapping = *item;
                    replace_index = (size_t)(item - sequence->data.sequence.items.start);
                    break;
                }
            }
        }
    }
    /* Fork an existing sequence before mutation. An extension may alias the
     * original dependency sequence; editing dependencies must not rewrite it. */
    if (existing_dependencies) {
        int original = dependencies;
        int copy = yaml_document_add_sequence(document, (yaml_char_t *)YAML_SEQ_TAG, YAML_BLOCK_SEQUENCE_STYLE);
        if (!copy) return false;
        yaml_node_t *sequence = yaml_document_get_node(document, original);
        size_t count = (size_t)(sequence->data.sequence.items.top - sequence->data.sequence.items.start);
        for (size_t i = 0; i < count; i++) {
            sequence = yaml_document_get_node(document, original);
            if (!yaml_document_append_sequence_item(document, copy, sequence->data.sequence.items.start[i]))
                return false;
        }
        root = yaml_document_get_node(document, root_id);
        for (yaml_node_pair_t *pair = root->data.mapping.pairs.start;
             pair < root->data.mapping.pairs.top; pair++) {
            yaml_node_t *key = yaml_document_get_node(document, pair->key);
            if (strcmp((const char *)key->data.scalar.value, "dependencies") == 0)
                pair->value = copy;
        }
        dependencies = copy;
    }
    int replacement = yaml_document_add_mapping(document, (yaml_char_t *)YAML_MAP_TAG, YAML_BLOCK_MAPPING_STYLE);
    if (!replacement) return false;
    if (previous_mapping) {
        /* Node additions can move document->nodes. Keep IDs, never cached node
         * pointers, across mutations. Unknown dependency fields remain intact. */
        yaml_node_t *mapping = yaml_document_get_node(document, previous_mapping);
        size_t count = (size_t)(mapping->data.mapping.pairs.top - mapping->data.mapping.pairs.start);
        for (size_t i = 0; i < count; i++) {
            mapping = yaml_document_get_node(document, previous_mapping);
            yaml_node_pair_t pair = mapping->data.mapping.pairs.start[i];
            const char *name = (const char *)yaml_document_get_node(document, pair.key)->data.scalar.value;
            if (strcmp(name, "name") && strcmp(name, "git") && strcmp(name, "tag") && strcmp(name, "branch"))
                if (!yaml_document_append_mapping_pair(document, replacement, pair.key, pair.value)) return false;
        }
    }
    if (!yaml_edit_pair(document, replacement, "name", dep->name) ||
        !yaml_edit_pair(document, replacement, "git", dep->git_url) ||
        !yaml_edit_pair(document, replacement, "tag", dep->tag) ||
        !yaml_edit_pair(document, replacement, "branch", dep->branch)) return false;
    if (previous_mapping) {
        yaml_node_t *sequence = yaml_document_get_node(document, dependencies);
        sequence->data.sequence.items.start[replace_index] = replacement;
        return true;
    }
    return yaml_document_append_sequence_item(document, dependencies, replacement) != 0;
}

bool package_yaml_add_dependency(const char *path, const PackageDependency *dep)
{
    if (!path || !dep) return false;
    PackageConfig config;
    if (!package_yaml_parse(path, &config)) return false;
    bool replaces = false;
    for (int i = 0; i < config.dependency_count; i++)
        if (strcmp(config.dependencies[i].name, dep->name) == 0) { replaces = true; break; }
    if (!replaces && config.dependency_count >= PKG_MAX_DEPS)
        return yaml_config_error(path, "maximum number of dependencies reached");
    FILE *file = fopen(path, "r");
    if (!file) return false;
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) { fclose(file); return false; }
    yaml_parser_set_input_file(&parser, file);
    yaml_document_t document;
    bool success = yaml_parser_load(&parser, &document) != 0;
    yaml_parser_delete(&parser);
    fclose(file);
    if (!success) return false;
    if (!yaml_edit_dependency(&document, dep)) { yaml_document_delete(&document); return false; }
    yaml_emitter_t emitter;
    if (!yaml_emitter_initialize(&emitter)) { yaml_document_delete(&document); return false; }
    PackageYamlBuffer buffer = {0};
    yaml_emitter_set_output(&emitter, yaml_edit_output, &buffer);
    yaml_emitter_set_unicode(&emitter, 1);
    success = yaml_emitter_open(&emitter) != 0;
    if (success) {
        /* dump takes ownership and deletes the document, including on failure. */
        success = yaml_emitter_dump(&emitter, &document) != 0;
        if (success) success = yaml_emitter_close(&emitter) != 0;
    } else yaml_document_delete(&document);
    yaml_emitter_delete(&emitter);
    /* Validate and serialize before opening the destination for replacement. */
    if (success) {
        file = fopen(path, "wb");
        if (!file) success = false;
        else {
            success = fwrite(buffer.data, 1, buffer.length, file) == buffer.length;
            if (fclose(file) != 0) success = false;
        }
    }
    free(buffer.data);
    return success;
}
