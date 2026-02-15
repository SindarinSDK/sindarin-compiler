#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "runtime_string_v2.h"

/* ============================================================================
 * Handle-Based String Functions V2 — Implementation
 * ============================================================================
 * Each function follows the pattern:
 *   1. Begin transaction on input handle(s) to safely read ptr
 *   2. Compute output size from input data
 *   3. RtHandleV2 *h = rt_arena_v2_alloc(arena, size);
 *   4. rt_handle_begin_transaction(h); char *ptr = (char *)h->ptr;
 *   5. Write string data to ptr
 *   6. rt_handle_end_transaction(h);
 *   7. End transactions on input handles
 *   8. return h;
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

RtHandleV2 *rt_str_concat_v2(RtArenaV2 *arena, RtHandleV2 *a_h, RtHandleV2 *b_h) {
    /* Get raw pointers with transactions */
    const char *a = "";
    const char *b = "";
    if (a_h) { rt_handle_begin_transaction(a_h); a = (const char *)a_h->ptr; if (!a) a = ""; }
    if (b_h) { rt_handle_begin_transaction(b_h); b = (const char *)b_h->ptr; if (!b) b = ""; }

    size_t la = strlen(a), lb = strlen(b);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, la + lb + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, a, la);
    memcpy(ptr + la, b, lb + 1);
    rt_handle_end_transaction(h);

    if (b_h) rt_handle_end_transaction(b_h);
    if (a_h) rt_handle_end_transaction(a_h);
    return h;
}

/* ============================================================================
 * String Append (for += operator)
 * ============================================================================ */

RtHandleV2 *rt_str_append_v2(RtArenaV2 *arena, RtHandleV2 *old_h, RtHandleV2 *suffix_h) {
    const char *old_str = "";
    const char *suffix = "";
    if (old_h) { rt_handle_begin_transaction(old_h); old_str = (const char *)old_h->ptr; if (!old_str) old_str = ""; }
    if (suffix_h) { rt_handle_begin_transaction(suffix_h); suffix = (const char *)suffix_h->ptr; if (!suffix) suffix = ""; }

    size_t la = strlen(old_str), lb = strlen(suffix);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, la + lb + 1);
    rt_handle_begin_transaction(h);
    char *ptr = (char *)h->ptr;
    memcpy(ptr, old_str, la);
    memcpy(ptr + la, suffix, lb + 1);
    rt_handle_end_transaction(h);

    if (suffix_h) rt_handle_end_transaction(suffix_h);
    if (old_h) rt_handle_end_transaction(old_h);
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

RtHandleV2 *rt_to_string_string_v2(RtArenaV2 *arena, RtHandleV2 *val_h) {
    if (!val_h) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(val_h);
    const char *s = (const char *)val_h->ptr;
    RtHandleV2 *result = rt_arena_v2_strdup(arena, s ? s : "");
    rt_handle_end_transaction(val_h);
    return result;
}

/* ============================================================================
 * Format Functions
 * Parse Sindarin format specs (e.g., "05d", ".2f", "10s") into C printf format
 * strings. Same parsing logic as V1 rt_format_* but returns RtHandleV2*.
 * ============================================================================ */

/* Helper: allocate a handle and copy buf into it */
static RtHandleV2 *format_result_to_handle(RtArenaV2 *arena, const char *buf) {
    size_t len = strlen(buf);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, len + 1);
    rt_handle_begin_transaction(h);
    memcpy((char *)h->ptr, buf, len + 1);
    rt_handle_end_transaction(h);
    return h;
}

RtHandleV2 *rt_format_long_v2(RtArenaV2 *arena, long long val, const char *fmt) {
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%lld", val);
        return format_result_to_handle(arena, buf);
    }

    int width = 0;
    int zero_pad = 0;
    const char *p = fmt;

    if (*p == '0') { zero_pad = 1; p++; }
    while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }

    char type = *p ? *p : 'd';

    switch (type) {
        case 'd':
            if (zero_pad && width > 0) snprintf(format_str, sizeof(format_str), "%%0%dlld", width);
            else if (width > 0)        snprintf(format_str, sizeof(format_str), "%%%dlld", width);
            else                        snprintf(format_str, sizeof(format_str), "%%lld");
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'x':
            if (zero_pad && width > 0) snprintf(format_str, sizeof(format_str), "%%0%dllx", width);
            else if (width > 0)        snprintf(format_str, sizeof(format_str), "%%%dllx", width);
            else                        snprintf(format_str, sizeof(format_str), "%%llx");
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'X':
            if (zero_pad && width > 0) snprintf(format_str, sizeof(format_str), "%%0%dllX", width);
            else if (width > 0)        snprintf(format_str, sizeof(format_str), "%%%dllX", width);
            else                        snprintf(format_str, sizeof(format_str), "%%llX");
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'o':
            if (zero_pad && width > 0) snprintf(format_str, sizeof(format_str), "%%0%dllo", width);
            else if (width > 0)        snprintf(format_str, sizeof(format_str), "%%%dllo", width);
            else                        snprintf(format_str, sizeof(format_str), "%%llo");
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'b': {
            char binbuf[65];
            int len = 0;
            unsigned long long uval = (unsigned long long)val;
            if (uval == 0) { binbuf[len++] = '0'; }
            else { while (uval > 0) { binbuf[len++] = (uval & 1) ? '1' : '0'; uval >>= 1; } }
            for (int i = 0; i < len / 2; i++) { char tmp = binbuf[i]; binbuf[i] = binbuf[len - 1 - i]; binbuf[len - 1 - i] = tmp; }
            if (width > len) {
                int pad = width - len;
                memmove(binbuf + pad, binbuf, len);
                for (int i = 0; i < pad; i++) binbuf[i] = zero_pad ? '0' : ' ';
                len = width;
            }
            binbuf[len] = '\0';
            return format_result_to_handle(arena, binbuf);
        }
        default:
            snprintf(buf, sizeof(buf), "%lld", val);
            break;
    }

    return format_result_to_handle(arena, buf);
}

RtHandleV2 *rt_format_double_v2(RtArenaV2 *arena, double val, const char *fmt) {
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%g", val);
        return format_result_to_handle(arena, buf);
    }

    int width = 0;
    int precision = -1;
    int zero_pad = 0;
    const char *p = fmt;

    if (*p == '0') { zero_pad = 1; p++; }
    while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
    if (*p == '.') { p++; precision = 0; while (*p >= '0' && *p <= '9') { precision = precision * 10 + (*p - '0'); p++; } }

    char type = *p ? *p : 'f';

    if (type == '%') {
        val *= 100.0;
        if (precision >= 0) snprintf(format_str, sizeof(format_str), "%%.%df%%%%", precision);
        else                snprintf(format_str, sizeof(format_str), "%%f%%%%");
        snprintf(buf, sizeof(buf), format_str, val);
        return format_result_to_handle(arena, buf);
    }

    int pos = 0;
    format_str[pos++] = '%';
    if (zero_pad) format_str[pos++] = '0';
    if (width > 0) pos += snprintf(format_str + pos, sizeof(format_str) - pos, "%d", width);
    if (precision >= 0) pos += snprintf(format_str + pos, sizeof(format_str) - pos, ".%d", precision);

    switch (type) {
        case 'f': format_str[pos++] = 'f'; break;
        case 'e': format_str[pos++] = 'e'; break;
        case 'E': format_str[pos++] = 'E'; break;
        case 'g': format_str[pos++] = 'g'; break;
        case 'G': format_str[pos++] = 'G'; break;
        default:  format_str[pos++] = 'f'; break;
    }
    format_str[pos] = '\0';

    snprintf(buf, sizeof(buf), format_str, val);
    return format_result_to_handle(arena, buf);
}

RtHandleV2 *rt_format_string_v2(RtArenaV2 *arena, RtHandleV2 *val_h, const char *fmt) {
    const char *val = "";
    if (val_h) { rt_handle_begin_transaction(val_h); val = (const char *)val_h->ptr; if (!val) val = ""; }

    if (fmt == NULL || fmt[0] == '\0') {
        RtHandleV2 *result = rt_arena_v2_strdup(arena, val);
        if (val_h) rt_handle_end_transaction(val_h);
        return result;
    }

    /* Parse format specifier: [-][width][.maxlen]s */
    int width = 0;
    int maxlen = -1;
    int left_align = 0;
    const char *p = fmt;

    if (*p == '-') { left_align = 1; p++; }
    while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
    if (*p == '.') { p++; maxlen = 0; while (*p >= '0' && *p <= '9') { maxlen = maxlen * 10 + (*p - '0'); p++; } }

    int len = strlen(val);
    if (maxlen >= 0 && len > maxlen) len = maxlen;

    int out_len = len;
    if (width > len) out_len = width;

    RtHandleV2 *h = rt_arena_v2_alloc(arena, out_len + 1);
    rt_handle_begin_transaction(h);
    char *result = (char *)h->ptr;

    if (width > len) {
        if (left_align) {
            memcpy(result, val, len);
            memset(result + len, ' ', width - len);
        } else {
            memset(result, ' ', width - len);
            memcpy(result + (width - len), val, len);
        }
        result[width] = '\0';
    } else {
        memcpy(result, val, len);
        result[len] = '\0';
    }

    rt_handle_end_transaction(h);
    if (val_h) rt_handle_end_transaction(val_h);
    return h;
}

/* ============================================================================
 * String Operations
 * ============================================================================ */

RtHandleV2 *rt_str_substring_v2(RtArenaV2 *arena, RtHandleV2 *str_h, long start, long end) {
    if (!str_h) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
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

    rt_handle_end_transaction(str_h);
    return h;
}

RtHandleV2 *rt_str_toUpper_v2(RtArenaV2 *arena, RtHandleV2 *str_h) {
    if (!str_h) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
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

    rt_handle_end_transaction(str_h);
    return h;
}

RtHandleV2 *rt_str_toLower_v2(RtArenaV2 *arena, RtHandleV2 *str_h) {
    if (!str_h) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
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

    rt_handle_end_transaction(str_h);
    return h;
}

RtHandleV2 *rt_str_trim_v2(RtArenaV2 *arena, RtHandleV2 *str_h) {
    if (!str_h) return rt_arena_v2_strdup(arena, "");
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
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

    rt_handle_end_transaction(str_h);
    return h;
}

RtHandleV2 *rt_str_replace_v2(RtArenaV2 *arena, RtHandleV2 *str_h, RtHandleV2 *old_h, RtHandleV2 *new_h) {
    if (!str_h) return rt_arena_v2_strdup(arena, "");

    /* Begin transactions on all inputs */
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    if (!str) str = "";

    const char *old_s = "";
    if (old_h) { rt_handle_begin_transaction(old_h); old_s = (const char *)old_h->ptr; if (!old_s) old_s = ""; }

    const char *new_s = "";
    if (new_h) { rt_handle_begin_transaction(new_h); new_s = (const char *)new_h->ptr; if (!new_s) new_s = ""; }

    if (!*old_s) {
        /* Empty search string: return copy of original */
        RtHandleV2 *result = rt_arena_v2_strdup(arena, str);
        if (new_h) rt_handle_end_transaction(new_h);
        if (old_h) rt_handle_end_transaction(old_h);
        rt_handle_end_transaction(str_h);
        return result;
    }

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
        RtHandleV2 *result = rt_arena_v2_strdup(arena, str);
        if (new_h) rt_handle_end_transaction(new_h);
        if (old_h) rt_handle_end_transaction(old_h);
        rt_handle_end_transaction(str_h);
        return result;
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

    if (new_h) rt_handle_end_transaction(new_h);
    if (old_h) rt_handle_end_transaction(old_h);
    rt_handle_end_transaction(str_h);
    return h;
}

/* ============================================================================
 * String Split
 * ============================================================================ */

RtHandleV2 *rt_str_split_v2(RtArenaV2 *arena, RtHandleV2 *str_h, RtHandleV2 *delim_h) {
    /* Get raw string data within transactions */
    const char *str = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    const char *delimiter = "";
    if (delim_h) { rt_handle_begin_transaction(delim_h); delimiter = (const char *)delim_h->ptr; if (!delimiter) delimiter = ""; }

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
        if (delim_h) rt_handle_end_transaction(delim_h);
        if (str_h) rt_handle_end_transaction(str_h);
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
    if (delim_h) rt_handle_end_transaction(delim_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return h;
}

/* Split with limit - handle version */
RtHandleV2 *rt_str_split_n_v2(RtArenaV2 *arena, RtHandleV2 *str_h, RtHandleV2 *delim_h, int limit) {
    /* Get raw string data within transactions */
    const char *str = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    const char *delimiter = "";
    if (delim_h) { rt_handle_begin_transaction(delim_h); delimiter = (const char *)delim_h->ptr; if (!delimiter) delimiter = ""; }

    /* If limit is 0 or negative, use unlimited split */
    if (limit <= 0) {
        /* End our transactions and delegate - split_v2 will re-acquire */
        if (delim_h) rt_handle_end_transaction(delim_h);
        if (str_h) rt_handle_end_transaction(str_h);
        return rt_str_split_v2(arena, str_h, delim_h);
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
        if (delim_h) rt_handle_end_transaction(delim_h);
        if (str_h) rt_handle_end_transaction(str_h);
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
    if (delim_h) rt_handle_end_transaction(delim_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return h;
}

/* Helper: Check if character is whitespace */
static int is_whitespace_v2(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

RtHandleV2 *rt_str_split_whitespace_v2(RtArenaV2 *arena, RtHandleV2 *str_h) {
    const char *str = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }

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
        if (str_h) rt_handle_end_transaction(str_h);
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
    if (str_h) rt_handle_end_transaction(str_h);
    return h;
}

RtHandleV2 *rt_str_split_lines_v2(RtArenaV2 *arena, RtHandleV2 *str_h) {
    const char *str = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }

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
        if (str_h) rt_handle_end_transaction(str_h);
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
    if (str_h) rt_handle_end_transaction(str_h);
    return h;
}

/* ============================================================================
 * String Query Functions (Handle-Based)
 * ============================================================================ */

long rt_str_length_v2(RtHandleV2 *str_h) {
    if (!str_h) return 0;
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    long result = (str ? (long)strlen(str) : 0);
    rt_handle_end_transaction(str_h);
    return result;
}

long rt_str_indexOf_v2(RtHandleV2 *str_h, RtHandleV2 *search_h) {
    const char *str = "";
    const char *search = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    if (search_h) { rt_handle_begin_transaction(search_h); search = (const char *)search_h->ptr; if (!search) search = ""; }

    const char *pos = strstr(str, search);
    long result = pos ? (long)(pos - str) : -1;

    if (search_h) rt_handle_end_transaction(search_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return result;
}

int rt_str_contains_v2(RtHandleV2 *str_h, RtHandleV2 *search_h) {
    const char *str = "";
    const char *search = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    if (search_h) { rt_handle_begin_transaction(search_h); search = (const char *)search_h->ptr; if (!search) search = ""; }

    int result = strstr(str, search) != NULL;

    if (search_h) rt_handle_end_transaction(search_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return result;
}

long rt_str_charAt_v2(RtHandleV2 *str_h, long index) {
    if (!str_h) return 0;
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    if (!str) { rt_handle_end_transaction(str_h); return 0; }

    long len = (long)strlen(str);
    if (index < 0) index = len + index;
    long result = (index >= 0 && index < len) ? (long)(unsigned char)str[index] : 0;

    rt_handle_end_transaction(str_h);
    return result;
}

int rt_str_startsWith_v2(RtHandleV2 *str_h, RtHandleV2 *prefix_h) {
    const char *str = "";
    const char *prefix = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    if (prefix_h) { rt_handle_begin_transaction(prefix_h); prefix = (const char *)prefix_h->ptr; if (!prefix) prefix = ""; }

    size_t prefix_len = strlen(prefix);
    int result = (strlen(str) >= prefix_len && strncmp(str, prefix, prefix_len) == 0);

    if (prefix_h) rt_handle_end_transaction(prefix_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return result;
}

int rt_str_endsWith_v2(RtHandleV2 *str_h, RtHandleV2 *suffix_h) {
    const char *str = "";
    const char *suffix = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    if (suffix_h) { rt_handle_begin_transaction(suffix_h); suffix = (const char *)suffix_h->ptr; if (!suffix) suffix = ""; }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    int result = (str_len >= suffix_len && strcmp(str + str_len - suffix_len, suffix) == 0);

    if (suffix_h) rt_handle_end_transaction(suffix_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return result;
}

int rt_str_region_equals_v2(RtHandleV2 *str_h, long start, long end, RtHandleV2 *pattern_h) {
    const char *str = "";
    const char *pattern = "";
    if (str_h) { rt_handle_begin_transaction(str_h); str = (const char *)str_h->ptr; if (!str) str = ""; }
    if (pattern_h) { rt_handle_begin_transaction(pattern_h); pattern = (const char *)pattern_h->ptr; if (!pattern) pattern = ""; }

    long pat_len = (long)strlen(pattern);
    int result = 0;
    if (end - start == pat_len) {
        result = 1;
        for (long i = 0; i < pat_len; i++) {
            if (str[start + i] != pattern[i]) { result = 0; break; }
        }
    }

    if (pattern_h) rt_handle_end_transaction(pattern_h);
    if (str_h) rt_handle_end_transaction(str_h);
    return result;
}

int rt_str_is_blank_v2(RtHandleV2 *str_h) {
    if (!str_h) return 1;
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    if (!str || *str == '\0') { rt_handle_end_transaction(str_h); return 1; }

    const char *p = str;
    while (*p) {
        if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && *p != '\v' && *p != '\f') {
            rt_handle_end_transaction(str_h);
            return 0;
        }
        p++;
    }
    rt_handle_end_transaction(str_h);
    return 1;
}

/* ============================================================================
 * String Parse Functions (Handle-Based)
 * ============================================================================ */

long long rt_str_to_int_v2(RtHandleV2 *str_h) {
    if (!str_h) return 0;
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    if (!str || *str == '\0') { rt_handle_end_transaction(str_h); return 0; }

    long long result = 0;
    int negative = 0;
    const char *p = str;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '-') { negative = 1; p++; } else if (*p == '+') { p++; }
    while (*p >= '0' && *p <= '9') { result = result * 10 + (*p - '0'); p++; }

    rt_handle_end_transaction(str_h);
    return negative ? -result : result;
}

long long rt_str_to_long_v2(RtHandleV2 *str_h) {
    return rt_str_to_int_v2(str_h);
}

double rt_str_to_double_v2(RtHandleV2 *str_h) {
    if (!str_h) return 0.0;
    rt_handle_begin_transaction(str_h);
    const char *str = (const char *)str_h->ptr;
    if (!str || *str == '\0') { rt_handle_end_transaction(str_h); return 0.0; }

    double result = 0.0;
    double fraction = 0.0;
    double fraction_div = 1.0;
    int negative = 0;
    int in_fraction = 0;
    int exp_negative = 0;
    int exponent = 0;
    const char *p = str;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '-') { negative = 1; p++; } else if (*p == '+') { p++; }
    while ((*p >= '0' && *p <= '9') || *p == '.') {
        if (*p == '.') { in_fraction = 1; p++; continue; }
        if (in_fraction) { fraction = fraction * 10 + (*p - '0'); fraction_div *= 10; }
        else { result = result * 10 + (*p - '0'); }
        p++;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-') { exp_negative = 1; p++; } else if (*p == '+') { p++; }
        while (*p >= '0' && *p <= '9') { exponent = exponent * 10 + (*p - '0'); p++; }
    }
    result = result + fraction / fraction_div;
    if (negative) result = -result;
    if (exponent > 0) {
        double multiplier = 1.0;
        for (int i = 0; i < exponent; i++) multiplier *= 10.0;
        if (exp_negative) result /= multiplier; else result *= multiplier;
    }

    rt_handle_end_transaction(str_h);
    return result;
}
