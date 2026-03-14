#include "sn_byte.h"
#include "sn_core.h"
#include "sn_array.h"

char *sn_byte_array_to_hex(const SnArray *arr)
{
    static const char hex_chars[] = "0123456789abcdef";
    if (!arr || arr->len == 0) return strdup("");
    size_t len = (size_t)arr->len * 2;
    char *result = sn_malloc(len + 1);
    const unsigned char *data = (const unsigned char *)arr->data;
    for (long long i = 0; i < arr->len; i++) {
        result[i * 2]     = hex_chars[data[i] >> 4];
        result[i * 2 + 1] = hex_chars[data[i] & 0x0F];
    }
    result[len] = '\0';
    return result;
}

char *sn_byte_array_to_base64(const SnArray *arr)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (!arr || arr->len == 0) return strdup("");
    const unsigned char *data = (const unsigned char *)arr->data;
    long long len = arr->len;
    size_t out_len = 4 * ((size_t)(len + 2) / 3);
    char *result = sn_malloc(out_len + 1);
    size_t j = 0;
    for (long long i = 0; i < len; i += 3) {
        unsigned int a = data[i];
        unsigned int b = (i + 1 < len) ? data[i + 1] : 0;
        unsigned int c = (i + 2 < len) ? data[i + 2] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;
        result[j++] = b64[(triple >> 18) & 0x3F];
        result[j++] = b64[(triple >> 12) & 0x3F];
        result[j++] = (i + 1 < len) ? b64[(triple >> 6) & 0x3F] : '=';
        result[j++] = (i + 2 < len) ? b64[triple & 0x3F] : '=';
    }
    result[j] = '\0';
    return result;
}
