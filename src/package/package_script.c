/* ==============================================================================
 * package_script.c - Package Install Script Handling
 * ==============================================================================
 * Functions for running post-install scripts.
 * ============================================================================== */

/* Run post-install script if it exists
 * pkg_path: Path to the installed package
 * Returns true on success or if no script exists, false on script failure */
static bool package_run_install_script(const char *pkg_path)
{
    /* Find the directory containing sn.yaml (could be pkg_path or a subdirectory) */
    char yaml_path[PKG_MAX_PATH_LEN];
    char script_path[PKG_MAX_PATH_LEN];
#ifdef _WIN32
    char abs_pkg_path[PKG_MAX_PATH_LEN];
    char abs_script_path[PKG_MAX_PATH_LEN];
#else
    /* realpath() requires PATH_MAX sized buffer (4096 bytes typically) */
    char abs_pkg_path[PATH_MAX];
    char abs_script_path[PATH_MAX];
#endif

    /* Check for sn.yaml in the package root */
    snprintf(yaml_path, sizeof(yaml_path), "%s%c%s", pkg_path, PATH_SEP, PKG_YAML_FILE);

    if (!file_exists(yaml_path)) {
        /* No sn.yaml, no install script to run */
        return true;
    }

#ifdef _WIN32
    snprintf(script_path, sizeof(script_path), "%s%cscripts%cinstall.ps1",
             pkg_path, PATH_SEP, PATH_SEP);
#else
    snprintf(script_path, sizeof(script_path), "%s%cscripts%cinstall.sh",
             pkg_path, PATH_SEP, PATH_SEP);
#endif

    if (!file_exists(script_path)) {
        /* No install script, that's OK */
        return true;
    }

    /* Convert to absolute paths to avoid issues with relative path interpretation */
#ifdef _WIN32
    if (_fullpath(abs_pkg_path, pkg_path, sizeof(abs_pkg_path)) == NULL) {
        strncpy(abs_pkg_path, pkg_path, sizeof(abs_pkg_path) - 1);
        abs_pkg_path[sizeof(abs_pkg_path) - 1] = '\0';
    }
    if (_fullpath(abs_script_path, script_path, sizeof(abs_script_path)) == NULL) {
        strncpy(abs_script_path, script_path, sizeof(abs_script_path) - 1);
        abs_script_path[sizeof(abs_script_path) - 1] = '\0';
    }
#else
    if (realpath(pkg_path, abs_pkg_path) == NULL) {
        strncpy(abs_pkg_path, pkg_path, sizeof(abs_pkg_path) - 1);
        abs_pkg_path[sizeof(abs_pkg_path) - 1] = '\0';
    }
    if (realpath(script_path, abs_script_path) == NULL) {
        strncpy(abs_script_path, script_path, sizeof(abs_script_path) - 1);
        abs_script_path[sizeof(abs_script_path) - 1] = '\0';
    }
#endif

    printf("    running install script...\n");
    fflush(stdout);

    /* Build and execute the command */
    char cmd[PKG_MAX_PATH_LEN * 3];

#ifdef _WIN32
    /* On Windows, use PowerShell to run the script */
    snprintf(cmd, sizeof(cmd),
             "powershell -NoProfile -ExecutionPolicy Bypass -Command \"Set-Location '%s'; & '%s'\"",
             abs_pkg_path, abs_script_path);
#else
    /* On Unix, cd to directory and run bash script */
    snprintf(cmd, sizeof(cmd), "cd '%s' && bash '%s'", abs_pkg_path, abs_script_path);
#endif

    int result = system(cmd);
    if (result != 0) {
        pkg_warning("install script failed with exit code %d", result);
        return false;
    }

    return true;
}
