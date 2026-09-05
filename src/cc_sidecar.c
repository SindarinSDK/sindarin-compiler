#include "cc_sidecar.h"
#include "gcc_backend_pkgconfig.h"
#include "debug.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
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

static bool file_exists(const char *path)
{
    return access(path, R_OK) == 0;
}

static char *copy_string(const char *value)
{
    return strdup(value ? value : "");
}

static bool add_owned_string(char ***values, int *count, const char *value)
{
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
    free(plan->mode_cflags);
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

static void make_error_file(char *error_file, size_t size)
{
#ifdef _WIN32
    const char *temp_dir = getenv("TEMP");
    if (!temp_dir) temp_dir = getenv("TMP");
    if (!temp_dir) temp_dir = ".";
    snprintf(error_file, size, "%s\\sn_cc_errors_%d.txt", temp_dir, (int)getpid());
#else
    snprintf(error_file, size, "/tmp/sn_cc_errors_XXXXXX");
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
        snprintf(error_file, size, "/tmp/sn_cc_errors.txt");
    else
        close(error_fd);
#endif
}

static bool plan_has_object(const CCSidecarBuildPlan *plan, const char *path)
{
    for (int i = 0; i < plan->object_file_count; i++)
        if (strcmp(plan->object_files[i], path) == 0) return true;
    return false;
}

static uint64_t source_identity_hash(const char *path)
{
    /* FNV-1a gives stable, compact object identities. Treat both separators as
     * the same identity so imported paths remain stable across host syntax. */
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)path; *p; p++)
    {
        unsigned char ch = (*p == '\\') ? '/' : *p;
        hash ^= ch;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool resolve_environment(const CCBackendConfig *config,
                                const CCSidecarBuildRequest *request,
                                CCSidecarBuildPlan *plan)
{
    char path[PATH_MAX];
    char package_compile_options[16384];
    char package_link_options[16384];
    char filtered_mode_cflags[1024];
    char link_library_options[PATH_MAX];

    plan->backend = detect_backend(config->cc);
    plan->sdk_root = copy_string(get_sdk_root(request->compiler_dir));
    if (!plan->sdk_root) return false;

    snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "%s" SN_PATH_SEP_STR "libsn_runtime_min.a",
             plan->sdk_root, backend_lib_subdir(plan->backend));
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
    snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "minimal",
             plan->sdk_root);
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;
    snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "include" SN_PATH_SEP_STR "platform",
             plan->sdk_root);
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;
    snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "include", plan->sdk_root);
    normalize_path_separators(path);
    if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;

    snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "include",
             plan->sdk_root);
    if (file_exists(path))
    {
        normalize_path_separators(path);
        if (!add_owned_string(&plan->include_paths, &plan->include_path_count, path)) return false;

        snprintf(path, sizeof(path), "%s" SN_PATH_SEP_STR "deps" SN_PATH_SEP_STR "lib",
                 plan->sdk_root);
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

    link_library_options[0] = '\0';
    if (request->link_libraries && request->link_library_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < request->link_library_count &&
                        offset < (int)sizeof(link_library_options) - 8; i++)
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
                    int written = snprintf(link_library_options + offset,
                                           sizeof(link_library_options) - (size_t)offset,
                                           " %s", replacement);
                    if (written > 0) offset += written;
                }
            }
            else
            {
                const char *translated = translate_lib_name(lib);
                char default_option[PATH_MAX];
                snprintf(default_option, sizeof(default_option), "-l%s", translated);
                if (!add_owned_string(&plan->requested_link_options,
                                      &plan->requested_link_option_count,
                                      default_option)) return false;
                int written = snprintf(link_library_options + offset,
                                       sizeof(link_library_options) - (size_t)offset,
                                       " -l%s", translated);
                if (written > 0) offset += written;
            }
        }
    }

    plan->link_library_options = copy_string(link_library_options);
    plan->configured_libraries = copy_string(config->ldlibs);
    plan->configured_linker_options = copy_string(config->ldflags);
    return plan->link_library_options && plan->configured_libraries &&
           plan->configured_linker_options;
}

static void build_dependency_include_option(const CCSidecarBuildPlan *plan,
                                            char *option, size_t size)
{
    if (plan->include_path_count > 4)
        snprintf(option, size, "-I\"%s\"", plan->include_paths[4]);
    else
        option[0] = '\0';
}

static bool compile_generated_sources(const CCBackendConfig *config,
                                      const CCSidecarBuildRequest *request,
                                      CCSidecarBuildPlan *plan,
                                      const char *error_file)
{
    char command[8192];
    char dependency_include_option[PATH_MAX + 8];
    const char *cc_quote = strchr(config->cc, ' ') ? "\"" : "";
    build_dependency_include_option(plan, dependency_include_option,
                                    sizeof(dependency_include_option));

    for (int i = 0; i < request->generated_source_count &&
                    plan->object_file_count < MAX_SIDECAR_OBJECTS; i++)
    {
        const char *source = request->generated_sources[i];
        char source_path[PATH_MAX];
        char object_path[PATH_MAX];
        snprintf(source_path, sizeof(source_path), "%s" SN_PATH_SEP_STR "%s",
                 request->build_dir, source);
        snprintf(object_path, sizeof(object_path), "%s" SN_PATH_SEP_STR "%.*s.o",
                 request->build_dir, (int)(strlen(source) - 2), source);

        snprintf(command, sizeof(command),
            "%s%s%s -c %s -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, plan->mode_cflags, config->std, config->cflags,
            plan->include_paths[0], plan->include_paths[1], plan->include_paths[2],
            plan->include_paths[3], dependency_include_option,
            plan->package_compile_options, source_path, object_path, error_file);

        if (!run_compile_cmd(command, error_file, request->verbose))
        {
            fprintf(stderr, "Error: failed to compile %s\n", source);
            return false;
        }
        if (!add_owned_string(&plan->object_files, &plan->object_file_count, object_path))
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
        snprintf(full_path, size, "%s", source);
    else
        snprintf(full_path, size, "%s/%s", source_dir, source);
    return true;
}

static void native_object_path(const char *build_dir, const char *full_path,
                               char *object_path, size_t size)
{
    const char *base = strrchr(full_path, '/');
#ifdef _WIN32
    const char *base_backslash = strrchr(full_path, '\\');
    if (base_backslash && (!base || base_backslash > base)) base = base_backslash;
#endif
    base = base ? base + 1 : full_path;
    const char *dot = strrchr(base, '.');
    int base_len = dot ? (int)(dot - base) : (int)strlen(base);
    unsigned long long identity = (unsigned long long)source_identity_hash(full_path);
    snprintf(object_path, size, "%s" SN_PATH_SEP_STR "pragma_%.*s_%016llx.o",
             build_dir, base_len, base, identity);
}

static bool compile_native_sources(const CCBackendConfig *config,
                                   const CCSidecarBuildRequest *request,
                                   CCSidecarBuildPlan *plan,
                                   const char *error_file)
{
    char command[8192];
    char dependency_include_option[PATH_MAX + 8];
    const char *cc_quote = strchr(config->cc, ' ') ? "\"" : "";
    build_dependency_include_option(plan, dependency_include_option,
                                    sizeof(dependency_include_option));

    for (int i = 0; i < request->native_source_count &&
                    plan->object_file_count < MAX_SIDECAR_OBJECTS; i++)
    {
        char full_path[PATH_MAX];
        char object_path[PATH_MAX];
        if (!resolve_native_source(request->native_sources[i],
                                   request->native_source_dirs[i],
                                   full_path, sizeof(full_path)))
        {
            fprintf(stderr, "Error: failed to compile pragma source %s\n",
                    request->native_sources[i]);
            return false;
        }
        native_object_path(request->build_dir, full_path, object_path,
                           sizeof(object_path));

        /* Repeated imports of the same source identity produce one object. */
        if (plan_has_object(plan, object_path)) continue;

        snprintf(command, sizeof(command),
            "%s%s%s -c %s -Werror=implicit-function-declaration -std=%s -D_GNU_SOURCE %s "
            "-include \"%s/sn_types.h\" "
            "-I\"%s\" -I\"%s\" -I\"%s\" -I\"%s\" %s %s "
            "\"%s\" -o \"%s\" 2>\"%s\"",
            cc_quote, config->cc, cc_quote, plan->mode_cflags, config->std, config->cflags,
            request->build_dir, plan->include_paths[0], plan->include_paths[1],
            plan->include_paths[2], plan->include_paths[3], dependency_include_option,
            plan->package_compile_options, full_path, object_path, error_file);

        if (!run_compile_cmd(command, error_file, request->verbose))
        {
            fprintf(stderr, "Error: failed to compile pragma source %s\n", full_path);
            return false;
        }
        if (!add_owned_string(&plan->object_files, &plan->object_file_count, object_path))
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
    if (!config || !request || !request->build_dir || !request->compiler_dir)
        return false;
    if (!resolve_environment(config, request, out_plan))
    {
        cc_sidecar_build_plan_free(out_plan);
        return false;
    }

    char error_file[PATH_MAX];
    make_error_file(error_file, sizeof(error_file));
    if (!compile_generated_sources(config, request, out_plan, error_file) ||
        !compile_native_sources(config, request, out_plan, error_file))
    {
        unlink(error_file);
        cc_sidecar_build_plan_free(out_plan);
        return false;
    }
    unlink(error_file);
    return true;
}
