// src/package/package_lfs_buffer.c
// CURL Response Buffer

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

#endif /* SN_HAS_CURL */
