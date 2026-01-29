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
    return _mkdir(path) == 0;
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

/* Recursively remove a directory and its contents */
static bool remove_directory_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return false;
    }

    struct dirent *entry;
    bool success = true;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PKG_MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s%c%s", path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* Recursively remove subdirectory */
                if (!remove_directory_recursive(full_path)) {
                    success = false;
                }
            } else {
                /* Remove file */
                if (unlink(full_path) != 0) {
                    success = false;
                }
            }
        }
    }

    closedir(dir);

    /* Remove the directory itself */
    if (rmdir(path) != 0) {
        success = false;
    }

    return success;
}

/* ============================================================================
 * Package Visit Tracking (for recursive installation cycle detection)
 * ============================================================================ */

/* Check if package has been visited during recursive install */
static bool package_is_visited(const PackageVisited *visited, const char *name)
{
    for (int i = 0; i < visited->count; i++) {
        if (strcmp(visited->names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

/* Get the ref (version) for a visited package, returns NULL if not found */
static const char *package_get_visited_ref(const PackageVisited *visited, const char *name)
{
    for (int i = 0; i < visited->count; i++) {
        if (strcmp(visited->names[i], name) == 0) {
            return visited->refs[i][0] ? visited->refs[i] : NULL;
        }
    }
    return NULL;
}

/* Mark package as visited, returns false if at capacity */
static bool package_mark_visited(PackageVisited *visited, const char *name, const char *ref)
{
    if (visited->count >= PKG_MAX_VISITED) {
        pkg_warning("too many packages, some may not be tracked for cycles");
        return false;
    }
    strncpy(visited->names[visited->count], name, PKG_MAX_NAME_LEN - 1);
    visited->names[visited->count][PKG_MAX_NAME_LEN - 1] = '\0';
    if (ref && ref[0]) {
        strncpy(visited->refs[visited->count], ref, PKG_MAX_REF_LEN - 1);
        visited->refs[visited->count][PKG_MAX_REF_LEN - 1] = '\0';
    } else {
        visited->refs[visited->count][0] = '\0';
    }
    visited->count++;
    return true;
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

/* ============================================================================
 * Package Synchronization
 * ============================================================================ */

bool package_sync(void)
{
    if (!package_yaml_exists()) {
        /* No sn.yaml - remove .sn directory if it exists */
        if (dir_exists(PKG_DEPS_DIR)) {
            printf("Removing orphaned %s directory (no sn.yaml found)...\n", PKG_DEPS_DIR);
            if (!remove_directory_recursive(PKG_DEPS_DIR)) {
                pkg_warning("failed to remove %s directory", PKG_DEPS_DIR);
            }
        }
        return true;
    }

    PackageConfig config;
    if (!package_yaml_parse(PKG_YAML_FILE, &config)) {
        pkg_error("failed to parse sn.yaml");
        return false;
    }

    /* If no .sn directory and no deps, nothing to do */
    if (!dir_exists(PKG_DEPS_DIR)) {
        if (config.dependency_count == 0) {
            return true;
        }
        /* Dependencies exist but no .sn dir - will be handled by install_all */
        return true;
    }

    /* Initialize git library */
    package_git_init();

    bool any_changes = false;
    bool success = true;

    /* Step 1: Remove packages that are not in sn.yaml */
    DIR *dir = opendir(PKG_DEPS_DIR);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            /* Skip . and .. and hidden files */
            if (entry->d_name[0] == '.') {
                continue;
            }

            char pkg_path[PKG_MAX_PATH_LEN];
            snprintf(pkg_path, sizeof(pkg_path), "%s%c%s",
                     PKG_DEPS_DIR, PATH_SEP, entry->d_name);

            /* Only consider directories */
            if (!dir_exists(pkg_path)) {
                continue;
            }

            /* Check if this package is in sn.yaml */
            bool found = false;
            for (int i = 0; i < config.dependency_count; i++) {
                if (strcmp(config.dependencies[i].name, entry->d_name) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                printf("Removing orphaned package: %s\n", entry->d_name);
                if (!remove_directory_recursive(pkg_path)) {
                    pkg_warning("failed to remove %s", entry->d_name);
                    success = false;
                } else {
                    any_changes = true;
                }
            }
        }
        closedir(dir);
    }

    /* Step 2: Check each dependency in sn.yaml */
    for (int i = 0; i < config.dependency_count; i++) {
        PackageDependency *dep = &config.dependencies[i];
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s",
                 PKG_DEPS_DIR, PATH_SEP, dep->name);

        /* Skip packages that don't exist yet (will be installed later) */
        if (!dir_exists(dep_path) || !package_git_is_repo(dep_path)) {
            continue;
        }

        const char *expected_ref = dep->tag[0] ? dep->tag : dep->branch;
        if (!expected_ref[0]) {
            /* No ref specified, skip sync check */
            continue;
        }

        bool is_tag = dep->tag[0] != '\0';

        if (is_tag) {
            /* For tags: verify the repo is at the correct SHA */
            char head_sha[41];
            char tag_sha[41];

            if (!package_git_get_head_sha(dep_path, head_sha)) {
                pkg_warning("could not get HEAD SHA for %s", dep->name);
                continue;
            }

            if (!package_git_get_ref_sha(dep_path, expected_ref, tag_sha)) {
                /* Tag doesn't exist locally, need to fetch */
                printf("Fetching tag %s for %s...\n", expected_ref, dep->name);
                if (!package_git_fetch(dep_path)) {
                    pkg_warning("failed to fetch %s", dep->name);
                    success = false;
                    continue;
                }
                /* Try again after fetch */
                if (!package_git_get_ref_sha(dep_path, expected_ref, tag_sha)) {
                    pkg_warning("tag %s not found for %s", expected_ref, dep->name);
                    success = false;
                    continue;
                }
            }

            if (strcmp(head_sha, tag_sha) != 0) {
                printf("Updating %s to tag %s...\n", dep->name, expected_ref);
                if (!package_git_checkout(dep_path, expected_ref)) {
                    pkg_warning("failed to checkout tag %s for %s", expected_ref, dep->name);
                    success = false;
                } else {
                    any_changes = true;
                    pkg_status(dep->name, expected_ref, "done");
                }
            }
        } else {
            /* For branches: check if on correct branch and up to date */
            char current_branch[PKG_MAX_REF_LEN];
            bool on_branch = package_git_get_current_branch(dep_path, current_branch,
                                                            sizeof(current_branch));

            bool need_switch = !on_branch || strcmp(current_branch, expected_ref) != 0;
            bool need_update = false;

            if (!need_switch) {
                /* On correct branch, check if up to date with remote */
                char head_sha[41];
                char remote_sha[41];

                if (package_git_get_head_sha(dep_path, head_sha)) {
                    /* Fetch to get latest remote state */
                    if (package_git_fetch(dep_path)) {
                        if (package_git_get_ref_sha(dep_path, expected_ref, remote_sha)) {
                            if (strcmp(head_sha, remote_sha) != 0) {
                                need_update = true;
                            }
                        }
                    }
                }
            }

            if (need_switch) {
                printf("Switching %s to branch %s...\n", dep->name, expected_ref);
                /* Fetch first to ensure we have the branch */
                if (!package_git_fetch(dep_path)) {
                    pkg_warning("failed to fetch %s", dep->name);
                    success = false;
                    continue;
                }
                if (!package_git_checkout(dep_path, expected_ref)) {
                    pkg_warning("failed to switch %s to branch %s", dep->name, expected_ref);
                    success = false;
                } else {
                    any_changes = true;
                    pkg_status(dep->name, expected_ref, "done");
                }
            } else if (need_update) {
                printf("Updating %s branch %s...\n", dep->name, expected_ref);
                if (!package_git_checkout(dep_path, expected_ref)) {
                    pkg_warning("failed to update %s", dep->name);
                    success = false;
                } else {
                    any_changes = true;
                    pkg_status(dep->name, expected_ref, "done");
                }
            }
        }
    }

    package_git_cleanup();

    if (any_changes) {
        printf("Package synchronization complete.\n");
    }

    return success;
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

    /* Write sn.yaml */
    if (!package_yaml_write(PKG_YAML_FILE, &config)) {
        pkg_error("failed to create sn.yaml");
        return false;
    }

    printf("\nCreated sn.yaml\n");
    return true;
}

/* ============================================================================
 * Recursive Dependency Installation
 * ============================================================================ */

/* Install dependencies recursively from a package directory.
 * base_path: Directory containing sn.yaml to process
 * visited: Set of already-processed packages (for cycle detection)
 * Returns true on success, false on failure */
static bool package_install_deps_recursive(const char *base_path, PackageVisited *visited)
{
    char yaml_path[PKG_MAX_PATH_LEN];
    snprintf(yaml_path, sizeof(yaml_path), "%s%c%s", base_path, PATH_SEP, PKG_YAML_FILE);

    /* Check if sn.yaml exists in this directory */
    if (!file_exists(yaml_path)) {
        return true;  /* No deps, that's ok */
    }

    PackageConfig config;
    if (!package_yaml_parse(yaml_path, &config)) {
        pkg_warning("failed to parse %s", yaml_path);
        return true;  /* Non-fatal, continue with other deps */
    }

    if (config.dependency_count == 0) {
        return true;
    }

    bool success = true;
    for (int i = 0; i < config.dependency_count; i++) {
        PackageDependency *dep = &config.dependencies[i];

        /* Determine ref to use */
        const char *ref = dep->tag[0] ? dep->tag : dep->branch;

        /* Check for cycle/already installed */
        if (package_is_visited(visited, dep->name)) {
            /* Check for version conflict */
            const char *existing_ref = package_get_visited_ref(visited, dep->name);
            if (ref[0] && existing_ref && strcmp(ref, existing_ref) != 0) {
                pkg_warning("version conflict for %s: %s requested but %s already installed",
                           dep->name, ref, existing_ref);
            }
            continue;  /* Already processed */
        }

        /* Mark as visited before installing (prevents cycles) */
        package_mark_visited(visited, dep->name, ref);

        /* Build path for this dependency (always in root .sn/) */
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s", PKG_DEPS_DIR, PATH_SEP, dep->name);

        const char *result = "done";

        if (dir_exists(dep_path) && package_git_is_repo(dep_path)) {
            /* Repository exists, fetch and checkout */
            if (!package_git_fetch(dep_path)) {
                result = "fetch failed";
                success = false;
            } else if (ref[0] && !package_git_checkout(dep_path, ref)) {
                result = "checkout failed";
                success = false;
            } else if (!ref[0]) {
                /* No checkout, pull LFS content now */
                package_lfs_pull(dep_path);
            }
            /* Note: if ref[0] && checkout succeeded, LFS pull happens in checkout */
        } else {
            /* Clone repository */
            if (!package_git_clone(dep->git_url, dep_path)) {
                result = "clone failed";
                success = false;
                pkg_status(dep->name, ref[0] ? ref : NULL, result);
                continue;  /* Can't recurse if clone failed */
            } else if (ref[0] && !package_git_checkout(dep_path, ref)) {
                result = "checkout failed";
                success = false;
            } else if (!ref[0]) {
                /* No checkout, pull LFS content now */
                package_lfs_pull(dep_path);
            }
            /* Note: if ref[0] && checkout succeeded, LFS pull happens in checkout */
        }

        pkg_status(dep->name, ref[0] ? ref : NULL, result);

        /* Recurse into the installed package to install its dependencies */
        if (!package_install_deps_recursive(dep_path, visited)) {
            success = false;  /* Non-fatal, continue with other deps */
        }
    }

    return success;
}

bool package_install_all(void)
{
    if (!package_yaml_exists()) {
        pkg_error("no sn.yaml found in current directory");
        return false;
    }

    PackageConfig config;
    if (!package_yaml_parse(PKG_YAML_FILE, &config)) {
        pkg_error("failed to parse sn.yaml");
        return false;
    }

    if (config.dependency_count == 0) {
        printf("No dependencies to install\n");
        return true;
    }

    /* Ensure .sn directory exists */
    if (!ensure_dir(PKG_DEPS_DIR)) {
        pkg_error("failed to create %s directory", PKG_DEPS_DIR);
        return false;
    }

    printf("Installing dependencies from sn.yaml...\n");

    /* Initialize git library */
    package_git_init();

    /* Initialize visited set for cycle detection */
    PackageVisited visited;
    memset(&visited, 0, sizeof(visited));

    /* Recursively install all dependencies starting from current directory */
    bool success = package_install_deps_recursive(".", &visited);

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
        pkg_error("cannot determine package name from URL: %s", url);
        return false;
    }

    /* Ensure .sn directory exists */
    if (!ensure_dir(PKG_DEPS_DIR)) {
        pkg_error("failed to create %s directory", PKG_DEPS_DIR);
        return false;
    }

    char dep_path[PKG_MAX_PATH_LEN];
    snprintf(dep_path, sizeof(dep_path), "%s%c%s", PKG_DEPS_DIR, PATH_SEP, name);

    printf("Installing %s", name);
    if (has_ref) {
        printf(" (%s)", ref);
    }
    printf("...\n");

    /* Initialize git library */
    package_git_init();

    const char *result = "done";
    bool success = true;

    if (dir_exists(dep_path) && package_git_is_repo(dep_path)) {
        /* Repository exists, fetch and checkout */
        if (!package_git_fetch(dep_path)) {
            result = "fetch failed";
            success = false;
        } else if (has_ref && !package_git_checkout(dep_path, ref)) {
            result = "checkout failed";
            success = false;
        }
    } else {
        /* Clone repository */
        if (!package_git_clone(url, dep_path)) {
            result = "clone failed";
            success = false;
        } else if (has_ref && !package_git_checkout(dep_path, ref)) {
            result = "checkout failed";
            success = false;
        }
    }

    pkg_status(name, has_ref ? ref : NULL, result);

    package_git_cleanup();

    if (!success) {
        return false;
    }

    /* Install transitive dependencies of the new package */
    char yaml_check[PKG_MAX_PATH_LEN];
    snprintf(yaml_check, sizeof(yaml_check), "%s%c%s", dep_path, PATH_SEP, PKG_YAML_FILE);
    if (file_exists(yaml_check)) {
        printf("Installing transitive dependencies...\n");
        package_git_init();

        PackageVisited visited;
        memset(&visited, 0, sizeof(visited));
        package_mark_visited(&visited, name, has_ref ? ref : NULL);

        if (!package_install_deps_recursive(dep_path, &visited)) {
            pkg_warning("some transitive dependencies failed to install");
        }

        package_git_cleanup();
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
            pkg_warning("failed to create sn.yaml");
        } else {
            printf("Created sn.yaml with %s dependency\n", name);
        }
    } else {
        /* Add dependency to existing sn.yaml */
        if (!package_yaml_add_dependency(PKG_YAML_FILE, &dep)) {
            pkg_warning("failed to update sn.yaml");
        } else {
            printf("Added %s to sn.yaml\n", name);
        }
    }

    return true;
}
