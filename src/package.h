/* ==============================================================================
 * package.h - Sindarin Package Manager
 * ==============================================================================
 * Provides package management capabilities for Sindarin projects.
 *
 * Commands:
 *   sn --init                Initialize a new project with sn.yaml
 *   sn --install             Install all dependencies from sn.yaml
 *   sn --install <url@ref>   Install a specific package
 *
 * YAML Schema (sn.yaml):
 *   name: my-project
 *   version: 1.0.0
 *   author: "John Doe"
 *   description: "A Sindarin project"
 *   license: MIT
 *   dependencies:
 *     - name: utils
 *       git: https://github.com/user/sn-utils.git
 *       tag: v1.2.0
 * ============================================================================== */

#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

/* Maximum lengths for package fields */
#define PKG_MAX_NAME_LEN 128
#define PKG_MAX_VERSION_LEN 32
#define PKG_MAX_URL_LEN 512
#define PKG_MAX_REF_LEN 128
#define PKG_MAX_PATH_LEN 1024
#define PKG_MAX_DEPS 64
#define PKG_MAX_VISITED 256

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/* Represents a single package dependency */
typedef struct {
    char name[PKG_MAX_NAME_LEN];      /* Package name (derived from URL) */
    char git_url[PKG_MAX_URL_LEN];    /* Git repository URL */
    char tag[PKG_MAX_REF_LEN];        /* Tag reference (e.g., "v1.0.0") */
    char branch[PKG_MAX_REF_LEN];     /* Branch reference (e.g., "main") */
} PackageDependency;

/* Represents the project's package configuration (sn.yaml) */
typedef struct {
    char name[PKG_MAX_NAME_LEN];
    char version[PKG_MAX_VERSION_LEN];
    char author[PKG_MAX_NAME_LEN];
    char description[PKG_MAX_URL_LEN];
    char license[PKG_MAX_NAME_LEN];
    PackageDependency dependencies[PKG_MAX_DEPS];
    int dependency_count;
} PackageConfig;

/* Tracks visited packages during recursive installation (for cycle detection) */
typedef struct {
    char names[PKG_MAX_VISITED][PKG_MAX_NAME_LEN];
    char refs[PKG_MAX_VISITED][PKG_MAX_REF_LEN];   /* For version conflict detection */
    int count;
} PackageVisited;

/* Result structure for package operations */
typedef struct {
    bool success;
    char message[PKG_MAX_URL_LEN];
} PackageResult;

/* ============================================================================
 * Package Manager API
 * ============================================================================ */

/* Initialize a new project (--init command)
 * Prompts for project metadata and creates sn.yaml
 * Returns true on success, false on failure */
bool package_init(void);

/* Install dependencies from sn.yaml (--install without args)
 * Clones/updates all packages in the dependencies list
 * Returns true on success, false on failure */
bool package_install_all(void);

/* Install a specific package (--install <url@ref>)
 * url_ref: URL with optional @tag or @branch suffix
 * Returns true on success, false on failure */
bool package_install(const char *url_ref);

/* Check if sn.yaml exists in current directory */
bool package_yaml_exists(void);

/* Check if all dependencies are installed */
bool package_deps_installed(void);

/* ============================================================================
 * YAML Operations (package_yaml.c)
 * ============================================================================ */

/* Parse sn.yaml file into PackageConfig structure
 * Returns true on success, false on failure */
bool package_yaml_parse(const char *path, PackageConfig *config);

/* Write PackageConfig to sn.yaml file
 * Returns true on success, false on failure */
bool package_yaml_write(const char *path, const PackageConfig *config);

/* Add a dependency to sn.yaml
 * Returns true on success, false on failure */
bool package_yaml_add_dependency(const char *path, const PackageDependency *dep);

/* ============================================================================
 * Git Operations (package_git.c)
 * ============================================================================ */

/* Clone a repository to the specified path
 * url: Git repository URL
 * dest_path: Destination directory
 * Returns true on success, false on failure */
bool package_git_clone(const char *url, const char *dest_path);

/* Checkout a specific tag or branch
 * repo_path: Path to the repository
 * ref_name: Tag or branch name to checkout
 * Returns true on success, false on failure */
bool package_git_checkout(const char *repo_path, const char *ref_name);

/* Fetch updates from remote
 * repo_path: Path to the repository
 * Returns true on success, false on failure */
bool package_git_fetch(const char *repo_path);

/* Check if a path is a valid git repository */
bool package_git_is_repo(const char *path);

/* Initialize libgit2 (called once at startup) */
void package_git_init(void);

/* Cleanup libgit2 resources */
void package_git_cleanup(void);

/* Get the current HEAD SHA of a repository
 * repo_path: Path to the repository
 * out_sha: Output buffer for SHA string (at least 41 bytes)
 * Returns true on success, false on failure */
bool package_git_get_head_sha(const char *repo_path, char *out_sha);

/* Get the SHA for a specific ref (tag or branch)
 * repo_path: Path to the repository
 * ref_name: Tag or branch name
 * out_sha: Output buffer for SHA string (at least 41 bytes)
 * Returns true on success, false on failure */
bool package_git_get_ref_sha(const char *repo_path, const char *ref_name, char *out_sha);

/* Get the current branch name (or empty string if detached)
 * repo_path: Path to the repository
 * out_branch: Output buffer for branch name
 * out_len: Size of output buffer
 * Returns true if on a branch, false if detached or error */
bool package_git_get_current_branch(const char *repo_path, char *out_branch, size_t out_len);

/* ============================================================================
 * Package Synchronization
 * ============================================================================ */

/* Synchronize packages with sn.yaml
 * - Removes packages not in sn.yaml
 * - Updates packages with changed branches
 * - Verifies tag packages are at correct SHA
 * Returns true on success, false on failure */
bool package_sync(void);

/* ============================================================================
 * URL Parsing Utilities
 * ============================================================================ */

/* Parse a URL@ref string into components
 * url_ref: Input string (e.g., "https://github.com/user/repo.git@v1.0")
 * out_url: Output URL buffer
 * out_ref: Output ref buffer (tag/branch)
 * Returns true if ref was found, false if using default branch */
bool package_parse_url_ref(const char *url_ref, char *out_url, char *out_ref);

/* Extract package name from URL
 * url: Git repository URL
 * out_name: Output buffer for package name
 * Returns true on success, false on failure */
bool package_extract_name(const char *url, char *out_name);

#endif /* PACKAGE_H */
