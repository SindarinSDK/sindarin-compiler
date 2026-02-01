/* ==============================================================================
 * package_sync.c - Package Synchronization
 * ==============================================================================
 * Functions for synchronizing packages with sn.yaml.
 * ============================================================================== */

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

    /* Collect ALL transitive dependencies (not just direct deps) */
    PackageVisited all_deps;
    package_collect_all_deps(&all_deps);

    /* Step 1: Remove packages that are not in the transitive dependency tree */
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

            /* Check if this package is in the transitive dependency tree */
            bool found = package_is_visited(&all_deps, entry->d_name);

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
