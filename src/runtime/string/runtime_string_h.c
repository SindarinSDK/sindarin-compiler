#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "runtime_string_h.h"
#include "runtime_arena.h"
#include "../array/runtime_array.h"

/* ============================================================================
 * Handle-Based String Functions — Implementation
 * ============================================================================
 * Each function follows the pattern:
 *   1. Compute output size
 *   2. RtHandle h = rt_managed_alloc(arena, old, size);
 *   3. char *ptr = (char *)rt_managed_pin(arena, h);
 *   4. Write string data to ptr
 *   5. rt_managed_unpin(arena, h);
 *   6. return h;
 * ============================================================================ */

/* ============================================================================
 * String Concatenation
 * ============================================================================ */

RtHandle rt_str_concat_h(RtManagedArena *arena, RtHandle old, const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    RtHandle h = rt_managed_alloc(arena, old, la + lb + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, a, la);
    memcpy(ptr + la, b, lb + 1);
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * String Append (for += operator)
 * ============================================================================ */

RtHandle rt_str_append_h(RtManagedArena *arena, RtHandle old, const char *old_str, const char *suffix) {
    if (!old_str) old_str = "";
    if (!suffix) suffix = "";
    size_t la = strlen(old_str), lb = strlen(suffix);
    RtHandle h = rt_managed_alloc(arena, old, la + lb + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, old_str, la);
    memcpy(ptr + la, suffix, lb + 1);
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * Type-to-String Conversions
 * ============================================================================ */

RtHandle rt_to_string_long_h(RtManagedArena *arena, long long val) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lld", val);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_to_string_double_h(RtManagedArena *arena, double val) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%g", val);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_to_string_char_h(RtManagedArena *arena, char val) {
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, 2);
    char *ptr = (char *)rt_managed_pin(arena, h);
    ptr[0] = val;
    ptr[1] = '\0';
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_to_string_bool_h(RtManagedArena *arena, int val) {
    const char *s = val ? "true" : "false";
    size_t len = val ? 4 : 5;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, s, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_to_string_byte_h(RtManagedArena *arena, unsigned char val) {
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_to_string_string_h(RtManagedArena *arena, const char *val) {
    return rt_managed_strdup(arena, RT_HANDLE_NULL, val ? val : "");
}

/* ============================================================================
 * Format Functions
 * ============================================================================ */

RtHandle rt_format_long_h(RtManagedArena *arena, long long val, const char *fmt) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), fmt, val);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_format_double_h(RtManagedArena *arena, double val, const char *fmt) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), fmt, val);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_format_string_h(RtManagedArena *arena, const char *val, const char *fmt) {
    char buf[256];
    int len = snprintf(buf, sizeof(buf), fmt, val ? val : "");
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, buf, len + 1);
    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * String Operations
 * ============================================================================ */

RtHandle rt_str_substring_h(RtManagedArena *arena, const char *str, long start, long end) {
    if (!str) str = "";
    size_t len = strlen(str);

    /* Clamp start and end to valid range */
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if ((size_t)start > len) start = (long)len;
    if ((size_t)end > len) end = (long)len;
    if (start > end) start = end;

    size_t sub_len = (size_t)(end - start);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, sub_len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, str + start, sub_len);
    ptr[sub_len] = '\0';
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_str_toUpper_h(RtManagedArena *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    for (size_t i = 0; i < len; i++) {
        ptr[i] = (char)toupper((unsigned char)str[i]);
    }
    ptr[len] = '\0';
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_str_toLower_h(RtManagedArena *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    for (size_t i = 0; i < len; i++) {
        ptr[i] = (char)tolower((unsigned char)str[i]);
    }
    ptr[len] = '\0';
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_str_trim_h(RtManagedArena *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);

    /* Skip leading whitespace */
    const char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;

    /* Skip trailing whitespace */
    const char *end = str + len;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;

    size_t trimmed_len = (size_t)(end - start);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, trimmed_len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);
    memcpy(ptr, start, trimmed_len);
    ptr[trimmed_len] = '\0';
    rt_managed_unpin(arena, h);
    return h;
}

RtHandle rt_str_replace_h(RtManagedArena *arena, const char *str, const char *old_s, const char *new_s) {
    if (!str) str = "";
    if (!old_s || !*old_s) {
        /* Empty search string: return copy of original */
        return rt_managed_strdup(arena, RT_HANDLE_NULL, str);
    }
    if (!new_s) new_s = "";

    size_t old_len = strlen(old_s);
    size_t new_len = strlen(new_s);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old_s)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) {
        /* No occurrences: return copy of original */
        return rt_managed_strdup(arena, RT_HANDLE_NULL, str);
    }

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, result_len + 1);
    char *ptr = (char *)rt_managed_pin(arena, h);

    /* Build replaced string */
    char *dst = ptr;
    p = str;
    const char *found;
    while ((found = strstr(p, old_s)) != NULL) {
        size_t seg_len = (size_t)(found - p);
        memcpy(dst, p, seg_len);
        dst += seg_len;
        memcpy(dst, new_s, new_len);
        dst += new_len;
        p = found + old_len;
    }
    /* Copy remaining tail */
    strcpy(dst, p);

    rt_managed_unpin(arena, h);
    return h;
}

/* ============================================================================
 * String Split
 * ============================================================================ */

RtHandle rt_str_split_h(RtManagedArena *arena, const char *str, const char *delimiter) {
    if (!str) str = "";
    if (!delimiter) delimiter = "";

    size_t del_len = strlen(delimiter);

    if (del_len == 0) {
        /* Empty delimiter: split into individual characters */
        size_t count = strlen(str);
        if (count == 0) count = 1;

        size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
        RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *raw = rt_managed_pin(arena, h);
        RtArrayMetadata *meta = (RtArrayMetadata *)raw;
        meta->arena = (RtArena *)arena;
        meta->size = count;
        meta->capacity = count;
        RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));

        if (strlen(str) == 0) {
            arr[0] = rt_managed_strdup(arena, RT_HANDLE_NULL, "");
        } else {
            for (size_t i = 0; i < count; i++) {
                char ch_buf[2] = { str[i], '\0' };
                arr[i] = rt_managed_strdup(arena, RT_HANDLE_NULL, ch_buf);
            }
        }

        rt_managed_unpin(arena, h);
        return h;
    }

    /* Count the number of parts */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        count++;
        p += del_len;
    }

    /* Allocate array: [RtArrayMetadata][RtHandle elements...] */
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));

    /* Split and store each substring as a managed handle */
    size_t idx = 0;
    p = str;
    const char *found;
    while ((found = strstr(p, delimiter)) != NULL && idx < count - 1) {
        size_t seg_len = (size_t)(found - p);
        char *seg = rt_arena_alloc((RtArena *)arena, seg_len + 1);
        memcpy(seg, p, seg_len);
        seg[seg_len] = '\0';
        arr[idx++] = rt_managed_strdup(arena, RT_HANDLE_NULL, seg);
        p = found + del_len;
    }
    /* Copy the remaining tail */
    arr[idx] = rt_managed_strdup(arena, RT_HANDLE_NULL, p);

    rt_managed_unpin(arena, h);
    return h;
}

/* Split with limit - handle version */
RtHandle rt_str_split_n_h(RtManagedArena *arena, const char *str, const char *delimiter, int limit) {
    if (!str) str = "";
    if (!delimiter) delimiter = "";

    /* If limit is 0 or negative, use unlimited split */
    if (limit <= 0) {
        return rt_str_split_h(arena, str, delimiter);
    }

    /* If limit is 1, return the whole string as one part */
    if (limit == 1) {
        size_t alloc_size = sizeof(RtArrayMetadata) + sizeof(RtHandle);
        RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
        void *raw = rt_managed_pin(arena, h);
        RtArrayMetadata *meta = (RtArrayMetadata *)raw;
        meta->arena = (RtArena *)arena;
        meta->size = 1;
        meta->capacity = 1;
        RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
        arr[0] = rt_managed_strdup(arena, RT_HANDLE_NULL, str);
        rt_managed_unpin(arena, h);
        return h;
    }

    size_t del_len = strlen(delimiter);

    /* Count the number of parts (up to limit) */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL && count < (size_t)limit) {
        count++;
        p += del_len;
    }

    /* Allocate array: [RtArrayMetadata][RtHandle elements...] */
    size_t alloc_size = sizeof(RtArrayMetadata) + count * sizeof(RtHandle);
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, alloc_size);
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *arr = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));

    /* Split and store each substring as a managed handle (up to limit - 1 splits) */
    size_t idx = 0;
    p = str;
    const char *found;
    while ((found = strstr(p, delimiter)) != NULL && idx < count - 1) {
        size_t seg_len = (size_t)(found - p);
        char *seg = rt_arena_alloc((RtArena *)arena, seg_len + 1);
        memcpy(seg, p, seg_len);
        seg[seg_len] = '\0';
        arr[idx++] = rt_managed_strdup(arena, RT_HANDLE_NULL, seg);
        p = found + del_len;
    }
    /* Copy the remaining tail (unsplit) */
    arr[idx] = rt_managed_strdup(arena, RT_HANDLE_NULL, p);

    rt_managed_unpin(arena, h);
    return h;
}
