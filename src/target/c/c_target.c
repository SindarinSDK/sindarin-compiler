#include "target/target.h"
#include "cgen/gen_model.h"
#include "cgen/gen_model_render.h"
#include "cgen/gen_model_split.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    CCBackendConfig config;
    ModularModel *split;
} CTargetBuildData;

static void c_target_build_data_free(void *opaque)
{
    CTargetBuildData *data = opaque;
    if (!data) return;
    modular_model_free(data->split);
    free(data);
}

static bool c_check_toolchain(const CompilerOptions *options)
{
    CCBackendConfig config;
    cc_backend_load_config(options->compiler_dir);
    cc_backend_init_config(&config);
    return gcc_check_available(&config, options->verbose);
}

static bool c_emit(CompilerOptions *options, Module *module,
                   TargetEmitMode mode, GeneratedFileSet *result)
{
    json_object *model = gen_model_build(&options->arena, module,
                                          &options->symbol_table,
                                          options->arithmetic_mode);
    if (!model) return false;
    gen_model_flatten_chains(model);

    char template_dir[1024];
    snprintf(template_dir, sizeof(template_dir), "%s/templates/c", options->compiler_dir);

    if (mode == TARGET_EMIT_SINGLE)
    {
        char *code = gen_model_render_min_c(model, template_dir);
        json_object_put(model);
        if (!code) return false;
        if (!generated_file_set_add(result, "module.c", code, GENERATED_SOURCE, true))
        {
            free(code);
            return false;
        }
        return true;
    }

    ModularModel *split = gen_model_split(model, options->source_file);
    json_object_put(model);
    if (!split)
    {
        fprintf(stderr, "Error: model splitting failed\n");
        return false;
    }

    register_helpers_fn register_helpers = gen_model_get_min_c_register_fn();
    ModularRenderResult *rendered = gen_model_render_modular_min_c(
        split, template_dir, register_helpers);
    if (!rendered)
    {
        fprintf(stderr, "Error: modular C rendering failed\n");
        modular_model_free(split);
        return false;
    }

    CTargetBuildData *data = calloc(1, sizeof(*data));
    if (!data)
    {
        modular_render_result_free(rendered);
        modular_model_free(split);
        return false;
    }
    cc_backend_init_config(&data->config);
    data->split = split;
    result->target_data = data;
    result->free_target_data = c_target_build_data_free;

    char *header = rendered->header_code;
    rendered->header_code = NULL;
    if (!generated_file_set_add(result, "sn_types.h", header, GENERATED_HEADER, false))
    {
        free(header);
        modular_render_result_free(rendered);
        return false;
    }

    for (int i = 0; i < rendered->impl_count; i++)
    {
        size_t path_len = strlen(rendered->impl_names[i]) + 3;
        char *path = malloc(path_len);
        if (!path)
        {
            modular_render_result_free(rendered);
            return false;
        }
        snprintf(path, path_len, "%s.c", rendered->impl_names[i]);
        char *code = rendered->impl_codes[i];
        rendered->impl_codes[i] = NULL;
        bool added = generated_file_set_add(result, path, code, GENERATED_SOURCE, i == 0);
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

static bool c_build(const CompilerOptions *options, const char *build_dir,
                    const GeneratedFileSet *files)
{
    CTargetBuildData *data = files->target_data;
    if (!data || !data->split) return false;

    int source_count = 0;
    for (int i = 0; i < files->file_count; i++)
        if (files->files[i].kind == GENERATED_SOURCE) source_count++;

    const char **source_files = calloc((size_t)source_count, sizeof(char *));
    if (!source_files) return false;
    int source_index = 0;
    for (int i = 0; i < files->file_count; i++)
        if (files->files[i].kind == GENERATED_SOURCE)
            source_files[source_index++] = files->files[i].relative_path;

    bool ok = gcc_compile_modular(&data->config, build_dir,
                                  source_files, source_count,
                                  options->executable_file, options->compiler_dir,
                                  options->verbose, options->debug_build,
                                  options->profile_build,
                                  data->split->link_libs, data->split->link_lib_count,
                                  data->split->source_files, data->split->source_dirs,
                                  data->split->source_file_count);
    free(source_files);
    return ok;
}

const TargetCompiler sn_c_target = {
    TARGET_C,
    "c",
    ".c",
    "c",
    c_check_toolchain,
    c_emit,
    c_build
};
