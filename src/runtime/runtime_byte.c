#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_byte.h"
#include "array/runtime_array_v2.h"

/* ============================================================================
 * Byte Array Conversion Functions
 * ============================================================================
 * Implementation of byte array to string and string to byte array conversions.
 * ============================================================================ */

/* ============================================================================
 * Internal Helper: Create uninitialized byte array
 * ============================================================================ */

static unsigned char *create_byte_array(RtArenaV2 *arena, size_t count) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(unsigned char);
    RtHandleV2 *raw_h = rt_arena_v2_alloc(arena, alloc_size);
    if (raw_h == NULL) return NULL;
    rt_handle_v2_pin(raw_h);
    void *raw = raw_h->ptr;

    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = NULL;  /* Not using V2 arena here */
    meta->size = count;
    meta->capacity = count;

    return (unsigned char *)((char *)raw + sizeof(RtArrayMetadataV2));
}

/* ============================================================================
 * Byte Array to String Conversions
 * ============================================================================ */

/* Base64 encoding table */
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Convert byte array to string using UTF-8 decoding
 * Note: This is a simple passthrough since our strings are already UTF-8.
 * Invalid UTF-8 sequences are passed through as-is. */
RtHandleV2 *rt_byte_array_to_string(RtArenaV2 *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        RtHandleV2 *result_h = rt_arena_v2_alloc(arena, 1);
        rt_handle_v2_pin(result_h);
        char *result = (char *)result_h->ptr;
        result[0] = '\0';
        rt_handle_v2_unpin(result_h);
        return result_h;
    }

    size_t len = rt_v2_data_array_length(bytes);
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    for (size_t i = 0; i < len; i++) {
        result[i] = (char)bytes[i];
    }
    result[len] = '\0';

    rt_handle_v2_unpin(result_h);
    return result_h;
}

/* Convert byte array to string using Latin-1/ISO-8859-1 decoding
 * Each byte directly maps to its Unicode code point (0x00-0xFF).
 * This requires UTF-8 encoding for values 0x80-0xFF. */
RtHandleV2 *rt_byte_array_to_string_latin1(RtArenaV2 *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        RtHandleV2 *result_h = rt_arena_v2_alloc(arena, 1);
        rt_handle_v2_pin(result_h);
        char *result = (char *)result_h->ptr;
        result[0] = '\0';
        rt_handle_v2_unpin(result_h);
        return result_h;
    }

    size_t len = rt_v2_data_array_length(bytes);

    /* Calculate output size: bytes 0x00-0x7F = 1 byte, 0x80-0xFF = 2 bytes in UTF-8 */
    size_t out_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] < 0x80) {
            out_len += 1;
        } else {
            out_len += 2;  /* UTF-8 encoding for 0x80-0xFF */
        }
    }

    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, out_len + 1);
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;
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

    rt_handle_v2_unpin(result_h);
    return result_h;
}

/* Convert byte array to hexadecimal string */
RtHandleV2 *rt_byte_array_to_hex(RtArenaV2 *arena, unsigned char *bytes) {
    static const char hex_chars[] = "0123456789abcdef";

    if (bytes == NULL) {
        RtHandleV2 *result_h = rt_arena_v2_alloc(arena, 1);
        rt_handle_v2_pin(result_h);
        char *result = (char *)result_h->ptr;
        result[0] = '\0';
        rt_handle_v2_unpin(result_h);
        return result_h;
    }

    size_t len = rt_v2_data_array_length(bytes);
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, len * 2 + 1);
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    for (size_t i = 0; i < len; i++) {
        result[i * 2] = hex_chars[(bytes[i] >> 4) & 0xF];
        result[i * 2 + 1] = hex_chars[bytes[i] & 0xF];
    }
    result[len * 2] = '\0';

    rt_handle_v2_unpin(result_h);
    return result_h;
}

/* Convert byte array to Base64 string */
RtHandleV2 *rt_byte_array_to_base64(RtArenaV2 *arena, unsigned char *bytes) {
    if (bytes == NULL) {
        RtHandleV2 *result_h = rt_arena_v2_alloc(arena, 1);
        rt_handle_v2_pin(result_h);
        char *result = (char *)result_h->ptr;
        result[0] = '\0';
        rt_handle_v2_unpin(result_h);
        return result_h;
    }

    size_t len = rt_v2_data_array_length(bytes);

    /* Calculate output size: 4 output chars for every 3 input bytes, rounded up */
    size_t out_len = ((len + 2) / 3) * 4;
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, out_len + 1);
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

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

    rt_handle_v2_unpin(result_h);
    return result_h;
}

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================ */

/* Convert string to UTF-8 byte array
 * Since our strings are already UTF-8, this is a simple copy. */
unsigned char *rt_string_to_bytes(RtArenaV2 *arena, const char *str) {
    if (str == NULL) {
        /* Return empty byte array */
        return create_byte_array(arena, 0);
    }

    size_t len = strlen(str);
    unsigned char *bytes = create_byte_array(arena, len);

    for (size_t i = 0; i < len; i++) {
        bytes[i] = (unsigned char)str[i];
    }

    return bytes;
}

