// src/package/package_lfs_remote.c
// URL Parsing and LFS Server Detection

#if SN_HAS_CURL

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

#endif /* SN_HAS_CURL */
