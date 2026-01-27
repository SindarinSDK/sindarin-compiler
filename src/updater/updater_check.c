/*
 * Sindarin Compiler Auto-Update Module - Background Check Implementation
 * Handles non-blocking update checking via background thread.
 */

#include "../updater.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-specific includes */
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #if defined(__MINGW32__) || defined(__MINGW64__)
        /* MinGW provides native pthreads */
        #include <pthread.h>
    #else
        /* MSVC needs compatibility layer */
        #include "../platform/compat_pthread.h"
    #endif
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

/* Conditional curl support */
#if SN_HAS_CURL
    #include <curl/curl.h>
    #include <yyjson.h>
#endif

/* Thread state */
static pthread_t g_check_thread;
static pthread_mutex_t g_result_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_check_running = 0;
static volatile int g_check_completed = 0;
static UpdateInfo g_update_info;

#if SN_HAS_CURL

/* Response buffer for curl */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

/* cURL write callback */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    /* Expand buffer if needed */
    if (buf->size + realsize + 1 > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->size + realsize + 1) {
            new_cap = buf->size + realsize + 1;
        }
        char *new_data = realloc(buf->data, new_cap);
        if (!new_data) {
            return 0;  /* Out of memory */
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';

    return realsize;
}

/* Parse GitHub API JSON response */
static void parse_github_response(const char *json_str)
{
    yyjson_doc *doc = yyjson_read(json_str, strlen(json_str), 0);
    if (!doc) {
        return;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return;
    }

    /* Extract tag_name (e.g., "v1.2.3-alpha") */
    yyjson_val *tag_name_val = yyjson_obj_get(root, "tag_name");
    if (!yyjson_is_str(tag_name_val)) {
        yyjson_doc_free(doc);
        return;
    }

    const char *tag = yyjson_get_str(tag_name_val);
    strncpy(g_update_info.tag_name, tag, SN_UPDATE_TAG_MAX - 1);
    g_update_info.tag_name[SN_UPDATE_TAG_MAX - 1] = '\0';

    /* Extract version number (strip 'v' prefix and suffix) */
    const char *ver = tag;
    if (ver[0] == 'v') ver++;

    /* Find dash for suffix (e.g., "-alpha") */
    const char *dash = strchr(ver, '-');
    if (dash) {
        size_t len = (size_t)(dash - ver);
        if (len >= SN_UPDATE_VERSION_MAX) {
            len = SN_UPDATE_VERSION_MAX - 1;
        }
        strncpy(g_update_info.version, ver, len);
        g_update_info.version[len] = '\0';
    } else {
        strncpy(g_update_info.version, ver, SN_UPDATE_VERSION_MAX - 1);
        g_update_info.version[SN_UPDATE_VERSION_MAX - 1] = '\0';
    }

    /* Extract release notes (body field) - truncate if too long */
    yyjson_val *body = yyjson_obj_get(root, "body");
    if (yyjson_is_str(body)) {
        const char *notes = yyjson_get_str(body);
        strncpy(g_update_info.release_notes, notes, SN_UPDATE_NOTES_MAX - 1);
        g_update_info.release_notes[SN_UPDATE_NOTES_MAX - 1] = '\0';
    }

    /* Find platform-specific asset */
    yyjson_val *assets = yyjson_obj_get(root, "assets");
    if (yyjson_is_arr(assets)) {
        const char *platform_suffix = updater_get_platform_suffix();

        yyjson_arr_iter iter;
        yyjson_arr_iter_init(assets, &iter);
        yyjson_val *asset;

        while ((asset = yyjson_arr_iter_next(&iter)) != NULL) {
            yyjson_val *name = yyjson_obj_get(asset, "name");
            if (yyjson_is_str(name)) {
                const char *asset_name = yyjson_get_str(name);
                if (strstr(asset_name, platform_suffix)) {
                    yyjson_val *url = yyjson_obj_get(asset, "browser_download_url");
                    if (yyjson_is_str(url)) {
                        strncpy(g_update_info.download_url, yyjson_get_str(url),
                                SN_UPDATE_URL_MAX - 1);
                        g_update_info.download_url[SN_UPDATE_URL_MAX - 1] = '\0';
                    }
                    break;
                }
            }
        }
    }

    /* Check if update is available by comparing versions */
    if (updater_version_compare(g_update_info.version, SN_VERSION_STRING) > 0) {
        g_update_info.update_available = true;
    }

    yyjson_doc_free(doc);
}

/* Background thread function */
static void *check_thread_func(void *arg)
{
    (void)arg;

    CURL *curl = curl_easy_init();
    if (!curl) {
        pthread_mutex_lock(&g_result_mutex);
        g_check_completed = 1;
        pthread_mutex_unlock(&g_result_mutex);
        return NULL;
    }

    /* Initialize response buffer */
    ResponseBuffer response = {0};
    response.capacity = 16384;
    response.data = malloc(response.capacity);
    if (!response.data) {
        curl_easy_cleanup(curl);
        pthread_mutex_lock(&g_result_mutex);
        g_check_completed = 1;
        pthread_mutex_unlock(&g_result_mutex);
        return NULL;
    }
    response.data[0] = '\0';

    /* Setup request to GitHub API */
    curl_easy_setopt(curl, CURLOPT_URL, SN_GITHUB_API_URL);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sn-compiler/" SN_VERSION_STRING);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        /* 5 second timeout */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L); /* 3 second connect timeout */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    /* Disable verbose output - we want silent operation */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    /* Perform request - silently ignore all errors */
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200 && response.data) {
            /* Parse JSON response */
            parse_github_response(response.data);
        }
    }
    /* Silently ignore errors - don't print anything */

    free(response.data);
    curl_easy_cleanup(curl);

    pthread_mutex_lock(&g_result_mutex);
    g_check_completed = 1;
    pthread_mutex_unlock(&g_result_mutex);

    return NULL;
}

#endif /* SN_HAS_CURL */

void updater_check_start(void)
{
#if SN_HAS_CURL
    if (updater_is_disabled() || g_check_running) {
        return;
    }

    g_check_running = 1;
    g_check_completed = 0;
    memset(&g_update_info, 0, sizeof(g_update_info));

    /* Initialize curl globally (thread-safe) */
    static int curl_initialized = 0;
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_initialized = 1;
    }

    /* Create detached thread for background check */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&g_check_thread, &attr, check_thread_func, NULL) != 0) {
        /* Failed to create thread - mark as completed with no update */
        g_check_running = 0;
        g_check_completed = 1;
    }

    pthread_attr_destroy(&attr);
#else
    /* No curl - mark as completed immediately */
    g_check_completed = 1;
#endif
}

bool updater_check_done(void)
{
    int done;
    pthread_mutex_lock(&g_result_mutex);
    done = g_check_completed;
    pthread_mutex_unlock(&g_result_mutex);
    return done != 0;
}

const UpdateInfo *updater_get_result(void)
{
    if (!g_check_completed) {
        return NULL;
    }

    if (!g_update_info.update_available) {
        return NULL;
    }

    return &g_update_info;
}
