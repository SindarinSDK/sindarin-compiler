/* ==============================================================================
 * sdk/net/tcp.sn.c - Self-contained TCP Implementation (TcpStream + TcpListener)
 * ==============================================================================
 * This file provides the C implementation for SnTcpStream and SnTcpListener.
 * It is compiled via #pragma source and linked with Sindarin code.
 * ============================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Include runtime for proper memory management */
#include "runtime/runtime_arena.h"
#include "runtime/runtime_array.h"

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
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>

    typedef int socket_t;
    #define INVALID_SOCKET_VAL (-1)
    #define SOCKET_ERROR_VAL (-1)
    #define CLOSE_SOCKET(s) close(s)
    #define GET_SOCKET_ERROR() errno
#endif

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

typedef struct RtTcpStream {
    socket_t socket_fd;     /* Socket file descriptor */
    char *remote_addr;      /* Remote address string (host:port) */
} RtTcpStream;

typedef struct RtTcpListener {
    socket_t socket_fd;     /* Listening socket file descriptor */
    int bound_port;         /* Port number we're bound to */
} RtTcpListener;

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
            fprintf(stderr, "WSAStartup failed: %d\n", result);
            exit(1);
        }
        winsock_initialized = 1;
    }
}
#else
#define ensure_winsock_initialized() ((void)0)
#endif

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static RtTcpStream *sn_tcp_stream_create(RtArena *arena, socket_t sock, const char *remote_addr) {
    RtTcpStream *stream = (RtTcpStream *)rt_arena_alloc(arena, sizeof(RtTcpStream));
    if (stream == NULL) {
        fprintf(stderr, "sn_tcp_stream_create: allocation failed\n");
        exit(1);
    }
    stream->socket_fd = sock;

    /* Copy remote address string */
    if (remote_addr) {
        size_t len = strlen(remote_addr) + 1;
        stream->remote_addr = (char *)rt_arena_alloc(arena, len);
        if (stream->remote_addr) {
            memcpy(stream->remote_addr, remote_addr, len);
        }
    } else {
        stream->remote_addr = NULL;
    }

    return stream;
}

/* Parse address string "host:port" into host and port components */
static int parse_address(const char *address, char *host, size_t host_len, int *port) {
    if (address == NULL) return 0;

    const char *last_colon = NULL;

    /* Handle IPv6 addresses like [::1]:8080 */
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
            return 0; /* No port specified */
        }
        return 1;
    }

    /* Find the last colon (for host:port format) */
    for (const char *p = address; *p; p++) {
        if (*p == ':') last_colon = p;
    }

    if (last_colon == NULL) return 0;

    size_t len = last_colon - address;
    if (len >= host_len) return 0;

    if (len == 0) {
        /* Empty host means all interfaces (0.0.0.0) */
        strcpy(host, "0.0.0.0");
    } else {
        memcpy(host, address, len);
        host[len] = '\0';
    }

    *port = atoi(last_colon + 1);
    return 1;
}

/* ============================================================================
 * TcpStream Creation
 * ============================================================================ */

RtTcpStream *sn_tcp_stream_connect(RtArena *arena, const char *address) {
    ensure_winsock_initialized();

    if (address == NULL) {
        fprintf(stderr, "sn_tcp_stream_connect: NULL address\n");
        exit(1);
    }

    char host[256];
    int port;

    if (!parse_address(address, host, sizeof(host), &port)) {
        fprintf(stderr, "sn_tcp_stream_connect: invalid address format '%s'\n", address);
        exit(1);
    }

    /* Resolve hostname */
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* TCP */

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(host, port_str, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "sn_tcp_stream_connect: DNS resolution failed for '%s': %s\n",
                host, gai_strerror(status));
        exit(1);
    }

    socket_t sock = INVALID_SOCKET_VAL;

    /* Try each address until we successfully connect */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCKET_VAL) continue;

        if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) != SOCKET_ERROR_VAL) {
            break; /* Success */
        }

        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET_VAL;
    }

    freeaddrinfo(result);

    if (sock == INVALID_SOCKET_VAL) {
        fprintf(stderr, "sn_tcp_stream_connect: connection failed to '%s'\n", address);
        exit(1);
    }

    return sn_tcp_stream_create(arena, sock, address);
}

/* ============================================================================
 * TcpStream Read Operations
 * ============================================================================ */

/* Read up to maxBytes (may return fewer) */
unsigned char *sn_tcp_stream_read(RtArena *arena, RtTcpStream *stream, long maxBytes) {
    if (stream == NULL || maxBytes <= 0) {
        /* Return empty byte array */
        return rt_array_create_byte(arena, 0, NULL);
    }

    /* Allocate temporary buffer for receiving */
    unsigned char *temp = (unsigned char *)malloc((size_t)maxBytes);
    if (temp == NULL) {
        fprintf(stderr, "sn_tcp_stream_read: malloc failed\n");
        exit(1);
    }

    /* Read from socket */
    int bytes_read = recv(stream->socket_fd, (char *)temp, (int)maxBytes, 0);

    if (bytes_read < 0) {
        free(temp);
        fprintf(stderr, "sn_tcp_stream_read: recv failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }

    /* Create runtime array with received data */
    unsigned char *result = rt_array_create_byte(arena, (size_t)bytes_read, temp);
    free(temp);

    return result;
}

/* Read until connection closes */
unsigned char *sn_tcp_stream_read_all(RtArena *arena, RtTcpStream *stream) {
    if (stream == NULL) {
        return rt_array_create_byte(arena, 0, NULL);
    }

    /* Use a growing buffer approach */
    size_t capacity = 4096;
    size_t total_read = 0;
    unsigned char *temp_buffer = (unsigned char *)malloc(capacity);

    if (temp_buffer == NULL) {
        fprintf(stderr, "sn_tcp_stream_read_all: malloc failed\n");
        exit(1);
    }

    while (1) {
        /* Ensure we have space */
        if (total_read + 1024 > capacity) {
            capacity *= 2;
            unsigned char *new_buffer = (unsigned char *)realloc(temp_buffer, capacity);
            if (new_buffer == NULL) {
                free(temp_buffer);
                fprintf(stderr, "sn_tcp_stream_read_all: realloc failed\n");
                exit(1);
            }
            temp_buffer = new_buffer;
        }

        int bytes_read = recv(stream->socket_fd, (char *)(temp_buffer + total_read), 1024, 0);

        if (bytes_read < 0) {
            free(temp_buffer);
            fprintf(stderr, "sn_tcp_stream_read_all: recv failed (%d)\n", GET_SOCKET_ERROR());
            exit(1);
        }

        if (bytes_read == 0) {
            break; /* Connection closed */
        }

        total_read += bytes_read;
    }

    /* Create runtime array with received data */
    unsigned char *result = rt_array_create_byte(arena, total_read, temp_buffer);
    free(temp_buffer);

    return result;
}

/* Read until newline */
char *sn_tcp_stream_read_line(RtArena *arena, RtTcpStream *stream) {
    if (stream == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t capacity = 256;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);

    if (buffer == NULL) {
        fprintf(stderr, "sn_tcp_stream_read_line: malloc failed\n");
        exit(1);
    }

    while (1) {
        char ch;
        int bytes_read = recv(stream->socket_fd, &ch, 1, 0);

        if (bytes_read < 0) {
            free(buffer);
            fprintf(stderr, "sn_tcp_stream_read_line: recv failed (%d)\n", GET_SOCKET_ERROR());
            exit(1);
        }

        if (bytes_read == 0) {
            break; /* Connection closed */
        }

        if (ch == '\n') {
            break; /* End of line */
        }

        /* Skip carriage return */
        if (ch == '\r') {
            continue;
        }

        /* Grow buffer if needed */
        if (length + 2 > capacity) {
            capacity *= 2;
            char *new_buffer = (char *)realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                fprintf(stderr, "sn_tcp_stream_read_line: realloc failed\n");
                exit(1);
            }
            buffer = new_buffer;
        }

        buffer[length++] = ch;
    }

    buffer[length] = '\0';

    /* Copy to arena */
    char *result = (char *)rt_arena_alloc(arena, length + 1);
    if (result == NULL) {
        free(buffer);
        fprintf(stderr, "sn_tcp_stream_read_line: arena alloc failed\n");
        exit(1);
    }

    memcpy(result, buffer, length + 1);
    free(buffer);

    return result;
}

/* ============================================================================
 * TcpStream Write Operations
 * ============================================================================ */

/* Write bytes, return count written */
long sn_tcp_stream_write(RtTcpStream *stream, unsigned char *data) {
    if (stream == NULL || data == NULL) return 0;

    size_t length = rt_array_length(data);
    if (length == 0) return 0;

    int bytes_sent = send(stream->socket_fd, (const char *)data, (int)length, 0);

    if (bytes_sent < 0) {
        fprintf(stderr, "sn_tcp_stream_write: send failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }

    return bytes_sent;
}

/* Write string + newline */
void sn_tcp_stream_write_line(RtTcpStream *stream, const char *text) {
    if (stream == NULL) return;

    if (text != NULL) {
        size_t len = strlen(text);
        if (len > 0) {
            int result = send(stream->socket_fd, text, (int)len, 0);
            if (result < 0) {
                fprintf(stderr, "sn_tcp_stream_write_line: send failed (%d)\n", GET_SOCKET_ERROR());
                exit(1);
            }
        }
    }

    /* Send newline */
    int result = send(stream->socket_fd, "\r\n", 2, 0);
    if (result < 0) {
        fprintf(stderr, "sn_tcp_stream_write_line: send newline failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }
}

/* ============================================================================
 * TcpStream Getters
 * ============================================================================ */

char *sn_tcp_stream_get_remote_address(RtArena *arena, RtTcpStream *stream) {
    if (stream == NULL || stream->remote_addr == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t len = strlen(stream->remote_addr) + 1;
    char *result = (char *)rt_arena_alloc(arena, len);
    if (result == NULL) {
        fprintf(stderr, "sn_tcp_stream_get_remote_address: allocation failed\n");
        exit(1);
    }

    memcpy(result, stream->remote_addr, len);
    return result;
}

/* ============================================================================
 * TcpStream Lifecycle
 * ============================================================================ */

void sn_tcp_stream_close(RtTcpStream *stream) {
    if (stream == NULL) return;

    if (stream->socket_fd != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(stream->socket_fd);
        stream->socket_fd = INVALID_SOCKET_VAL;
    }
}

/* ============================================================================
 * TcpListener Creation
 * ============================================================================ */

static RtTcpListener *sn_tcp_listener_create(RtArena *arena, socket_t sock, int port) {
    RtTcpListener *listener = (RtTcpListener *)rt_arena_alloc(arena, sizeof(RtTcpListener));
    if (listener == NULL) {
        fprintf(stderr, "sn_tcp_listener_create: allocation failed\n");
        exit(1);
    }
    listener->socket_fd = sock;
    listener->bound_port = port;
    return listener;
}

RtTcpListener *sn_tcp_listener_bind(RtArena *arena, const char *address) {
    ensure_winsock_initialized();

    if (address == NULL) {
        fprintf(stderr, "sn_tcp_listener_bind: NULL address\n");
        exit(1);
    }

    char host[256];
    int port;

    if (!parse_address(address, host, sizeof(host), &port)) {
        fprintf(stderr, "sn_tcp_listener_bind: invalid address format '%s'\n", address);
        exit(1);
    }

    /* Create socket */
    socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET_VAL) {
        fprintf(stderr, "sn_tcp_listener_bind: socket creation failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }

    /* Allow address reuse */
    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    /* Bind to address */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (strcmp(host, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            /* Try to resolve hostname */
            struct addrinfo hints, *result;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if (getaddrinfo(host, NULL, &hints, &result) != 0) {
                CLOSE_SOCKET(sock);
                fprintf(stderr, "sn_tcp_listener_bind: invalid host '%s'\n", host);
                exit(1);
            }

            struct sockaddr_in *resolved = (struct sockaddr_in *)result->ai_addr;
            addr.sin_addr = resolved->sin_addr;
            freeaddrinfo(result);
        }
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
        CLOSE_SOCKET(sock);
        fprintf(stderr, "sn_tcp_listener_bind: bind failed on '%s' (%d)\n", address, GET_SOCKET_ERROR());
        exit(1);
    }

    /* Get the actual port (in case port was 0) */
    struct sockaddr_in bound_addr;
    socklen_t addr_len = sizeof(bound_addr);
    if (getsockname(sock, (struct sockaddr *)&bound_addr, &addr_len) == SOCKET_ERROR_VAL) {
        CLOSE_SOCKET(sock);
        fprintf(stderr, "sn_tcp_listener_bind: getsockname failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }
    int actual_port = ntohs(bound_addr.sin_port);

    /* Listen for connections */
    if (listen(sock, SOMAXCONN) == SOCKET_ERROR_VAL) {
        CLOSE_SOCKET(sock);
        fprintf(stderr, "sn_tcp_listener_bind: listen failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }

    return sn_tcp_listener_create(arena, sock, actual_port);
}

/* ============================================================================
 * TcpListener Accept
 * ============================================================================ */

RtTcpStream *sn_tcp_listener_accept(RtArena *arena, RtTcpListener *listener) {
    if (listener == NULL) {
        fprintf(stderr, "sn_tcp_listener_accept: NULL listener\n");
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    socket_t client_sock = accept(listener->socket_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_sock == INVALID_SOCKET_VAL) {
        fprintf(stderr, "sn_tcp_listener_accept: accept failed (%d)\n", GET_SOCKET_ERROR());
        exit(1);
    }

    /* Format remote address as "ip:port" */
    char remote_addr[64];
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    snprintf(remote_addr, sizeof(remote_addr), "%s:%d", ip_str, ntohs(client_addr.sin_port));

    return sn_tcp_stream_create(arena, client_sock, remote_addr);
}

/* ============================================================================
 * TcpListener Getters
 * ============================================================================ */

long sn_tcp_listener_get_port(RtTcpListener *listener) {
    if (listener == NULL) return 0;
    return listener->bound_port;
}

/* ============================================================================
 * TcpListener Lifecycle
 * ============================================================================ */

void sn_tcp_listener_close(RtTcpListener *listener) {
    if (listener == NULL) return;

    if (listener->socket_fd != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(listener->socket_fd);
        listener->socket_fd = INVALID_SOCKET_VAL;
    }
}