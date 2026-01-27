/*
 * Sindarin Compiler Auto-Update Module
 * Provides background update checking and self-update capability.
 */

#ifndef SN_UPDATER_H
#define SN_UPDATER_H

#include <stdbool.h>
#include <stddef.h>

/* GitHub API endpoint for releases */
#define SN_GITHUB_API_URL "https://api.github.com/repos/SindarinSDK/sindarin-compiler/releases/latest"

/* Maximum buffer sizes */
#define SN_UPDATE_VERSION_MAX 32
#define SN_UPDATE_TAG_MAX 64
#define SN_UPDATE_URL_MAX 512
#define SN_UPDATE_NOTES_MAX 2048

/* Update check result */
typedef struct {
    bool update_available;
    char version[SN_UPDATE_VERSION_MAX];      /* e.g., "1.2.3" */
    char tag_name[SN_UPDATE_TAG_MAX];         /* e.g., "v1.2.3-alpha" */
    char download_url[SN_UPDATE_URL_MAX];     /* Platform-specific asset URL */
    char release_notes[SN_UPDATE_NOTES_MAX];  /* Brief release description */
} UpdateInfo;

/* Initialize the updater subsystem (must be called once at startup) */
void updater_init(void);

/* Start background update check (non-blocking)
 * Call this at compiler startup, before compilation begins.
 * Silently does nothing if curl is not available or disabled.
 */
void updater_check_start(void);

/* Check if background update check completed
 * Returns true if a result is available, false if still checking.
 */
bool updater_check_done(void);

/* Get update result (call only after updater_check_done returns true)
 * Returns NULL if check failed or no update available.
 * Returned pointer is valid until next updater_check_start() call.
 */
const UpdateInfo *updater_get_result(void);

/* Display update notification to user (call at end of compilation)
 * Only displays if an update is available and check completed.
 */
void updater_notify_if_available(void);

/* Perform self-update (blocking)
 * Downloads new version and replaces current binary.
 * Returns true on success, false on failure.
 */
bool updater_perform_update(bool verbose);

/* Cleanup updater resources */
void updater_cleanup(void);

/* Disable update checking (for testing or CI environments)
 * Also checks SN_DISABLE_UPDATE_CHECK and CI environment variables.
 */
void updater_disable(void);

/* Check if updates are disabled */
bool updater_is_disabled(void);

/* Internal functions (used by updater/ *.c files) */

/* Compare two version strings (e.g., "1.2.3" vs "1.2.4")
 * Returns: >0 if v1 > v2, <0 if v1 < v2, 0 if equal
 */
int updater_version_compare(const char *v1, const char *v2);

/* Get platform-specific archive suffix */
const char *updater_get_platform_suffix(void);

/* Get the path to the current executable */
bool updater_get_exe_path(char *buf, size_t bufsize);

/* Download a file to a destination path */
bool updater_download_file(const char *url, const char *dest_path, bool verbose);

/* Extract an archive to a destination directory */
bool updater_extract_archive(const char *archive_path, const char *dest_dir, bool verbose);

/* Install the new binary (platform-specific) */
bool updater_install_binary(const char *new_exe_path, bool verbose);

#endif /* SN_UPDATER_H */
