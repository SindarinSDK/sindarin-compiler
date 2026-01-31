#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Nested Array ToString Functions Implementation
 * ============================================================================
 * Convert nested arrays (2D/3D) to string representation.
 * Format: {{elem1, elem2}, {elem3, elem4}}
 * ============================================================================ */

char *rt_to_string_array2_long(RtArena *arena, long long **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);

    /* First pass: convert all inner arrays and calculate total size */
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2; /* {} */

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_long(arena, arr[i]);
        if (i > 0) total_len += 2; /* ", " */
        total_len += strlen(inner_strs[i]);
    }

    /* Build result string */
    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_double(RtArena *arena, double **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_double(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_char(RtArena *arena, char **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_char(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_bool(RtArena *arena, int **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_bool(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_byte(RtArena *arena, unsigned char **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_byte(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_string(RtArena *arena, char ***arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_string(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_any(RtArena *arena, RtAny **arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_any(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_any(RtArena *arena, RtAny ***arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "{}");
    }

    size_t outer_len = rt_array_length(arr);
    char **inner_strs = rt_arena_alloc(arena, outer_len * sizeof(char *));
    size_t total_len = 2;

    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array2_any(arena, arr[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    char *result = rt_arena_alloc(arena, total_len + 1);
    char *p = result;
    *p++ = '{';

    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }

    *p++ = '}';
    *p = '\0';
    return result;
}
