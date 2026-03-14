#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ---- MockStream ---- */

/* NOTE: We do NOT define our own typedef here.
 * The compiler generates __sn__MockStream from the .sn struct declaration.
 * After template Fix 1 (c_alias for native struct fields), the generated
 * struct will use c_alias field names: remote_addr, buffer, capacity, etc.
 *
 * typedef struct {
 *     char *remote_addr;
 *     unsigned char *buffer;
 *     long long capacity;
 *     long long length;
 *     int32_t is_open;
 * } __sn__MockStream;
 */

/* Accept uses a typedef alias for readability */
typedef __sn__MockStream MockStream;
typedef __sn__MockListener MockListener;

#define MOCK_BUF_SIZE 4096

MockStream *mock_stream_create(const char *remote_addr) {
    MockStream *s = calloc(1, sizeof(MockStream));
    if (!s) { fprintf(stderr, "MockStream: alloc failed\n"); exit(1); }
    s->remote_addr = remote_addr ? strdup(remote_addr) : NULL;
    s->buffer = (unsigned char *)calloc(1, MOCK_BUF_SIZE);
    s->capacity = MOCK_BUF_SIZE;
    s->length = 0;
    s->is_open = 1;
    return s;
}

void mock_stream_write(MockStream *stream, const char *data) {
    if (!stream || !stream->is_open) {
        fprintf(stderr, "MockStream.write: stream not open\n"); exit(1);
    }
    if (!data) return;
    size_t len = strlen(data);
    if ((size_t)stream->length + len > (size_t)stream->capacity) {
        size_t new_cap = (size_t)stream->capacity * 2;
        while (new_cap < (size_t)stream->length + len) new_cap *= 2;
        stream->buffer = realloc(stream->buffer, new_cap);
        stream->capacity = (long long)new_cap;
    }
    memcpy(stream->buffer + stream->length, data, len);
    stream->length += (long long)len;
}

char *mock_stream_read(MockStream *stream) {
    if (!stream || !stream->is_open) {
        fprintf(stderr, "MockStream.read: stream not open\n"); exit(1);
    }
    char *result = calloc(1, (size_t)stream->length + 1);
    if (stream->length > 0) {
        memcpy(result, stream->buffer, (size_t)stream->length);
    }
    result[stream->length] = '\0';
    stream->length = 0;  /* consume the data */
    return result;
}

char *mock_stream_get_remote_addr(MockStream *stream) {
    if (!stream) return strdup("");
    return stream->remote_addr ? strdup(stream->remote_addr) : strdup("");
}

void mock_stream_close(MockStream *stream) {
    if (!stream) return;
    if (stream->is_open) {
        free(stream->buffer);
        free(stream->remote_addr);
        stream->buffer = NULL;
        stream->remote_addr = NULL;
        stream->is_open = 0;
    }
}

/* ---- MockListener ---- */

/* Uses the compiler-generated __sn__MockListener typedef:
 * typedef struct {
 *     char *address;
 *     long long port;
 *     int32_t is_bound;
 * } __sn__MockListener;
 */

static int mock_port_counter = 8080;

MockListener *mock_listener_bind(const char *address) {
    MockListener *l = calloc(1, sizeof(MockListener));
    if (!l) { fprintf(stderr, "MockListener: alloc failed\n"); exit(1); }
    l->address = address ? strdup(address) : strdup("0.0.0.0:0");
    l->port = mock_port_counter++;
    l->is_bound = 1;
    return l;
}

MockStream *mock_listener_accept(MockListener *listener) {
    if (!listener || !listener->is_bound) {
        fprintf(stderr, "MockListener.accept: not bound\n"); exit(1);
    }
    /* Simulate accepting a connection */
    char addr_buf[64];
    snprintf(addr_buf, sizeof(addr_buf), "192.168.1.%d:%d",
             (int)(listener->port % 255), (int)(listener->port + 1000));
    return mock_stream_create(addr_buf);
}

long long mock_listener_get_port(MockListener *listener) {
    if (!listener) return 0;
    return listener->port;
}

void mock_listener_close(MockListener *listener) {
    if (!listener) return;
    if (listener->is_bound) {
        free(listener->address);
        listener->address = NULL;
        listener->is_bound = 0;
    }
}
