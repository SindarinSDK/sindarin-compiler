/*
 * Sindarin Compiler Auto-Update Module - Download Implementation
 * Handles downloading update archives and extracting them.
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
    #ifndef PATH_MAX
        #define PATH_MAX MAX_PATH
    #endif
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <limits.h>
#endif

/* Conditional curl support */
#if SN_HAS_CURL
    #include <curl/curl.h>
#endif

#if SN_HAS_CURL

/* Progress callback for verbose mode */
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                              curl_off_t ultotal, curl_off_t ulnow)
{
    (void)ultotal;
    (void)ulnow;
    int *verbose = (int *)clientp;

    if (*verbose && dltotal > 0) {
        int percent = (int)((dlnow * 100) / dltotal);
        fprintf(stderr, "\rDownloading: %d%%", percent);
        fflush(stderr);
    }

    return 0;  /* Continue download */
}

/* File write callback */
static size_t file_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

#endif /* SN_HAS_CURL */

bool updater_download_file(const char *url, const char *dest_path, bool verbose)
{
#if SN_HAS_CURL
    CURL *curl = curl_easy_init();
    if (!curl) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to initialize curl\n");
        }
        return false;
    }

    FILE *fp = fopen(dest_path, "wb");
    if (!fp) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to open file for writing: %s\n", dest_path);
        }
        curl_easy_cleanup(curl);
        return false;
    }

    int verbose_flag = verbose ? 1 : 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sn-compiler/" SN_VERSION_STRING);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  /* 5 minute timeout for download */

    if (verbose) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &verbose_flag);
    }

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);

    if (verbose) {
        fprintf(stderr, "\n");  /* New line after progress */
    }

    if (res != CURLE_OK) {
        if (verbose) {
            fprintf(stderr, "Error: Download failed: %s\n", curl_easy_strerror(res));
        }
        /* Remove partial file */
        remove(dest_path);
        curl_easy_cleanup(curl);
        return false;
    }

    /* Check HTTP status code */
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        if (verbose) {
            fprintf(stderr, "Error: Download failed with HTTP code %ld\n", http_code);
        }
        remove(dest_path);
        return false;
    }

    return true;
#else
    (void)url;
    (void)dest_path;
    if (verbose) {
        fprintf(stderr, "Error: Auto-update not available (compiled without libcurl)\n");
    }
    return false;
#endif
}

bool updater_extract_archive(const char *archive_path, const char *dest_dir, bool verbose)
{
#ifdef _WIN32
    /* Use PowerShell's Expand-Archive on Windows */
    char cmd[PATH_MAX * 3];

    /* Ensure destination directory exists */
    _mkdir(dest_dir);

    /* Build PowerShell command */
    snprintf(cmd, sizeof(cmd),
        "powershell -NoProfile -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\" 2>nul",
        archive_path, dest_dir);

    if (verbose) {
        fprintf(stderr, "Extracting archive...\n");
    }

    int result = system(cmd);
    if (result != 0) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to extract archive (exit code %d)\n", result);
        }
        return false;
    }

    return true;
#else
    /* Use tar on Unix */
    char cmd[PATH_MAX * 3];

    /* Ensure destination directory exists */
    mkdir(dest_dir, 0755);

    /* Build tar command */
    snprintf(cmd, sizeof(cmd), "tar -xzf '%s' -C '%s' 2>/dev/null", archive_path, dest_dir);

    if (verbose) {
        fprintf(stderr, "Extracting archive...\n");
    }

    int result = system(cmd);
    if (result != 0) {
        if (verbose) {
            fprintf(stderr, "Error: Failed to extract archive (exit code %d)\n", result);
        }
        return false;
    }

    return true;
#endif
}

/* Get the temporary directory path */
static const char *get_temp_dir(void)
{
#ifdef _WIN32
    static char temp_path[PATH_MAX];
    DWORD len = GetTempPathA(sizeof(temp_path), temp_path);
    if (len > 0 && len < sizeof(temp_path)) {
        return temp_path;
    }
    return ".";
#else
    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir) return tmpdir;
    tmpdir = getenv("TMP");
    if (tmpdir) return tmpdir;
    tmpdir = getenv("TEMP");
    if (tmpdir) return tmpdir;
    return "/tmp";
#endif
}

/* Generate a unique temporary file path */
bool updater_get_temp_path(char *buf, size_t bufsize, const char *suffix)
{
    const char *temp_dir = get_temp_dir();

#ifdef _WIN32
    DWORD pid = GetCurrentProcessId();
    snprintf(buf, bufsize, "%ssn_update_%lu%s", temp_dir, (unsigned long)pid, suffix);
#else
    pid_t pid = getpid();
    snprintf(buf, bufsize, "%s/sn_update_%d%s", temp_dir, (int)pid, suffix);
#endif

    return true;
}
