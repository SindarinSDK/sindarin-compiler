#include "target/rust/rust_native_internal.h"
#include "cc_sidecar.h"
#include "debug.h"
#include "gcc_backend.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} NativeBuffer;

typedef struct {
    char *compiler_command;
    char *c_standard;
    char *mode_cflags;
    char *configured_compile_options;
} RustNativeLinkConfig;

static void rust_native_link_config_free(RustNativeLinkConfig *config)
{
    if (!config) return;
    free(config->compiler_command);
    free(config->c_standard);
    free(config->mode_cflags);
    free(config->configured_compile_options);
    memset(config, 0, sizeof(*config));
}

static bool rust_native_link_config_init(
    RustNativeLinkConfig *owned, const CCBackendConfig *config,
    bool debug_mode, bool profile_mode)
{
    if (!owned || !config) return false;
    memset(owned, 0, sizeof(*owned));
    const char *mode = profile_mode ? config->profile_cflags
                     : debug_mode   ? config->debug_cflags
                     :                config->release_cflags;
    char filtered_mode[1024];
    if (detect_backend(config->cc) == BACKEND_TINYCC)
        mode = filter_tinycc_flags(mode, filtered_mode, sizeof(filtered_mode));
    owned->compiler_command = strdup(config->cc);
    owned->c_standard = strdup(config->std);
    owned->mode_cflags = strdup(mode);
    owned->configured_compile_options = strdup(config->cflags);
    if (owned->compiler_command && owned->c_standard && owned->mode_cflags &&
        owned->configured_compile_options)
        return true;
    rust_native_link_config_free(owned);
    return false;
}

static bool buffer_appendf(NativeBuffer *buffer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (needed < 0)
    {
        va_end(args);
        return false;
    }
    size_t required = buffer->length + (size_t)needed + 1;
    if (required > buffer->capacity)
    {
        size_t capacity = buffer->capacity ? buffer->capacity : 512;
        while (capacity < required)
        {
            if (capacity > SIZE_MAX / 2)
            {
                va_end(args);
                return false;
            }
            capacity *= 2;
        }
        char *grown = realloc(buffer->data, capacity);
        if (!grown)
        {
            va_end(args);
            return false;
        }
        buffer->data = grown;
        buffer->capacity = capacity;
    }
    vsnprintf(buffer->data + buffer->length,
              buffer->capacity - buffer->length, format, args);
    va_end(args);
    buffer->length += (size_t)needed;
    return true;
}

static char *shell_quote(const char *value)
{
    if (!value) value = "";
#ifdef _WIN32
    size_t length = strlen(value);
    char *quoted = malloc(length * 2 + 3);
    if (!quoted) return NULL;
    char *out = quoted;
    *out++ = '"';
    for (const char *p = value; *p; p++)
    {
        if (*p == '"') *out++ = '"';
        *out++ = *p;
    }
    *out++ = '"';
    *out = '\0';
    return quoted;
#else
    size_t apostrophes = 0;
    for (const char *p = value; *p; p++)
        if (*p == '\'') apostrophes++;
    size_t length = strlen(value);
    char *quoted = malloc(length + apostrophes * 3 + 3);
    if (!quoted) return NULL;
    char *out = quoted;
    *out++ = '\'';
    for (const char *p = value; *p; p++)
    {
        if (*p == '\'')
        {
            memcpy(out, "'\\''", 4);
            out += 4;
        }
        else *out++ = *p;
    }
    *out++ = '\'';
    *out = '\0';
    return quoted;
#endif
}

static bool write_linker_proxy(const char *path,
                               const RustNativeLinkConfig *link_config,
                               const CCSidecarBuildPlan *sidecar)
{
    FILE *file = fopen(path, "wb");
    if (!file)
    {
        fprintf(stderr, "Error: cannot create Rust native linker proxy '%s': %s\n",
                path, strerror(errno));
        return false;
    }

#ifdef _WIN32
    const char *cc_prefix = strchr(link_config->compiler_command, ' ') ? "\"" : "";
    bool ok = fprintf(file,
        "@echo off\r\n%s%s%s %s -w -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s %%* %s%s %s %s\r\nexit /b %%errorlevel%%\r\n",
        cc_prefix, link_config->compiler_command, cc_prefix,
        link_config->mode_cflags, link_config->c_standard,
        link_config->configured_compile_options, sidecar->package_link_options,
        sidecar->link_library_options, sidecar->configured_libraries,
        sidecar->configured_linker_options) >= 0;
#else
    char *quoted_cc = strchr(link_config->compiler_command, ' ')
        ? shell_quote(link_config->compiler_command)
        : strdup(link_config->compiler_command);
    bool ok = quoted_cc && fprintf(file,
        "#!/bin/sh\nexec %s %s -w -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s \"$@\" %s%s %s %s\n",
        quoted_cc, link_config->mode_cflags, link_config->c_standard,
        link_config->configured_compile_options, sidecar->package_link_options,
        sidecar->link_library_options, sidecar->configured_libraries,
        sidecar->configured_linker_options) >= 0;
    free(quoted_cc);
#endif
    if (fclose(file) != 0) ok = false;
#ifndef _WIN32
    if (ok && chmod(path, 0700) != 0)
    {
        fprintf(stderr, "Error: cannot make Rust native linker proxy executable: %s\n",
                strerror(errno));
        ok = false;
    }
#endif
    return ok;
}

static bool append_link_arg(NativeBuffer *command, const char *argument)
{
    char *quoted = shell_quote(argument);
    bool ok = quoted && buffer_appendf(command, " -C link-arg=%s", quoted);
    free(quoted);
    return ok;
}

static bool path_has_c_extension(const char *path)
{
    size_t length = path ? strlen(path) : 0;
    return length > 2 && strcmp(path + length - 2, ".c") == 0;
}

bool rust_native_build(const CompilerOptions *options, const char *build_dir,
                       const GeneratedFileSet *files, RustNativePlan *plan)
{
    ModularModel *split = rust_native_plan_split(plan);
    if (!options || !build_dir || !files || !split || files->primary_file < 0)
        return false;

    int generated_count = 0;
    for (int i = 0; i < files->file_count; i++)
        if (files->files[i].kind == GENERATED_SOURCE &&
            path_has_c_extension(files->files[i].relative_path)) generated_count++;
    const char **generated = generated_count
        ? calloc((size_t)generated_count, sizeof(*generated)) : NULL;
    if (generated_count && !generated) return false;
    int generated_index = 0;
    for (int i = 0; i < files->file_count; i++)
        if (files->files[i].kind == GENERATED_SOURCE &&
            path_has_c_extension(files->files[i].relative_path))
            generated[generated_index++] = files->files[i].relative_path;

    CCBackendConfig config;
    cc_backend_load_config(options->compiler_dir);
    cc_backend_init_config(&config);
    RustNativeLinkConfig link_config;
    if (!rust_native_link_config_init(&link_config, &config,
                                      options->debug_build,
                                      options->profile_build))
    {
        free(generated);
        return false;
    }
    CCSidecarBuildRequest request = {
        .build_dir = build_dir,
        .compiler_dir = options->compiler_dir,
        .generated_sources = generated,
        .generated_source_count = generated_count,
        .native_sources = (const char *const *)split->source_files,
        .native_source_dirs = (const char *const *)split->source_dirs,
        .native_source_count = split->source_file_count,
        .link_libraries = (const char *const *)split->link_libs,
        .link_library_count = split->link_lib_count,
        .verbose = options->verbose,
        .debug_mode = options->debug_build,
        .profile_mode = options->profile_build,
    };
    CCSidecarBuildPlan sidecar;
    bool sidecar_ok = cc_sidecar_build(&config, &request, &sidecar);
    free(generated);
    if (!sidecar_ok)
    {
        rust_native_link_config_free(&link_config);
        return false;
    }

#ifdef _WIN32
    const char *proxy_name = "sn_rust_linker_proxy.cmd";
#else
    const char *proxy_name = "sn_rust_linker_proxy.sh";
#endif
    char proxy_path[PATH_MAX];
    int written = snprintf(proxy_path, sizeof(proxy_path), "%s/%s",
                           build_dir, proxy_name);
    if (written < 0 || (size_t)written >= sizeof(proxy_path) ||
        !write_linker_proxy(proxy_path, &link_config, &sidecar))
    {
        cc_sidecar_build_plan_free(&sidecar);
        rust_native_link_config_free(&link_config);
        return false;
    }

    char source_path[PATH_MAX];
    written = snprintf(source_path, sizeof(source_path), "%s/%s", build_dir,
                       files->files[files->primary_file].relative_path);
    if (written < 0 || (size_t)written >= sizeof(source_path))
    {
        cc_sidecar_build_plan_free(&sidecar);
        rust_native_link_config_free(&link_config);
        return false;
    }

    const char *rustc = getenv("SN_RUSTC");
    if (!rustc || !rustc[0]) rustc = "rustc";
    const char *rustflags = getenv("SN_RUSTFLAGS");
    if (!rustflags) rustflags = "";
    const char *profile_flags = options->debug_build
        ? "-C debuginfo=2 -C opt-level=0"
        : options->profile_build
            ? "-C debuginfo=1 -C opt-level=3 -C force-frame-pointers=yes"
            : "-C opt-level=3";
    char *quoted_rustc = shell_quote(rustc);
    char *quoted_proxy = shell_quote(proxy_path);
    char *quoted_source = shell_quote(source_path);
    char *quoted_output = shell_quote(options->executable_file);
    NativeBuffer command = {0};
    bool ok = quoted_rustc && quoted_proxy && quoted_source && quoted_output &&
        buffer_appendf(&command, "%s --edition=2021 %s %s -C linker=%s",
                       quoted_rustc, profile_flags, rustflags, quoted_proxy);
    for (int i = 0; ok && i < sidecar.object_file_count; i++)
        ok = append_link_arg(&command, sidecar.object_files[i]);
    if (ok) ok = append_link_arg(&command, sidecar.runtime_archive);
    for (int i = 0; ok && i < sidecar.library_search_path_count; i++)
    {
        NativeBuffer option = {0};
        ok = buffer_appendf(&option, "-L%s", sidecar.library_search_paths[i]) &&
             append_link_arg(&command, option.data);
        free(option.data);
    }
#ifndef _WIN32
    for (int i = 0; ok && i < sidecar.runtime_search_path_count; i++)
    {
        NativeBuffer option = {0};
        ok = buffer_appendf(&option, "-Wl,-rpath,%s",
                            sidecar.runtime_search_paths[i]) &&
             append_link_arg(&command, option.data);
        free(option.data);
    }
#endif
    if (ok)
        ok = buffer_appendf(&command, " %s -o %s", quoted_source, quoted_output);

    free(quoted_rustc);
    free(quoted_proxy);
    free(quoted_source);
    free(quoted_output);
    cc_sidecar_build_plan_free(&sidecar);
    rust_native_link_config_free(&link_config);
    if (!ok)
    {
        free(command.data);
        fprintf(stderr, "Error: failed to build Rust native rustc command\n");
        return false;
    }
    if (options->verbose) DEBUG_INFO("Executing: %s", command.data);
    int status = system(command.data);
    free(command.data);
    if (status != 0)
    {
        fprintf(stderr, "Error: rustc failed to link generated Rust and native C objects\n");
        return false;
    }
    if (!options->keep_generated) remove(proxy_path);
    return true;
}
