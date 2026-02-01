// src/package/package_lfs_download.c
// File Download and Directory Scanning

#if SN_HAS_CURL

/* ============================================================================
 * File Download
 * ============================================================================ */

static size_t file_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *fp = (FILE *)userp;
    return fwrite(contents, size, nmemb, fp);
}

static bool download_lfs_object(const char *url, const char *auth_header,
                                 const char *dest_path, long long expected_size)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    /* Download to temp file first */
    char temp_path[MAX_PATH_LEN];
    snprintf(temp_path, sizeof(temp_path), "%s.lfs_tmp", dest_path);

    FILE *fp = fopen(temp_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    struct curl_slist *headers = NULL;
    if (auth_header && auth_header[0]) {
        char auth_line[MAX_URL_LEN + 16];
        snprintf(auth_line, sizeof(auth_line), "Authorization: %s", auth_header);
        headers = curl_slist_append(headers, auth_line);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "git-lfs/3.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  /* 5 minutes for large files */

    CURLcode res = curl_easy_perform(curl);

    if (headers) {
        curl_slist_free_all(headers);
    }
    fclose(fp);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        remove(temp_path);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        remove(temp_path);
        return false;
    }

    /* Verify size */
    struct stat st;
    if (stat(temp_path, &st) != 0 || st.st_size != expected_size) {
        remove(temp_path);
        return false;
    }

    /* Replace original file */
    remove(dest_path);
    if (rename(temp_path, dest_path) != 0) {
        remove(temp_path);
        return false;
    }

    return true;
}

/* ============================================================================
 * Directory Scanning for LFS Pointers
 * ============================================================================ */

typedef struct {
    char paths[1024][MAX_PATH_LEN];
    LfsPointer pointers[1024];
    int count;
} LfsPointerList;

static void scan_directory_for_lfs_pointers(const char *dir_path, LfsPointerList *list)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && list->count < 1024) {
        /* Skip hidden files and directories */
        if (entry->d_name[0] == '.') {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s%c%s", dir_path, PATH_SEP, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            /* Recurse into subdirectory */
            scan_directory_for_lfs_pointers(full_path, list);
        } else if (S_ISREG(st.st_mode)) {
            /* Check if file is an LFS pointer */
            LfsPointer ptr;
            if (is_lfs_pointer_file(full_path, &ptr)) {
                strncpy(list->paths[list->count], full_path, MAX_PATH_LEN - 1);
                list->pointers[list->count] = ptr;
                list->count++;
            }
        }
    }

    closedir(dir);
}

#endif /* SN_HAS_CURL */
