// src/package/package_lfs_batch.c
// LFS Batch API

#if SN_HAS_CURL

/* ============================================================================
 * LFS Batch API
 * ============================================================================ */

typedef struct {
    char download_url[MAX_URL_LEN];
    char auth_header[MAX_URL_LEN];
    bool valid;
} LfsDownloadInfo;

/* Request download URL from LFS batch API */
static LfsDownloadInfo lfs_batch_request(const char *base_url, const char *auth_header,
                                          const char *oid, long long size)
{
    LfsDownloadInfo info = {0};
    info.valid = false;

    CURL *curl = curl_easy_init();
    if (!curl) {
        return info;
    }

    /* Build batch URL */
    char batch_url[MAX_URL_LEN];
    snprintf(batch_url, sizeof(batch_url), "%s/objects/batch", base_url);

    /* Build JSON request body */
    char request_body[512];
    snprintf(request_body, sizeof(request_body),
             "{\"operation\":\"download\",\"transfers\":[\"basic\"],"
             "\"objects\":[{\"oid\":\"%s\",\"size\":%lld}]}",
             oid, size);

    ResponseBuffer *response = response_buffer_new();
    if (!response) {
        curl_easy_cleanup(curl);
        return info;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/vnd.git-lfs+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.git-lfs+json");

    if (auth_header && auth_header[0]) {
        char auth_line[MAX_URL_LEN + 16];
        snprintf(auth_line, sizeof(auth_line), "Authorization: %s", auth_header);
        headers = curl_slist_append(headers, auth_line);
    } else {
        /* Try environment variables for HTTPS auth */
        const char *username = getenv("SN_GIT_USERNAME");
        const char *token = getenv("SN_GIT_TOKEN");
        if (!token) token = getenv("SN_GIT_PASSWORD");

        if (username && token) {
            curl_easy_setopt(curl, CURLOPT_USERNAME, username);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, token);
        } else if (token) {
            /* Token-only auth (common for GitHub) */
            char auth_line[MAX_URL_LEN + 16];
            snprintf(auth_line, sizeof(auth_line), "Authorization: Bearer %s", token);
            headers = curl_slist_append(headers, auth_line);
        }
    }

    curl_easy_setopt(curl, CURLOPT_URL, batch_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "git-lfs/3.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        response_buffer_free(response);
        return info;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        response_buffer_free(response);
        return info;
    }

    /* Parse JSON response */
    yyjson_doc *doc = yyjson_read(response->data, response->size, 0);
    response_buffer_free(response);

    if (!doc) {
        return info;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *objects = yyjson_obj_get(root, "objects");

    if (objects && yyjson_is_arr(objects) && yyjson_arr_size(objects) > 0) {
        yyjson_val *obj = yyjson_arr_get_first(objects);
        yyjson_val *actions = yyjson_obj_get(obj, "actions");

        if (actions) {
            yyjson_val *download = yyjson_obj_get(actions, "download");
            if (download) {
                yyjson_val *href = yyjson_obj_get(download, "href");
                if (href && yyjson_is_str(href)) {
                    strncpy(info.download_url, yyjson_get_str(href), sizeof(info.download_url) - 1);

                    /* Check for auth header in response */
                    yyjson_val *resp_header = yyjson_obj_get(download, "header");
                    if (resp_header) {
                        yyjson_val *resp_auth = yyjson_obj_get(resp_header, "Authorization");
                        if (resp_auth && yyjson_is_str(resp_auth)) {
                            strncpy(info.auth_header, yyjson_get_str(resp_auth),
                                    sizeof(info.auth_header) - 1);
                        }
                    }
                    info.valid = true;
                }
            }
        }
    }

    yyjson_doc_free(doc);
    return info;
}

#endif /* SN_HAS_CURL */
