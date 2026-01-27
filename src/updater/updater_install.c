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

#endif /* _WIN32 */

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

    /* Find the new binary in the extracted directory */
    char new_exe_path[PATH_MAX];

#ifdef _WIN32
    /* On Windows, the archive extracts to a sindarin-VERSION directory */
    snprintf(new_exe_path, sizeof(new_exe_path),
             "%s\\sindarin-%s-windows-x64\\lib\\sindarin\\sn.exe",
             extract_dir, info->version);

    /* Try alternate path structure */
    if (access(new_exe_path, 0) != 0) {
        snprintf(new_exe_path, sizeof(new_exe_path),
                 "%s\\lib\\sindarin\\sn.exe", extract_dir);
    }
    if (access(new_exe_path, 0) != 0) {
        snprintf(new_exe_path, sizeof(new_exe_path),
                 "%s\\sn.exe", extract_dir);
    }
#else
    /* On Unix, similar structure */
    snprintf(new_exe_path, sizeof(new_exe_path),
             "%s/sindarin-%s-%s/lib/sindarin/sn",
             extract_dir, info->version,
#ifdef __APPLE__
             "macos-x64"
#else
             "linux-x64"
#endif
             );

    /* Try alternate path structures */
    if (access(new_exe_path, X_OK) != 0) {
        snprintf(new_exe_path, sizeof(new_exe_path),
                 "%s/lib/sindarin/sn", extract_dir);
    }
    if (access(new_exe_path, X_OK) != 0) {
        snprintf(new_exe_path, sizeof(new_exe_path),
                 "%s/sn", extract_dir);
    }
#endif

    if (verbose) {
        printf("Installing from: %s\n", new_exe_path);
    }

    /* Install the new binary */
    bool success = updater_install_binary(new_exe_path, verbose);

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
