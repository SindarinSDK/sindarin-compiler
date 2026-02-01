#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Join Functions Implementation
 * ============================================================================
 * Join array elements into a string with separator.
 * ============================================================================ */

char *rt_array_join_long(RtArena *arena, long long *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_long: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lld", arr[i]);
    }
    return result;
}

char *rt_array_join_double(RtArena *arena, double *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_double: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", arr[i]);
    }
    return result;
}

char *rt_array_join_char(RtArena *arena, char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_char: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        *ptr++ = arr[i];
    }
    *ptr = '\0';
    return result;
}

char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_bool: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%s", arr[i] ? "true" : "false");
    }
    return result;
}

char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "0xXX" (4 chars) + separators */
    size_t buf_size = len * 4 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_byte: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "0x%02X", arr[i]);
    }
    return result;
}

char *rt_array_join_string(RtArena *arena, char **arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i]) {
            total_len += strlen(arr[i]);
        }
    }
    total_len += (len - 1) * sep_len + 1;

    char *result = rt_arena_alloc(arena, total_len);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_string: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            size_t l = strlen(separator);
            memcpy(ptr, separator, l);
            ptr += l;
        }
        if (arr[i]) {
            size_t l = strlen(arr[i]);
            memcpy(ptr, arr[i], l);
            ptr += l;
        }
    }
    *ptr = '\0';
    return result;
}

char *rt_array_join_int32(RtArena *arena, int32_t *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each int32 can be up to 11 chars + separators */
    size_t buf_size = len * 12 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_int32: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%d", arr[i]);
    }
    return result;
}

char *rt_array_join_uint32(RtArena *arena, uint32_t *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint32 can be up to 10 chars + separators */
    size_t buf_size = len * 11 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_uint32: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%u", arr[i]);
    }
    return result;
}

char *rt_array_join_uint(RtArena *arena, uint64_t *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint64 can be up to 20 chars + separators */
    size_t buf_size = len * 21 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_uint: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lu", (unsigned long)arr[i]);
    }
    return result;
}

char *rt_array_join_float(RtArena *arena, float *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_float: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", (double)arr[i]);
    }
    return result;
}
