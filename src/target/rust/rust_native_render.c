#include "target/rust/rust_native_internal.h"
#include "cgen/gen_model_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool impl_has_native_body(json_object *impl)
{
    json_object *functions = NULL;
    if (!impl || !json_object_object_get_ex(impl, "functions", &functions))
        return false;
    size_t count = json_object_array_length(functions);
    for (size_t i = 0; i < count; i++)
    {
        json_object *function = json_object_array_get_idx(functions, i);
        json_object *is_native = NULL, *has_body = NULL;
        if (json_object_object_get_ex(function, "is_native", &is_native) &&
            json_object_get_boolean(is_native) &&
            json_object_object_get_ex(function, "has_body", &has_body) &&
            json_object_get_boolean(has_body))
            return true;
    }
    return false;
}

bool rust_native_emit_support(RustNativePlan *plan, GeneratedFileSet *files,
                              const char *compiler_dir)
{
    if (!rust_native_plan_has_work(plan)) return true;
    ModularModel *split = rust_native_plan_split(plan);
    if (!split || !files || !compiler_dir) return false;

    char template_dir[1024];
    int written = snprintf(template_dir, sizeof(template_dir),
                           "%s/templates/c", compiler_dir);
    if (written < 0 || (size_t)written >= sizeof(template_dir)) return false;

    ModularRenderResult *rendered = gen_model_render_modular_min_c(
        split, template_dir, gen_model_get_min_c_register_fn());
    if (!rendered)
    {
        fprintf(stderr, "Error: Rust target could not render native C support\n");
        return false;
    }

    char *header = rendered->header_code;
    rendered->header_code = NULL;
    if (!generated_file_set_add(files, "sn_types.h", header,
                                GENERATED_HEADER, false))
    {
        free(header);
        modular_render_result_free(rendered);
        return false;
    }

    for (int i = 0; i < rendered->impl_count; i++)
    {
        if (!impl_has_native_body(split->impl_models[i])) continue;
        size_t path_size = strlen(rendered->impl_names[i]) +
                           sizeof("sn_native_bridge_.c");
        char *path = malloc(path_size);
        if (!path)
        {
            modular_render_result_free(rendered);
            return false;
        }
        snprintf(path, path_size, "sn_native_bridge_%s.c",
                 rendered->impl_names[i]);
        char *code = rendered->impl_codes[i];
        rendered->impl_codes[i] = NULL;
        bool added = generated_file_set_add(files, path, code,
                                            GENERATED_SOURCE, false);
        free(path);
        if (!added)
        {
            free(code);
            modular_render_result_free(rendered);
            return false;
        }
    }
    modular_render_result_free(rendered);
    return true;
}
