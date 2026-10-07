#include "target/rust/rust_native_internal.h"
#include "cgen/gen_model_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool rust_native_impl_has_callable_body(json_object *impl)
{
    json_object *definitions = NULL;
    const char *keys[] = {"functions", "lambdas", "threads", "fn_wrappers"};
    for (size_t k = 0; k < sizeof(keys) / sizeof(keys[0]); k++)
    {
        if (!json_object_object_get_ex(impl, keys[k], &definitions)) continue;
        for (size_t i = 0; i < json_object_array_length(definitions); i++)
        {
            json_object *definition = json_object_array_get_idx(definitions, i), *has_body = NULL;
            if (k != 0 || (json_object_object_get_ex(definition, "has_body", &has_body) && json_object_get_boolean(has_body))) return true;
        }
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

    json_object *callback_support = rust_native_plan_callback_support(plan);
    if (callback_support) {
        const char *keys[] = {"rust_native_callback_credit_lock", "rust_native_callback_credit_unlock"};
        for (size_t i = 0; i < 2; i++) {
            json_object *value = NULL;
            json_object_object_get_ex(callback_support, keys[i], &value);
            json_object_object_add(split->common_header, keys[i], json_object_get(value));
        }
    }
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
        if (!rust_native_impl_has_callable_body(split->impl_models[i])) continue;
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
    json_object *handles = rust_native_plan_handles(plan);
    json_object *prepared_arrays = rust_native_plan_array_support(plan);
    if ((handles && json_object_array_length(handles)) || prepared_arrays)
    {
        written = snprintf(template_dir, sizeof(template_dir),
                           "%s/templates/rust/native_handles", compiler_dir);
        if (written < 0 || (size_t)written >= sizeof(template_dir)) return false;
        json_object *model = json_object_new_object();
        json_object_object_add(model, "handles", json_object_get(handles));
        json_object *arrays = NULL;
        if ((arrays = prepared_arrays))
            json_object_object_add(model, "arrays", json_object_get(arrays));
        char *code = render_with_helpers(model, template_dir,
            gen_model_get_min_c_register_fn(), "Rust native handles");
        json_object_put(model);
        if (!code) return false;
        if (!generated_file_set_add(files, "sn_native_handles.c", code,
                                    GENERATED_SOURCE, false))
        {
            free(code);
            return false;
        }
    }
    json_object *records = rust_native_plan_record_support(plan);
    if (records)
    {
        written = snprintf(template_dir, sizeof(template_dir),
                           "%s/templates/rust/native_records", compiler_dir);
        if (written < 0 || (size_t)written >= sizeof(template_dir)) return false;
        char *code = render_with_helpers(records, template_dir,
            gen_model_get_min_c_register_fn(), "Rust native records");
        if (!code) return false;
        if (!generated_file_set_add(files, "sn_native_records.c", code,
                                    GENERATED_SOURCE, false))
        {
            free(code);
            return false;
        }
    }
    json_object *callbacks = rust_native_plan_callback_support(plan);
    if (callbacks)
    {
        snprintf(template_dir, sizeof(template_dir), "%s/templates/rust/native_callbacks", compiler_dir);
        char *code = render_with_helpers(callbacks, template_dir,
            gen_model_get_min_c_register_fn(), "Rust native callbacks");
        if (!code) return false;
        if (!generated_file_set_add(files, "sn_native_callbacks.c", code, GENERATED_SOURCE, false))
        { free(code); return false; }
    }
    return true;
}
