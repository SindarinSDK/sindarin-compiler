#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Any Conversion Functions Implementation
 * ============================================================================
 * Convert typed arrays to any[] (boxing) and any[] to typed arrays (unboxing).
 * ============================================================================ */

/* ============================================================================
 * Array to Any Conversion Functions
 * ============================================================================
 * Convert typed arrays to any[] by boxing each element.
 */

RtAny *rt_array_to_any_long(RtArena *arena, long long *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_long(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_double(RtArena *arena, double *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_double(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_char(RtArena *arena, char *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_char(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_bool(RtArena *arena, int *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_bool(arr[i] != 0));
    }
    return result;
}

RtAny *rt_array_to_any_byte(RtArena *arena, unsigned char *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_byte(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_string(RtArena *arena, char **arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_string(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_int32(RtArena *arena, int32_t *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_int32(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_uint32(RtArena *arena, uint32_t *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_uint32(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_uint(RtArena *arena, uint64_t *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_uint(arr[i]));
    }
    return result;
}

RtAny *rt_array_to_any_float(RtArena *arena, float *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any(arena, result, rt_box_float(arr[i]));
    }
    return result;
}

/* ============================================================================
 * Any Array to Typed Array Conversion Functions
 * ============================================================================
 * Convert any[] to typed arrays by unboxing each element.
 */

long long *rt_array_from_any_long(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    long long *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_long(arena, result, rt_unbox_long(arr[i]));
    }
    return result;
}

double *rt_array_from_any_double(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    double *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_double(arena, result, rt_unbox_double(arr[i]));
    }
    return result;
}

char *rt_array_from_any_char(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    char *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_char(arena, result, rt_unbox_char(arr[i]));
    }
    return result;
}

int *rt_array_from_any_bool(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    int *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_bool(arena, result, rt_unbox_bool(arr[i]) ? 1 : 0);
    }
    return result;
}

unsigned char *rt_array_from_any_byte(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    unsigned char *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_byte(arena, result, rt_unbox_byte(arr[i]));
    }
    return result;
}

char **rt_array_from_any_string(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    char **result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_string(arena, result, rt_unbox_string(arr[i]));
    }
    return result;
}

int32_t *rt_array_from_any_int32(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    int32_t *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_int32(arena, result, rt_unbox_int32(arr[i]));
    }
    return result;
}

uint32_t *rt_array_from_any_uint32(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    uint32_t *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_uint32(arena, result, rt_unbox_uint32(arr[i]));
    }
    return result;
}

uint64_t *rt_array_from_any_uint(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    uint64_t *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_uint(arena, result, rt_unbox_uint(arr[i]));
    }
    return result;
}

float *rt_array_from_any_float(RtArena *arena, RtAny *arr) {
    if (arr == NULL) return NULL;
    size_t len = rt_array_length(arr);
    if (len == 0) return NULL;

    float *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_float(arena, result, rt_unbox_float(arr[i]));
    }
    return result;
}
