/* ==============================================================================
 * sdk/net/dtls.sn.c - Self-contained DTLS Connection Implementation
 * ==============================================================================
 * This file provides the C implementation for DtlsConnection using OpenSSL.
 * DTLS (Datagram TLS) provides TLS security over UDP datagrams.
 * It is compiled via @source and linked with Sindarin code.
 *
 * Certificate loading priority:
 *   1. SN_CERTS environment variable (path to PEM file or directory)
 *   2. Platform-native certificate store
 * ============================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Include runtime for proper memory management */
#include "runtime/runtime_arena.h"
#include "runtime/runtime_array.h"

/* OpenSSL includes */
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

/* Platform-specific socket includes */
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <wincrypt.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "crypt32.lib")

    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define CLOSE_SOCKET(s) closesocket(s)
    #define GET_SOCKET_ERROR() WSAGetLastError()
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <Security/Security.h>

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

typedef struct RtDtlsConnection {
    socket_t socket_fd;         /* Underlying UDP socket (connected) */
    void *ssl_ptr;              /* SSL* - opaque to Sindarin */
    char *remote_addr;          /* Remote address string (host:port) */

    /* SSL context - owned per connection */
    SSL_CTX *ctx;
} RtDtlsConnection;

/* ============================================================================
 * OpenSSL Initialization (one-time)
 * ============================================================================ */

static int openssl_initialized = 0;

static void ensure_openssl_initialized(void) {
    if (!openssl_initialized) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
#else
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
#endif
        openssl_initialized = 1;
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
            fprintf(stderr, "DtlsConnection: WSAStartup failed: %d\n", result);
            exit(1);
        }
        winsock_initialized = 1;
    }
}
#else
#define ensure_winsock_initialized() ((void)0)
#endif

/* ============================================================================
 * Certificate Loading
 * ============================================================================ */

#ifdef _WIN32
static int dtls_load_windows_certs(SSL_CTX *ctx) {
    HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
    if (hStore == NULL) {
        return 0;
    }

    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    PCCERT_CONTEXT pCert = NULL;
    int count = 0;

    while ((pCert = CertEnumCertificatesInStore(hStore, pCert)) != NULL) {
        const unsigned char *cert_data = pCert->pbCertEncoded;
        X509 *x509 = d2i_X509(NULL, &cert_data, pCert->cbCertEncoded);
        if (x509 != NULL) {
            if (X509_STORE_add_cert(store, x509) == 1) {
                count++;
            }
            X509_free(x509);
        }
    }

    CertCloseStore(hStore, 0);
    return count;
}
#endif

#ifdef __APPLE__
static int dtls_load_macos_certs(SSL_CTX *ctx) {
    CFArrayRef certs = NULL;
    OSStatus status = SecTrustCopyAnchorCertificates(&certs);
    if (status != errSecSuccess || certs == NULL) {
        return 0;
    }

    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    CFIndex cert_count = CFArrayGetCount(certs);
    int loaded = 0;

    for (CFIndex i = 0; i < cert_count; i++) {
        SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex(certs, i);
        CFDataRef der_data = SecCertificateCopyData(cert);
        if (der_data == NULL) continue;

        const unsigned char *ptr = CFDataGetBytePtr(der_data);
        CFIndex length = CFDataGetLength(der_data);

        X509 *x509 = d2i_X509(NULL, &ptr, (long)length);
        if (x509 != NULL) {
            if (X509_STORE_add_cert(store, x509) == 1) {
                loaded++;
            }
            X509_free(x509);
        }

        CFRelease(der_data);
    }

    CFRelease(certs);
    return loaded;
}
#endif

static void dtls_load_certificates(SSL_CTX *ctx) {
    /* Priority 1: SN_CERTS environment variable */
    const char *sn_certs = getenv("SN_CERTS");
    if (sn_certs != NULL && sn_certs[0] != '\0') {
        if (SSL_CTX_load_verify_locations(ctx, sn_certs, NULL) == 1) {
            return;
        }
        if (SSL_CTX_load_verify_locations(ctx, NULL, sn_certs) == 1) {
            return;
        }
        fprintf(stderr, "DtlsConnection: warning: SN_CERTS='%s' could not be loaded, "
                "falling back to system certs\n", sn_certs);
    }

    /* Priority 2: Platform-native certificate store */
#ifdef _WIN32
    int count = dtls_load_windows_certs(ctx);
    if (count == 0) {
        fprintf(stderr, "DtlsConnection: warning: no certificates loaded from Windows store\n");
    }
#elif defined(__APPLE__)
    int count = dtls_load_macos_certs(ctx);
    if (count == 0) {
        SSL_CTX_set_default_verify_paths(ctx);
    }
#else
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        fprintf(stderr, "DtlsConnection: warning: failed to load default certificate paths\n");
    }
#endif
}

/* ============================================================================
 * Address Parsing
 * ============================================================================ */

static int dtls_parse_address(const char *address, char *host, size_t host_len, int *port) {
    if (address == NULL) return 0;

    const char *last_colon = NULL;

    /* Handle IPv6 addresses like [::1]:4433 */
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
            *port = 4433; /* Default DTLS port */
        }
        return 1;
    }

    /* Find the last colon (for host:port format) */
    for (const char *p = address; *p; p++) {
        if (*p == ':') last_colon = p;
    }

    if (last_colon == NULL) {
        size_t len = strlen(address);
        if (len >= host_len) return 0;
        strcpy(host, address);
        *port = 4433;
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
 * DtlsConnection Connect
 * ============================================================================ */

RtDtlsConnection *sn_dtls_connection_connect(RtArena *arena, const char *address) {
    ensure_winsock_initialized();
    ensure_openssl_initialized();

    if (address == NULL) {
        fprintf(stderr, "DtlsConnection.connect: NULL address\n");
        exit(1);
    }

    char host[256];
    int port;

    if (!dtls_parse_address(address, host, sizeof(host), &port)) {
        fprintf(stderr, "DtlsConnection.connect: invalid address format '%s'\n", address);
        exit(1);
    }

    /* --- UDP Connection --- */

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;  /* UDP */

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(host, port_str, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "DtlsConnection.connect: DNS resolution failed for '%s': %s\n",
                host, gai_strerror(status));
        exit(1);
    }

    socket_t sock = INVALID_SOCKET_VAL;

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCKET_VAL) continue;

        /* Connect the UDP socket to the remote address.
         * This allows us to use send/recv instead of sendto/recvfrom,
         * and lets OpenSSL manage the DTLS connection via SSL_set_fd. */
        if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) != SOCKET_ERROR_VAL) {
            break;
        }

        CLOSE_SOCKET(sock);
        sock = INVALID_SOCKET_VAL;
    }

    freeaddrinfo(result);

    if (sock == INVALID_SOCKET_VAL) {
        fprintf(stderr, "DtlsConnection.connect: UDP socket creation failed for '%s'\n", address);
        exit(1);
    }

    /* --- DTLS Handshake --- */

    SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
    if (ctx == NULL) {
        CLOSE_SOCKET(sock);
        fprintf(stderr, "DtlsConnection.connect: SSL_CTX_new failed\n");
        exit(1);
    }

    /* Load certificates */
    dtls_load_certificates(ctx);

    /* Enable certificate verification */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) {
        SSL_CTX_free(ctx);
        CLOSE_SOCKET(sock);
        fprintf(stderr, "DtlsConnection.connect: SSL_new failed\n");
        exit(1);
    }

    /* Set SNI hostname */
    SSL_set_tlsext_host_name(ssl, host);

    /* Enable hostname verification */
    SSL_set1_host(ssl, host);

    /* Create BIO from connected socket for DTLS */
    BIO *bio = BIO_new_dgram((int)sock, BIO_NOCLOSE);
    if (bio == NULL) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CLOSE_SOCKET(sock);
        fprintf(stderr, "DtlsConnection.connect: BIO_new_dgram failed\n");
        exit(1);
    }

    /* Set the connected peer address on the BIO */
    {
        struct addrinfo *peer_result;
        struct addrinfo peer_hints;
        memset(&peer_hints, 0, sizeof(peer_hints));
        peer_hints.ai_family = AF_UNSPEC;
        peer_hints.ai_socktype = SOCK_DGRAM;

        if (getaddrinfo(host, port_str, &peer_hints, &peer_result) == 0) {
            BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, peer_result->ai_addr);
            freeaddrinfo(peer_result);
        }
    }

    SSL_set_bio(ssl, bio, bio);

    /* Set DTLS timeout for retransmission */
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_RECV_TIMEOUT, 0, &timeout);
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_SEND_TIMEOUT, 0, &timeout);

    /* Perform DTLS handshake */
    int ssl_result = SSL_connect(ssl);
    if (ssl_result != 1) {
        int ssl_err = SSL_get_error(ssl, ssl_result);
        unsigned long err_code = ERR_get_error();
        char err_buf[256];
        ERR_error_string_n(err_code, err_buf, sizeof(err_buf));

        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CLOSE_SOCKET(sock);

        if (ssl_err == SSL_ERROR_SSL) {
            fprintf(stderr, "DtlsConnection.connect: DTLS handshake failed for '%s': %s\n",
                    address, err_buf);
        } else {
            fprintf(stderr, "DtlsConnection.connect: DTLS handshake failed for '%s' (error %d)\n",
                    address, ssl_err);
        }
        exit(1);
    }

    /* Verify certificate was validated */
    long verify_result = SSL_get_verify_result(ssl);
    if (verify_result != X509_V_OK) {
        const char *verify_str = X509_verify_cert_error_string(verify_result);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        CLOSE_SOCKET(sock);
        fprintf(stderr, "DtlsConnection.connect: certificate verification failed for '%s': %s\n",
                address, verify_str);
        exit(1);
    }

    /* Allocate and return connection */
    RtDtlsConnection *conn = (RtDtlsConnection *)rt_arena_alloc(arena, sizeof(RtDtlsConnection));
    if (conn == NULL) {
        fprintf(stderr, "DtlsConnection.connect: allocation failed\n");
        exit(1);
    }

    conn->socket_fd = sock;
    conn->ssl_ptr = ssl;
    conn->ctx = ctx;

    /* Copy remote address string */
    size_t addr_len = strlen(address) + 1;
    conn->remote_addr = (char *)rt_arena_alloc(arena, addr_len);
    if (conn->remote_addr) {
        memcpy(conn->remote_addr, address, addr_len);
    }

    return conn;
}

/* ============================================================================
 * DtlsConnection Send/Receive
 * ============================================================================ */

/* Send an encrypted datagram, return bytes sent */
long sn_dtls_connection_send(RtDtlsConnection *conn, unsigned char *data) {
    if (conn == NULL || data == NULL) return 0;

    size_t length = rt_array_length(data);
    if (length == 0) return 0;

    SSL *ssl = (SSL *)conn->ssl_ptr;
    int bytes_sent = SSL_write(ssl, data, (int)length);

    if (bytes_sent <= 0) {
        int ssl_err = SSL_get_error(ssl, bytes_sent);
        fprintf(stderr, "DtlsConnection.send: SSL_write failed (error %d)\n", ssl_err);
        exit(1);
    }

    return bytes_sent;
}

/* Receive an encrypted datagram (up to maxBytes) */
unsigned char *sn_dtls_connection_receive(RtArena *arena, RtDtlsConnection *conn, long maxBytes) {
    if (conn == NULL || maxBytes <= 0) {
        return rt_array_create_byte(arena, 0, NULL);
    }

    SSL *ssl = (SSL *)conn->ssl_ptr;

    /* Allocate a temporary buffer for receiving */
    unsigned char *temp = (unsigned char *)malloc((size_t)maxBytes);
    if (temp == NULL) {
        fprintf(stderr, "DtlsConnection.receive: malloc failed\n");
        exit(1);
    }

    int n = SSL_read(ssl, temp, (int)maxBytes);

    if (n <= 0) {
        int ssl_err = SSL_get_error(ssl, n);
        free(temp);

        if (ssl_err == SSL_ERROR_ZERO_RETURN) {
            /* Connection closed cleanly */
            return rt_array_create_byte(arena, 0, NULL);
        } else if (ssl_err == SSL_ERROR_WANT_READ) {
            /* Timeout - return empty */
            return rt_array_create_byte(arena, 0, NULL);
        } else {
            fprintf(stderr, "DtlsConnection.receive: SSL_read failed (error %d)\n", ssl_err);
            exit(1);
        }
    }

    /* Create runtime byte array with received data */
    unsigned char *result = rt_array_create_byte(arena, (size_t)n, temp);
    free(temp);

    return result;
}

/* ============================================================================
 * DtlsConnection Getters
 * ============================================================================ */

char *sn_dtls_connection_get_remote_address(RtArena *arena, RtDtlsConnection *conn) {
    if (conn == NULL || conn->remote_addr == NULL) {
        char *empty = (char *)rt_arena_alloc(arena, 1);
        if (empty) empty[0] = '\0';
        return empty;
    }

    size_t len = strlen(conn->remote_addr) + 1;
    char *result = (char *)rt_arena_alloc(arena, len);
    if (result == NULL) {
        fprintf(stderr, "DtlsConnection.remoteAddress: allocation failed\n");
        exit(1);
    }

    memcpy(result, conn->remote_addr, len);
    return result;
}

/* ============================================================================
 * DtlsConnection Lifecycle
 * ============================================================================ */

void sn_dtls_connection_close(RtDtlsConnection *conn) {
    if (conn == NULL) return;

    if (conn->ssl_ptr != NULL) {
        SSL *ssl = (SSL *)conn->ssl_ptr;
        SSL_shutdown(ssl);
        SSL_free(ssl);  /* Also frees the BIO */
        conn->ssl_ptr = NULL;
    }

    if (conn->ctx != NULL) {
        SSL_CTX_free(conn->ctx);
        conn->ctx = NULL;
    }

    if (conn->socket_fd != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(conn->socket_fd);
        conn->socket_fd = INVALID_SOCKET_VAL;
    }
}
