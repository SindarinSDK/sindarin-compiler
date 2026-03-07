#include "gen_model_render.h"
#include "handlebars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* ---- Custom Helpers ---- */

/* eq helper: {{#if (eq a b)}} - compares two values for equality */
static char *helper_eq(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 2 || !params[0] || !params[1]) {
        return strdup("");
    }

    const char *a = json_object_get_string(params[0]);
    const char *b = json_object_get_string(params[1]);

    if (a && b && strcmp(a, b) == 0) {
        json_object *t = json_object_new_boolean(1);
        char *s = strdup(json_object_get_string(t));
        json_object_put(t);
        return s;
    }

    return strdup("");
}

/* c_type helper: {{c_type type_obj}} - converts a type JSON object to a C type string */
static char *helper_c_type(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) {
        return strdup("void");
    }

    json_object *type_obj = params[0];

    /* If it's a string, use it directly */
    if (json_object_is_type(type_obj, json_type_string)) {
        return strdup(json_object_get_string(type_obj));
    }

    /* If it's an object, look at the "kind" field */
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) {
        return strdup("void");
    }

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("void");

    if (strcmp(kind, "int") == 0) return strdup("int");
    if (strcmp(kind, "long") == 0) return strdup("long long");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");
    if (strcmp(kind, "bool") == 0) return strdup("int");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("unsigned char");
    if (strcmp(kind, "str") == 0) return strdup("RtHandleV2 *");
    if (strcmp(kind, "void") == 0) return strdup("void");

    return strdup("void");
}

/* ---- File I/O Utilities ---- */

/* Read entire file into a malloc'd string. Returns NULL on error. */
static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len < 0) { fclose(f); return NULL; }

    char *buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, (size_t)len, f);
    buf[read] = '\0';
    fclose(f);
    return buf;
}

/* ---- Partial Registration ---- */

/* Register a single .hbs file as a partial with the given name */
static int register_partial_file(hbs_env_t *env, const char *file_path, const char *partial_name)
{
    char *source = read_file(file_path);
    if (!source) {
        fprintf(stderr, "gen_model_render: cannot read template file: %s\n", file_path);
        return -1;
    }

    hbs_error_t err = hbs_register_partial(env, partial_name, source);
    free(source);

    if (err != HBS_OK) {
        fprintf(stderr, "gen_model_render: failed to register partial '%s': %s\n",
                partial_name, hbs_error_string(err));
        return -1;
    }
    return 0;
}

/* Recursively register all .hbs files in dir as partials.
 * Partial names are derived from relative paths with '/' replaced by '_'.
 * e.g., "partials/stmt/return.hbs" -> "stmt_return" */
static int register_partials_recursive(hbs_env_t *env, const char *base_dir,
                                        const char *rel_prefix)
{
    DIR *dir = opendir(base_dir);
    if (!dir) return 0; /* empty dir is not an error */

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            /* Recurse into subdirectory */
            char new_prefix[512];
            if (rel_prefix[0]) {
                snprintf(new_prefix, sizeof(new_prefix), "%s_%s", rel_prefix, entry->d_name);
            } else {
                snprintf(new_prefix, sizeof(new_prefix), "%s", entry->d_name);
            }
            if (register_partials_recursive(env, full_path, new_prefix) != 0) {
                closedir(dir);
                return -1;
            }
        } else if (S_ISREG(st.st_mode)) {
            /* Check for .hbs extension */
            const char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".hbs") != 0) continue;

            /* Build partial name: prefix_filename (without .hbs) */
            size_t name_len = (size_t)(ext - entry->d_name);
            char name_part[256];
            if (name_len >= sizeof(name_part)) name_len = sizeof(name_part) - 1;
            memcpy(name_part, entry->d_name, name_len);
            name_part[name_len] = '\0';

            char partial_name[512];
            if (rel_prefix[0]) {
                snprintf(partial_name, sizeof(partial_name), "%s_%s", rel_prefix, name_part);
            } else {
                snprintf(partial_name, sizeof(partial_name), "%s", name_part);
            }

            if (register_partial_file(env, full_path, partial_name) != 0) {
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

/* ---- Public API ---- */

char *gen_model_render_c(json_object *model, const char *template_dir)
{
    if (!model || !template_dir) return NULL;

    /* Create handlebars environment */
    hbs_env_t *env = hbs_env_create();
    if (!env) {
        fprintf(stderr, "gen_model_render: failed to create handlebars environment\n");
        return NULL;
    }

    /* Disable HTML escaping — we're generating C code */
    hbs_env_set_no_escape(env, true);

    /* Register custom helpers */
    hbs_register_helper(env, "eq", helper_eq);
    hbs_register_helper(env, "c_type", helper_c_type);

    /* Register partials from the partials/ subdirectory */
    char partials_dir[1024];
    snprintf(partials_dir, sizeof(partials_dir), "%s/partials", template_dir);
    if (register_partials_recursive(env, partials_dir, "") != 0) {
        hbs_env_destroy(env);
        return NULL;
    }

    /* Read and compile the main module template */
    char module_path[1024];
    snprintf(module_path, sizeof(module_path), "%s/module.hbs", template_dir);

    char *module_source = read_file(module_path);
    if (!module_source) {
        fprintf(stderr, "gen_model_render: cannot read module template: %s\n", module_path);
        hbs_env_destroy(env);
        return NULL;
    }

    hbs_error_t err;
    hbs_template_t *tmpl = hbs_compile(env, module_source, &err);
    free(module_source);

    if (!tmpl) {
        fprintf(stderr, "gen_model_render: failed to compile module template: %s\n",
                hbs_error_string(err));
        hbs_env_destroy(env);
        return NULL;
    }

    /* Render the template with the JSON model as context */
    char *result = hbs_render(tmpl, model, &err);
    if (!result) {
        const char *detail = hbs_render_error_message(tmpl);
        fprintf(stderr, "gen_model_render: render failed: %s\n",
                detail ? detail : hbs_error_string(err));
    }

    hbs_template_destroy(tmpl);
    hbs_env_destroy(env);

    return result;
}
