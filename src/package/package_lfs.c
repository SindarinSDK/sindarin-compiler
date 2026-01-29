/* ==============================================================================
 * package_lfs.c - Git LFS Support for Package Manager
 * ==============================================================================
 * Implements Git LFS protocol for downloading large files stored in LFS.
 * Uses libssh2 for SSH authentication and libcurl for HTTPS downloads.
 *
 * LFS Protocol Overview:
 *   1. Detect LFS pointer files (small files with special format)
 *   2. For SSH remotes: authenticate via git-lfs-authenticate command
 *   3. POST to LFS batch API to get download URLs
 *   4. Download actual content and replace pointer files
 * ============================================================================== */

#include "../package.h"
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

/* ============================================================================
 * LFS Pointer Parsing
 * ============================================================================ */

typedef struct {
    char oid[MAX_OID_LEN];      /* SHA256 hash */
    long long size;              /* File size in bytes */
    bool valid;
} LfsPointer;

/* Parse an LFS pointer file content */
static LfsPointer parse_lfs_pointer(const char *content, size_t len)
{
    LfsPointer ptr = {0};
    ptr.valid = false;

    if (len > LFS_POINTER_MAX_SIZE) {
        return ptr;  /* Too large to be a pointer file */
    }

    /* Check version line */
    if (strncmp(content, LFS_POINTER_VERSION, strlen(LFS_POINTER_VERSION)) != 0) {
        return ptr;
    }

    /* Find oid line */
    const char *oid_start = strstr(content, LFS_OID_PREFIX);
    if (!oid_start) {
        return ptr;
    }
    oid_start += strlen(LFS_OID_PREFIX);

    /* Extract OID (64 hex chars) */
    size_t i;
    for (i = 0; i < 64 && oid_start[i] && oid_start[i] != '\n' && oid_start[i] != '\r'; i++) {
        ptr.oid[i] = oid_start[i];
    }
    ptr.oid[i] = '\0';

    if (i != 64) {
        return ptr;  /* Invalid OID length */
    }

    /* Find size line */
    const char *size_start = strstr(content, LFS_SIZE_PREFIX);
    if (!size_start) {
        return ptr;
    }
    size_start += strlen(LFS_SIZE_PREFIX);

    ptr.size = atoll(size_start);
    if (ptr.size <= 0) {
        return ptr;
    }

    ptr.valid = true;
    return ptr;
}

/* Check if a file is an LFS pointer */
static bool is_lfs_pointer_file(const char *path, LfsPointer *out_ptr)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* LFS pointers are small */
    if (fsize <= 0 || fsize > LFS_POINTER_MAX_SIZE) {
        fclose(f);
        return false;
    }

    char content[LFS_POINTER_MAX_SIZE + 1];
    size_t read_size = fread(content, 1, (size_t)fsize, f);
    fclose(f);

    if (read_size != (size_t)fsize) {
        return false;
    }
    content[read_size] = '\0';

    LfsPointer ptr = parse_lfs_pointer(content, read_size);
    if (ptr.valid && out_ptr) {
        *out_ptr = ptr;
    }
    return ptr.valid;
}

#if SN_HAS_CURL

/* ============================================================================
 * CURL Response Buffer
 * ============================================================================ */

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

static size_t response_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;

    if (buf->size + realsize >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->size + realsize + 1) {
            new_cap = buf->size + realsize + 1;
        }
        if (new_cap > MAX_RESPONSE_SIZE) {
            return 0;  /* Response too large */
        }
        char *new_data = realloc(buf->data, new_cap);
        if (!new_data) {
            return 0;
        }
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = '\0';

    return realsize;
}

static ResponseBuffer *response_buffer_new(void)
{
    ResponseBuffer *buf = malloc(sizeof(ResponseBuffer));
    if (!buf) return NULL;

    buf->capacity = 4096;
    buf->data = malloc(buf->capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    buf->size = 0;
    buf->data[0] = '\0';
    return buf;
}

static void response_buffer_free(ResponseBuffer *buf)
{
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

/* ============================================================================
 * URL Parsing and LFS Server Detection
 * ============================================================================ */

typedef struct {
    char host[256];
    char owner[256];
    char repo[256];
    bool is_ssh;
    char https_base[MAX_URL_LEN];  /* Base URL for LFS API */
} LfsRemoteInfo;

/* Parse git remote URL to extract host, owner, repo */
static bool parse_remote_url(const char *url, LfsRemoteInfo *info)
{
    memset(info, 0, sizeof(*info));

    /* SSH format: git@github.com:owner/repo.git */
    if (strncmp(url, "git@", 4) == 0) {
        info->is_ssh = true;
        const char *host_start = url + 4;
        const char *colon = strchr(host_start, ':');
        if (!colon) return false;

        size_t host_len = (size_t)(colon - host_start);
        if (host_len >= sizeof(info->host)) return false;
        strncpy(info->host, host_start, host_len);
        info->host[host_len] = '\0';

        const char *path_start = colon + 1;
        const char *slash = strchr(path_start, '/');
        if (!slash) return false;

        size_t owner_len = (size_t)(slash - path_start);
        if (owner_len >= sizeof(info->owner)) return false;
        strncpy(info->owner, path_start, owner_len);
        info->owner[owner_len] = '\0';

        const char *repo_start = slash + 1;
        const char *dot_git = strstr(repo_start, ".git");
        size_t repo_len = dot_git ? (size_t)(dot_git - repo_start) : strlen(repo_start);
        if (repo_len >= sizeof(info->repo)) return false;
        strncpy(info->repo, repo_start, repo_len);
        info->repo[repo_len] = '\0';

        /* Build HTTPS base URL */
        snprintf(info->https_base, sizeof(info->https_base),
                 "https://%s/%s/%s.git/info/lfs", info->host, info->owner, info->repo);
        return true;
    }

    /* HTTPS format: https://github.com/owner/repo.git */
    if (strncmp(url, "https://", 8) == 0) {
        info->is_ssh = false;
        const char *host_start = url + 8;
        const char *slash = strchr(host_start, '/');
        if (!slash) return false;

        size_t host_len = (size_t)(slash - host_start);
        if (host_len >= sizeof(info->host)) return false;
        strncpy(info->host, host_start, host_len);
        info->host[host_len] = '\0';

        const char *owner_start = slash + 1;
        slash = strchr(owner_start, '/');
        if (!slash) return false;

        size_t owner_len = (size_t)(slash - owner_start);
        if (owner_len >= sizeof(info->owner)) return false;
        strncpy(info->owner, owner_start, owner_len);
        info->owner[owner_len] = '\0';

        const char *repo_start = slash + 1;
        const char *dot_git = strstr(repo_start, ".git");
        size_t repo_len = dot_git ? (size_t)(dot_git - repo_start) : strlen(repo_start);
        if (repo_len >= sizeof(info->repo)) return false;
        strncpy(info->repo, repo_start, repo_len);
        info->repo[repo_len] = '\0';

        /* Build HTTPS base URL */
        snprintf(info->https_base, sizeof(info->https_base),
                 "https://%s/%s/%s.git/info/lfs", info->host, info->owner, info->repo);
        return true;
    }

    return false;
}

/* ============================================================================
 * SSH Authentication for LFS (git-lfs-authenticate)
 * ============================================================================ */

typedef struct {
    char href[MAX_URL_LEN];
    char auth_header[MAX_URL_LEN];
    bool valid;
} LfsAuthInfo;

/* Try to find SSH key in default locations (~/.ssh/) */
static const char *find_ssh_key(void)
{
    static char key_path[512];
    const char *home = NULL;

#ifdef _WIN32
    home = getenv("USERPROFILE");
#else
    home = getenv("HOME");
#endif

    if (!home) return NULL;

    const char *key_names[] = {"id_ed25519", "id_rsa", "id_ecdsa", "id_dsa", NULL};

    for (int i = 0; key_names[i] != NULL; i++) {
#ifdef _WIN32
        snprintf(key_path, sizeof(key_path), "%s\\.ssh\\%s", home, key_names[i]);
#else
        snprintf(key_path, sizeof(key_path), "%s/.ssh/%s", home, key_names[i]);
#endif
        FILE *f = fopen(key_path, "r");
        if (f) {
            fclose(f);
            return key_path;
        }
    }

    return NULL;
}

/* Authenticate via SSH for LFS access */
static LfsAuthInfo lfs_ssh_authenticate(const LfsRemoteInfo *remote)
{
    LfsAuthInfo auth = {0};
    auth.valid = false;

    /* Initialize libssh2 */
    if (libssh2_init(0) != 0) {
        return auth;
    }

    /* Create socket and connect */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        libssh2_exit();
        return auth;
    }

    struct hostent *host_entry = gethostbyname(remote->host);
    if (!host_entry) {
        close(sock);
        libssh2_exit();
        return auth;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr = *((struct in_addr *)host_entry->h_addr);

    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(sock);
        libssh2_exit();
        return auth;
    }

    /* Create SSH session */
    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        close(sock);
        libssh2_exit();
        return auth;
    }

    if (libssh2_session_handshake(session, sock) != 0) {
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return auth;
    }

    /* Try to authenticate */
    const char *username = "git";
    const char *key_path = getenv("SN_GIT_SSH_KEY");
    if (!key_path) {
        key_path = find_ssh_key();
    }

    bool authenticated = false;

    if (key_path) {
        char pubkey_path[512];
        snprintf(pubkey_path, sizeof(pubkey_path), "%s.pub", key_path);
        const char *passphrase = getenv("SN_GIT_SSH_PASSPHRASE");

        if (libssh2_userauth_publickey_fromfile(session, username,
                                                 pubkey_path, key_path,
                                                 passphrase ? passphrase : "") == 0) {
            authenticated = true;
        }
    }

    if (!authenticated) {
        libssh2_session_disconnect(session, "Auth failed");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return auth;
    }

    /* Execute git-lfs-authenticate command */
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        libssh2_session_disconnect(session, "Channel failed");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return auth;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "git-lfs-authenticate %s/%s download", remote->owner, remote->repo);

    if (libssh2_channel_exec(channel, cmd) != 0) {
        libssh2_channel_free(channel);
        libssh2_session_disconnect(session, "Exec failed");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
        return auth;
    }

    /* Read response */
    char response[4096] = {0};
    size_t response_len = 0;
    ssize_t rc;

    while ((rc = libssh2_channel_read(channel, response + response_len,
                                       sizeof(response) - response_len - 1)) > 0) {
        response_len += rc;
    }
    response[response_len] = '\0';

    libssh2_channel_free(channel);
    libssh2_session_disconnect(session, "Done");
    libssh2_session_free(session);
    close(sock);
    libssh2_exit();

    /* Parse JSON response */
    if (response_len == 0) {
        return auth;
    }

    yyjson_doc *doc = yyjson_read(response, response_len, 0);
    if (!doc) {
        return auth;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *href_val = yyjson_obj_get(root, "href");
    yyjson_val *header_val = yyjson_obj_get(root, "header");

    if (href_val && yyjson_is_str(href_val)) {
        strncpy(auth.href, yyjson_get_str(href_val), sizeof(auth.href) - 1);

        if (header_val && yyjson_is_obj(header_val)) {
            yyjson_val *auth_val = yyjson_obj_get(header_val, "Authorization");
            if (auth_val && yyjson_is_str(auth_val)) {
                strncpy(auth.auth_header, yyjson_get_str(auth_val), sizeof(auth.auth_header) - 1);
            }
        }
        auth.valid = true;
    }

    yyjson_doc_free(doc);
    return auth;
}

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

    /* Download each LFS object */
    bool success = true;
    for (int i = 0; i < pointers->count; i++) {
        LfsPointer *ptr = &pointers->pointers[i];
        const char *file_path = pointers->paths[i];

        /* Get download URL from batch API */
        LfsDownloadInfo dl_info = lfs_batch_request(lfs_base_url, auth_header,
                                                     ptr->oid, ptr->size);
        if (!dl_info.valid) {
            fprintf(stderr, "%swarning%s: failed to get LFS download URL for %s\n",
                    COLOR_YELLOW, COLOR_RESET, file_path);
            success = false;
            continue;
        }

        /* Download the actual content */
        const char *dl_auth = dl_info.auth_header[0] ? dl_info.auth_header : auth_header;
        if (!download_lfs_object(dl_info.download_url, dl_auth, file_path, ptr->size)) {
            fprintf(stderr, "%swarning%s: failed to download LFS object for %s\n",
                    COLOR_YELLOW, COLOR_RESET, file_path);
            success = false;
        }
    }

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
