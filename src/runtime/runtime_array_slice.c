#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Slice Operations Implementation
 * ============================================================================
 * This module provides array slicing and concatenation functions:
 * - Concat operations (combine two arrays)
 * - Slice operations (extract portion of array)
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array Concat Operations
 * ============================================================================
 * Return a NEW array containing elements from both arrays (non-mutating).
 * Both source arrays remain unchanged.
 */
#define DEFINE_ARRAY_CONCAT(suffix, elem_type)                                 \
elem_type *rt_array_concat_##suffix(RtArena *arena, elem_type *arr1, elem_type *arr2) { \
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;                            \
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;                            \
    size_t total = len1 + len2;                                                \
    size_t capacity = total > 4 ? total : 4;                                   \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                        \
        fprintf(stderr, "rt_array_concat_" #suffix ": allocation failed\n");   \
        exit(1);                                                               \
    }                                                                          \
    meta->arena = arena;                                                       \
    meta->size = total;                                                        \
    meta->capacity = capacity;                                                 \
    elem_type *result = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len1; i++) {                                        \
        result[i] = arr1[i];                                                   \
    }                                                                          \
    for (size_t i = 0; i < len2; i++) {                                        \
        result[len1 + i] = arr2[i];                                            \
    }                                                                          \
    return result;                                                             \
}

/* Generate array concat functions for each type */
DEFINE_ARRAY_CONCAT(long, long long)
DEFINE_ARRAY_CONCAT(double, double)
DEFINE_ARRAY_CONCAT(char, char)
DEFINE_ARRAY_CONCAT(bool, int)
DEFINE_ARRAY_CONCAT(byte, unsigned char)
DEFINE_ARRAY_CONCAT(ptr, void *)  /* For closures/function pointers and other pointer types */
DEFINE_ARRAY_CONCAT(int32, int32_t)
DEFINE_ARRAY_CONCAT(uint32, uint32_t)
DEFINE_ARRAY_CONCAT(uint, uint64_t)
DEFINE_ARRAY_CONCAT(float, float)

/* String arrays need special handling for strdup */
char **rt_array_concat_string(RtArena *arena, char **arr1, char **arr2) {
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;
    size_t total = len1 + len2;
    size_t capacity = total > 4 ? total : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_concat_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = total;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);
    for (size_t i = 0; i < len1; i++) {
        result[i] = arr1[i] ? rt_arena_strdup(arena, arr1[i]) : NULL;
    }
    for (size_t i = 0; i < len2; i++) {
        result[len1 + i] = arr2[i] ? rt_arena_strdup(arena, arr2[i]) : NULL;
    }
    return result;
}

/* ============================================================================
 * Array Slice Operations
 * ============================================================================
 * Create a new array from a portion of the source array.
 * start: starting index (inclusive), use LONG_MIN for beginning
 * end: ending index (exclusive), use LONG_MIN for end of array
 * step: step size, use LONG_MIN for default step of 1
 */
#define DEFINE_ARRAY_SLICE(suffix, elem_type)                                   \
elem_type *rt_array_slice_##suffix(RtArena *arena, elem_type *arr, long start, long end, long step) { \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    /* Handle step: LONG_MIN means step of 1 */                                 \
    long actual_step = (step == LONG_MIN) ? 1 : step;                           \
    if (actual_step <= 0) {                                                     \
        fprintf(stderr, "rt_array_slice_" #suffix ": step must be positive\n"); \
        return NULL;                                                            \
    }                                                                           \
    /* Handle start: LONG_MIN means "from beginning", negative means from end */\
    long actual_start;                                                          \
    if (start == LONG_MIN) {                                                    \
        actual_start = 0;                                                       \
    } else if (start < 0) {                                                     \
        actual_start = (long)len + start;                                       \
        if (actual_start < 0) actual_start = 0;                                 \
    } else {                                                                    \
        actual_start = start;                                                   \
    }                                                                           \
    /* Handle end: LONG_MIN means "to end", negative means from end */          \
    long actual_end;                                                            \
    if (end == LONG_MIN) {                                                      \
        actual_end = (long)len;                                                 \
    } else if (end < 0) {                                                       \
        actual_end = (long)len + end;                                           \
        if (actual_end < 0) actual_end = 0;                                     \
    } else {                                                                    \
        actual_end = end;                                                       \
    }                                                                           \
    if (actual_start > (long)len) actual_start = (long)len;                     \
    if (actual_end > (long)len) actual_end = (long)len;                         \
    if (actual_start >= actual_end) {                                           \
        return NULL;                                                            \
    }                                                                           \
    /* Calculate slice length with step */                                      \
    size_t range = (size_t)(actual_end - actual_start);                         \
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step; \
    size_t capacity = slice_len > 4 ? slice_len : 4;                            \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_slice_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = slice_len;                                                     \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < slice_len; i++) {                                    \
        new_arr[i] = arr[actual_start + i * (size_t)actual_step];               \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array slice functions for each type */
DEFINE_ARRAY_SLICE(long, long long)
DEFINE_ARRAY_SLICE(double, double)
DEFINE_ARRAY_SLICE(char, char)
DEFINE_ARRAY_SLICE(bool, int)
DEFINE_ARRAY_SLICE(byte, unsigned char)
DEFINE_ARRAY_SLICE(int32, int32_t)
DEFINE_ARRAY_SLICE(uint32, uint32_t)
DEFINE_ARRAY_SLICE(uint, uint64_t)
DEFINE_ARRAY_SLICE(float, float)

/* String slice needs special handling to strdup elements */
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    /* Handle step: LONG_MIN means step of 1 */
    long actual_step = (step == LONG_MIN) ? 1 : step;
    if (actual_step <= 0) {
        fprintf(stderr, "rt_array_slice_string: step must be positive\n");
        return NULL;
    }
    /* Handle start: LONG_MIN means "from beginning", negative means from end */
    long actual_start;
    if (start == LONG_MIN) {
        actual_start = 0;
    } else if (start < 0) {
        actual_start = (long)len + start;
        if (actual_start < 0) actual_start = 0;
    } else {
        actual_start = start;
    }
    /* Handle end: LONG_MIN means "to end", negative means from end */
    long actual_end;
    if (end == LONG_MIN) {
        actual_end = (long)len;
    } else if (end < 0) {
        actual_end = (long)len + end;
        if (actual_end < 0) actual_end = 0;
    } else {
        actual_end = end;
    }
    if (actual_start > (long)len) actual_start = (long)len;
    if (actual_end > (long)len) actual_end = (long)len;
    if (actual_start >= actual_end) {
        return NULL;
    }
    /* Calculate slice length with step */
    size_t range = (size_t)(actual_end - actual_start);
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step;
    size_t capacity = slice_len > 4 ? slice_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_slice_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = slice_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < slice_len; i++) {
        size_t src_idx = actual_start + i * (size_t)actual_step;
        new_arr[i] = arr[src_idx] ? rt_arena_strdup(arena, arr[src_idx]) : NULL;
    }
    return new_arr;
}
