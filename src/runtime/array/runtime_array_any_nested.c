#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Any Nested Conversion Functions Implementation
 * ============================================================================
 * Convert 2D and 3D typed arrays to any[][] and any[][][].
 * ============================================================================ */

/* 2D array to any[][] conversion functions */

RtAny **rt_array2_to_any_long(RtArena *arena, long long **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_long(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny **rt_array2_to_any_double(RtArena *arena, double **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_double(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny **rt_array2_to_any_char(RtArena *arena, char **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_char(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny **rt_array2_to_any_bool(RtArena *arena, int **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_bool(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny **rt_array2_to_any_byte(RtArena *arena, unsigned char **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_byte(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny **rt_array2_to_any_string(RtArena *arena, char ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny **result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *inner = rt_array_to_any_string(arena, arr[i]);
        result = (RtAny **)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

/* 3D array to any[][][] conversion functions */

RtAny ***rt_array3_to_any_long(RtArena *arena, long long ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_long(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny ***rt_array3_to_any_double(RtArena *arena, double ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_double(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny ***rt_array3_to_any_char(RtArena *arena, char ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_char(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny ***rt_array3_to_any_bool(RtArena *arena, int ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_bool(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny ***rt_array3_to_any_byte(RtArena *arena, unsigned char ***arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_byte(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}

RtAny ***rt_array3_to_any_string(RtArena *arena, char ****arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny ***result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny **inner = rt_array2_to_any_string(arena, arr[i]);
        result = (RtAny ***)rt_array_push_ptr(arena, (void **)result, (void *)inner);
    }
    return result;
}
