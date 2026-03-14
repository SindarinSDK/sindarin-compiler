#include "cgen/gen_model_render.h"
#include "cgen/gen_model_render_internal.h"
#include "cgen/gen_model_split.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Forward declarations for min_c helpers (defined in c_min/gen_model_render_min_c.c) ---- */
/* We re-use the same helper registration function via a wrapper. */

/* This file uses render_with_template() instead of render_with_helpers() to support
 * rendering with a named template file instead of always using module.hbs */

static char *render_template_with_helpers(json_object *model, const char *template_dir,
                                           const char *template_name,
                                           register_helpers_fn register_fn,
                                           const char *backend_name)
{
    if (!model || !template_dir || !template_name) return NULL;

    hbs_env_t *env = hbs_env_create();
    if (!env)
    {
        fprintf(stderr, "gen_model_render_%s: failed to create handlebars environment\n", backend_name);
        return NULL;
    }

    hbs_env_set_no_escape(env, true);

    /* Register backend-specific helpers */
    register_fn(env);

    /* Register partials */
    char partials_dir[1024];
    snprintf(partials_dir, sizeof(partials_dir), "%s/partials", template_dir);
    if (render_register_partials_recursive(env, partials_dir, "") != 0)
    {
        hbs_env_destroy(env);
        return NULL;
    }

    /* Read and compile the named template */
    char template_path[1024];
    snprintf(template_path, sizeof(template_path), "%s/%s", template_dir, template_name);

    char *template_source = render_read_file(template_path);
    if (!template_source)
    {
        fprintf(stderr, "gen_model_render_%s: cannot read template: %s\n", backend_name, template_path);
        hbs_env_destroy(env);
        return NULL;
    }

    hbs_error_t err;
    hbs_template_t *tmpl = hbs_compile(env, template_source, &err);
    free(template_source);

    if (!tmpl)
    {
        fprintf(stderr, "gen_model_render_%s: failed to compile template: %s\n",
                backend_name, hbs_error_string(err));
        hbs_env_destroy(env);
        return NULL;
    }

    char *result = hbs_render(tmpl, model, &err);
    if (!result)
    {
        const char *detail = hbs_render_error_message(tmpl);
        fprintf(stderr, "gen_model_render_%s: render failed: %s\n",
                backend_name, detail ? detail : hbs_error_string(err));
    }

    hbs_template_destroy(tmpl);
    hbs_env_destroy(env);

    return result;
}

/* ---- Public API ---- */

ModularRenderResult *gen_model_render_modular_min_c(ModularModel *model,
                                                      const char *template_dir,
                                                      register_helpers_fn register_fn)
{
    if (!model || !template_dir) return NULL;

    ModularRenderResult *r = calloc(1, sizeof(ModularRenderResult));
    if (!r) return NULL;

    /* Render common header */
    r->header_code = render_template_with_helpers(model->common_header,
                                                    template_dir,
                                                    "common_header.hbs",
                                                    register_fn,
                                                    "modular_min_c");
    if (!r->header_code)
    {
        fprintf(stderr, "Error: failed to render common header\n");
        modular_render_result_free(r);
        return NULL;
    }

    /* Render each implementation module */
    r->impl_count = model->impl_count;
    r->impl_codes = calloc(model->impl_count, sizeof(char *));
    r->impl_names = calloc(model->impl_count, sizeof(const char *));

    for (int i = 0; i < model->impl_count; i++)
    {
        r->impl_names[i] = strdup(model->impl_names[i]);
        r->impl_codes[i] = render_template_with_helpers(model->impl_models[i],
                                                          template_dir,
                                                          "module_impl.hbs",
                                                          register_fn,
                                                          "modular_min_c");
        if (!r->impl_codes[i])
        {
            fprintf(stderr, "Error: failed to render module '%s'\n", model->impl_names[i]);
            modular_render_result_free(r);
            return NULL;
        }
    }

    return r;
}

void modular_render_result_free(ModularRenderResult *r)
{
    if (!r) return;

    free(r->header_code);

    for (int i = 0; i < r->impl_count; i++)
    {
        free(r->impl_codes[i]);
        free((void *)r->impl_names[i]);
    }
    free(r->impl_codes);
    free((void *)r->impl_names);
    free(r);
}
