/*
 * Runtime Array V2 Join Implementation
 * =====================================
 * Array join functions that concatenate array elements into strings.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * Array Join Functions (V2)
 * ============================================================================
 * Join array elements into a string with separator.
 * Arena derived from input handle.
 * ============================================================================ */

char *rt_array_join_long_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const long long *arr = (const long long *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lld", arr[i]);
    }
    return result;
}

char *rt_array_join_double_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const double *arr = (const double *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", arr[i]);
    }
    return result;
}

char *rt_array_join_char_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const char *arr = (const char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

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

char *rt_array_join_bool_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const int *arr = (const int *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%s", arr[i] ? "true" : "false");
    }
    return result;
}

char *rt_array_join_byte_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const unsigned char *arr = (const unsigned char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "0xXX" (4 chars) + separators */
    size_t buf_size = len * 4 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "0x%02X", arr[i]);
    }
    return result;
}

char *rt_array_join_int32_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const int32_t *arr = (const int32_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each int32 can be up to 11 chars + separators */
    size_t buf_size = len * 12 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%d", arr[i]);
    }
    return result;
}

char *rt_array_join_uint32_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const uint32_t *arr = (const uint32_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint32 can be up to 10 chars + separators */
    size_t buf_size = len * 11 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%u", arr[i]);
    }
    return result;
}

char *rt_array_join_uint_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const uint64_t *arr = (const uint64_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each uint64 can be up to 20 chars + separators */
    size_t buf_size = len * 21 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%lu", (unsigned long)arr[i]);
    }
    return result;
}

char *rt_array_join_float_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    const float *arr = (const float *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, buf_size);
    if (handle == NULL) return "";
    char *result = (char *)rt_handle_v2_pin(handle);

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", (double)arr[i]);
    }
    return result;
}

char *rt_array_join_string_v2(RtHandleV2 *arr_h, const char *separator) {
    if (arr_h == NULL) return "";
    RtArenaV2 *arena = arr_h->arena;
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }

    /* Calculate total size needed */
    size_t sep_len = separator ? strlen(separator) : 0;
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != NULL) {
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            if (s) total_len += strlen(s);
            rt_handle_v2_unpin(arr[i]);
        }
        if (i < len - 1) total_len += sep_len;
    }

    /* Allocate result buffer */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    if (!handle) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "");
        return (char *)rt_handle_v2_pin(h);
    }
    char *result = (char *)rt_handle_v2_pin(handle);

    /* Build result string */
    char *p = result;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != NULL) {
            const char *s = (const char *)rt_handle_v2_pin(arr[i]);
            if (s) {
                size_t slen = strlen(s);
                memcpy(p, s, slen);
                p += slen;
            }
            rt_handle_v2_unpin(arr[i]);
        }
        if (i < len - 1 && separator) {
            memcpy(p, separator, sep_len);
            p += sep_len;
        }
    }
    *p = '\0';

    return result;
}
