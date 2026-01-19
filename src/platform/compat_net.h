/*
 * Network compatibility layer for Windows
 * Provides BSD socket API compatibility using Winsock2
 */

#ifndef SN_COMPAT_NET_H
#define SN_COMPAT_NET_H

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>

/* Link with Winsock library */
#pragma comment(lib, "ws2_32.lib")

/* ============================================================================
 * Type mappings
 * ============================================================================ */

/* Socket type is SOCKET on Windows, int on POSIX */
/* We'll use SOCKET directly in Windows code */

/* ssize_t is defined in compat_windows.h (via platform.h) */

/* ============================================================================
 * Constants
 * ============================================================================ */

/* Error code mapping - only define if not already defined by errno.h */
#ifndef EWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
    #define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef ECONNREFUSED
    #define ECONNREFUSED WSAECONNREFUSED
#endif
#ifndef ENOTCONN
    #define ENOTCONN WSAENOTCONN
#endif
#ifndef ETIMEDOUT
    #define ETIMEDOUT WSAETIMEDOUT
#endif

/* Shutdown modes */
#ifndef SHUT_RD
    #define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
    #define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
    #define SHUT_RDWR SD_BOTH
#endif

/* ============================================================================
 * Winsock initialization
 * ============================================================================ */

/* Initialize Winsock (must be called before any socket operations) */
static inline int sn_net_init(void) {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    return (result == 0) ? 0 : -1;
}

/* Cleanup Winsock (call at program exit) */
static inline void sn_net_cleanup(void) {
    WSACleanup();
}

/* ============================================================================
 * Socket operations compatibility
 * ============================================================================ */

/* close() for sockets - Windows uses closesocket() */
static inline int close_socket(SOCKET sock) {
    return closesocket(sock);
}

/* Get last socket error */
static inline int sn_socket_errno(void) {
    return WSAGetLastError();
}

/* Set socket to non-blocking mode */
static inline int sn_set_nonblocking(SOCKET sock, int nonblocking) {
    u_long mode = nonblocking ? 1 : 0;
    return ioctlsocket(sock, FIONBIO, &mode);
}

/* ============================================================================
 * Address conversion compatibility
 * ============================================================================ */

/* inet_pton is available on Windows Vista+ */
/* inet_ntop is available on Windows Vista+ */

/* For older Windows compatibility, you might need: */
#if 0
static inline int inet_pton_compat(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN + 1];

    strncpy(src_copy, src, INET6_ADDRSTRLEN);
    src_copy[INET6_ADDRSTRLEN] = '\0';

    if (WSAStringToAddressA(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch (af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}
#endif

/* ============================================================================
 * Poll compatibility (Windows uses WSAPoll or select)
 * ============================================================================ */

/* Note: WSAPoll is available on Windows Vista+ */
/* For compatibility, you might need to use select() instead */

#ifndef POLLIN
    #define POLLIN   0x0001
    #define POLLOUT  0x0004
    #define POLLERR  0x0008
    #define POLLHUP  0x0010
    #define POLLNVAL 0x0020
#endif

/* pollfd structure - use WSAPOLLFD on Windows */
#ifndef pollfd
    #define pollfd WSAPOLLFD
#endif

/* poll function - use WSAPoll on Windows */
static inline int poll(struct pollfd *fds, unsigned long nfds, int timeout) {
    return WSAPoll(fds, nfds, timeout);
}

/* ============================================================================
 * getaddrinfo_a (async DNS) - not available on Windows
 * Use getaddrinfo (synchronous) instead
 * ============================================================================ */

/* ============================================================================
 * Socket option compatibility
 * ============================================================================ */

/* TCP_NODELAY is the same on both platforms */

/* SO_REUSEADDR behavior differs slightly but works similarly */

/* SO_KEEPALIVE is the same on both platforms */

/* ============================================================================
 * Signal handling for sockets
 * ============================================================================ */

/* SIGPIPE doesn't exist on Windows - send() returns error instead */
/* No need to ignore SIGPIPE on Windows */

#endif /* _WIN32 */

#endif /* SN_COMPAT_NET_H */
