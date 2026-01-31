#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Search Operations Implementation
 * ============================================================================
 * This module provides array search and copy functions:
 * - IndexOf (find first index of element)
 * - Contains (check if element exists)
 * - Clone (create deep copy)
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array IndexOf Functions
 * ============================================================================
 * Find first index of element, returns -1 if not found.
 */
#define DEFINE_ARRAY_INDEXOF(suffix, elem_type, compare_expr)                   \
long rt_array_indexOf_##suffix(elem_type *arr, elem_type elem) {                \
    if (arr == NULL) {                                                          \
        return -1L;                                                             \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    for (size_t i = 0; i < len; i++) {                                          \
        if (compare_expr) {                                                     \
            return (long)i;                                                     \
        }                                                                       \
    }                                                                           \
    return -1L;                                                                 \
}

/* Generate array indexOf functions for each type */
DEFINE_ARRAY_INDEXOF(long, long long, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(double, double, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(char, char, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(bool, int, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(byte, unsigned char, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(int32, int32_t, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(uint32, uint32_t, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(uint, uint64_t, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(float, float, arr[i] == elem)

/* String indexOf needs special comparison */
long rt_array_indexOf_string(char **arr, const char *elem) {
    if (arr == NULL) {
        return -1L;
    }
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        if (arr[i] == NULL && elem == NULL) {
            return (long)i;
        }
        if (arr[i] != NULL && elem != NULL && strcmp(arr[i], elem) == 0) {
            return (long)i;
        }
    }
    return -1L;
}

/* ============================================================================
 * Array Contains Functions
 * ============================================================================
 * Check if element exists in array.
 */
#define DEFINE_ARRAY_CONTAINS(suffix, elem_type)                                \
int rt_array_contains_##suffix(elem_type *arr, elem_type elem) {                \
    return rt_array_indexOf_##suffix(arr, elem) >= 0;                           \
}

/* Generate array contains functions for each type */
DEFINE_ARRAY_CONTAINS(long, long long)
DEFINE_ARRAY_CONTAINS(double, double)
DEFINE_ARRAY_CONTAINS(char, char)
DEFINE_ARRAY_CONTAINS(bool, int)
DEFINE_ARRAY_CONTAINS(byte, unsigned char)
DEFINE_ARRAY_CONTAINS(int32, int32_t)
DEFINE_ARRAY_CONTAINS(uint32, uint32_t)
DEFINE_ARRAY_CONTAINS(uint, uint64_t)
DEFINE_ARRAY_CONTAINS(float, float)

/* String contains uses string indexOf */
int rt_array_contains_string(char **arr, const char *elem) {
    return rt_array_indexOf_string(arr, elem) >= 0;
}

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a deep copy of the array.
 */
#define DEFINE_ARRAY_CLONE(suffix, elem_type)                                   \
elem_type *rt_array_clone_##suffix(RtArena *arena, elem_type *arr) {            \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_clone_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    memcpy(new_arr, arr, len * sizeof(elem_type));                              \
    return new_arr;                                                             \
}

/* Generate array clone functions for each type */
DEFINE_ARRAY_CLONE(long, long long)
DEFINE_ARRAY_CLONE(double, double)
DEFINE_ARRAY_CLONE(char, char)
DEFINE_ARRAY_CLONE(bool, int)
DEFINE_ARRAY_CLONE(byte, unsigned char)
DEFINE_ARRAY_CLONE(int32, int32_t)
DEFINE_ARRAY_CLONE(uint32, uint32_t)
DEFINE_ARRAY_CLONE(uint, uint64_t)
DEFINE_ARRAY_CLONE(float, float)

/* String clone needs special handling to strdup elements */
char **rt_array_clone_string(RtArena *arena, char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_clone_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    return new_arr;
}
