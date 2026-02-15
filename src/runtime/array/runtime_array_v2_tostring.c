/*
 * Runtime Array V2 ToString Implementation
 * =========================================
 * Array to string conversion functions for 1D, 2D, and 3D arrays.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * 1D Array toString V2 Functions (primitive types)
 * ============================================================================
 * These take a handle directly and use handle->arena for allocations.
 * Return RtHandleV2* with the string result.
 * ============================================================================ */

RtHandleV2 *rt_to_string_array_long_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    long long *arr = (long long *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 22;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%lld", arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_double_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    double *arr = (double *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 32;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%g", arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_char_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    char *arr = (char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 5;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        *p++ = '\''; *p++ = arr[i]; *p++ = '\'';
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_bool_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    int *arr = (int *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 8;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = arr[i] ? "true" : "false";
        while (*s) *p++ = *s++;
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_byte_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    unsigned char *arr = (unsigned char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 6;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%u", (unsigned)arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_int32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    int32_t *arr = (int32_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 14;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%d", arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_uint32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    uint32_t *arr = (uint32_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 14;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%u", arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_uint_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    uint64_t *arr = (uint64_t *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 22;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%llu", (unsigned long long)arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_array_float_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    float *arr = (float *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    size_t buf_size = 2 + len * 32;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, buf_size);
    if (!h) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        p += snprintf(p, buf_size - (p - result), "%g", (double)arr[i]);
    }
    *p++ = '}'; *p = '\0';
    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * String Array toString V2
 * ============================================================================ */

RtHandleV2 *rt_to_string_array_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);

    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    /* Calculate total length needed */
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        if (i > 0) total_len += 2; /* ", " */
        if (arr[i] != NULL) {
            rt_handle_begin_transaction(arr[i]);
            const char *s = (const char *)arr[i]->ptr;
            total_len += strlen(s) + 2; /* "str" */
            rt_handle_end_transaction(arr[i]);
        } else {
            total_len += 4; /* null */
        }
    }

    /* Allocate result buffer */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    if (!handle) return rt_arena_v2_strdup(arena, "{}");
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;

    /* Build result string */
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        if (arr[i] != NULL) {
            *p++ = '"';
            rt_handle_begin_transaction(arr[i]);
            const char *s = (const char *)arr[i]->ptr;
            while (*s) *p++ = *s++;
            rt_handle_end_transaction(arr[i]);
            *p++ = '"';
        } else {
            const char *null_str = "null";
            while (*null_str) *p++ = *null_str++;
        }
    }
    *p++ = '}';
    *p = '\0';

    rt_handle_end_transaction(handle);
    return handle;
}

/* ============================================================================
 * 2D Array toString V2 Functions
 * ============================================================================ */

/* Helper: generic 1D array to string (used by 2D functions for inner arrays) */
static RtHandleV2 *rt_to_string_array1_v2_generic(RtArenaV2 *arena, void *arr, size_t elem_size,
                                             const char *(*elem_fmt)(void *elem, char *buf, size_t buflen)) {
    if (arr == NULL) return rt_arena_v2_strdup(arena, "{}");

    size_t len = get_array_len_from_data(arr);
    if (len == 0) return rt_arena_v2_strdup(arena, "{}");

    /* Build each element string */
    RtHandleV2 *elem_strs_h = rt_arena_v2_alloc(arena, len * sizeof(char *));
    rt_handle_begin_transaction(elem_strs_h);
    char **elem_strs = (char **)elem_strs_h->ptr;
    size_t total_len = 2; /* {} */
    char buf[64];
    for (size_t i = 0; i < len; i++) {
        void *elem = (char *)arr + i * elem_size;
        const char *s = elem_fmt(elem, buf, sizeof(buf));
        RtHandleV2 *str_h = rt_arena_v2_strdup(arena, s);
        rt_handle_begin_transaction(str_h);
        elem_strs[i] = (char *)str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(elem_strs[i]);
    }

    /* Allocate and build result */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

/* Element formatters */
static const char *fmt_long(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%lld", *(long long *)elem);
    return buf;
}
static const char *fmt_double(void *elem, char *buf, size_t buflen) {
    double d = *(double *)elem;
    if (d == (long long)d) {
        snprintf(buf, buflen, "%lld", (long long)d);
    } else {
        snprintf(buf, buflen, "%g", d);
    }
    return buf;
}
static const char *fmt_char(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "'%c'", *(char *)elem);
    return buf;
}
static const char *fmt_bool(void *elem, char *buf, size_t buflen) {
    (void)buf;
    (void)buflen;
    return *(int *)elem ? "true" : "false";
}
static const char *fmt_byte(void *elem, char *buf, size_t buflen) {
    snprintf(buf, buflen, "%u", *(unsigned char *)elem);
    return buf;
}

/* 2D array toString: iterate outer, call 1D on each inner */
RtHandleV2 *rt_to_string_array2_long_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        long long *inner = outer[i] ? (long long *)rt_array_data_v2(outer[i]) : NULL;
        RtHandleV2 *inner_str_h = rt_to_string_array1_v2_generic(arena, inner, sizeof(long long), fmt_long);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_double_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        double *inner = outer[i] ? (double *)rt_array_data_v2(outer[i]) : NULL;
        RtHandleV2 *inner_str_h = rt_to_string_array1_v2_generic(arena, inner, sizeof(double), fmt_double);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_char_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        char *inner = outer[i] ? (char *)rt_array_data_v2(outer[i]) : NULL;
        RtHandleV2 *inner_str_h = rt_to_string_array1_v2_generic(arena, inner, sizeof(char), fmt_char);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_bool_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        int *inner = outer[i] ? (int *)rt_array_data_v2(outer[i]) : NULL;
        RtHandleV2 *inner_str_h = rt_to_string_array1_v2_generic(arena, inner, sizeof(int), fmt_bool);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_byte_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        unsigned char *inner = outer[i] ? (unsigned char *)rt_array_data_v2(outer[i]) : NULL;
        RtHandleV2 *inner_str_h = rt_to_string_array1_v2_generic(arena, inner, sizeof(unsigned char), fmt_byte);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_string_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        /* rt_to_string_array_string_v2 now returns RtHandleV2* */
        RtHandleV2 *inner_str_h = rt_to_string_array_string_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

/* ============================================================================
 * 3D Array ToString Functions
 * ============================================================================
 * 3D arrays: iterate outermost, call 2D on each middle layer.
 * ============================================================================ */

RtHandleV2 *rt_to_string_array3_long_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        /* rt_to_string_array2_long_v2 now returns RtHandleV2* */
        RtHandleV2 *inner_str_h = rt_to_string_array2_long_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_double_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 *inner_str_h = rt_to_string_array2_double_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_char_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 *inner_str_h = rt_to_string_array2_char_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_bool_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 *inner_str_h = rt_to_string_array2_bool_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_byte_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 *inner_str_h = rt_to_string_array2_byte_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_string_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);
    if (outer_len == 0) return rt_arena_v2_strdup(arena, "{}");

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandleV2 *inner_str_h = rt_to_string_array2_string_v2(outer[i]);
        rt_handle_begin_transaction(inner_str_h);
        inner_strs[i] = (char *)inner_str_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_str_h);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(handle);
    return handle;
}
