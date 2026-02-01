/* ==============================================================================
 * package_lfs.c - Git LFS Support for Package Manager
 * ==============================================================================
 * Implements Git LFS protocol for downloading large files stored in LFS.
 * Uses libssh2 for SSH authentication and libcurl for HTTPS downloads.
 * Downloads are parallelized using one thread per CPU core.
 *
 * LFS Protocol Overview:
 *   1. Detect LFS pointer files (small files with special format)
 *   2. For SSH remotes: authenticate via git-lfs-authenticate command
 *   3. POST to LFS batch API to get download URLs
 *   4. Download actual content and replace pointer files (multi-threaded)
 * ============================================================================== */

#include "../../package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <direct.h>
    #define PATH_SEP '\\'
    #define close closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #define PATH_SEP '/'
#endif

/* Threading support */
#if defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__)
    /* MSVC - use compatibility layer */
    #include "../../platform/compat_pthread.h"
#else
    /* POSIX systems and MinGW (which has native pthreads) */
    #include <pthread.h>
#endif

/* Conditional curl/ssh support - matches updater pattern */
#if SN_HAS_CURL
    #include <curl/curl.h>
    #include <yyjson.h>
    #include <libssh2.h>
#endif

/* ANSI color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"

/* LFS pointer file constants */
#define LFS_POINTER_VERSION "version https://git-lfs.github.com/spec/v1"
#define LFS_POINTER_MAX_SIZE 512
#define LFS_OID_PREFIX "oid sha256:"
#define LFS_SIZE_PREFIX "size "

/* Maximum sizes */
#define MAX_PATH_LEN 1024
#define MAX_URL_LEN 512
#define MAX_OID_LEN 65
#define MAX_RESPONSE_SIZE (1024 * 1024)  /* 1MB for API responses */

/* Include split modules */
#include "package_lfs_pointer.c"
#include "package_lfs_buffer.c"
#include "package_lfs_remote.c"
#include "package_lfs_ssh.c"
#include "package_lfs_batch.c"
#include "package_lfs_download.c"

/* ============================================================================
 * Multi-threaded Download Support
 * ============================================================================ */

#if SN_HAS_CURL

/* Shared state for download worker threads */
typedef struct {
    /* Input data (read-only after init) */
    LfsPointerList *pointers;
    const char *lfs_base_url;
    const char *auth_header;

    /* Shared mutable state (protected by mutex) */
    pthread_mutex_t mutex;
    int next_index;          /* Next file index to download */
    int completed_count;     /* Number of completed downloads */
    int failed_count;        /* Number of failed downloads */
} LfsDownloadContext;

/* Worker thread function for parallel downloads */
static void *lfs_download_worker(void *arg)
{
    LfsDownloadContext *ctx = (LfsDownloadContext *)arg;

    while (1) {
        /* Get next task */
        pthread_mutex_lock(&ctx->mutex);
        int index = ctx->next_index++;
        int total = ctx->pointers->count;
        pthread_mutex_unlock(&ctx->mutex);

        if (index >= total) {
            break;  /* No more work */
        }

        LfsPointer *ptr = &ctx->pointers->pointers[index];
        const char *file_path = ctx->pointers->paths[index];

        /* Extract just the filename for display */
        const char *filename = strrchr(file_path, PATH_SEP);
        if (filename) {
            filename++;
        } else {
            filename = file_path;
        }

        /* Update progress (with mutex for clean output) */
        pthread_mutex_lock(&ctx->mutex);
        printf("\r    fetching LFS [%d/%d] %s...                              ",
               ctx->completed_count + 1, total, filename);
        fflush(stdout);
        pthread_mutex_unlock(&ctx->mutex);

        /* Get download URL from batch API */
        LfsDownloadInfo dl_info = lfs_batch_request(ctx->lfs_base_url, ctx->auth_header,
                                                     ptr->oid, ptr->size);
        bool ok = false;
        if (dl_info.valid) {
            /* Download the actual content */
            const char *dl_auth = dl_info.auth_header[0] ? dl_info.auth_header : ctx->auth_header;
            ok = download_lfs_object(dl_info.download_url, dl_auth, file_path, ptr->size);
        }

        /* Update counters */
        pthread_mutex_lock(&ctx->mutex);
        ctx->completed_count++;
        if (!ok) {
            ctx->failed_count++;
            /* Print warning (clear progress line first) */
            printf("\r                                                              \r");
            fprintf(stderr, "%swarning%s: failed to download LFS object for %s\n",
                    COLOR_YELLOW, COLOR_RESET, file_path);
        }
        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}

#endif /* SN_HAS_CURL */

/* ============================================================================
 * Public API
 * ============================================================================ */

bool package_lfs_pull(const char *repo_path)
{
#if SN_HAS_CURL
    /* Check for .gitattributes with LFS config */
    char gitattributes_path[MAX_PATH_LEN];
    snprintf(gitattributes_path, sizeof(gitattributes_path), "%s%c.gitattributes",
             repo_path, PATH_SEP);

    FILE *f = fopen(gitattributes_path, "r");
    if (!f) {
        return true;  /* No .gitattributes, nothing to do */
    }

    bool uses_lfs = false;
    char line[256];
    while (fgets(line, sizeof(line), f) != NULL) {
        if (strstr(line, "filter=lfs") != NULL) {
            uses_lfs = true;
            break;
        }
    }
    fclose(f);

    if (!uses_lfs) {
        return true;  /* No LFS tracking */
    }

    /* Get remote URL */
    char config_path[MAX_PATH_LEN];
    snprintf(config_path, sizeof(config_path), "%s%c.git%cconfig", repo_path, PATH_SEP, PATH_SEP);

    f = fopen(config_path, "r");
    if (!f) {
        return false;
    }

    char remote_url[MAX_URL_LEN] = {0};
    bool in_remote_origin = false;

    while (fgets(line, sizeof(line), f) != NULL) {
        /* Trim whitespace */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (strncmp(p, "[remote \"origin\"]", 17) == 0) {
            in_remote_origin = true;
        } else if (p[0] == '[') {
            in_remote_origin = false;
        } else if (in_remote_origin && strncmp(p, "url = ", 6) == 0) {
            char *url_start = p + 6;
            size_t url_len = strlen(url_start);
            while (url_len > 0 && (url_start[url_len-1] == '\n' ||
                                    url_start[url_len-1] == '\r')) {
                url_len--;
            }
            if (url_len < sizeof(remote_url)) {
                strncpy(remote_url, url_start, url_len);
                remote_url[url_len] = '\0';
            }
            break;
        }
    }
    fclose(f);

    if (!remote_url[0]) {
        return false;
    }

    /* Parse remote URL */
    LfsRemoteInfo remote;
    if (!parse_remote_url(remote_url, &remote)) {
        fprintf(stderr, "%swarning%s: could not parse remote URL for LFS: %s\n",
                COLOR_YELLOW, COLOR_RESET, remote_url);
        return false;
    }

    /* Scan for LFS pointer files */
    LfsPointerList *pointers = malloc(sizeof(LfsPointerList));
    if (!pointers) {
        return false;
    }
    pointers->count = 0;

    scan_directory_for_lfs_pointers(repo_path, pointers);

    if (pointers->count == 0) {
        free(pointers);
        return true;  /* No LFS pointers found */
    }

    /* Get authentication info */
    LfsAuthInfo auth = {0};
    const char *lfs_base_url = remote.https_base;
    const char *auth_header = NULL;

    if (remote.is_ssh) {
        auth = lfs_ssh_authenticate(&remote);
        if (auth.valid) {
            lfs_base_url = auth.href;
            auth_header = auth.auth_header;
        }
        /* If SSH auth fails, fall back to HTTPS with env credentials */
    }

    /* Download LFS objects using multiple threads */
    int num_threads = get_cpu_count();
    if (num_threads > pointers->count) {
        num_threads = pointers->count;  /* Don't create more threads than files */
    }
    if (num_threads < 1) {
        num_threads = 1;
    }

    /* Initialize download context */
    LfsDownloadContext ctx = {0};
    ctx.pointers = pointers;
    ctx.lfs_base_url = lfs_base_url;
    ctx.auth_header = auth_header;
    ctx.next_index = 0;
    ctx.completed_count = 0;
    ctx.failed_count = 0;
    pthread_mutex_init(&ctx.mutex, NULL);

    /* Create worker threads */
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    if (!threads) {
        pthread_mutex_destroy(&ctx.mutex);
        free(pointers);
        return false;
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, lfs_download_worker, &ctx);
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);

    /* Clear the progress line */
    printf("\r                                                              \r");
    fflush(stdout);

    bool success = (ctx.failed_count == 0);
    pthread_mutex_destroy(&ctx.mutex);
    free(pointers);
    return success;

#else
    (void)repo_path;
    return true;  /* No-op without curl support */
#endif
}

bool package_lfs_available(void)
{
#if SN_HAS_CURL
    return true;
#else
    return false;
#endif
}
