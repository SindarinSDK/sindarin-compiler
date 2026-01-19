#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_byte.h"
#include "runtime_array.h"

/* ============================================================================
 * Byte Array Conversion Functions
 * ============================================================================
 * Implementation of byte array to string and string to byte array conversions.
 * ============================================================================ */

/* ============================================================================
 * Byte Array to String Conversions
 * ============================================================================ */

/* Base64 encoding table */
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Convert byte array to string using UTF-8 decoding
 * Note: This is a simple passthrough since our strings are already UTF-8.
 * Invalid UTF-8 sequences are passed through as-is. */
char *rt_byte_array_to_string(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);
    char *result = rt_arena_alloc(arena, len + 1);

    for (size_t i = 0; i < len; i++) {
        result[i] = (char)bytes[i];
    }
    result[len] = '\0';

    return result;
}

/* Convert byte array to string using Latin-1/ISO-8859-1 decoding
 * Each byte directly maps to its Unicode code point (0x00-0xFF).
 * This requires UTF-8 encoding for values 0x80-0xFF. */
char *rt_byte_array_to_string_latin1(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);

    /* Calculate output size: bytes 0x00-0x7F = 1 byte, 0x80-0xFF = 2 bytes in UTF-8 */
    size_t out_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] < 0x80) {
            out_len += 1;
        } else {
            out_len += 2;  /* UTF-8 encoding for 0x80-0xFF */
        }
    }

    char *result = rt_arena_alloc(arena, out_len + 1);
    size_t out_idx = 0;

    for (size_t i = 0; i < len; i++) {
        if (bytes[i] < 0x80) {
            result[out_idx++] = (char)bytes[i];
        } else {
            /* UTF-8 encoding for code points 0x80-0xFF: 110xxxxx 10xxxxxx */
            result[out_idx++] = (char)(0xC0 | (bytes[i] >> 6));
            result[out_idx++] = (char)(0x80 | (bytes[i] & 0x3F));
        }
    }
    result[out_idx] = '\0';

    return result;
}

/* Convert byte array to hexadecimal string */
char *rt_byte_array_to_hex(RtArena *arena, unsigned char *bytes) {
    static const char hex_chars[] = "0123456789abcdef";

    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);
    char *result = rt_arena_alloc(arena, len * 2 + 1);

    for (size_t i = 0; i < len; i++) {
        result[i * 2] = hex_chars[(bytes[i] >> 4) & 0xF];
        result[i * 2 + 1] = hex_chars[bytes[i] & 0xF];
    }
    result[len * 2] = '\0';

    return result;
}

/* Convert byte array to Base64 string */
char *rt_byte_array_to_base64(RtArena *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = rt_array_length(bytes);

    /* Calculate output size: 4 output chars for every 3 input bytes, rounded up */
    size_t out_len = ((len + 2) / 3) * 4;
    char *result = rt_arena_alloc(arena, out_len + 1);

    size_t i = 0;
    size_t out_idx = 0;

    while (i + 2 < len) {
        /* Process 3 bytes at a time */
        unsigned int val = ((unsigned int)bytes[i] << 16) |
                          ((unsigned int)bytes[i + 1] << 8) |
                          ((unsigned int)bytes[i + 2]);

        result[out_idx++] = base64_chars[(val >> 18) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 12) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 6) & 0x3F];
        result[out_idx++] = base64_chars[val & 0x3F];

        i += 3;
    }

    /* Handle remaining bytes */
    if (i < len) {
        unsigned int val = (unsigned int)bytes[i] << 16;
        if (i + 1 < len) {
            val |= (unsigned int)bytes[i + 1] << 8;
        }

        result[out_idx++] = base64_chars[(val >> 18) & 0x3F];
        result[out_idx++] = base64_chars[(val >> 12) & 0x3F];

        if (i + 1 < len) {
            result[out_idx++] = base64_chars[(val >> 6) & 0x3F];
        } else {
            result[out_idx++] = '=';
        }
        result[out_idx++] = '=';
    }

    result[out_idx] = '\0';

    return result;
}

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================ */

/* Convert string to UTF-8 byte array
 * Since our strings are already UTF-8, this is a simple copy. */
unsigned char *rt_string_to_bytes(RtArena *arena, const char *str) {
    if (str == NULL) {
        /* Return empty byte array */
        return rt_array_create_byte_uninit(arena, 0);
    }

    size_t len = strlen(str);
    unsigned char *bytes = rt_array_create_byte_uninit(arena, len);

    for (size_t i = 0; i < len; i++) {
        bytes[i] = (unsigned char)str[i];
    }

    return bytes;
}

