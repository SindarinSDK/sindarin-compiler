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

    if (strcmp(kind, "int") == 0) return strdup("long long");
    if (strcmp(kind, "long") == 0) return strdup("long long");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");
    if (strcmp(kind, "bool") == 0) return strdup("bool");
    if (strcmp(kind, "char") == 0) return strdup("char");
    if (strcmp(kind, "byte") == 0) return strdup("unsigned char");
    if (strcmp(kind, "string") == 0) return strdup("RtHandleV2 *");
    if (strcmp(kind, "void") == 0) return strdup("void");
    if (strcmp(kind, "array") == 0) return strdup("RtHandleV2 *");

    /* Struct type: return __sn__<name> */
    if (strcmp(kind, "struct") == 0) {
        json_object *name_obj = NULL;
        if (json_object_object_get_ex(type_obj, "name", &name_obj)) {
            const char *sname = json_object_get_string(name_obj);
            if (sname) {
                char buf[256];
                snprintf(buf, sizeof(buf), "__sn__%s", sname);
                return strdup(buf);
            }
        }
    }

    return strdup("void");
}

/* default_value helper: {{default_value type_obj}} - returns the zero value for a type */
static char *helper_default_value(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("0");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("0");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("0");

    if (strcmp(kind, "int") == 0) return strdup("0");
    if (strcmp(kind, "long") == 0) return strdup("0");
    if (strcmp(kind, "double") == 0) return strdup("0.0");
    if (strcmp(kind, "float") == 0) return strdup("0.0f");
    if (strcmp(kind, "bool") == 0) return strdup("false");
    if (strcmp(kind, "char") == 0) return strdup("'\\0'");
    if (strcmp(kind, "byte") == 0) return strdup("0");
    if (strcmp(kind, "string") == 0) return strdup("NULL");
    if (strcmp(kind, "array") == 0) return strdup("NULL");
    if (strcmp(kind, "void") == 0) return strdup("");

    return strdup("0");
}

/* type_suffix helper: {{type_suffix type_obj}} - returns the runtime function suffix for a type */
static char *helper_type_suffix(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("long");

    json_object *type_obj = params[0];
    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &kind_obj)) return strdup("long");

    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return strdup("long");

    if (strcmp(kind, "int") == 0) return strdup("long");
    if (strcmp(kind, "long") == 0) return strdup("long");
    if (strcmp(kind, "double") == 0) return strdup("double");
    if (strcmp(kind, "float") == 0) return strdup("float");

    return strdup("long");
}

/* op_symbol helper: {{op_symbol "add"}} - returns the C operator symbol */
static char *helper_op_symbol(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("+");

    const char *op = json_object_get_string(params[0]);
    if (!op) return strdup("+");

    if (strcmp(op, "add") == 0) return strdup("+");
    if (strcmp(op, "subtract") == 0) return strdup("-");
    if (strcmp(op, "multiply") == 0) return strdup("*");
    if (strcmp(op, "divide") == 0) return strdup("/");
    if (strcmp(op, "modulo") == 0) return strdup("%");
    if (strcmp(op, "eq") == 0) return strdup("==");
    if (strcmp(op, "neq") == 0) return strdup("!=");
    if (strcmp(op, "lt") == 0) return strdup("<");
    if (strcmp(op, "gt") == 0) return strdup(">");
    if (strcmp(op, "lte") == 0) return strdup("<=");
    if (strcmp(op, "gte") == 0) return strdup(">=");
    if (strcmp(op, "and") == 0) return strdup("&&");
    if (strcmp(op, "or") == 0) return strdup("||");

    return strdup("+");
}

/* return_label helper: {{return_label}} - returns the goto label for the current function.
 * Walks up the frame stack to find the enclosing function context (has "name" + "return_type").
 * For main, returns "main_return"; for others, returns "__sn__<name>_return". */
static char *helper_return_label(json_object **params, int param_count, hbs_options_t *options)
{
    (void)params;
    (void)param_count;

    if (!options || !options->_internal) return strdup("main_return");

    /* Walk up the frame stack looking for a function context */
    typedef struct hbs_context_frame {
        json_object *data;
        json_object *private_data;
        bool owns_private_data;
        char **block_param_names;
        json_object **block_param_values;
        bool *block_param_owned;
        int block_param_count;
        void *partial_block;
        json_object *partial_block_context;
        void *inline_partials;
        struct hbs_context_frame *parent;
    } frame_t;

    typedef struct {
        void *env;
        frame_t *frame;
    } state_t;

    state_t *state = (state_t *)options->_internal;
    frame_t *frame = state->frame;

    while (frame) {
        if (frame->data && json_object_is_type(frame->data, json_type_object)) {
            json_object *name_obj = NULL;
            json_object *ret_type = NULL;
            if (json_object_object_get_ex(frame->data, "name", &name_obj) &&
                json_object_object_get_ex(frame->data, "return_type", &ret_type)) {
                const char *fn_name = json_object_get_string(name_obj);
                if (fn_name) {
                    if (strcmp(fn_name, "main") == 0) {
                        return strdup("main_return");
                    }
                    char buf[256];
                    snprintf(buf, sizeof(buf), "__sn__%s_return", fn_name);
                    return strdup(buf);
                }
            }
        }
        frame = frame->parent;
    }

    return strdup("main_return");
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
    hbs_register_helper(env, "default_value", helper_default_value);
    hbs_register_helper(env, "type_suffix", helper_type_suffix);
    hbs_register_helper(env, "op_symbol", helper_op_symbol);
    hbs_register_helper(env, "return_label", helper_return_label);

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
