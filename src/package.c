/* ==============================================================================
 * package.c - Sindarin Package Manager Implementation
 * ==============================================================================
 * Main orchestration for package management operations.
 * ============================================================================== */

#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEP '\\'
#else
    #include <unistd.h>
    #define PATH_SEP '/'
#endif

/* Dependencies directory */
#define PKG_DEPS_DIR ".sn"
#define PKG_YAML_FILE "sn.yaml"

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Check if a file exists */
static bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* Check if a directory exists */
static bool dir_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Create directory if it doesn't exist */
static bool ensure_dir(const char *path)
{
    if (dir_exists(path)) {
        return true;
    }

#ifdef _WIN32
    return mkdir(path) == 0;
#else
    return mkdir(path, 0755) == 0;
#endif
}

/* Read a line from stdin, stripping newline */
static void read_line(char *buffer, size_t size, const char *default_val)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
    }

    /* Strip newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    /* Use default if empty */
    if (len == 0 && default_val != NULL) {
        strncpy(buffer, default_val, size - 1);
        buffer[size - 1] = '\0';
    }
}

/* Get current directory name for default project name */
static void pkg_get_dirname(char *buffer, size_t size)
{
    char cwd[PKG_MAX_PATH_LEN];

#ifdef _WIN32
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
#else
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
#endif
        strncpy(buffer, "my-project", size - 1);
        buffer[size - 1] = '\0';
        return;
    }

    /* Find last path separator */
    const char *last_sep = strrchr(cwd, PATH_SEP);
    if (last_sep != NULL) {
        strncpy(buffer, last_sep + 1, size - 1);
    } else {
        strncpy(buffer, cwd, size - 1);
    }
    buffer[size - 1] = '\0';
}

/* ============================================================================
 * URL Parsing
 * ============================================================================ */

bool package_parse_url_ref(const char *url_ref, char *out_url, char *out_ref)
{
    if (url_ref == NULL || out_url == NULL || out_ref == NULL) {
        return false;
    }

    /* Look for @ that separates URL from ref */
    const char *at_pos = strrchr(url_ref, '@');

    /* Check if @ is part of git@ URL scheme */
    if (at_pos != NULL) {
        /* git@github.com:user/repo.git has @ at position 3 */
        /* We want the @ after .git or at the end for ref */
        const char *git_ext = strstr(url_ref, ".git");
        if (git_ext != NULL && at_pos > git_ext) {
            /* @ is after .git, so it's a ref separator */
            size_t url_len = (size_t)(at_pos - url_ref);
            strncpy(out_url, url_ref, url_len);
            out_url[url_len] = '\0';
            strncpy(out_ref, at_pos + 1, PKG_MAX_REF_LEN - 1);
            out_ref[PKG_MAX_REF_LEN - 1] = '\0';
            return true;
        } else if (git_ext == NULL) {
            /* No .git extension, check if @ is after the last / */
            const char *last_slash = strrchr(url_ref, '/');
            if (last_slash != NULL && at_pos > last_slash) {
                size_t url_len = (size_t)(at_pos - url_ref);
                strncpy(out_url, url_ref, url_len);
                out_url[url_len] = '\0';
                strncpy(out_ref, at_pos + 1, PKG_MAX_REF_LEN - 1);
                out_ref[PKG_MAX_REF_LEN - 1] = '\0';
                return true;
            }
        }
    }

    /* No ref specified, copy URL as-is */
    strncpy(out_url, url_ref, PKG_MAX_URL_LEN - 1);
    out_url[PKG_MAX_URL_LEN - 1] = '\0';
    out_ref[0] = '\0';
    return false;
}

bool package_extract_name(const char *url, char *out_name)
{
    if (url == NULL || out_name == NULL) {
        return false;
    }

    /* Find last / or : */
    const char *start = strrchr(url, '/');
    if (start == NULL) {
        start = strrchr(url, ':');
    }

    if (start == NULL) {
        return false;
    }

    start++; /* Skip the separator */

    /* Copy name, stripping .git extension */
    const char *dot_git = strstr(start, ".git");
    size_t len;

    if (dot_git != NULL) {
        len = (size_t)(dot_git - start);
    } else {
        len = strlen(start);
    }

    if (len >= PKG_MAX_NAME_LEN) {
        len = PKG_MAX_NAME_LEN - 1;
    }

    strncpy(out_name, start, len);
    out_name[len] = '\0';

    return len > 0;
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
        fprintf(stderr, "Error: sn.yaml already exists in this directory\n");
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

    /* Write sn.yaml */
    if (!package_yaml_write(PKG_YAML_FILE, &config)) {
        fprintf(stderr, "Error: Failed to create sn.yaml\n");
        return false;
    }

    printf("\nCreated sn.yaml\n");
    return true;
}

bool package_install_all(void)
{
    if (!package_yaml_exists()) {
        fprintf(stderr, "Error: No sn.yaml found in current directory\n");
        return false;
    }

    PackageConfig config;
    if (!package_yaml_parse(PKG_YAML_FILE, &config)) {
        fprintf(stderr, "Error: Failed to parse sn.yaml\n");
        return false;
    }

    if (config.dependency_count == 0) {
        printf("No dependencies to install\n");
        return true;
    }

    /* Ensure .sn directory exists */
    if (!ensure_dir(PKG_DEPS_DIR)) {
        fprintf(stderr, "Error: Failed to create %s directory\n", PKG_DEPS_DIR);
        return false;
    }

    printf("Installing dependencies from sn.yaml...\n");

    /* Initialize git library */
    package_git_init();

    bool success = true;
    for (int i = 0; i < config.dependency_count; i++) {
        PackageDependency *dep = &config.dependencies[i];
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s",
                 PKG_DEPS_DIR, PATH_SEP, dep->name);

        /* Determine ref to use */
        const char *ref = dep->tag[0] ? dep->tag : dep->branch;

        printf("  %s", dep->name);
        if (ref[0]) {
            printf(" (%s)", ref);
        }
        printf(" ... ");
        fflush(stdout);

        if (dir_exists(dep_path) && package_git_is_repo(dep_path)) {
            /* Repository exists, fetch and checkout */
            if (!package_git_fetch(dep_path)) {
                printf("fetch failed\n");
                success = false;
                continue;
            }

            if (ref[0] && !package_git_checkout(dep_path, ref)) {
                printf("checkout failed\n");
                success = false;
                continue;
            }
        } else {
            /* Clone repository */
            if (!package_git_clone(dep->git_url, dep_path)) {
                printf("clone failed\n");
                success = false;
                continue;
            }

            if (ref[0] && !package_git_checkout(dep_path, ref)) {
                printf("checkout failed\n");
                success = false;
                continue;
            }
        }

        printf("done\n");
    }

    package_git_cleanup();
    return success;
}

bool package_install(const char *url_ref)
{
    if (url_ref == NULL || url_ref[0] == '\0') {
        return package_install_all();
    }

    /* Parse URL and ref */
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];
    bool has_ref = package_parse_url_ref(url_ref, url, ref);

    /* Extract package name */
    char name[PKG_MAX_NAME_LEN];
    if (!package_extract_name(url, name)) {
        fprintf(stderr, "Error: Cannot determine package name from URL: %s\n", url);
        return false;
    }

    /* Ensure .sn directory exists */
    if (!ensure_dir(PKG_DEPS_DIR)) {
        fprintf(stderr, "Error: Failed to create %s directory\n", PKG_DEPS_DIR);
        return false;
    }

    char dep_path[PKG_MAX_PATH_LEN];
    snprintf(dep_path, sizeof(dep_path), "%s%c%s", PKG_DEPS_DIR, PATH_SEP, name);

    printf("Installing %s", name);
    if (has_ref) {
        printf(" (%s)", ref);
    }
    printf(" ... ");
    fflush(stdout);

    /* Initialize git library */
    package_git_init();

    bool success = true;

    if (dir_exists(dep_path) && package_git_is_repo(dep_path)) {
        /* Repository exists, fetch and checkout */
        if (!package_git_fetch(dep_path)) {
            printf("fetch failed\n");
            success = false;
        } else if (has_ref && !package_git_checkout(dep_path, ref)) {
            printf("checkout failed\n");
            success = false;
        } else {
            printf("done\n");
        }
    } else {
        /* Clone repository */
        if (!package_git_clone(url, dep_path)) {
            printf("clone failed\n");
            success = false;
        } else if (has_ref && !package_git_checkout(dep_path, ref)) {
            printf("checkout failed\n");
            success = false;
        } else {
            printf("done\n");
        }
    }

    package_git_cleanup();

    if (!success) {
        return false;
    }

    /* Update sn.yaml with new dependency */
    PackageDependency dep;
    memset(&dep, 0, sizeof(dep));
    strncpy(dep.name, name, sizeof(dep.name) - 1);
    strncpy(dep.git_url, url, sizeof(dep.git_url) - 1);

    /* Determine if ref is a tag (starts with 'v') or branch */
    if (has_ref) {
        if (ref[0] == 'v' && (ref[1] >= '0' && ref[1] <= '9')) {
            strncpy(dep.tag, ref, sizeof(dep.tag) - 1);
        } else {
            strncpy(dep.branch, ref, sizeof(dep.branch) - 1);
        }
    }

    /* Create sn.yaml if it doesn't exist */
    if (!package_yaml_exists()) {
        PackageConfig config;
        memset(&config, 0, sizeof(config));

        char default_name[PKG_MAX_NAME_LEN];
        pkg_get_dirname(default_name, sizeof(default_name));
        strncpy(config.name, default_name, sizeof(config.name) - 1);
        strncpy(config.version, "1.0.0", sizeof(config.version) - 1);
        strncpy(config.license, "MIT", sizeof(config.license) - 1);

        config.dependencies[0] = dep;
        config.dependency_count = 1;

        if (!package_yaml_write(PKG_YAML_FILE, &config)) {
            fprintf(stderr, "Warning: Failed to create sn.yaml\n");
        } else {
            printf("Created sn.yaml with %s dependency\n", name);
        }
    } else {
        /* Add dependency to existing sn.yaml */
        if (!package_yaml_add_dependency(PKG_YAML_FILE, &dep)) {
            fprintf(stderr, "Warning: Failed to update sn.yaml\n");
        } else {
            printf("Added %s to sn.yaml\n", name);
        }
    }

    return true;
}
