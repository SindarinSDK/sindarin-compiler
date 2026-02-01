/* ==============================================================================
 * package_visit.c - Package Visit Tracking
 * ==============================================================================
 * Functions for tracking visited packages during recursive installation
 * to detect cycles and version conflicts.
 * ============================================================================== */

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

/* ==============================================================================
 * Transitive Dependency Collection
 * ==============================================================================
 * Collects all transitive dependencies using FLAT paths (.sn/<name>).
 * Used by both package_sync and gcc_backend for consistent dependency handling.
 * ============================================================================== */

/* Recursively collect all transitive dependencies from a package directory.
 * pkg_path: Path to the package (e.g., ".sn/sindarin-pkg-sdk")
 * all_deps: Output structure to collect all dependency names
 * Uses FLAT paths - all packages are at .sn/<name> level */
static void package_collect_deps_recursive(const char *pkg_path, PackageVisited *all_deps)
{
    char yaml_path[PKG_MAX_PATH_LEN];
    snprintf(yaml_path, sizeof(yaml_path), "%s%c%s", pkg_path, PATH_SEP, PKG_YAML_FILE);

    /* Check if sn.yaml exists in this package */
    if (!file_exists(yaml_path)) {
        return;
    }

    PackageConfig config;
    if (!package_yaml_parse(yaml_path, &config)) {
        return;
    }

    /* Process each dependency */
    for (int i = 0; i < config.dependency_count; i++) {
        const char *dep_name = config.dependencies[i].name;

        /* Skip if already collected (cycle detection) */
        if (package_is_visited(all_deps, dep_name)) {
            continue;
        }

        /* Add to collected deps */
        package_mark_visited(all_deps, dep_name, NULL);

        /* Recurse into this dependency (using FLAT path at root .sn/) */
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s", PKG_DEPS_DIR, PATH_SEP, dep_name);

        if (dir_exists(dep_path)) {
            package_collect_deps_recursive(dep_path, all_deps);
        }
    }
}

/* Collect all transitive dependencies for the current project.
 * Reads sn.yaml from current directory and recursively collects all deps.
 * all_deps: Output structure - will contain names of ALL required packages
 * Returns true if sn.yaml exists and was parsed, false otherwise */
static bool package_collect_all_deps(PackageVisited *all_deps)
{
    memset(all_deps, 0, sizeof(PackageVisited));

    if (!package_yaml_exists()) {
        return false;
    }

    PackageConfig config;
    if (!package_yaml_parse(PKG_YAML_FILE, &config)) {
        return false;
    }

    /* First add all direct dependencies */
    for (int i = 0; i < config.dependency_count; i++) {
        const char *dep_name = config.dependencies[i].name;

        if (package_is_visited(all_deps, dep_name)) {
            continue;
        }

        package_mark_visited(all_deps, dep_name, NULL);

        /* Then recurse to find transitive dependencies */
        char dep_path[PKG_MAX_PATH_LEN];
        snprintf(dep_path, sizeof(dep_path), "%s%c%s", PKG_DEPS_DIR, PATH_SEP, dep_name);

        if (dir_exists(dep_path)) {
            package_collect_deps_recursive(dep_path, all_deps);
        }
    }

    return true;
}
