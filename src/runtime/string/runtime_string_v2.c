#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "runtime_string_v2.h"

/* ============================================================================
 * Handle-Based String Functions V2 — Implementation
 * ============================================================================
 * Each function follows the pattern:
 *   1. Compute output size
 *   2. RtHandleV2 *h = rt_arena_v2_alloc(arena, size);
 *   3. rt_handle_begin_transaction(h); char *ptr = (char *)h->ptr;
 *   4. Write string data to ptr
 *   5. rt_handle_end_transaction(h);
 *   6. return h;
 *
 * Key differences from V1:
 *   - No 'old' handle parameter - caller manages old handle
 *   - pin/unpin don't take arena parameter
 * ============================================================================ */

/* ============================================================================
 * Array Metadata V2 (local copy to avoid include chain issues)
 * ============================================================================ */

typedef struct {
    RtArenaV2 *arena;   /* Arena that owns this array */
    size_t size;        /* Number of elements currently in array */
    size_t capacity;    /* Total allocated space for elements */
} RtArrayMetaV2Local;

/* ============================================================================
 * String Concatenation
 * ============================================================================ */

RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, la + lb + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, a, la);
    memcpy(ptr + la, b, lb + 1);
    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * String Append (for += operator)
 * ============================================================================ */

RtHandleV2 *rt_str_append_v2(RtArenaV2 *arena, const char *old_str, const char *suffix) {
    if (!old_str) old_str = "";
    if (!suffix) suffix = "";
    size_t la = strlen(old_str), lb = strlen(suffix);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, la + lb + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, old_str, la);
    memcpy(ptr + la, suffix, lb + 1);
    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * Type-to-String Conversions
 * ============================================================================ */

RtHandleV2 *rt_to_string_long_v2(RtArenaV2 *arena, long long val) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%lld", val);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_double_v2(RtArenaV2 *arena, double val) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%.5f", val);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_char_v2(RtArenaV2 *arena, char val) {
    RtHandleV2 *h = rt_arena_v2_alloc(arena, 2);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    ptr[0] = val;
    ptr[1] = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_bool_v2(RtArenaV2 *arena, int val) {
    const char *s = val ? "true" : "false";
    size_t len = val ? 4 : 5;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, s, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_byte_v2(RtArenaV2 *arena, unsigned char val) {
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_to_string_string_v2(RtArenaV2 *arena, const char *val) {
    return rt_arena_v2_strdup(arena, val ? val : "");
}

/* ============================================================================
 * Format Functions
 * ============================================================================ */

RtHandleV2 *rt_format_long_v2(RtArenaV2 *arena, long long val, const char *fmt) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), fmt, val);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_format_double_v2(RtArenaV2 *arena, double val, const char *fmt) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), fmt, val);
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_format_string_v2(RtArenaV2 *arena, const char *val, const char *fmt) {
    char buf[256];
    int len = snprintf(buf, sizeof(buf), fmt, val ? val : "");
    if (len < 0) len = 0;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * String Operations
 * ============================================================================ */

RtHandleV2 *rt_str_substring_v2(RtArenaV2 *arena, const char *str, long start, long end) {
    if (!str) str = "";
    size_t len = strlen(str);

    /* Clamp start and end to valid range */
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if ((size_t)start > len) start = (long)len;
    if ((size_t)end > len) end = (long)len;
    if (start > end) start = end;

    size_t sub_len = (size_t)(end - start);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, sub_len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, str + start, sub_len);
    ptr[sub_len] = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_str_toUpper_v2(RtArenaV2 *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    for (size_t i = 0; i < len; i++) {
        ptr[i] = (char)toupper((unsigned char)str[i]);
    }
    ptr[len] = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_str_toLower_v2(RtArenaV2 *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    for (size_t i = 0; i < len; i++) {
        ptr[i] = (char)tolower((unsigned char)str[i]);
    }
    ptr[len] = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_str_trim_v2(RtArenaV2 *arena, const char *str) {
    if (!str) str = "";
    size_t len = strlen(str);

    /* Skip leading whitespace */
    const char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;

    /* Skip trailing whitespace */
    const char *end = str + len;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;

    size_t trimmed_len = (size_t)(end - start);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, trimmed_len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, start, trimmed_len);
    ptr[trimmed_len] = '\0';
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_str_replace_v2(RtArenaV2 *arena, const char *str, const char *old_s, const char *new_s) {
    if (!str) str = "";
    if (!old_s || !*old_s) {
        /* Empty search string: return copy of original */
        return rt_arena_v2_strdup(arena, str);
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
        return rt_arena_v2_strdup(arena, str);
    }

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    RtHandleV2 *h = rt_arena_v2_alloc(arena, result_len + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;

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

    rt_handle_end_transaction(h);
    return h;
}

/* ============================================================================
 * String Split
 * ============================================================================ */

RtHandleV2 *rt_str_split_v2(RtArenaV2 *arena, const char *str, const char *delimiter) {
    if (!str) str = "";
    if (!delimiter) delimiter = "";

    size_t del_len = strlen(delimiter);

    if (del_len == 0) {
        /* Empty delimiter: split into individual characters */
        size_t count = strlen(str);
        if (count == 0) count = 1;

        size_t alloc_size = sizeof(RtArrayMetaV2Local) + count * sizeof(RtHandleV2 *);
        RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
        rt_handle_begin_transaction(h);
    void *raw = h->ptr;
        RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
        meta->arena = arena;
        meta->size = count;
        meta->capacity = count;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));

        if (strlen(str) == 0) {
            arr[0] = rt_arena_v2_strdup(arena, "");
        } else {
            for (size_t i = 0; i < count; i++) {
                char ch_buf[2] = { str[i], '\0' };
                arr[i] = rt_arena_v2_strdup(arena, ch_buf);
            }
        }

        rt_handle_end_transaction(h);
        return h;
    }

    /* Count the number of parts */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        count++;
        p += del_len;
    }

    /* Allocate array: [RtArrayMetaV2Local][RtHandleV2* elements...] */
    size_t alloc_size = sizeof(RtArrayMetaV2Local) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));

    /* Split and store each substring as a managed handle */
    size_t idx = 0;
    p = str;
    const char *found;
    while ((found = strstr(p, delimiter)) != NULL && idx < count - 1) {
        size_t seg_len = (size_t)(found - p);
        /* Allocate temporary buffer for segment (on stack if small enough) */
        char stack_buf[256];
        char *seg;
        if (seg_len < sizeof(stack_buf)) {
            seg = stack_buf;
        } else {
            seg = malloc(seg_len + 1);
        }
        memcpy(seg, p, seg_len);
        seg[seg_len] = '\0';
        arr[idx++] = rt_arena_v2_strdup(arena, seg);
        if (seg != stack_buf) free(seg);
        p = found + del_len;
    }
    /* Copy the remaining tail */
    arr[idx] = rt_arena_v2_strdup(arena, p);

    rt_handle_end_transaction(h);
    return h;
}

/* Split with limit - handle version */
RtHandleV2 *rt_str_split_n_v2(RtArenaV2 *arena, const char *str, const char *delimiter, int limit) {
    if (!str) str = "";
    if (!delimiter) delimiter = "";

    /* If limit is 0 or negative, use unlimited split */
    if (limit <= 0) {
        return rt_str_split_v2(arena, str, delimiter);
    }

    /* If limit is 1, return the whole string as one part */
    if (limit == 1) {
        size_t alloc_size = sizeof(RtArrayMetaV2Local) + sizeof(RtHandleV2 *);
        RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
        rt_handle_begin_transaction(h);
    void *raw = h->ptr;
        RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = 1;
        RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));
        arr[0] = rt_arena_v2_strdup(arena, str);
        rt_handle_end_transaction(h);
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

    /* Allocate array: [RtArrayMetaV2Local][RtHandleV2* elements...] */
    size_t alloc_size = sizeof(RtArrayMetaV2Local) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));

    /* Split and store each substring as a managed handle (up to limit - 1 splits) */
    size_t idx = 0;
    p = str;
    const char *found;
    while ((found = strstr(p, delimiter)) != NULL && idx < count - 1) {
        size_t seg_len = (size_t)(found - p);
        /* Allocate temporary buffer for segment */
        char stack_buf[256];
        char *seg;
        if (seg_len < sizeof(stack_buf)) {
            seg = stack_buf;
        } else {
            seg = malloc(seg_len + 1);
        }
        memcpy(seg, p, seg_len);
        seg[seg_len] = '\0';
        arr[idx++] = rt_arena_v2_strdup(arena, seg);
        if (seg != stack_buf) free(seg);
        p = found + del_len;
    }
    /* Copy the remaining tail (unsplit) */
    arr[idx] = rt_arena_v2_strdup(arena, p);

    rt_handle_end_transaction(h);
    return h;
}

/* Helper: Check if character is whitespace */
static int is_whitespace_v2(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

RtHandleV2 *rt_str_split_whitespace_v2(RtArenaV2 *arena, const char *str) {
    if (!str) str = "";

    /* Count words first */
    size_t count = 0;
    const char *p = str;
    while (*p) {
        while (*p && is_whitespace_v2(*p)) p++;
        if (*p == '\0') break;
        count++;
        while (*p && !is_whitespace_v2(*p)) p++;
    }

    /* Empty string or only whitespace returns empty array */
    if (count == 0) {
        size_t alloc_size = sizeof(RtArrayMetaV2Local);
        RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
        rt_handle_begin_transaction(h);
    void *raw = h->ptr;
        RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = 0;
        rt_handle_end_transaction(h);
        return h;
    }

    /* Allocate array: [RtArrayMetaV2Local][RtHandleV2* elements...] */
    size_t alloc_size = sizeof(RtArrayMetaV2Local) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));

    /* Split on whitespace */
    size_t idx = 0;
    p = str;
    while (*p && idx < count) {
        while (*p && is_whitespace_v2(*p)) p++;
        if (*p == '\0') break;
        const char *start = p;
        while (*p && !is_whitespace_v2(*p)) p++;
        size_t len = p - start;
        char stack_buf[256];
        char *word;
        if (len < sizeof(stack_buf)) {
            word = stack_buf;
        } else {
            word = malloc(len + 1);
        }
        memcpy(word, start, len);
        word[len] = '\0';
        arr[idx++] = rt_arena_v2_strdup(arena, word);
        if (word != stack_buf) free(word);
    }

    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_str_split_lines_v2(RtArenaV2 *arena, const char *str) {
    if (!str) str = "";

    /* Empty string returns empty array */
    if (*str == '\0') {
        size_t alloc_size = sizeof(RtArrayMetaV2Local);
        RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
        rt_handle_begin_transaction(h);
    void *raw = h->ptr;
        RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = 0;
        rt_handle_end_transaction(h);
        return h;
    }

    /* Count line separators first */
    size_t separators = 0;
    const char *p = str;
    while (*p) {
        if (*p == '\n') {
            separators++;
            p++;
        } else if (*p == '\r') {
            separators++;
            p++;
            if (*p == '\n') p++; /* \r\n is one line ending */
        } else {
            p++;
        }
    }

    /* Number of lines = separators + 1 if string doesn't end with separator,
     * otherwise separators (trailing separator doesn't add empty line) */
    size_t len = strlen(str);
    bool ends_with_sep = (str[len-1] == '\n' || str[len-1] == '\r');
    size_t count = ends_with_sep ? separators : separators + 1;

    /* Allocate array: [RtArrayMetaV2Local][RtHandleV2* elements...] */
    size_t alloc_size = sizeof(RtArrayMetaV2Local) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetaV2Local *meta = (RtArrayMetaV2Local *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetaV2Local));

    /* Split on line endings */
    size_t idx = 0;
    p = str;
    const char *start = str;
    while (*p && idx < count) {
        if (*p == '\n') {
            size_t line_len = p - start;
            char stack_buf[256];
            char *line;
            if (line_len < sizeof(stack_buf)) {
                line = stack_buf;
            } else {
                line = malloc(line_len + 1);
            }
            memcpy(line, start, line_len);
            line[line_len] = '\0';
            arr[idx++] = rt_arena_v2_strdup(arena, line);
            if (line != stack_buf) free(line);
            p++;
            start = p;
        } else if (*p == '\r') {
            size_t line_len = p - start;
            char stack_buf[256];
            char *line;
            if (line_len < sizeof(stack_buf)) {
                line = stack_buf;
            } else {
                line = malloc(line_len + 1);
            }
            memcpy(line, start, line_len);
            line[line_len] = '\0';
            arr[idx++] = rt_arena_v2_strdup(arena, line);
            if (line != stack_buf) free(line);
            p++;
            if (*p == '\n') p++;
            start = p;
        } else {
            p++;
        }
    }
    /* Add the last line if there's content after the last separator */
    if (idx < count && *start) {
        arr[idx] = rt_arena_v2_strdup(arena, start);
    }

    rt_handle_end_transaction(h);
    return h;
}

