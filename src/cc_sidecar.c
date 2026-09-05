#include "cc_sidecar.h"
#include "gcc_backend_pkgconfig.h"
#include "debug.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
#include <windows.h>
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <unistd.h>
#endif
#define SN_PATH_SEP_STR "\\"
static void normalize_path_separators(char *path)
{
    for (char *p = path; *p; p++)
        if (*p == '/') *p = '\\';
}
#else
#include <unistd.h>
#define SN_PATH_SEP_STR "/"
#define normalize_path_separators(path) ((void)0)
#endif

#define MAX_SIDECAR_OBJECTS 510

typedef struct {
    char source_path[PATH_MAX];
    char object_path[PATH_MAX];
} GeneratedSourceInput;

typedef struct {
    char source_path[PATH_MAX];
    char identity_path[PATH_MAX];
    char object_path[PATH_MAX];
    bool duplicate;
} NativeSourceInput;

static bool file_exists(const char *path)
{
    return access(path, R_OK) == 0;
}

static char *copy_string(const char *value)
{
    return strdup(value ? value : "");
}

static bool copy_path(char *destination, size_t size, const char *source)
{
    size_t length = strlen(source);
    if (length >= size) return false;
    memcpy(destination, source, length + 1);
    return true;
}

static bool add_owned_string(char ***values, int *count, const char *value)
{
    if (*count == INT_MAX) return false;
    char *copy = copy_string(value);
    if (!copy) return false;

    char **grown = realloc(*values, (size_t)(*count + 1) * sizeof(char *));
    if (!grown)
    {
        free(copy);
        return false;
    }
    *values = grown;
    (*values)[*count] = copy;
    (*count)++;
    return true;
}

static void free_strings(char **values, int count)
{
    for (int i = 0; i < count; i++) free(values[i]);
    free(values);
}

void cc_sidecar_build_plan_init(CCSidecarBuildPlan *plan)
{
    if (plan) memset(plan, 0, sizeof(*plan));
}

void cc_sidecar_build_plan_free(CCSidecarBuildPlan *plan)
{
    if (!plan) return;
    free(plan->sdk_root);
    free(plan->runtime_archive);
    free(plan->compiler_command);
    free(plan->c_standard);
    free(plan->mode_cflags);
    free(plan->configured_compile_options);
    free_strings(plan->include_paths, plan->include_path_count);
    free_strings(plan->library_search_paths, plan->library_search_path_count);
    free_strings(plan->runtime_search_paths, plan->runtime_search_path_count);
    free(plan->package_compile_options);
    free(plan->package_link_options);
    free_strings(plan->requested_link_options, plan->requested_link_option_count);
    free(plan->link_library_options);
    free(plan->configured_libraries);
    free(plan->configured_linker_options);
    free_strings(plan->object_files, plan->object_file_count);
    memset(plan, 0, sizeof(*plan));
}

static bool run_compile_cmd(const char *command, const char *error_file, bool verbose)
{
    if (verbose)
        DEBUG_INFO("Executing: %s", command);

    int result = system(command);
    if (result != 0)
    {
        FILE *errfile = fopen(error_file, "r");
        if (errfile)
        {
            char line[1024];
            fprintf(stderr, "\n");
            while (fgets(line, sizeof(line), errfile))
                fprintf(stderr, "%s", line);
            fclose(errfile);
        }
        unlink(error_file);
        return false;
    }
    unlink(error_file);
    return true;
}

static bool make_error_file(char *error_file, size_t size)
{
#ifdef _WIN32
    const char *temp_dir = getenv("TEMP");
    if (!temp_dir) temp_dir = getenv("TMP");
    if (!temp_dir) temp_dir = ".";
    int written = snprintf(error_file, size, "%s\\sn_cc_errors_%d.txt",
                           temp_dir, (int)getpid());
    return written >= 0 && (size_t)written < size;
#else
    if (!copy_path(error_file, size, "/tmp/sn_cc_errors_XXXXXX")) return false;
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
    {
        if (!copy_path(error_file, size, "/tmp/sn_cc_errors.txt")) return false;
    }
    else
        close(error_fd);
    return true;
#endif
}

static uint64_t source_identity_hash(const char *path)
{
    /* FNV-1a gives stable, compact object identities. On Windows only, both
     * accepted path separators describe the same identity. A backslash is an
     * ordinary filename byte on Unix and must remain distinct there. */
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)path; *p; p++)
    {
#ifdef _WIN32
        unsigned char ch = (*p == '\\') ? '/' : *p;
#else
        unsigned char ch = *p;
#endif
        hash ^= ch;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool invalid_request(const char *reason)
{
    fprintf(stderr, "Error: invalid C sidecar build request: %s\n", reason);
    return false;
}

static bool validate_request(const CCBackendConfig *config,
                             const CCSidecarBuildRequest *request)
{
    if (!config) return invalid_request("missing compiler configuration");
    if (!config->cc || !config->std || !config->debug_cflags ||
        !config->release_cflags || !config->profile_cflags || !config->cflags ||
        !config->ldflags || !config->ldlibs)
        return invalid_request("compiler configuration contains a null value");
    if (!request) return invalid_request("missing request");
    if (!request->build_dir || !request->compiler_dir)
        return invalid_request("missing build or compiler directory");
    if (request->generated_source_count < 0 || request->native_source_count < 0 ||
        request->link_library_count < 0)
        return invalid_request("negative list count");
    if (request->generated_source_count > 0 && !request->generated_sources)
        return invalid_request("missing generated source list");
    if (request->native_source_count > 0 &&
        (!request->native_sources || !request->native_source_dirs))
        return invalid_request("missing native source or origin list");
    if (request->link_library_count > 0 && !request->link_libraries)
        return invalid_request("missing link library list");

    if (request->generated_source_count > MAX_SIDECAR_OBJECTS ||
        request->native_source_count >
            MAX_SIDECAR_OBJECTS - request->generated_source_count)
        return invalid_request("more than 510 C translation units");

    for (int i = 0; i < request->generated_source_count; i++)
    {
        const char *source = request->generated_sources[i];
        if (!source) return invalid_request("null generated source entry");
        size_t length = strlen(source);
        if (length <= 2 || strcmp(source + length - 2, ".c") != 0)
            return invalid_request("generated source name must end in .c");
    }
    for (int i = 0; i < request->native_source_count; i++)
    {
        if (!request->native_sources[i] || !request->native_source_dirs[i])
            return invalid_request("null native source or origin entry");
    }
    for (int i = 0; i < request->link_library_count; i++)
    {
        if (!request->link_libraries[i])
            return invalid_request("null link library entry");
    }
    return true;
}

static bool build_joined_link_options(CCSidecarBuildPlan *plan)
{
    size_t size = 1;
    for (int i = 0; i < plan->requested_link_option_count; i++)
    {
        size_t length = strlen(plan->requested_link_options[i]);
        if (length > SIZE_MAX - size - 1) return false;
        size += length + 1;
    }

    plan->link_library_options = malloc(size);
    if (!plan->link_library_options) return false;

    char *cursor = plan->link_library_options;
    for (int i = 0; i < plan->requested_link_option_count; i++)
    {
        const char *option = plan->requested_link_options[i];
        size_t length = strlen(option);
        *cursor++ = ' ';
        memcpy(cursor, option, length);
        cursor += length;
    }
    *cursor = '\0';
    return true;
}

static bool resolve_environment(const CCBackendConfig *config,
                                const CCSidecarBuildRequest *request,
                                CCSidecarBuildPlan *plan)
{
    char path[PATH_MAX];
    char package_compile_options[16384];
    char package_link_options[16384];
    char filtered_mode_cflags[1024];

    plan->backend = detect_backend(config->cc);
    plan->compiler_command = copy_string(config->cc);
    plan->c_standard = copy_string(config->std);
    plan->configured_compile_options = copy_string(config->cflags);
    if (!plan->compiler_command || !plan->c_standard ||
        !plan->configured_compile_options) return false;
    plan->sdk_root = copy_string(get_sdk_root(request->compiler_dir));
    if (!plan->sdk_root) return false;

    int written = snprintf(path, sizeof(path),
                           "%s" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "libsn_runtime_min.a",
                           plan->sdk_root, backend_lib_subdir(plan->backend));
    if (written < 0 || (size_t)written >= sizeof(path))
        return invalid_request("runtime archive path is too long");
    normalize_path_separators(path);
    plan->runtime_archive = copy_string(path);
    if (!plan->runtime_archive) return false;

    if (plan->backend != BACKEND_MSVC && !file_exists(plan->runtime_archive))
    {
        fprintf(stderr, "Error: Runtime library not found: %s\n", plan->runtime_archive);
        fprintf(stderr, "The '%s' backend runtime is not built.\n", backend_name(plan->backend));
        fprintf(stderr, "Run 'make build' to build the runtime.\n");
        return false;
    }

    const char *mode_cflags = request->profile_mode ? config->profile_cflags
                            : request->debug_mode   ? config->debug_cflags
                            :                         config->release_cflags;
    if (plan->backend == BACKEND_TINYCC)
        mode_cflags = filter_tinycc_flags(mode_cflags, filtered_mode_cflags,
                                          sizeof(filtered_mode_cflags));
    plan->mode_cflags = copy_string(mode_cflags);
    if (!plan->mode_cflags) return false;

    if (!add_owned_string(&plan->include_paths, &plan->include_path_count,
                          request->build_dir)) return false;
    written = snprintf(path, sizeof(path),
                       "%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "minimal",
                       plan->sdk_root);
    if (written < 0 || (size_t)written >= sizeof(path))
        return invalid_request("SDK minimal include path is too long");
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;
    written = snprintf(path, sizeof(path),
                       "%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "platform",
                       plan->sdk_root);
    if (written < 0 || (size_t)written >= sizeof(path))
        return invalid_request("SDK platform include path is too long");
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;
    written = snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "include",
                       plan->sdk_root);
    if (written < 0 || (size_t)written >= sizeof(path))
        return invalid_request("SDK include path is too long");
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;

    written = snprintf(path, sizeof(path),
                       "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "include",
                       plan->sdk_root);
    if (written < 0 || (size_t)written >= sizeof(path))
        return invalid_request("SDK dependency include path is too long");
    if (file_exists(path))
    {
        normalize_path_separators(path);
        if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;

        written = snprintf(path, sizeof(path),
                           "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "lib",
                           plan->sdk_root);
        if (written < 0 || (size_t)written >= sizeof(path))
            return invalid_request("SDK dependency library path is too long");
        normalize_path_separators(path);
        if (!add_owned_string(&plan->library_search_paths,
                              &plan->library_search_path_count, path) ||
            !add_owned_string(&plan->runtime_search_paths,
                              &plan->runtime_search_path_count, path)) return false;
    }

    build_package_lib_paths(package_compile_options, sizeof(package_compile_options),
                            package_link_options, sizeof(package_link_options));
    if (request->verbose && package_compile_options[0])
    {
        DEBUG_INFO("Package includes: %s", package_compile_options);
        DEBUG_INFO("Package libs: %s", package_link_options);
    }
    plan->package_compile_options = copy_string(package_compile_options);
    plan->package_link_options = copy_string(package_link_options);
    if (!plan->package_compile_options || !plan->package_link_options) return false;

    if (request->link_libraries && request->link_library_count > 0)
    {
        for (int i = 0; i < request->link_library_count; i++)
        {
            const char *lib = request->link_libraries[i];
            const char *replacement = cc_backend_get_link_library_options(lib);
            if (replacement)
            {
                if (replacement[0])
                {
                    if (!add_owned_string(&plan->requested_link_options,
                                          &plan->requested_link_option_count,
                                          replacement)) return false;
                }
            }
            else
            {
                const char *translated = translate_lib_name(lib);
                size_t translated_length = strlen(translated);
                if (translated_length > SIZE_MAX - 3) return false;
                size_t option_size = translated_length + 3;
                char *default_option = malloc(option_size);
                if (!default_option) return false;
                snprintf(default_option, option_size, "-l%s", translated);
                if (!add_owned_string(&plan->requested_link_options,
                                      &plan->requested_link_option_count,
                                      default_option))
                {
                    free(default_option);
                    return false;
                }
                free(default_option);
            }
        }
    }

    if (!build_joined_link_options(plan)) return false;
    plan->configured_libraries = copy_string(config->ldlibs);
    plan->configured_linker_options = copy_string(config->ldflags);
    return plan->link_library_options && plan->configured_libraries &&
           plan->configured_linker_options;
}

static bool build_dependency_include_option(const CCSidecarBuildPlan *plan,
                                            char *option, size_t size)
{
    if (plan->include_path_count > 4)
    {
        int written = snprintf(option, size, "-I\"%s\"", plan->include_paths[4]);
        return written >= 0 && (size_t)written < size;
    }
    else
        option[0] = '\0';
    return true;
}

static bool prepare_generated_sources(const CCSidecarBuildRequest *request,
                                      GeneratedSourceInput *inputs)
{
    for (int i = 0; i < request->generated_source_count; i++)
    {
        const char *source = request->generated_sources[i];
        int written = snprintf(inputs[i].source_path, sizeof(inputs[i].source_path),
                               "%s" SN_PATH_SEP_STR "%s",
                               request->build_dir, source);
        if (written < 0 || (size_t)written >= sizeof(inputs[i].source_path))
            return invalid_request("generated source path is too long");

        written = snprintf(inputs[i].object_path, sizeof(inputs[i].object_path),
                           "%s" SN_PATH_SEP_STR "%s",
                           request->build_dir, source);
        if (written < 0 || (size_t)written >= sizeof(inputs[i].object_path))
            return invalid_request("generated object path is too long");
        inputs[i].object_path[written - 1] = 'o';
    }
    return true;
}

static bool compile_generated_sources(const CCBackendConfig *config,
                                      const CCSidecarBuildRequest *request,
                                      const GeneratedSourceInput *inputs,
                                      CCSidecarBuildPlan *plan,
                                      const char *error_file)
{
    char command[8192];
    char dependency_include_option[PATH_MAX + 8];
    const char *cc_quote = strchr(config->cc, ' ') ? "\"" : "";
    if (!build_dependency_include_option(plan, dependency_include_option,
                                         sizeof(dependency_include_option)))
        return invalid_request("dependency include option is too long");

    for (int i = 0; i < request->generated_source_count; i++)
    {
        const char *source = request->generated_sources[i];
        int written = snprintf(command, sizeof(command),
            "%s%s%s -c %s -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, plan->mode_cflags, config->std, config->cflags,
            plan->include_paths[0], plan->include_paths[1], plan->include_paths[2],
            plan->include_paths[3], dependency_include_option,
            plan->package_compile_options, inputs[i].source_path,
            inputs[i].object_path, error_file);
        if (written < 0 || (size_t)written >= sizeof(command))
            return invalid_request("generated source compile command is too long");

        if (!run_compile_cmd(command, error_file, request->verbose))
        {
            fprintf(stderr, "Error: failed to compile %s\n", source);
            return false;
        }
        if (!add_owned_string(&plan->object_files, &plan->object_file_count,
                              inputs[i].object_path))
            return false;
    }
    return true;
}

static bool resolve_native_source(const char *source_value, const char *source_dir,
                                  char *full_path, size_t size)
{
    const char *source = source_value;
    size_t len = strlen(source);
    char unquoted[PATH_MAX];
    if (len >= 2 && source[0] == '"' && source[len - 1] == '"')
    {
        if (len - 2 >= sizeof(unquoted)) return false;
        memcpy(unquoted, source + 1, len - 2);
        unquoted[len - 2] = '\0';
        source = unquoted;
    }

#ifdef _WIN32
    if (source[0] == '/' || (source[0] && source[1] == ':'))
#else
    if (source[0] == '/')
#endif
    {
        if (!copy_path(full_path, size, source)) return false;
    }
    else
    {
        int written = snprintf(full_path, size, "%s/%s", source_dir, source);
        if (written < 0 || (size_t)written >= size) return false;
    }
    return true;
}

static bool canonical_source_identity(const char *source_path,
                                      char *identity_path, size_t size)
{
#ifdef _WIN32
    HANDLE source = CreateFileA(source_path, 0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (source != INVALID_HANDLE_VALUE)
    {
        DWORD written = GetFinalPathNameByHandleA(source, identity_path, (DWORD)size,
                                                  FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        CloseHandle(source);
        if (written > 0 && written < size) return true;
        if (written >= size)
            return invalid_request("canonical native source path is too long");
    }
    if (!copy_path(identity_path, size, source_path))
        return invalid_request("native source identity path is too long");
    normalize_path_separators(identity_path);
    return true;
#else
    errno = 0;
    if (realpath(source_path, identity_path)) return true;
    if (errno == ENAMETOOLONG)
        return invalid_request("canonical native source path is too long");
    if (!copy_path(identity_path, size, source_path))
        return invalid_request("native source identity path is too long");
    return true;
#endif
}

static bool native_object_path(const char *build_dir, const char *source_path,
                               const char *identity_path,
                               char *object_path, size_t size)
{
    const char *base = strrchr(source_path, '/');
#ifdef _WIN32
    const char *base_backslash = strrchr(source_path, '\\');
    if (base_backslash && (!base || base_backslash > base)) base = base_backslash;
#endif
    base = base ? base + 1 : source_path;
    const char *dot = strrchr(base, '.');
    int base_len = dot ? (int)(dot - base) : (int)strlen(base);
    unsigned long long identity = (unsigned long long)source_identity_hash(identity_path);
    int written = snprintf(object_path, size,
                           "%s" SN_PATH_SEP_STR "pragma_%.*s_%016llx.o",
                           build_dir, base_len, base, identity);
    return written >= 0 && (size_t)written < size;
}

static bool prepare_native_sources(const CCSidecarBuildRequest *request,
                                   NativeSourceInput *inputs)
{
    for (int i = 0; i < request->native_source_count; i++)
    {
        if (!resolve_native_source(request->native_sources[i],
                                   request->native_source_dirs[i],
                                   inputs[i].source_path,
                                   sizeof(inputs[i].source_path)))
            return invalid_request("native source path is too long");
        if (!canonical_source_identity(inputs[i].source_path,
                                       inputs[i].identity_path,
                                       sizeof(inputs[i].identity_path)))
            return false;
        if (!native_object_path(request->build_dir, inputs[i].source_path,
                                inputs[i].identity_path, inputs[i].object_path,
                                sizeof(inputs[i].object_path)))
            return invalid_request("native object path is too long");

        for (int j = 0; j < i; j++)
        {
            if (strcmp(inputs[i].identity_path, inputs[j].identity_path) == 0)
            {
                inputs[i].duplicate = true;
                if (!copy_path(inputs[i].object_path, sizeof(inputs[i].object_path),
                               inputs[j].object_path))
                    return invalid_request("native object identity path is too long");
                break;
            }
            if (!inputs[j].duplicate &&
                strcmp(inputs[i].object_path, inputs[j].object_path) == 0)
                return invalid_request("native source identity hash collision");
        }
    }
    return true;
}

static bool compile_native_sources(const CCBackendConfig *config,
                                   const CCSidecarBuildRequest *request,
                                   const NativeSourceInput *inputs,
                                   CCSidecarBuildPlan *plan,
                                   const char *error_file)
{
    char command[8192];
    char dependency_include_option[PATH_MAX + 8];
    const char *cc_quote = strchr(config->cc, ' ') ? "\"" : "";
    if (!build_dependency_include_option(plan, dependency_include_option,
                                         sizeof(dependency_include_option)))
        return invalid_request("dependency include option is too long");

    for (int i = 0; i < request->native_source_count; i++)
    {
        if (inputs[i].duplicate) continue;

        int written = snprintf(command, sizeof(command),
            "%s%s%s -c %s -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-include \"%s/sn_types.h\" "
            "-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, plan->mode_cflags, config->std, config->cflags,
            request->build_dir, plan->include_paths[0], plan->include_paths[1],
            plan->include_paths[2], plan->include_paths[3], dependency_include_option,
            plan->package_compile_options, inputs[i].source_path,
            inputs[i].object_path, error_file);
        if (written < 0 || (size_t)written >= sizeof(command))
            return invalid_request("native source compile command is too long");

        if (!run_compile_cmd(command, error_file, request->verbose))
        {
            fprintf(stderr, "Error: failed to compile pragma source %s\n",
                    inputs[i].source_path);
            return false;
        }
        if (!add_owned_string(&plan->object_files, &plan->object_file_count,
                              inputs[i].object_path))
            return false;
    }
    return true;
}

bool cc_sidecar_build(const CCBackendConfig *config,
                      const CCSidecarBuildRequest *request,
                      CCSidecarBuildPlan *out_plan)
{
    if (!out_plan) return false;
    cc_sidecar_build_plan_init(out_plan);
    if (!validate_request(config, request)) return false;

    GeneratedSourceInput *generated_inputs = NULL;
    NativeSourceInput *native_inputs = NULL;
    if (request->generated_source_count > 0)
    {
        generated_inputs = calloc((size_t)request->generated_source_count,
                                  sizeof(*generated_inputs));
        if (!generated_inputs) goto fail;
    }
    if (request->native_source_count > 0)
    {
        native_inputs = calloc((size_t)request->native_source_count,
                               sizeof(*native_inputs));
        if (!native_inputs) goto fail;
    }
    if (!prepare_generated_sources(request, generated_inputs) ||
        !prepare_native_sources(request, native_inputs) ||
        !resolve_environment(config, request, out_plan))
        goto fail;

    char error_file[PATH_MAX];
    if (!make_error_file(error_file, sizeof(error_file)))
    {
        invalid_request("compiler error-file path is too long");
        goto fail;
    }
    if (!compile_generated_sources(config, request, generated_inputs,
                                   out_plan, error_file) ||
        !compile_native_sources(config, request, native_inputs,
                                out_plan, error_file))
    {
        unlink(error_file);
        goto fail;
    }

    unlink(error_file);
    free(generated_inputs);
    free(native_inputs);
    return true;

fail:
    free(generated_inputs);
    free(native_inputs);
    cc_sidecar_build_plan_free(out_plan);
    return false;
}
