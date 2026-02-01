/* ==============================================================================
 * package.c - Sindarin Package Manager Implementation
 * ==============================================================================
 * Main orchestration for package management operations.
 * ============================================================================== */

#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
    #define rmdir _rmdir
#else
    #include <unistd.h>
    #include <limits.h>
    #define PATH_SEP '/'
#endif

/* ANSI color codes for terminal output (matching diagnostic.c) */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"

/* Consistent error/warning/info output */
static void pkg_error(const char *fmt, ...)
{
    fprintf(stderr, "%serror%s: ", COLOR_RED, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static void pkg_warning(const char *fmt, ...)
{
    fprintf(stderr, "%swarning%s: ", COLOR_YELLOW, COLOR_RESET);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static void pkg_status(const char *name, const char *ref, const char *result)
{
    printf("  %s", name);
    if (ref && ref[0]) {
        printf(" (%s)", ref);
    }
    printf(" ... ");
    if (strcmp(result, "done") == 0) {
        printf("%sdone%s\n", COLOR_CYAN, COLOR_RESET);
    } else {
        printf("%s%s%s\n", COLOR_RED, result, COLOR_RESET);
    }
}

/* Dependencies directory */
#define PKG_DEPS_DIR ".sn"
#define PKG_YAML_FILE "sn.yaml"
#define PKG_CACHE_DIR ".sn-cache"

/* Include split modules (unity build style for internal helpers) */
#include "package/package_util.c"
#include "package/package_script.c"
#include "package/package_visit.c"
#include "package/package_url.c"

/* ============================================================================
 * Package Cache API
 * ============================================================================ */

/* Get the package cache directory path */
bool package_get_cache_dir(char *out_path, size_t out_len)
{
    char home[PKG_MAX_PATH_LEN];
    if (!get_home_dir(home, sizeof(home))) {
        return false;
    }
    snprintf(out_path, out_len, "%s%c%s", home, PATH_SEP, PKG_CACHE_DIR);
    return true;
}

/* Clear the package cache directory */
bool package_clear_cache(void)
{
    char cache_dir[PKG_MAX_PATH_LEN];
    if (!package_get_cache_dir(cache_dir, sizeof(cache_dir))) {
        pkg_error("failed to determine cache directory");
        return false;
    }

    if (!dir_exists(cache_dir)) {
        printf("Cache directory is empty.\n");
        return true;
    }

    printf("Clearing package cache: %s\n", cache_dir);

    if (!remove_directory_recursive(cache_dir)) {
        pkg_error("failed to remove cache directory");
        return false;
    }

    printf("Package cache cleared.\n");
    return true;
}

/* ============================================================================
 * Package Manager API Implementation
 * ============================================================================ */

bool package_yaml_exists(void)
{
    return file_exists(PKG_YAML_FILE);
}

bool package_deps_installed(void)
{
    if (!package_yaml_exists()) {
        return true; /* No config means no deps needed */
    }

    PackageConfig config;
    if (!package_yaml_parse(PKG_YAML_FILE, &config)) {
        return false;
    }

    /* Check each dependency */
    for (int i = 0; i < config.dependency_count; i++) {
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s",
                 PKG_DEPS_DIR, PATH_SEP, config.dependencies[i].name);

        if (!dir_exists(dep_path) || !package_git_is_repo(dep_path)) {
            return false;
        }
    }

    return true;
}

bool package_init(void)
{
    /* Check if sn.yaml already exists */
    if (package_yaml_exists()) {
        pkg_error("sn.yaml already exists in this directory");
        return false;
    }

    PackageConfig config;
    memset(&config, 0, sizeof(config));

    char default_name[PKG_MAX_NAME_LEN];
    pkg_get_dirname(default_name, sizeof(default_name));

    /* Prompt for project metadata */
    printf("Project name [%s]: ", default_name);
    fflush(stdout);
    read_line(config.name, sizeof(config.name), default_name);

    printf("Version [1.0.0]: ");
    fflush(stdout);
    read_line(config.version, sizeof(config.version), "1.0.0");

    printf("Author []: ");
    fflush(stdout);
    read_line(config.author, sizeof(config.author), "");

    printf("Description []: ");
    fflush(stdout);
    read_line(config.description, sizeof(config.description), "");

    printf("License [MIT]: ");
    fflush(stdout);
    read_line(config.license, sizeof(config.license), "MIT");

    /* Add default SDK dependency (provides sindarin-libs transitively) */
    PackageDependency *sdk_dep = &config.dependencies[0];
    strncpy(sdk_dep->name, "sindarin-pkg-sdk", sizeof(sdk_dep->name) - 1);
    strncpy(sdk_dep->git_url, "git@github.com:SindarinSDK/sindarin-pkg-sdk.git", sizeof(sdk_dep->git_url) - 1);
    strncpy(sdk_dep->branch, "main", sizeof(sdk_dep->branch) - 1);
    config.dependency_count = 1;

    /* Write sn.yaml */
    if (!package_yaml_write(PKG_YAML_FILE, &config)) {
        pkg_error("failed to create sn.yaml");
        return false;
    }

    printf("\nCreated sn.yaml with sindarin-pkg-sdk dependency\n");
    return true;
}

/* Include remaining split modules */
#include "package/package_sync.c"
#include "package/package_install.c"
