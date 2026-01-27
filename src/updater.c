/*
 * Sindarin Compiler Auto-Update Module - Main Implementation
 * Handles initialization, cleanup, and update notification display.
 */

#include "updater.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <limits.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

/* Global state */
static bool g_updater_initialized = false;
static bool g_updater_disabled = false;

void updater_init(void)
{
    if (g_updater_initialized) {
        return;
    }

    /* Check environment variables for disable flags */
    const char *disable_env = getenv("SN_DISABLE_UPDATE_CHECK");
    if (disable_env && (disable_env[0] == '1' || disable_env[0] == 'y' || disable_env[0] == 'Y')) {
        g_updater_disabled = true;
    }

    /* Also check CI environment variable */
    const char *ci_env = getenv("CI");
    if (ci_env && (ci_env[0] == '1' || strcmp(ci_env, "true") == 0)) {
        g_updater_disabled = true;
    }

    g_updater_initialized = true;
}

void updater_disable(void)
{
    g_updater_disabled = true;
}

bool updater_is_disabled(void)
{
    return g_updater_disabled;
}

void updater_cleanup(void)
{
    /* Cleanup is handled by updater_check.c */
    g_updater_initialized = false;
}

void updater_notify_if_available(void)
{
    if (!g_updater_initialized || g_updater_disabled) {
        return;
    }

    if (!updater_check_done()) {
        return;  /* Check still running, don't block */
    }

    const UpdateInfo *info = updater_get_result();
    if (!info || !info->update_available) {
        return;
    }

    /* Use ANSI colors for notification (consistent with diagnostic.c) */
    fprintf(stderr, "\n");
#ifdef _WIN32
    /* Windows might not support ANSI codes in all terminals */
    fprintf(stderr, "[Update Available] Sindarin %s is available (current: %s)\n",
            info->version, SN_VERSION_STRING);
    fprintf(stderr, "  Run 'sn --update' to update automatically.\n");
#else
    fprintf(stderr, "\033[1;33m[Update Available]\033[0m Sindarin %s is available (current: %s)\n",
            info->version, SN_VERSION_STRING);
    fprintf(stderr, "  Run '\033[1msn --update\033[0m' to update automatically.\n");
#endif
    fprintf(stderr, "\n");
}

int updater_version_compare(const char *v1, const char *v2)
{
    int major1 = 0, minor1 = 0, patch1 = 0;
    int major2 = 0, minor2 = 0, patch2 = 0;

    if (sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1) < 1) {
        return 0;  /* Invalid v1 format */
    }
    if (sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2) < 1) {
        return 0;  /* Invalid v2 format */
    }

    if (major1 != major2) return major1 - major2;
    if (minor1 != minor2) return minor1 - minor2;
    return patch1 - patch2;
}

const char *updater_get_platform_suffix(void)
{
#if defined(_WIN32)
    return "-windows-x64.zip";
#elif defined(__APPLE__)
    return "-macos-x64.tar.gz";
#else
    return "-linux-x64.tar.gz";
#endif
}

bool updater_get_exe_path(char *buf, size_t bufsize)
{
#if defined(_WIN32)
    DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)bufsize);
    return len > 0 && len < bufsize;
#elif defined(__APPLE__)
    uint32_t size = (uint32_t)bufsize;
    if (_NSGetExecutablePath(buf, &size) == 0) {
        /* Resolve symlinks */
        char resolved[PATH_MAX];
        if (realpath(buf, resolved)) {
            strncpy(buf, resolved, bufsize - 1);
            buf[bufsize - 1] = '\0';
        }
        return true;
    }
    return false;
#else
    /* Linux */
    ssize_t len = readlink("/proc/self/exe", buf, bufsize - 1);
    if (len == -1) {
        return false;
    }
    buf[len] = '\0';
    return true;
#endif
}
