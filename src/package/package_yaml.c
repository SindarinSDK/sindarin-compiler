/* ==============================================================================
 * package_yaml.c - YAML Parsing/Writing for Package Manager
 * ==============================================================================
 * Handles reading and writing sn.yaml files using libyaml.
 * ============================================================================== */

#include "../package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "colors.h"

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static void safe_strncpy(char *dest, const char *src, size_t size)
{
    if (src == NULL) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

/* ============================================================================
 * YAML Parsing
 * ============================================================================ */

static bool yaml_config_error(const char *path, const char *message)
{
    fprintf(stderr, "%serror%s: %s: %s\n", COLOR_RED, COLOR_RESET, path, message);
    return false;
}

/* Read direct mapping entries only. Nested extension fields must never become
 * root fields (especially runtime), nor corrupt dependency parsing. */
static bool parse_dependency(yaml_document_t *document, yaml_node_t *node,
                             PackageDependency *dep, const char *path)
{
    if (node->type != YAML_MAPPING_NODE)
        return yaml_config_error(path, "each dependency must be a mapping");

    for (yaml_node_pair_t *pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(document, pair->key);
        yaml_node_t *value = yaml_document_get_node(document, pair->value);
        if (key->type != YAML_SCALAR_NODE)
            return yaml_config_error(path, "dependency keys must be scalars");
        const char *name = (const char *)key->data.scalar.value;
        if (strlen(name) != key->data.scalar.length)
            return yaml_config_error(path, "YAML keys must not contain NUL bytes");
        char *dest = NULL;
        size_t size = 0;
        if (strcmp(name, "name") == 0) { dest = dep->name; size = sizeof(dep->name); }
        else if (strcmp(name, "git") == 0) { dest = dep->git_url; size = sizeof(dep->git_url); }
        else if (strcmp(name, "tag") == 0) { dest = dep->tag; size = sizeof(dep->tag); }
        else if (strcmp(name, "branch") == 0) { dest = dep->branch; size = sizeof(dep->branch); }
        if (dest) {
            if (value->type != YAML_SCALAR_NODE)
                return yaml_config_error(path, "dependency fields must be scalars");
            safe_strncpy(dest, (const char *)value->data.scalar.value, size);
        }
    }
    return true;
}

static bool parse_config_document(yaml_document_t *document, PackageConfig *config,
                                  const char *path)
{
    yaml_node_t *root = yaml_document_get_root_node(document);
    if (!root) return true; /* Preserve empty legacy manifests. */
    if (root->type != YAML_MAPPING_NODE)
        return yaml_config_error(path, "package manifest must be a mapping");
    bool runtime_seen = false;
    for (yaml_node_pair_t *pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(document, pair->key);
        yaml_node_t *value = yaml_document_get_node(document, pair->value);
        if (key->type != YAML_SCALAR_NODE)
            return yaml_config_error(path, "manifest keys must be scalars");
        const char *name = (const char *)key->data.scalar.value;
        if (strlen(name) != key->data.scalar.length)
            return yaml_config_error(path, "YAML keys must not contain NUL bytes");
        if (strcmp(name, "runtime") == 0) {
            if (runtime_seen)
                return yaml_config_error(path, "duplicate runtime field");
            runtime_seen = true;
            if (value->type != YAML_SCALAR_NODE)
                return yaml_config_error(path, "runtime must be C, RS or GO");
            const char *spelling = (const char *)value->data.scalar.value;
            if (strlen(spelling) != value->data.scalar.length)
                return yaml_config_error(path, "runtime must be C, RS or GO");
            if (strcmp(spelling, "C") == 0) config->runtime = PACKAGE_RUNTIME_C;
            else if (strcmp(spelling, "RS") == 0) config->runtime = PACKAGE_RUNTIME_RS;
            else if (strcmp(spelling, "GO") == 0) config->runtime = PACKAGE_RUNTIME_GO;
            else return yaml_config_error(path, "runtime must be C, RS or GO");
        } else if (strcmp(name, "dependencies") == 0) {
            if (value->type != YAML_SEQUENCE_NODE)
                return yaml_config_error(path, "dependencies must be a sequence");
            for (yaml_node_item_t *item = value->data.sequence.items.start;
                 item < value->data.sequence.items.top; item++) {
                if (config->dependency_count >= PKG_MAX_DEPS)
                    return yaml_config_error(path, "maximum number of dependencies reached");
                if (!parse_dependency(document, yaml_document_get_node(document, *item),
                                      &config->dependencies[config->dependency_count], path))
                    return false;
                config->dependency_count++;
            }
        } else {
            char *dest = NULL;
            size_t size = 0;
            if (strcmp(name, "name") == 0) { dest = config->name; size = sizeof(config->name); }
            else if (strcmp(name, "version") == 0) { dest = config->version; size = sizeof(config->version); }
            else if (strcmp(name, "author") == 0) { dest = config->author; size = sizeof(config->author); }
            else if (strcmp(name, "description") == 0) { dest = config->description; size = sizeof(config->description); }
            else if (strcmp(name, "license") == 0) { dest = config->license; size = sizeof(config->license); }
            if (dest) {
                if (value->type != YAML_SCALAR_NODE)
                    return yaml_config_error(path, "package metadata fields must be scalars");
                safe_strncpy(dest, (const char *)value->data.scalar.value, size);
            }
        }
    }
    return true;
}

bool package_yaml_parse(const char *path, PackageConfig *config)
{
    if (!path || !config) return false;
    memset(config, 0, sizeof(*config));
    FILE *f = fopen(path, "r");
    if (!f) return false;
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) { fclose(f); return false; }
    yaml_parser_set_input_file(&parser, f);
    yaml_document_t document;
    bool success = yaml_parser_load(&parser, &document) != 0;
    if (success) {
        success = parse_config_document(&document, config, path);
        yaml_document_delete(&document);
        /* A manifest is one document; never combine runtime declarations from
         * multiple YAML documents or overlook an invalid trailing document. */
        if (success) {
            success = yaml_parser_load(&parser, &document) != 0;
            if (success) {
                if (yaml_document_get_root_node(&document))
                    success = yaml_config_error(path, "manifest must contain one YAML document");
                yaml_document_delete(&document);
            }
        }
    }
    if (!success && parser.problem)
        yaml_config_error(path, parser.problem);
    yaml_parser_delete(&parser);
    fclose(f);
    if (!success) memset(config, 0, sizeof(*config));
    return success;
}

/* ============================================================================
 * YAML Writing
 * ============================================================================ */

/* Helper to emit a scalar */
static bool emit_scalar(yaml_emitter_t *emitter, const char *value)
{
    yaml_event_t event;
    yaml_scalar_event_initialize(&event, NULL, NULL,
        (yaml_char_t *)(value ? value : ""), (int)(value ? strlen(value) : 0),
        1, 1, YAML_ANY_SCALAR_STYLE);

    if (!yaml_emitter_emit(emitter, &event)) {
        return false;
    }
    return true;
}

/* Helper to emit a key-value pair */
static bool emit_key_value(yaml_emitter_t *emitter, const char *key, const char *value)
{
    if (!emit_scalar(emitter, key)) return false;
    if (!emit_scalar(emitter, value)) return false;
    return true;
}

bool package_yaml_write(const char *path, const PackageConfig *config)
{
    if (path == NULL || config == NULL) {
        return false;
    }

    if ((config->runtime != PACKAGE_RUNTIME_INHERIT && !package_runtime_name(config->runtime)) ||
        config->dependency_count < 0 || config->dependency_count > PKG_MAX_DEPS) {
        return yaml_config_error(path, "invalid package configuration");
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return false;
    }

    yaml_emitter_t emitter;
    yaml_event_t event;

    if (!yaml_emitter_initialize(&emitter)) {
        fclose(f);
        return false;
    }

    yaml_emitter_set_output_file(&emitter, f);
    yaml_emitter_set_unicode(&emitter, 1);

    /* Stream start */
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    /* Document start */
    yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    /* Root mapping start */
    yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    /* Emit fields */
    if (config->name[0]) {
        if (!emit_key_value(&emitter, "name", config->name)) goto error;
    }
    if (config->version[0]) {
        if (!emit_key_value(&emitter, "version", config->version)) goto error;
    }
    if (config->author[0]) {
        if (!emit_key_value(&emitter, "author", config->author)) goto error;
    }
    if (config->description[0]) {
        if (!emit_key_value(&emitter, "description", config->description)) goto error;
    }
    if (config->license[0]) {
        if (!emit_key_value(&emitter, "license", config->license)) goto error;
    }

    if (config->runtime != PACKAGE_RUNTIME_INHERIT) {
        if (!emit_key_value(&emitter, "runtime", package_runtime_name(config->runtime))) goto error;
    }

    /* Dependencies */
    if (config->dependency_count > 0) {
        if (!emit_scalar(&emitter, "dependencies")) goto error;

        /* Sequence start */
        yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;

        for (int i = 0; i < config->dependency_count; i++) {
            const PackageDependency *dep = &config->dependencies[i];

            /* Dependency mapping start */
            yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
            if (!yaml_emitter_emit(&emitter, &event)) goto error;

            if (dep->name[0]) {
                if (!emit_key_value(&emitter, "name", dep->name)) goto error;
            }
            if (dep->git_url[0]) {
                if (!emit_key_value(&emitter, "git", dep->git_url)) goto error;
            }
            if (dep->tag[0]) {
                if (!emit_key_value(&emitter, "tag", dep->tag)) goto error;
            }
            if (dep->branch[0]) {
                if (!emit_key_value(&emitter, "branch", dep->branch)) goto error;
            }

            /* Dependency mapping end */
            yaml_mapping_end_event_initialize(&event);
            if (!yaml_emitter_emit(&emitter, &event)) goto error;
        }

        /* Sequence end */
        yaml_sequence_end_event_initialize(&event);
        if (!yaml_emitter_emit(&emitter, &event)) goto error;
    }

    /* Root mapping end */
    yaml_mapping_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    /* Document end */
    yaml_document_end_event_initialize(&event, 1);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    /* Stream end */
    yaml_stream_end_event_initialize(&event);
    if (!yaml_emitter_emit(&emitter, &event)) goto error;

    yaml_emitter_delete(&emitter);
    fclose(f);
    return true;

error:
    yaml_emitter_delete(&emitter);
    fclose(f);
    return false;
}

bool package_yaml_add_dependency(const char *path, const PackageDependency *dep)
{
    if (path == NULL || dep == NULL) {
        return false;
    }

    /* Parse existing config */
    PackageConfig config;
    if (!package_yaml_parse(path, &config)) {
        return false;
    }

    /* Check if dependency already exists */
    for (int i = 0; i < config.dependency_count; i++) {
        if (strcmp(config.dependencies[i].name, dep->name) == 0) {
            /* Update existing dependency */
            config.dependencies[i] = *dep;
            return package_yaml_write(path, &config);
        }
    }

    /* Add new dependency */
    if (config.dependency_count >= PKG_MAX_DEPS) {
        fprintf(stderr, "%serror%s: maximum number of dependencies reached\n",
                COLOR_RED, COLOR_RESET);
        return false;
    }

    config.dependencies[config.dependency_count] = *dep;
    config.dependency_count++;

    return package_yaml_write(path, &config);
}
