/* ==============================================================================
 * package_install.c - Package Installation
 * ==============================================================================
 * Functions for installing packages with caching and recursive dependency handling.
 * ============================================================================== */

/* Install a single package using the cache.
 * This function:
 * 1. Checks if package exists in destination - if so, fetches/updates
 * 2. Checks if package exists in cache - if so, copies from cache
 * 3. Otherwise clones to cache, then copies to destination
 * 4. Runs install script if present
 *
 * Returns true on success, false on failure */
static bool package_install_single_cached(const char *name, const char *git_url,
                                          const char *ref, const char *dep_path)
{
    char cache_dir[PKG_MAX_PATH_LEN];
    char cache_pkg_path[PKG_MAX_PATH_LEN];
    bool need_copy_from_cache = false;

    /* Get cache directory */
    if (!package_get_cache_dir(cache_dir, sizeof(cache_dir))) {
        pkg_warning("failed to determine cache directory, falling back to direct clone");
        cache_dir[0] = '\0';
    } else {
        snprintf(cache_pkg_path, sizeof(cache_pkg_path), "%s%c%s",
                 cache_dir, PATH_SEP, name);
    }

    const char *result = "done";

    /* Case 1: Package already exists in destination */
    if (dir_exists(dep_path) && package_git_is_repo(dep_path)) {
        /* Repository exists, fetch and checkout */
        if (!package_git_fetch(dep_path)) {
            result = "fetch failed";
            pkg_status(name, ref[0] ? ref : NULL, result);
            return false;
        }
        if (ref[0] && !package_git_checkout(dep_path, ref)) {
            result = "checkout failed";
            pkg_status(name, ref[0] ? ref : NULL, result);
            return false;
        }
        if (!ref[0]) {
            package_lfs_pull(dep_path);
        }
        pkg_status(name, ref[0] ? ref : NULL, result);
        return true;
    }

    /* Case 2: Check cache */
    if (cache_dir[0] != '\0') {
        /* Ensure cache directory exists */
        if (!ensure_dir(cache_dir)) {
            pkg_warning("failed to create cache directory");
            cache_dir[0] = '\0';  /* Fall back to direct clone */
        }
    }

    if (cache_dir[0] != '\0' && dir_exists(cache_pkg_path) && package_git_is_repo(cache_pkg_path)) {
        /* Package exists in cache - fetch latest and copy */
        printf("  %s (cached)", name);
        if (ref[0]) {
            printf(" (%s)", ref);
        }
        printf(" ... ");
        fflush(stdout);

        /* Fetch to update cache */
        if (!package_git_fetch(cache_pkg_path)) {
            printf("%sfetch failed%s\n", COLOR_RED, COLOR_RESET);
            /* Try to use stale cache anyway */
        }

        /* Checkout the required ref in cache */
        if (ref[0] && !package_git_checkout(cache_pkg_path, ref)) {
            printf("%scheckout failed%s\n", COLOR_RED, COLOR_RESET);
            return false;
        }

        need_copy_from_cache = true;
    } else if (cache_dir[0] != '\0') {
        /* Clone to cache first */
        printf("  %s", name);
        if (ref[0]) {
            printf(" (%s)", ref);
        }
        printf(" ... ");
        fflush(stdout);

        if (!package_git_clone(git_url, cache_pkg_path)) {
            printf("%sclone failed%s\n", COLOR_RED, COLOR_RESET);
            return false;
        }

        if (ref[0] && !package_git_checkout(cache_pkg_path, ref)) {
            printf("%scheckout failed%s\n", COLOR_RED, COLOR_RESET);
            return false;
        }

        if (!ref[0]) {
            package_lfs_pull(cache_pkg_path);
        }

        need_copy_from_cache = true;
    }

    /* Copy from cache to destination */
    if (need_copy_from_cache) {
        if (!copy_directory_recursive(cache_pkg_path, dep_path)) {
            printf("%scopy failed%s\n", COLOR_RED, COLOR_RESET);
            return false;
        }
        printf("%sdone%s\n", COLOR_CYAN, COLOR_RESET);
        fflush(stdout);

        /* Run install script after copy */
        package_run_install_script(dep_path);
    } else {
        /* No cache available, clone directly to destination */
        if (!package_git_clone(git_url, dep_path)) {
            result = "clone failed";
            pkg_status(name, ref[0] ? ref : NULL, result);
            return false;
        }
        if (ref[0] && !package_git_checkout(dep_path, ref)) {
            result = "checkout failed";
            pkg_status(name, ref[0] ? ref : NULL, result);
            return false;
        }
        if (!ref[0]) {
            package_lfs_pull(dep_path);
        }
        pkg_status(name, ref[0] ? ref : NULL, result);
        fflush(stdout);

        /* Run install script after clone */
        package_run_install_script(dep_path);
    }

    return true;
}

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

        /* Install using cache */
        if (!package_install_single_cached(dep->name, dep->git_url, ref, dep_path)) {
            success = false;
            continue;  /* Can't recurse if install failed */
        }

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

    /* Install using cache */
    bool success = package_install_single_cached(name, url, has_ref ? ref : "", dep_path);

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
