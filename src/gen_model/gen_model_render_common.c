#include "gen_model_render_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* ---- Shared Helpers ---- */

char *helper_eq(json_object **params, int param_count, hbs_options_t *options)
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

char *helper_op_symbol(json_object **params, int param_count, hbs_options_t *options)
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

char *helper_count(json_object **params, int param_count, hbs_options_t *options)
{
    (void)options;
    if (param_count < 1 || !params[0]) return strdup("0");

    json_object *arr = params[0];
    if (!json_object_is_type(arr, json_type_array)) return strdup("0");

    char buf[32];
    snprintf(buf, sizeof(buf), "%zu", json_object_array_length(arr));
    return strdup(buf);
}

char *helper_return_label(json_object **params, int param_count, hbs_options_t *options)
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

/* ---- File I/O ---- */

char *render_read_file(const char *path)
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

static int register_partial_file(hbs_env_t *env, const char *file_path, const char *partial_name)
{
    char *source = render_read_file(file_path);
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

int render_register_partials_recursive(hbs_env_t *env, const char *base_dir,
                                       const char *rel_prefix)
{
    DIR *dir = opendir(base_dir);
    if (!dir) return 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            char new_prefix[512];
            if (rel_prefix[0]) {
                snprintf(new_prefix, sizeof(new_prefix), "%s_%s", rel_prefix, entry->d_name);
            } else {
                snprintf(new_prefix, sizeof(new_prefix), "%s", entry->d_name);
            }
            if (render_register_partials_recursive(env, full_path, new_prefix) != 0) {
                closedir(dir);
                return -1;
            }
        } else if (S_ISREG(st.st_mode)) {
            const char *ext = strrchr(entry->d_name, '.');
            if (!ext || strcmp(ext, ".hbs") != 0) continue;

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

/* ---- Common Render Scaffold ---- */

char *render_with_helpers(json_object *model, const char *template_dir,
                          register_helpers_fn register_fn, const char *backend_name)
{
    if (!model || !template_dir) return NULL;

    hbs_env_t *env = hbs_env_create();
    if (!env) {
        fprintf(stderr, "gen_model_render_%s: failed to create handlebars environment\n", backend_name);
        return NULL;
    }

    hbs_env_set_no_escape(env, true);

    /* Register backend-specific helpers */
    register_fn(env);

    /* Register partials */
    char partials_dir[1024];
    snprintf(partials_dir, sizeof(partials_dir), "%s/partials", template_dir);
    if (render_register_partials_recursive(env, partials_dir, "") != 0) {
        hbs_env_destroy(env);
        return NULL;
    }

    /* Read and compile main template */
    char module_path[1024];
    snprintf(module_path, sizeof(module_path), "%s/module.hbs", template_dir);

    char *module_source = render_read_file(module_path);
    if (!module_source) {
        fprintf(stderr, "gen_model_render_%s: cannot read module template: %s\n", backend_name, module_path);
        hbs_env_destroy(env);
        return NULL;
    }

    hbs_error_t err;
    hbs_template_t *tmpl = hbs_compile(env, module_source, &err);
    free(module_source);

    if (!tmpl) {
        fprintf(stderr, "gen_model_render_%s: failed to compile module template: %s\n",
                backend_name, hbs_error_string(err));
        hbs_env_destroy(env);
        return NULL;
    }

    char *result = hbs_render(tmpl, model, &err);
    if (!result) {
        const char *detail = hbs_render_error_message(tmpl);
        fprintf(stderr, "gen_model_render_%s: render failed: %s\n",
                backend_name, detail ? detail : hbs_error_string(err));
    }

    hbs_template_destroy(tmpl);
    hbs_env_destroy(env);

    return result;
}
