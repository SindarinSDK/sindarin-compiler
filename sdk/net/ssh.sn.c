/* ==============================================================================
 * sdk/net/ssh.sn.c - Self-contained SSH Connection Implementation
 * ==============================================================================
 * This file provides the C implementation for SshConnection using libssh2
 * with OpenSSL backend.
 * It is compiled via @source and linked with Sindarin code.
 *
 * Known hosts verification priority:
 *   1. SN_SSH_KNOWN_HOSTS environment variable (path to known_hosts file)
 *   2. Platform default (~/.ssh/known_hosts or %USERPROFILE%\.ssh\known_hosts)
 * ============================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Include runtime for proper memory management */
#include "runtime/runtime_arena.h"
#include "runtime/runtime_array.h"

/* libssh2 includes */
#include <libssh2.h>

/* Platform-specific socket includes */
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")

    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()

    #ifndef PATH_MAX
        #define PATH_MAX 260
    #endif
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <limits.h>

    typedef int socket_t;
    #define INVALID_SOCKET_VAL (-1)
    #define SOCKET_ERROR_VAL (-1)
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <limits.h>

    typedef int socket_t;
    #define INVALID_SOCKET_VAL (-1)
    #define SOCKET_ERROR_VAL (-1)
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
#endif

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

typedef struct RtSshExecResult {
    char *stdout_str;
    char *stderr_str;
    long exit_code;
} RtSshExecResult;

typedef struct RtSshConnection {
    socket_t socket_fd;         /* Underlying TCP socket */
    void *session_ptr;          /* LIBSSH2_SESSION* - opaque to Sindarin */
    char *remote_addr;          /* Remote address string (host:port) */
} RtSshConnection;

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

void sn_ssh_close(RtSshConnection *conn);

/* ============================================================================
 * libssh2 Initialization (one-time)
 * ============================================================================ */

static int libssh2_lib_initialized = 0;

static void ensure_libssh2_initialized(void) {
    if (!libssh2_lib_initialized) {
        int rc = libssh2_init(0);
        if (rc != 0) {
            fprintf(stderr, "SshConnection: libssh2_init failed: %d\n", rc);
            exit(1);
        }
        libssh2_lib_initialized = 1;
    }
}

/* ============================================================================
 * WinSock Initialization (Windows only)
 * ============================================================================ */

#ifdef _WIN32
static int winsock_initialized = 0;

static void ensure_winsock_initialized(void) {
    if (!winsock_initialized) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            fprintf(stderr, "SshConnection: WSAStartup failed: %d\n", result);
            exit(1);
        }
        winsock_initialized = 1;
    }
}
#else
#define ensure_winsock_initialized() ((void)0)
#endif

/* ============================================================================
 * Known Hosts Verification
 * ============================================================================ */

static void ssh_verify_known_host(LIBSSH2_SESSION *session, const char *host, int port) {
    LIBSSH2_KNOWNHOSTS *nh = libssh2_knownhost_init(session);
    if (!nh) return; /* Skip verification if init fails */

    /* Determine known_hosts file path */
    char known_hosts_path[PATH_MAX];
    const char *env_path = getenv("SN_SSH_KNOWN_HOSTS");
    if (env_path && env_path[0] != '\0') {
        strncpy(known_hosts_path, env_path, sizeof(known_hosts_path) - 1);
        known_hosts_path[sizeof(known_hosts_path) - 1] = '\0';
    } else {
#ifdef _WIN32
        const char *home = getenv("USERPROFILE");
#else
        const char *home = getenv("HOME");
#endif
        if (home) {
            snprintf(known_hosts_path, sizeof(known_hosts_path),
                     "%s/.ssh/known_hosts", home);
        } else {
            libssh2_knownhost_free(nh);
            return; /* Can't find known_hosts, skip */
        }
    }

    /* Try to read the file - if it doesn't exist, skip verification (TOFU) */
    int rc = libssh2_knownhost_readfile(nh, known_hosts_path,
                                         LIBSSH2_KNOWNHOST_FILE_OPENSSH);
    if (rc < 0) {
        libssh2_knownhost_free(nh);
        return; /* File not found or unreadable, trust on first use */
    }

    /* Get the server's host key */
    size_t len;
    int type;
    const char *fingerprint = libssh2_session_hostkey(session, &len, &type);
    if (!fingerprint) {
        libssh2_knownhost_free(nh);
        fprintf(stderr, "SshConnection: unable to get host key\n");
        exit(1);
    }

    /* Map libssh2 key type to knownhost key type */
    int knownhost_key_type;
    switch (type) {
        case LIBSSH2_HOSTKEY_TYPE_RSA:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_SSHRSA;
            break;
        case LIBSSH2_HOSTKEY_TYPE_DSS:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_SSHDSS;
            break;
#ifdef LIBSSH2_HOSTKEY_TYPE_ECDSA_256
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_256:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_ECDSA_256;
            break;
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_384:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_ECDSA_384;
            break;
        case LIBSSH2_HOSTKEY_TYPE_ECDSA_521:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_ECDSA_521;
            break;
#endif
#ifdef LIBSSH2_HOSTKEY_TYPE_ED25519
        case LIBSSH2_HOSTKEY_TYPE_ED25519:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_ED25519;
            break;
#endif
        default:
            knownhost_key_type = LIBSSH2_KNOWNHOST_KEY_UNKNOWN;
            break;
    }

    /* Check the host key against known_hosts */
    struct libssh2_knownhost *kh = NULL;
    int check = libssh2_knownhost_checkp(nh, host, port,
                                          fingerprint, len,
                                          LIBSSH2_KNOWNHOST_TYPE_PLAIN |
                                          LIBSSH2_KNOWNHOST_KEYENC_RAW |
                                          knownhost_key_type,
                                          &kh);
    libssh2_knownhost_free(nh);

    if (check == LIBSSH2_KNOWNHOST_CHECK_MISMATCH) {
        fprintf(stderr, "SshConnection: HOST KEY MISMATCH for '%s:%d' - "
                "possible man-in-the-middle attack!\n", host, port);
        exit(1);
    }
    /* LIBSSH2_KNOWNHOST_CHECK_MATCH = OK */
    /* LIBSSH2_KNOWNHOST_CHECK_NOTFOUND = trust on first use */
    /* LIBSSH2_KNOWNHOST_CHECK_FAILURE = check failed, proceed anyway */
}

/* ============================================================================
 * Address Parsing (host:port with default port 22)
 * ============================================================================ */

static int ssh_parse_address(const char *address, char *host, size_t host_len, int *port) {
    if (address == NULL) return 0;

    const char *last_colon = NULL;

    /* Handle IPv6 addresses like [::1]:22 */
    if (address[0] == '[') {
        const char *bracket = strchr(address, ']');
        if (bracket == NULL) return 0;

        size_t ipv6_len = bracket - address - 1;
        if (ipv6_len >= host_len) return 0;

        memcpy(host, address + 1, ipv6_len);
        host[ipv6_len] = '\0';

        if (bracket[1] == ':') {
            *port = atoi(bracket + 2);
        } else {
            *port = 22; /* Default SSH port */
        }
        return 1;
    }

    /* Find the last colon (for host:port format) */
    for (const char *p = address; *p; p++) {
        if (*p == ':') last_colon = p;
    }

    if (last_colon == NULL) {
        /* No port specified - use hostname as-is with default port 22 */
        size_t len = strlen(address);
        if (len >= host_len) return 0;
        strcpy(host, address);
        *port = 22;
        return 1;
    }

    size_t len = last_colon - address;
    if (len >= host_len) return 0;

    if (len == 0) {
        strcpy(host, "0.0.0.0");
    } else {
        memcpy(host, address, len);
        host[len] = '\0';
    }

    *port = atoi(last_colon + 1);
    return 1;
}

/* ============================================================================
 * Internal: TCP Connect + SSH Session Handshake
 * ============================================================================ */

static RtSshConnection *ssh_connect_and_handshake(RtArena *arena, const char *address) {
    ensure_winsock_initialized();
    ensure_libssh2_initialized();

    if (address == NULL) {
        fprintf(stderr, "SshConnection: NULL address\n");
        exit(1);
    }

    char host[256];
    int port;

    if (!ssh_parse_address(address, host, sizeof(host), &port)) {
        fprintf(stderr, "SshConnection: invalid address format '%s'\n", address);
        exit(1);
    }

    /* --- TCP Connection --- */

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(host, port_str, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "SshConnection: DNS resolution failed for '%s': %s\n",
                host, gai_strerror(status));
        exit(1);
    }

    socket_t sock = INVALID_SOCKET_VAL;

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCKET_VAL) continue;

        if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) != SOCKET_ERROR_VAL) {
            break;
        }

        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET_VAL;
    }

    freeaddrinfo(result);

    if (sock == INVALID_SOCKET_VAL) {
        fprintf(stderr, "SshConnection: TCP connection failed to '%s'\n", address);
        exit(1);
    }

    /* --- SSH Session Setup --- */

    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        CLOSE_SOCKET(sock);
        fprintf(stderr, "SshConnection: session init failed\n");
        exit(1);
    }

    /* Set session to blocking mode */
    libssh2_session_set_blocking(session, 1);

    /* Perform SSH handshake */
    int rc = libssh2_session_handshake(session, (libssh2_socket_t)sock);
    if (rc) {
        char *err_msg = NULL;
        libssh2_session_last_error(session, &err_msg, NULL, 0);
        fprintf(stderr, "SshConnection: handshake failed for '%s': %s\n",
                address, err_msg ? err_msg : "unknown error");
        libssh2_session_free(session);
        CLOSE_SOCKET(sock);
        exit(1);
    }

    /* Verify known hosts */
    ssh_verify_known_host(session, host, port);

    /* --- Allocate Connection Struct --- */

    RtSshConnection *conn = (RtSshConnection *)rt_arena_alloc(arena, sizeof(RtSshConnection));
    if (!conn) {
        fprintf(stderr, "SshConnection: allocation failed\n");
        libssh2_session_disconnect(session, "allocation failure");
        libssh2_session_free(session);
        CLOSE_SOCKET(sock);
        exit(1);
    }

    conn->socket_fd = sock;
    conn->session_ptr = session;

    /* Copy address string into arena */
    size_t addr_len = strlen(address) + 1;
    conn->remote_addr = (char *)rt_arena_alloc(arena, addr_len);
    if (conn->remote_addr) {
        memcpy(conn->remote_addr, address, addr_len);
    }

    return conn;
}

/* ============================================================================
 * Authentication: Password
 * ============================================================================ */

RtSshConnection *sn_ssh_connect_password(RtArena *arena, const char *address,
                                           const char *username, const char *password) {
    RtSshConnection *conn = ssh_connect_and_handshake(arena, address);
    LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;

    int rc = libssh2_userauth_password(session, username, password);
    if (rc) {
        char *err_msg = NULL;
        libssh2_session_last_error(session, &err_msg, NULL, 0);
        fprintf(stderr, "SshConnection.connectPassword: auth failed for '%s@%s': %s\n",
                username, address, err_msg ? err_msg : "unknown error");
        sn_ssh_close(conn);
        exit(1);
    }

    return conn;
}

/* ============================================================================
 * Authentication: Public Key
 * ============================================================================ */

RtSshConnection *sn_ssh_connect_key(RtArena *arena, const char *address,
                                      const char *username, const char *privateKeyPath,
                                      const char *passphrase) {
    RtSshConnection *conn = ssh_connect_and_handshake(arena, address);
    LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;

    /* If passphrase is empty string, pass NULL to libssh2 */
    const char *pp = (passphrase && passphrase[0] != '\0') ? passphrase : NULL;

    /* Try with derived public key path first (privateKeyPath + ".pub") */
    char pub_key_path[PATH_MAX];
    snprintf(pub_key_path, sizeof(pub_key_path), "%s.pub", privateKeyPath);

    int rc = libssh2_userauth_publickey_fromfile(session, username,
                                                  pub_key_path, privateKeyPath, pp);
    if (rc) {
        /* Retry without explicit public key file (libssh2 can derive it) */
        rc = libssh2_userauth_publickey_fromfile(session, username,
                                                  NULL, privateKeyPath, pp);
    }

    if (rc) {
        char *err_msg = NULL;
        libssh2_session_last_error(session, &err_msg, NULL, 0);
        fprintf(stderr, "SshConnection.connectKey: auth failed for '%s@%s' with key '%s': %s\n",
                username, address, privateKeyPath, err_msg ? err_msg : "unknown error");
        sn_ssh_close(conn);
        exit(1);
    }

    return conn;
}

/* ============================================================================
 * Authentication: SSH Agent
 * ============================================================================ */

RtSshConnection *sn_ssh_connect_agent(RtArena *arena, const char *address,
                                        const char *username) {
    RtSshConnection *conn = ssh_connect_and_handshake(arena, address);
    LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;

    LIBSSH2_AGENT *agent = libssh2_agent_init(session);
    if (!agent) {
        fprintf(stderr, "SshConnection.connectAgent: agent init failed\n");
        sn_ssh_close(conn);
        exit(1);
    }

    if (libssh2_agent_connect(agent)) {
        fprintf(stderr, "SshConnection.connectAgent: unable to connect to SSH agent\n");
        libssh2_agent_free(agent);
        sn_ssh_close(conn);
        exit(1);
    }

    if (libssh2_agent_list_identities(agent)) {
        fprintf(stderr, "SshConnection.connectAgent: unable to list agent identities\n");
        libssh2_agent_disconnect(agent);
        libssh2_agent_free(agent);
        sn_ssh_close(conn);
        exit(1);
    }

    struct libssh2_agent_publickey *identity = NULL;
    struct libssh2_agent_publickey *prev = NULL;
    int auth_success = 0;

    while (1) {
        int rc = libssh2_agent_get_identity(agent, &identity, prev);
        if (rc == 1) break;  /* No more identities */
        if (rc < 0) break;   /* Error */

        if (libssh2_agent_userauth(agent, username, identity) == 0) {
            auth_success = 1;
            break;
        }
        prev = identity;
    }

    libssh2_agent_disconnect(agent);
    libssh2_agent_free(agent);

    if (!auth_success) {
        fprintf(stderr, "SshConnection.connectAgent: no valid identity for '%s@%s'\n",
                username, address);
        sn_ssh_close(conn);
        exit(1);
    }

    return conn;
}

/* ============================================================================
 * Authentication: Keyboard-Interactive
 * ============================================================================ */

/* Static password for keyboard-interactive callback (not thread-safe, but
 * Sindarin SDK modules are single-threaded for SSH operations) */
static const char *ki_password = NULL;

static void kbd_interactive_callback(const char *name, int name_len,
                                      const char *instruction, int instruction_len,
                                      int num_prompts,
                                      const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                                      LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                                      void **abstract) {
    (void)name; (void)name_len;
    (void)instruction; (void)instruction_len;
    (void)prompts; (void)abstract;

    for (int i = 0; i < num_prompts; i++) {
        const char *pwd = ki_password ? ki_password : "";
        size_t pwd_len = strlen(pwd);
        responses[i].text = (char *)malloc(pwd_len + 1);
        if (responses[i].text) {
            memcpy(responses[i].text, pwd, pwd_len + 1);
            responses[i].length = (unsigned int)pwd_len;
        } else {
            responses[i].text = NULL;
            responses[i].length = 0;
        }
    }
}

RtSshConnection *sn_ssh_connect_interactive(RtArena *arena, const char *address,
                                              const char *username, const char *password) {
    RtSshConnection *conn = ssh_connect_and_handshake(arena, address);
    LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;

    ki_password = password;
    int rc = libssh2_userauth_keyboard_interactive(session, username, kbd_interactive_callback);
    ki_password = NULL;

    if (rc) {
        char *err_msg = NULL;
        libssh2_session_last_error(session, &err_msg, NULL, 0);
        fprintf(stderr, "SshConnection.connectInteractive: auth failed for '%s@%s': %s\n",
                username, address, err_msg ? err_msg : "unknown error");
        sn_ssh_close(conn);
        exit(1);
    }

    return conn;
}

/* ============================================================================
 * Command Execution (Internal)
 * ============================================================================ */

static RtSshExecResult *ssh_exec_internal(RtArena *arena, RtSshConnection *conn,
                                            const char *command) {
    if (!conn || !conn->session_ptr) {
        fprintf(stderr, "SshConnection.exec: connection is closed\n");
        exit(1);
    }

    LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;

    /* Open a channel */
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        char *err_msg = NULL;
        libssh2_session_last_error(session, &err_msg, NULL, 0);
        fprintf(stderr, "SshConnection.exec: channel open failed: %s\n",
                err_msg ? err_msg : "unknown error");
        exit(1);
    }

    /* Execute the command */
    int rc = libssh2_channel_exec(channel, command);
    if (rc) {
        fprintf(stderr, "SshConnection.exec: exec failed for command '%s'\n", command);
        libssh2_channel_free(channel);
        exit(1);
    }

    /* Read stdout */
    size_t out_cap = 4096, out_len = 0;
    char *out_buf = (char *)malloc(out_cap);
    if (!out_buf) {
        fprintf(stderr, "SshConnection.exec: allocation failed\n");
        libssh2_channel_free(channel);
        exit(1);
    }

    /* Read stderr */
    size_t err_cap = 4096, err_len = 0;
    char *err_buf = (char *)malloc(err_cap);
    if (!err_buf) {
        fprintf(stderr, "SshConnection.exec: allocation failed\n");
        free(out_buf);
        libssh2_channel_free(channel);
        exit(1);
    }

    /* Read both stdout and stderr until EOF */
    while (1) {
        ssize_t n;

        /* Read stdout */
        if (out_cap - out_len > 1) {
            n = libssh2_channel_read(channel, out_buf + out_len, out_cap - out_len - 1);
            if (n > 0) {
                out_len += n;
                if (out_len >= out_cap - 1) {
                    out_cap *= 2;
                    char *new_buf = (char *)realloc(out_buf, out_cap);
                    if (!new_buf) {
                        fprintf(stderr, "SshConnection.exec: realloc failed\n");
                        free(out_buf);
                        free(err_buf);
                        libssh2_channel_free(channel);
                        exit(1);
                    }
                    out_buf = new_buf;
                }
            }
        }

        /* Read stderr */
        if (err_cap - err_len > 1) {
            n = libssh2_channel_read_stderr(channel, err_buf + err_len, err_cap - err_len - 1);
            if (n > 0) {
                err_len += n;
                if (err_len >= err_cap - 1) {
                    err_cap *= 2;
                    char *new_buf = (char *)realloc(err_buf, err_cap);
                    if (!new_buf) {
                        fprintf(stderr, "SshConnection.exec: realloc failed\n");
                        free(out_buf);
                        free(err_buf);
                        libssh2_channel_free(channel);
                        exit(1);
                    }
                    err_buf = new_buf;
                }
            }
        }

        /* Check if channel is at EOF */
        if (libssh2_channel_eof(channel)) break;
    }

    out_buf[out_len] = '\0';
    err_buf[err_len] = '\0';

    /* Close channel and get exit status */
    libssh2_channel_close(channel);
    libssh2_channel_wait_closed(channel);
    int exit_code = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);

    /* Allocate result in arena */
    RtSshExecResult *result = (RtSshExecResult *)rt_arena_alloc(arena, sizeof(RtSshExecResult));
    if (!result) {
        fprintf(stderr, "SshConnection.exec: result allocation failed\n");
        free(out_buf);
        free(err_buf);
        exit(1);
    }

    /* Copy stdout to arena */
    result->stdout_str = (char *)rt_arena_alloc(arena, out_len + 1);
    if (result->stdout_str) {
        memcpy(result->stdout_str, out_buf, out_len + 1);
    }

    /* Copy stderr to arena */
    result->stderr_str = (char *)rt_arena_alloc(arena, err_len + 1);
    if (result->stderr_str) {
        memcpy(result->stderr_str, err_buf, err_len + 1);
    }

    result->exit_code = exit_code;

    free(out_buf);
    free(err_buf);

    return result;
}

/* ============================================================================
 * Public API: Command Execution
 * ============================================================================ */

/* Execute command, return stdout only */
char *sn_ssh_run(RtArena *arena, RtSshConnection *conn, const char *command) {
    RtSshExecResult *result = ssh_exec_internal(arena, conn, command);
    return result->stdout_str;
}

/* Execute command, return full result struct */
RtSshExecResult *sn_ssh_exec(RtArena *arena, RtSshConnection *conn, const char *command) {
    return ssh_exec_internal(arena, conn, command);
}

/* ============================================================================
 * Getters
 * ============================================================================ */

char *sn_ssh_get_remote_address(RtArena *arena, RtSshConnection *conn) {
    if (conn == NULL || conn->remote_addr == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t len = strlen(conn->remote_addr) + 1;
    char *result = (char *)rt_arena_alloc(arena, len);
    if (result == NULL) {
        fprintf(stderr, "SshConnection.remoteAddress: allocation failed\n");
        exit(1);
    }
    memcpy(result, conn->remote_addr, len);
    return result;
}

char *sn_ssh_exec_result_get_stdout(RtArena *arena, RtSshExecResult *result) {
    if (result == NULL || result->stdout_str == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t len = strlen(result->stdout_str) + 1;
    char *copy = (char *)rt_arena_alloc(arena, len);
    if (copy == NULL) {
        fprintf(stderr, "SshExecResult.stdout: allocation failed\n");
        exit(1);
    }
    memcpy(copy, result->stdout_str, len);
    return copy;
}

char *sn_ssh_exec_result_get_stderr(RtArena *arena, RtSshExecResult *result) {
    if (result == NULL || result->stderr_str == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t len = strlen(result->stderr_str) + 1;
    char *copy = (char *)rt_arena_alloc(arena, len);
    if (copy == NULL) {
        fprintf(stderr, "SshExecResult.stderr: allocation failed\n");
        exit(1);
    }
    memcpy(copy, result->stderr_str, len);
    return copy;
}

long sn_ssh_exec_result_get_exit_code(RtSshExecResult *result) {
    if (result == NULL) return -1;
    return result->exit_code;
}

/* ============================================================================
 * Lifecycle: Close
 * ============================================================================ */

void sn_ssh_close(RtSshConnection *conn) {
    if (conn == NULL) return;

    if (conn->session_ptr != NULL) {
        LIBSSH2_SESSION *session = (LIBSSH2_SESSION *)conn->session_ptr;
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
        conn->session_ptr = NULL;
    }

    if (conn->socket_fd != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(conn->socket_fd);
        conn->socket_fd = INVALID_SOCKET_VAL;
    }
}
