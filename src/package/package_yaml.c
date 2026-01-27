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

/* Parser state for tracking position in YAML document */
typedef enum {
    PARSE_ROOT,
    PARSE_ROOT_KEY,
    PARSE_ROOT_VALUE,
    PARSE_DEPS_LIST,
    PARSE_DEP_MAP,
    PARSE_DEP_KEY,
    PARSE_DEP_VALUE
} ParseState;

bool package_yaml_parse(const char *path, PackageConfig *config)
{
    if (path == NULL || config == NULL) {
        return false;
    }

    memset(config, 0, sizeof(PackageConfig));

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return false;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        fclose(f);
        return false;
    }

    yaml_parser_set_input_file(&parser, f);

    ParseState state = PARSE_ROOT;
    char current_key[PKG_MAX_NAME_LEN] = {0};
    char dep_key[PKG_MAX_NAME_LEN] = {0};
    PackageDependency *current_dep = NULL;

    bool done = false;
    bool success = true;

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "YAML parse error: %s\n", parser.problem);
            success = false;
            break;
        }

        switch (event.type) {
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
                break;

            case YAML_STREAM_END_EVENT:
                done = true;
                break;

            case YAML_MAPPING_START_EVENT:
                if (state == PARSE_ROOT) {
                    state = PARSE_ROOT_KEY;
                } else if (state == PARSE_DEPS_LIST) {
                    /* Start of a dependency mapping */
                    if (config->dependency_count < PKG_MAX_DEPS) {
                        current_dep = &config->dependencies[config->dependency_count];
                        memset(current_dep, 0, sizeof(PackageDependency));
                        state = PARSE_DEP_KEY;
                    }
                }
                break;

            case YAML_MAPPING_END_EVENT:
                if (state == PARSE_DEP_KEY || state == PARSE_DEP_VALUE) {
                    /* End of dependency mapping */
                    if (current_dep != NULL) {
                        config->dependency_count++;
                        current_dep = NULL;
                    }
                    state = PARSE_DEPS_LIST;
                } else if (state == PARSE_ROOT_KEY || state == PARSE_ROOT_VALUE) {
                    state = PARSE_ROOT;
                }
                break;

            case YAML_SEQUENCE_START_EVENT:
                if (state == PARSE_ROOT_VALUE && strcmp(current_key, "dependencies") == 0) {
                    state = PARSE_DEPS_LIST;
                }
                break;

            case YAML_SEQUENCE_END_EVENT:
                if (state == PARSE_DEPS_LIST) {
                    state = PARSE_ROOT_KEY;
                }
                break;

            case YAML_SCALAR_EVENT: {
                const char *value = (const char *)event.data.scalar.value;

                if (state == PARSE_ROOT_KEY) {
                    safe_strncpy(current_key, value, sizeof(current_key));
                    state = PARSE_ROOT_VALUE;
                } else if (state == PARSE_ROOT_VALUE) {
                    if (strcmp(current_key, "name") == 0) {
                        safe_strncpy(config->name, value, sizeof(config->name));
                    } else if (strcmp(current_key, "version") == 0) {
                        safe_strncpy(config->version, value, sizeof(config->version));
                    } else if (strcmp(current_key, "author") == 0) {
                        safe_strncpy(config->author, value, sizeof(config->author));
                    } else if (strcmp(current_key, "description") == 0) {
                        safe_strncpy(config->description, value, sizeof(config->description));
                    } else if (strcmp(current_key, "license") == 0) {
                        safe_strncpy(config->license, value, sizeof(config->license));
                    }
                    state = PARSE_ROOT_KEY;
                } else if (state == PARSE_DEP_KEY) {
                    safe_strncpy(dep_key, value, sizeof(dep_key));
                    state = PARSE_DEP_VALUE;
                } else if (state == PARSE_DEP_VALUE && current_dep != NULL) {
                    if (strcmp(dep_key, "name") == 0) {
                        safe_strncpy(current_dep->name, value, sizeof(current_dep->name));
                    } else if (strcmp(dep_key, "git") == 0) {
                        safe_strncpy(current_dep->git_url, value, sizeof(current_dep->git_url));
                    } else if (strcmp(dep_key, "tag") == 0) {
                        safe_strncpy(current_dep->tag, value, sizeof(current_dep->tag));
                    } else if (strcmp(dep_key, "branch") == 0) {
                        safe_strncpy(current_dep->branch, value, sizeof(current_dep->branch));
                    }
                    state = PARSE_DEP_KEY;
                }
                break;
            }

            default:
                break;
        }

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(f);

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
        fprintf(stderr, "Error: Maximum number of dependencies reached\n");
        return false;
    }

    config.dependencies[config.dependency_count] = *dep;
    config.dependency_count++;

    return package_yaml_write(path, &config);
}
