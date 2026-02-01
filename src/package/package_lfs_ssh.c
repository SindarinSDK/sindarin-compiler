// src/package/package_lfs_ssh.c
// SSH Authentication for LFS

#if SN_HAS_CURL

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

#ifdef _WIN32
    /* Initialize Winsock (required for socket operations on Windows) */
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return auth;
    }
#endif

    /* Initialize libssh2 */
    if (libssh2_init(0) != 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return auth;
    }

    /* Create socket and connect */
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        libssh2_exit();
        WSACleanup();
        return auth;
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        libssh2_exit();
        return auth;
    }
#endif

    struct hostent *host_entry = gethostbyname(remote->host);
    if (!host_entry) {
        close(sock);
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
        return auth;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr = *((struct in_addr *)host_entry->h_addr);

    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
        close(sock);
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
        return auth;
    }

    /* Create SSH session */
    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        close(sock);
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
        return auth;
    }

    if (libssh2_session_handshake(session, sock) != 0) {
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
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
#ifdef _WIN32
        WSACleanup();
#endif
        return auth;
    }

    /* Execute git-lfs-authenticate command */
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        libssh2_session_disconnect(session, "Channel failed");
        libssh2_session_free(session);
        close(sock);
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
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
#ifdef _WIN32
        WSACleanup();
#endif
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
#ifdef _WIN32
    WSACleanup();
#endif

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

#endif /* SN_HAS_CURL */
