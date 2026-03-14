#ifndef SN_BYTE_H
#define SN_BYTE_H

#include "sn_core.h"
#include "sn_array.h"

/* ---- str.toBytes() — convert string to byte array (O(n) memcpy) ---- */

static inline SnArray *sn_string_to_bytes(const char *s) {
    size_t len = s ? strlen(s) : 0;
    SnArray *arr = sn_array_new(sizeof(unsigned char), (long long)len);
    arr->elem_tag = SN_TAG_BYTE;
    if (len > 0) {
        memcpy(arr->data, s, len);
        arr->len = (long long)len;
    }
    return arr;
}
#define __sn___toBytes(str_ptr) sn_string_to_bytes(*(str_ptr))

/* ---- Byte array encoding (declared, defined in sn_byte.c) ---- */

char *sn_byte_array_to_hex(const SnArray *arr);
char *sn_byte_array_to_base64(const SnArray *arr);

#define __sn__arr_toHex(arr_ptr) sn_byte_array_to_hex(*(arr_ptr))
#define __sn__arr_toBase64(arr_ptr) sn_byte_array_to_base64(*(arr_ptr))

/* ---- byte[].toStringLatin1() — Latin-1 to UTF-8 conversion ---- */

static inline char *sn_byte_array_to_string_latin1(const SnArray *arr) {
    if (!arr || arr->len == 0) return strdup("");
    /* Worst case: every byte is >127 and needs 2 UTF-8 bytes */
    char *s = sn_malloc((size_t)arr->len * 2 + 1);
    size_t out = 0;
    const unsigned char *data = (const unsigned char *)arr->data;
    for (size_t i = 0; i < (size_t)arr->len; i++) {
        unsigned char b = data[i];
        if (b < 128) {
            s[out++] = (char)b;
        } else {
            s[out++] = (char)(0xC0 | (b >> 6));
            s[out++] = (char)(0x80 | (b & 0x3F));
        }
    }
    s[out] = '\0';
    return s;
}
#define __sn__arr_toStringLatin1(arr_ptr) sn_byte_array_to_string_latin1(*(arr_ptr))

#endif
