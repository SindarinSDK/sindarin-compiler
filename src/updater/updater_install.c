/*
 * Sindarin Compiler Auto-Update Module - Installation Implementation
 * Handles platform-specific binary self-replacement.
 */

#include "../updater.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <direct.h>
    #include <io.h>
    #ifndef PATH_MAX
        #define PATH_MAX MAX_PATH
    #endif
    #define access _access
    #define W_OK 2
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <limits.h>
    #include <errno.h>
#endif

/* Forward declarations */
bool updater_get_temp_path(char *buf, size_t bufsize, const char *suffix);

/* Helper to run system() and intentionally ignore the result.
 * Used for best-effort operations where failure is acceptable.
 */
static inline void system_ignore_result(const char *cmd) {
    int result __attribute__((unused)) = system(cmd);
}

/* Components to update (relative to SDK root)
 * Used by Unix implementation; Windows batch script has these hardcoded.
 */
#ifndef _WIN32
static const char *SDK_COMPONENTS[] = {
    "include",
    "lib",
    "sdk",
    "deps",
    NULL
};
#endif

/*
 * Get the SDK root directory from the current executable path.
 * The SDK root is the directory containing the sn binary.
 */
static bool get_sdk_root(char *buf, size_t bufsize)
{
    char exe_path[PATH_MAX];
    if (!updater_get_exe_path(exe_path, sizeof(exe_path))) {
        return false;
    }

    /* Find the last path separator and truncate to get directory */
#ifdef _WIN32
    char *last_sep = strrchr(exe_path, '\\');
    if (!last_sep) {
        last_sep = strrchr(exe_path, '/');
    }
#else
    char *last_sep = strrchr(exe_path, '/');
#endif

    if (!last_sep) {
        return false;
    }

    size_t dir_len = (size_t)(last_sep - exe_path);
    if (dir_len >= bufsize) {
        return false;
    }

    memcpy(buf, exe_path, dir_len);
    buf[dir_len] = '\0';
    return true;
}

/*
 * Check if a path exists and is a directory
 */
static bool dir_exists(const char *path)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

/*
 * Check if a file exists
 */
static bool file_exists(const char *path)
{
    return access(path, 0) == 0;
}

/*
 * Find the package root in the extracted directory.
 * The package may be extracted as:
 *   extract_dir/sindarin-VERSION-PLATFORM/lib/sindarin/
 *   extract_dir/lib/sindarin/
 *   extract_dir/  (if files are at root)
 */
static bool find_package_root(const char *extract_dir, const char *version,
                               char *package_root, size_t bufsize)
{
#ifdef _WIN32
    const char *sep = "\\";
#else
    const char *sep = "/";
#endif

    /* Try versioned directory first: extract_dir/sindarin-VERSION-PLATFORM/lib/sindarin */
#ifdef _WIN32
    snprintf(package_root, bufsize, "%s%ssindarin-%s-windows-x64%slib%ssindarin",
             extract_dir, sep, version, sep, sep);
#elif defined(__APPLE__)
    snprintf(package_root, bufsize, "%s%ssindarin-%s-macos-x64%slib%ssindarin",
             extract_dir, sep, version, sep, sep);
#else
    snprintf(package_root, bufsize, "%s%ssindarin-%s-linux-x64%slib%ssindarin",
             extract_dir, sep, version, sep, sep);
#endif

    if (dir_exists(package_root)) {
        return true;
    }

    /* Try lib/sindarin structure */
    snprintf(package_root, bufsize, "%s%slib%ssindarin", extract_dir, sep, sep);
    if (dir_exists(package_root)) {
        return true;
    }

    /* Try extract_dir directly (flat structure) */
    snprintf(package_root, bufsize, "%s", extract_dir);
    if (dir_exists(package_root)) {
        /* Verify it has expected content (at least sn binary or sdk directory) */
        char test_path[PATH_MAX];
#ifdef _WIN32
        snprintf(test_path, sizeof(test_path), "%s%ssn.exe", package_root, sep);
#else
        snprintf(test_path, sizeof(test_path), "%s%ssn", package_root, sep);
#endif
        if (file_exists(test_path)) {
            return true;
        }
        snprintf(test_path, sizeof(test_path), "%s%ssdk", package_root, sep);
        if (dir_exists(test_path)) {
            return true;
        }
    }

    return false;
}

#ifdef _WIN32

/*
 * Windows: Cannot overwrite running executable
 * Strategy:
 * 1. Download new version to temp directory
 * 2. Create a batch script that:
 *    a. Waits for current process to exit
 *    b. Copies new binary over old
 *    c. Deletes temp files
 * 3. Execute batch script and exit
 */
static bool install_windows(const char *new_exe_path, bool verbose)
{
    char current_exe[PATH_MAX];
    char script_path[PATH_MAX];

    /* Get current executable path */
    if (!updater_get_exe_path(current_exe, sizeof(current_exe))) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to get current executable path\n");
        }
        return false;
    }

    /* Create update script in temp directory */
    updater_get_temp_path(script_path, sizeof(script_path), ".bat");

    FILE *script = fopen(script_path, "w");
    if (!script) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to create update script: %s\n", script_path);
        }
        return false;
    }

    /* Write batch script */
    fprintf(script,
        "@echo off\n"
        "echo Updating Sindarin compiler...\n"
        "timeout /t 2 /nobreak > nul\n"
        "del /f \"%s.old\" 2>nul\n"
        "move /y \"%s\" \"%s.old\" >nul 2>&1\n"
        "copy /y \"%s\" \"%s\" >nul\n"
        "if errorlevel 1 (\n"
        "    echo Error: Failed to copy new binary\n"
        "    move /y \"%s.old\" \"%s\" >nul 2>&1\n"
        "    pause\n"
        "    exit /b 1\n"
        ")\n"
        "del /f \"%s\" 2>nul\n"
        "del /f \"%s.old\" 2>nul\n"
        "echo.\n"
        "echo Update complete! Run 'sn --version' to verify.\n"
        "echo.\n"
        "del /f \"%%~f0\"\n",
        current_exe,                    /* Delete old backup */
        current_exe, current_exe,       /* Backup current */
        new_exe_path, current_exe,      /* Copy new over */
        current_exe, current_exe,       /* Restore on error */
        new_exe_path,                   /* Delete temp new */
        current_exe                     /* Delete backup */
    );
    fclose(script);

    if (verbose) {
        fprintf(stderr, "Running update script...\n");
    }

    /* Execute script and exit immediately */
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "cmd.exe /c \"%s\"", script_path);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to start update script (error %lu)\n",
                    GetLastError());
        }
        remove(script_path);
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    printf("Update initiated. Please wait for the update window to complete.\n");
    exit(0);  /* Exit to allow script to run */
}

/*
 * Windows: Install full package via batch script
 * Similar to binary install but copies all SDK components
 */
static bool install_full_package_windows(const char *package_root, bool verbose)
{
    char sdk_root[PATH_MAX];
    char script_path[PATH_MAX];
    char cfg_backup[PATH_MAX];

    /* Get SDK root (where current sn.exe lives) */
    if (!get_sdk_root(sdk_root, sizeof(sdk_root))) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to get SDK root directory\n");
        }
        return false;
    }

    /* Create update script in temp directory */
    updater_get_temp_path(script_path, sizeof(script_path), ".bat");

    FILE *script = fopen(script_path, "w");
    if (!script) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to create update script: %s\n", script_path);
        }
        return false;
    }

    /* Generate unique backup directory name */
    char backup_dir[PATH_MAX];
    snprintf(backup_dir, sizeof(backup_dir), "%s.update_backup", sdk_root);
    snprintf(cfg_backup, sizeof(cfg_backup), "%s\\sn.cfg.user_backup", sdk_root);

    /* Write batch script for full package update */
    fprintf(script,
        "@echo off\n"
        "setlocal enabledelayedexpansion\n"
        "echo Updating Sindarin SDK...\n"
        "timeout /t 2 /nobreak > nul\n"
        "\n"
        "set \"SDK_ROOT=%s\"\n"
        "set \"PACKAGE_ROOT=%s\"\n"
        "set \"BACKUP_DIR=%s\"\n"
        "set \"CFG_BACKUP=%s\"\n"
        "\n"
        "REM Backup user's sn.cfg if it exists\n"
        "if exist \"%%SDK_ROOT%%\\sn.cfg\" (\n"
        "    copy /y \"%%SDK_ROOT%%\\sn.cfg\" \"%%CFG_BACKUP%%\" >nul 2>&1\n"
        "    echo Backed up user configuration.\n"
        ")\n"
        "\n"
        "REM Remove old backup if exists\n"
        "if exist \"%%BACKUP_DIR%%\" rmdir /s /q \"%%BACKUP_DIR%%\" 2>nul\n"
        "\n"
        "REM Create backup of current installation\n"
        "mkdir \"%%BACKUP_DIR%%\" 2>nul\n"
        "echo Creating backup of current installation...\n"
        "\n"
        "REM Backup sn.exe\n"
        "if exist \"%%SDK_ROOT%%\\sn.exe\" (\n"
        "    move /y \"%%SDK_ROOT%%\\sn.exe\" \"%%BACKUP_DIR%%\\sn.exe\" >nul 2>&1\n"
        ")\n"
        "\n"
        "REM Backup directories\n"
        "for %%%%d in (include lib sdk deps) do (\n"
        "    if exist \"%%SDK_ROOT%%\\%%%%d\" (\n"
        "        move /y \"%%SDK_ROOT%%\\%%%%d\" \"%%BACKUP_DIR%%\\%%%%d\" >nul 2>&1\n"
        "    )\n"
        ")\n"
        "\n"
        "echo Installing new version...\n"
        "\n"
        "REM Copy new sn.exe\n"
        "copy /y \"%%PACKAGE_ROOT%%\\sn.exe\" \"%%SDK_ROOT%%\\sn.exe\" >nul\n"
        "if errorlevel 1 goto :restore\n"
        "\n"
        "REM Copy new sn.cfg (will be overwritten by user backup later)\n"
        "if exist \"%%PACKAGE_ROOT%%\\sn.cfg\" (\n"
        "    copy /y \"%%PACKAGE_ROOT%%\\sn.cfg\" \"%%SDK_ROOT%%\\sn.cfg\" >nul 2>&1\n"
        ")\n"
        "\n"
        "REM Copy directories\n"
        "for %%%%d in (include lib sdk deps) do (\n"
        "    if exist \"%%PACKAGE_ROOT%%\\%%%%d\" (\n"
        "        xcopy /e /i /q /y \"%%PACKAGE_ROOT%%\\%%%%d\" \"%%SDK_ROOT%%\\%%%%d\" >nul 2>&1\n"
        "        if errorlevel 1 (\n"
        "            echo Warning: Failed to copy %%%%d directory\n"
        "        )\n"
        "    )\n"
        ")\n"
        "\n"
        "REM Restore user's sn.cfg\n"
        "if exist \"%%CFG_BACKUP%%\" (\n"
        "    copy /y \"%%CFG_BACKUP%%\" \"%%SDK_ROOT%%\\sn.cfg\" >nul 2>&1\n"
        "    del /f \"%%CFG_BACKUP%%\" 2>nul\n"
        "    echo Restored user configuration.\n"
        ")\n"
        "\n"
        "REM Cleanup backup directory\n"
        "rmdir /s /q \"%%BACKUP_DIR%%\" 2>nul\n"
        "\n"
        "echo.\n"
        "echo Update complete! Run 'sn --version' to verify.\n"
        "echo.\n"
        "goto :cleanup\n"
        "\n"
        ":restore\n"
        "echo Error occurred. Restoring previous installation...\n"
        "REM Restore sn.exe\n"
        "if exist \"%%BACKUP_DIR%%\\sn.exe\" (\n"
        "    move /y \"%%BACKUP_DIR%%\\sn.exe\" \"%%SDK_ROOT%%\\sn.exe\" >nul 2>&1\n"
        ")\n"
        "REM Restore directories\n"
        "for %%%%d in (include lib sdk deps) do (\n"
        "    if exist \"%%BACKUP_DIR%%\\%%%%d\" (\n"
        "        move /y \"%%BACKUP_DIR%%\\%%%%d\" \"%%SDK_ROOT%%\\%%%%d\" >nul 2>&1\n"
        "    )\n"
        ")\n"
        "rmdir /s /q \"%%BACKUP_DIR%%\" 2>nul\n"
        "echo Restoration complete.\n"
        "pause\n"
        "exit /b 1\n"
        "\n"
        ":cleanup\n"
        "REM Delete the script itself\n"
        "del /f \"%%~f0\"\n",
        sdk_root,
        package_root,
        backup_dir,
        cfg_backup
    );
    fclose(script);

    if (verbose) {
        fprintf(stderr, "Running update script...\n");
    }

    /* Execute script and exit immediately */
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "cmd.exe /c \"%s\"", script_path);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to start update script (error %lu)\n",
                    GetLastError());
        }
        remove(script_path);
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    printf("Update initiated. Please wait for the update window to complete.\n");
    exit(0);  /* Exit to allow script to run */
}

#else /* Unix */

/*
 * Unix: Can use atomic rename for in-place update
 * Strategy:
 * 1. Download new version to temp file
 * 2. Copy permissions from current binary
 * 3. Rename temp file over current binary (atomic on same filesystem)
 * 4. If different filesystem, copy then delete
 */
static bool install_unix(const char *new_exe_path, bool verbose)
{
    char current_exe[PATH_MAX];

    /* Get current executable path */
    if (!updater_get_exe_path(current_exe, sizeof(current_exe))) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to get current executable path\n");
        }
        return false;
    }

    /* Check for Homebrew installation on macOS */
#ifdef __APPLE__
    if (strstr(current_exe, "/Cellar/") != NULL ||
        strstr(current_exe, "/homebrew/") != NULL) {
        fprintf(stderr, "Sindarin was installed via Homebrew.\n");
        fprintf(stderr, "To update, run: brew upgrade sindarin\n");
        return false;
    }
#endif

    /* Check write permission */
    if (access(current_exe, W_OK) != 0) {
        fprintf(stderr, "Permission denied. Try running with sudo:\n");
        fprintf(stderr, "  sudo sn --update\n");
        return false;
    }

    /* Copy permissions from current binary */
    struct stat st;
    if (stat(current_exe, &st) != 0) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to get current binary permissions\n");
        }
        return false;
    }

    /* Set permissions on new binary */
    if (chmod(new_exe_path, st.st_mode) != 0) {
        if (verbose) {
            fprintf(stderr, "Warning: Failed to copy permissions to new binary\n");
        }
    }

    /* Attempt atomic rename */
    if (rename(new_exe_path, current_exe) == 0) {
        if (verbose) {
            fprintf(stderr, "Updated %s successfully\n", current_exe);
        }
        return true;
    }

    /* If rename fails due to cross-device link, fall back to copy */
    if (errno == EXDEV) {
        char backup_path[PATH_MAX];
        snprintf(backup_path, sizeof(backup_path), "%s.old", current_exe);

        /* Backup current */
        if (rename(current_exe, backup_path) != 0) {
            if (verbose) {
                fprintf(stderr, "Error: Failed to backup current binary\n");
            }
            return false;
        }

        /* Copy new binary using shell cp for simplicity */
        char cmd[PATH_MAX * 3];
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null", new_exe_path, current_exe);
        if (system(cmd) != 0) {
            /* Restore backup on failure */
            rename(backup_path, current_exe);
            if (verbose) {
                fprintf(stderr, "Error: Failed to copy new binary\n");
            }
            return false;
        }

        /* Set permissions and cleanup */
        chmod(current_exe, st.st_mode);
        unlink(backup_path);
        unlink(new_exe_path);

        if (verbose) {
            fprintf(stderr, "Updated %s successfully\n", current_exe);
        }
        return true;
    }

    /* Other error */
    if (verbose) {
        fprintf(stderr, "Error: Failed to install new binary: %s\n", strerror(errno));
    }
    return false;
}

/*
 * Unix: Install full package
 * Copies all SDK components with backup/restore on failure
 */
static bool install_full_package_unix(const char *package_root, bool verbose)
{
    char sdk_root[PATH_MAX];
    char backup_dir[PATH_MAX];
    char cfg_backup[PATH_MAX];
    char cmd[PATH_MAX * 4];

    /* Get SDK root (where current sn binary lives) */
    if (!get_sdk_root(sdk_root, sizeof(sdk_root))) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to get SDK root directory\n");
        }
        return false;
    }

    /* Check for Homebrew installation on macOS */
#ifdef __APPLE__
    if (strstr(sdk_root, "/Cellar/") != NULL ||
        strstr(sdk_root, "/homebrew/") != NULL) {
        fprintf(stderr, "Sindarin was installed via Homebrew.\n");
        fprintf(stderr, "To update, run: brew upgrade sindarin\n");
        return false;
    }
#endif

    /* Check write permission on SDK root */
    if (access(sdk_root, W_OK) != 0) {
        fprintf(stderr, "Permission denied. Try running with sudo:\n");
        fprintf(stderr, "  sudo sn --update\n");
        return false;
    }

    snprintf(backup_dir, sizeof(backup_dir), "%s.update_backup", sdk_root);
    snprintf(cfg_backup, sizeof(cfg_backup), "%s/sn.cfg.user_backup", sdk_root);

    if (verbose) {
        printf("SDK root: %s\n", sdk_root);
        printf("Package root: %s\n", package_root);
    }

    /* Backup user's sn.cfg if it exists */
    char cfg_path[PATH_MAX];
    snprintf(cfg_path, sizeof(cfg_path), "%s/sn.cfg", sdk_root);
    if (file_exists(cfg_path)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null", cfg_path, cfg_backup);
        if (system(cmd) == 0 && verbose) {
            printf("Backed up user configuration.\n");
        }
    }

    /* Remove old backup if exists */
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", backup_dir);
    system_ignore_result(cmd);  /* Ignore failure */

    /* Create backup directory */
    if (mkdir(backup_dir, 0755) != 0 && errno != EEXIST) {
        if (verbose) {
            fprintf(stderr, "Warning: Could not create backup directory\n");
        }
    }

    if (verbose) {
        printf("Creating backup of current installation...\n");
    }

    /* Backup current sn binary */
    char current_exe[PATH_MAX];
    snprintf(current_exe, sizeof(current_exe), "%s/sn", sdk_root);
    if (file_exists(current_exe)) {
        snprintf(cmd, sizeof(cmd), "mv '%s' '%s/sn' 2>/dev/null", current_exe, backup_dir);
        system_ignore_result(cmd);  /* Ignore failure - might not exist */
    }

    /* Backup directories */
    for (int i = 0; SDK_COMPONENTS[i] != NULL; i++) {
        char comp_path[PATH_MAX];
        snprintf(comp_path, sizeof(comp_path), "%s/%s", sdk_root, SDK_COMPONENTS[i]);
        if (dir_exists(comp_path)) {
            snprintf(cmd, sizeof(cmd), "mv '%s' '%s/%s' 2>/dev/null",
                     comp_path, backup_dir, SDK_COMPONENTS[i]);
            system_ignore_result(cmd);  /* Ignore failure */
        }
    }

    if (verbose) {
        printf("Installing new version...\n");
    }

    /* Install new sn binary */
    char new_exe[PATH_MAX];
    snprintf(new_exe, sizeof(new_exe), "%s/sn", package_root);
    if (file_exists(new_exe)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null", new_exe, current_exe);
        if (system(cmd) != 0) {
            fprintf(stderr, "Error: Failed to install new binary\n");
            goto restore;
        }
        /* Make executable */
        chmod(current_exe, 0755);
    } else {
        fprintf(stderr, "Error: New binary not found at %s\n", new_exe);
        goto restore;
    }

    /* Copy new sn.cfg (will be overwritten by user backup later) */
    char new_cfg[PATH_MAX];
    snprintf(new_cfg, sizeof(new_cfg), "%s/sn.cfg", package_root);
    if (file_exists(new_cfg)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null", new_cfg, cfg_path);
        system_ignore_result(cmd);  /* Ignore failure - optional */
    }

    /* Copy directories */
    for (int i = 0; SDK_COMPONENTS[i] != NULL; i++) {
        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", package_root, SDK_COMPONENTS[i]);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", sdk_root, SDK_COMPONENTS[i]);

        if (dir_exists(src_path)) {
            snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s' 2>/dev/null", src_path, dst_path);
            if (system(cmd) != 0 && verbose) {
                fprintf(stderr, "Warning: Failed to copy %s directory\n", SDK_COMPONENTS[i]);
            }
        }
    }

    /* Restore user's sn.cfg */
    if (file_exists(cfg_backup)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s' 2>/dev/null", cfg_backup, cfg_path);
        if (system(cmd) == 0) {
            if (verbose) {
                printf("Restored user configuration.\n");
            }
        }
        unlink(cfg_backup);
    }

    /* Cleanup backup directory */
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", backup_dir);
    system_ignore_result(cmd);  /* Ignore failure */

    if (verbose) {
        printf("Updated %s successfully\n", sdk_root);
    }
    return true;

restore:
    fprintf(stderr, "Error occurred. Restoring previous installation...\n");

    /* Restore sn binary */
    char backup_exe[PATH_MAX];
    snprintf(backup_exe, sizeof(backup_exe), "%s/sn", backup_dir);
    if (file_exists(backup_exe)) {
        snprintf(cmd, sizeof(cmd), "mv '%s' '%s' 2>/dev/null", backup_exe, current_exe);
        system_ignore_result(cmd);  /* Best-effort restore */
    }

    /* Restore directories */
    for (int i = 0; SDK_COMPONENTS[i] != NULL; i++) {
        char backup_comp[PATH_MAX];
        char dst_comp[PATH_MAX];
        snprintf(backup_comp, sizeof(backup_comp), "%s/%s", backup_dir, SDK_COMPONENTS[i]);
        snprintf(dst_comp, sizeof(dst_comp), "%s/%s", sdk_root, SDK_COMPONENTS[i]);

        if (dir_exists(backup_comp)) {
            /* Remove partially copied directory first */
            snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", dst_comp);
            system_ignore_result(cmd);  /* Best-effort cleanup */
            snprintf(cmd, sizeof(cmd), "mv '%s' '%s' 2>/dev/null", backup_comp, dst_comp);
            system_ignore_result(cmd);  /* Best-effort restore */
        }
    }

    /* Cleanup backup directory */
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", backup_dir);
    system_ignore_result(cmd);  /* Best-effort cleanup */

    fprintf(stderr, "Restoration complete.\n");
    return false;
}

#endif /* _WIN32 */

/*
 * Install full SDK package (public function)
 */
bool updater_install_full_package(const char *package_root, bool verbose)
{
#ifdef _WIN32
    return install_full_package_windows(package_root, verbose);
#else
    return install_full_package_unix(package_root, verbose);
#endif
}

bool updater_install_binary(const char *new_exe_path, bool verbose)
{
#ifdef _WIN32
    return install_windows(new_exe_path, verbose);
#else
    return install_unix(new_exe_path, verbose);
#endif
}

/*
 * Main update function - orchestrates the full update process
 */
bool updater_perform_update(bool verbose)
{
#if !SN_HAS_CURL
    (void)verbose;  /* Unused when curl not available */
    fprintf(stderr, "Error: Auto-update not available (compiled without libcurl)\n");
    return false;
#else
    if (verbose) {
        printf("Checking for updates...\n");
    }

    /* Start synchronous update check */
    updater_init();
    updater_check_start();

    /* Wait for check to complete */
    while (!updater_check_done()) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif
    }

    const UpdateInfo *info = updater_get_result();
    if (!info || !info->update_available) {
        printf("Already running the latest version (%s)\n", SN_VERSION_STRING);
        return true;  /* Not an error - just nothing to do */
    }

    if (verbose) {
        printf("Found update: %s -> %s\n", SN_VERSION_STRING, info->version);
    }

    /* Check for valid download URL */
    if (info->download_url[0] == '\0') {
        fprintf(stderr, "Error: No download URL found for this platform\n");
        fprintf(stderr, "  Expected asset suffix: %s\n", updater_get_platform_suffix());
        fprintf(stderr, "  Release version: %s (tag: %s)\n", info->version, info->tag_name);
        fprintf(stderr, "  The release may not have platform-specific assets uploaded yet.\n");
        return false;
    }

    /* Generate temp paths */
    char archive_path[PATH_MAX];
    char extract_dir[PATH_MAX];

#ifdef _WIN32
    updater_get_temp_path(archive_path, sizeof(archive_path), ".zip");
    updater_get_temp_path(extract_dir, sizeof(extract_dir), "_extract");
#else
    updater_get_temp_path(archive_path, sizeof(archive_path), ".tar.gz");
    updater_get_temp_path(extract_dir, sizeof(extract_dir), "_extract");
#endif

    /* Download the archive */
    if (verbose) {
        printf("Downloading from: %s\n", info->download_url);
    }

    if (!updater_download_file(info->download_url, archive_path, verbose)) {
        fprintf(stderr, "Error: Failed to download update\n");
        return false;
    }

    if (verbose) {
        printf("Download complete. Extracting...\n");
    }

    /* Extract the archive */
    if (!updater_extract_archive(archive_path, extract_dir, verbose)) {
        fprintf(stderr, "Error: Failed to extract update\n");
        remove(archive_path);
        return false;
    }

    /* Find the package root in the extracted directory */
    char package_root[PATH_MAX];
    if (!find_package_root(extract_dir, info->version, package_root, sizeof(package_root))) {
        fprintf(stderr, "Error: Could not find package root in extracted archive\n");
        fprintf(stderr, "  Extract directory: %s\n", extract_dir);
#ifndef _WIN32
        char cmd[PATH_MAX * 2];
        snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", extract_dir);
        system_ignore_result(cmd);  /* Best-effort cleanup */
#endif
        remove(archive_path);
        return false;
    }

    if (verbose) {
        printf("Installing from: %s\n", package_root);
    }

    /* Install the full package (binary + SDK + deps + include + lib) */
    bool success = updater_install_full_package(package_root, verbose);

    /* Cleanup (on Unix - Windows script handles cleanup) */
#ifndef _WIN32
    /* Remove extracted files - ignore failure, cleanup is best-effort */
    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' 2>/dev/null", extract_dir);
    if (system(cmd) != 0) { /* Intentionally ignored */ }

    /* Remove archive */
    remove(archive_path);
#endif

    if (success) {
        printf("Update successful! Run 'sn --version' to verify.\n");
    }

    return success;
#endif /* SN_HAS_CURL */
}
