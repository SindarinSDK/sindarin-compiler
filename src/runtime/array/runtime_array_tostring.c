#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array ToString Functions Implementation
 * ============================================================================
 * Convert array to string representation for interpolation.
 * Format: {elem1, elem2, elem3}
 * ============================================================================ */

char *rt_to_string_array_long(RtArena *arena, long long *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    /* Estimate: "{" + numbers (up to 20 chars each) + ", " separators + "}" */
    size_t buf_size = 2 + len * 22;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%lld", arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_double(RtArena *arena, double *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    size_t buf_size = 2 + len * 32;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%g", arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_char(RtArena *arena, char *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    /* Each char: 'x' = 3 chars, plus ", " */
    size_t buf_size = 2 + len * 5;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        *p++ = '\'';
        *p++ = arr[i];
        *p++ = '\'';
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_bool(RtArena *arena, int *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    /* "true" = 4, "false" = 5, plus ", " */
    size_t buf_size = 2 + len * 8;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        const char *val = arr[i] ? "true" : "false";
        while (*val) *p++ = *val++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_byte(RtArena *arena, unsigned char *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    /* Each byte: up to 3 digits, plus ", " */
    size_t buf_size = 2 + len * 6;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%u", arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_string(RtArena *arena, char **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);

    /* Calculate required size: strings with quotes */
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        if (i > 0) total_len += 2; /* ", " */
        if (arr[i]) {
            total_len += strlen(arr[i]) + 2; /* "str" */
        } else {
            total_len += 4; /* null */
        }
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        if (arr[i]) {
            *p++ = '"';
            const char *s = arr[i];
            while (*s) *p++ = *s++;
            *p++ = '"';
        } else {
            const char *null_str = "null";
            while (*null_str) *p++ = *null_str++;
        }
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_any(RtArena *arena, RtAny *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);

    /* First pass: convert each element to string and calculate total length */
    char **elem_strs = rt_arena_alloc(arena, len * sizeof(char *));
    if (elem_strs == NULL) return "{}";

    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        elem_strs[i] = rt_any_to_string(arena, arr[i]);
        if (i > 0) total_len += 2; /* ", " */
        total_len += strlen(elem_strs[i]);
    }

    /* Build result string */
    char *result = rt_arena_alloc(arena, total_len + 1);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_int32(RtArena *arena, int32_t *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    /* Estimate: "{" + numbers (up to 11 chars each) + ", " separators + "}" */
    size_t buf_size = 2 + len * 13;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%d", arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_uint32(RtArena *arena, uint32_t *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    size_t buf_size = 2 + len * 13;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%u", arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_uint(RtArena *arena, uint64_t *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    size_t buf_size = 2 + len * 22;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%lu", (unsigned long)arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array_float(RtArena *arena, float *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t len = rt_array_length(arr);
    size_t buf_size = 2 + len * 32;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) return "{}";

    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < len; i++) {
        if (i > 0) {
            *p++ = ',';
            *p++ = ' ';
        }
        p += snprintf(p, buf_size - (p - result), "%g", (double)arr[i]);
    }

    *p++ = '}';
    *p = '\0';
    return result;
}
