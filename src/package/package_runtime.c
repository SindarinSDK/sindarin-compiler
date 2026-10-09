/* Package runtime selection and source ownership. */
#include "../package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

const char *package_runtime_name(PackageRuntime runtime)
{
    switch (runtime) {
        case PACKAGE_RUNTIME_C: return "C";
        case PACKAGE_RUNTIME_RS: return "RS";
        case PACKAGE_RUNTIME_GO: return "GO";
        default: return NULL;
    }
}

PackageRuntime package_runtime_resolve(PackageRuntime declared, PackageRuntime application)
{
    return declared == PACKAGE_RUNTIME_INHERIT ? application : declared;
}

/* Return the nearest enclosing manifest, using canonical absolute paths so
 * relative, parent and symlink imports identify the same package. The caller
 * owns the result. Existing source files are required by the compilation path. */
char *package_source_manifest(const char *source)
{
#ifdef _WIN32
    char *directory = _fullpath(NULL, source, 0);
#else
    char *directory = realpath(source, NULL);
#endif
    if (!directory) return NULL;
#ifdef _WIN32
    for (char *p = directory; *p; p++)
        if (*p == '\\') *p = '/';
#endif
    char *separator = strrchr(directory, '/');
    if (!separator) { free(directory); return NULL; }
    if (separator == directory) separator[1] = '\0';
    else *separator = '\0';

    char *candidate = malloc(strlen(directory) + sizeof("/sn.yaml"));
    if (!candidate) { free(directory); return NULL; }
    for (;;) {
        size_t len = strlen(directory);
        snprintf(candidate, len + sizeof("/sn.yaml"), "%s%ssn.yaml", directory,
                 len && directory[len - 1] == '/' ? "" : "/");
        struct stat st;
        if (stat(candidate, &st) == 0) {
            free(directory);
            return candidate;
        }
        separator = strrchr(directory, '/');
        if (!separator || (separator == directory && directory[1] == '\0')) break;
#ifdef _WIN32
        /* Do not walk from a drive or UNC share into another volume. */
        if (separator == directory + 2 && directory[1] == ':' && !directory[3]) break;
        if (directory[0] == '/' && directory[1] == '/') {
            char *server_end = strchr(directory + 2, '/');
            if (!server_end || separator == server_end) break;
        }
#endif
        if (separator == directory) separator[1] = '\0';
        else *separator = '\0';
    }
    free(candidate);
    free(directory);
    return NULL;
}

static bool same_manifest(const char *left, const char *right)
{
#ifdef _WIN32
    return _stricmp(left, right) == 0;
#else
    return strcmp(left, right) == 0;
#endif
}

bool package_check_import_runtimes(const char *application_source,
                                  const char *const *imported_sources, int count,
                                  PackageRuntime application_runtime)
{
    if (!application_source || count < 0 || (count && !imported_sources) ||
        (application_runtime != PACKAGE_RUNTIME_C && application_runtime != PACKAGE_RUNTIME_RS))
        return false;
    char **manifests = calloc((size_t)count + 1, sizeof(*manifests));
    if (!manifests) return false;
    int manifest_count = 0;
    bool success = true;
    PackageConfig config;
    char *app_manifest = package_source_manifest(application_source);
    if (app_manifest) {
        manifests[manifest_count++] = app_manifest;
        success = package_yaml_parse(app_manifest, &config);

    }
    for (int i = 0; success && i < count; i++) {
        char *manifest = package_source_manifest(imported_sources[i]);
        if (!manifest) continue;
        bool seen = false;
        for (int j = 0; j < manifest_count; j++)
            if (same_manifest(manifest, manifests[j])) { seen = true; break; }
        if (seen) { free(manifest); continue; }
        manifests[manifest_count++] = manifest;
        if (!package_yaml_parse(manifest, &config)) { success = false; break; }
        if (config.has_native) continue; /* resolved by the typed import adapter pass */
        PackageRuntime runtime = package_runtime_resolve(config.runtime, application_runtime);
        if (runtime == PACKAGE_RUNTIME_GO) {
            fprintf(stderr, "error: %s: package '%s' selects runtime GO; the Sindarin Go backend "
                    "and Go-native package bridge are not implemented yet\n", manifest, config.name);
            success = false;
        } else if (runtime != application_runtime) {
            fprintf(stderr, "error: %s: package '%s' selects runtime %s but the application targets %s; "
                    "cross-runtime package artifacts and generated ABI adapters are not implemented yet\n",
                    manifest, config.name, package_runtime_name(runtime),
                    package_runtime_name(application_runtime));
            success = false;
        }
    }
    for (int i = 0; i < manifest_count; i++) free(manifests[i]);
    free(manifests);
    return success;
}
