#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Modify Operations Implementation
 * ============================================================================
 * This module provides array modification functions:
 * - Reverse (create reversed copy)
 * - Remove at index (create copy without element)
 * - Insert at index (create copy with element inserted)
 * - Push copy (create copy with element appended)
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return a new reversed array - the original array is not modified.
 */
#define DEFINE_ARRAY_REV(suffix, elem_type)                                     \
elem_type *rt_array_rev_##suffix(RtArena *arena, elem_type *arr) {              \
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
        fprintf(stderr, "rt_array_rev_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[len - 1 - i];                                          \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array reverse functions for each type */
DEFINE_ARRAY_REV(long, long long)
DEFINE_ARRAY_REV(double, double)
DEFINE_ARRAY_REV(char, char)
DEFINE_ARRAY_REV(bool, int)
DEFINE_ARRAY_REV(byte, unsigned char)
DEFINE_ARRAY_REV(int32, int32_t)
DEFINE_ARRAY_REV(uint32, uint32_t)
DEFINE_ARRAY_REV(uint, uint64_t)
DEFINE_ARRAY_REV(float, float)

/* String reverse needs special handling to strdup elements */
char **rt_array_rev_string(RtArena *arena, char **arr) {
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
        fprintf(stderr, "rt_array_rev_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[len - 1 - i] ? rt_arena_strdup(arena, arr[len - 1 - i]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return a new array without the element at the specified index.
 */
#define DEFINE_ARRAY_REM(suffix, elem_type)                                     \
elem_type *rt_array_rem_##suffix(RtArena *arena, elem_type *arr, long index) {  \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (index < 0 || (size_t)index >= len) {                                    \
        fprintf(stderr, "rt_array_rem_" #suffix ": index out of bounds\n");     \
        exit(1);                                                                \
    }                                                                           \
    if (len == 1) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t new_len = len - 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_rem_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    for (size_t i = (size_t)index; i < new_len; i++) {                          \
        new_arr[i] = arr[i + 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array remove functions for each type */
DEFINE_ARRAY_REM(long, long long)
DEFINE_ARRAY_REM(double, double)
DEFINE_ARRAY_REM(char, char)
DEFINE_ARRAY_REM(bool, int)
DEFINE_ARRAY_REM(byte, unsigned char)
DEFINE_ARRAY_REM(int32, int32_t)
DEFINE_ARRAY_REM(uint32, uint32_t)
DEFINE_ARRAY_REM(uint, uint64_t)
DEFINE_ARRAY_REM(float, float)

/* String remove needs special handling to strdup elements */
char **rt_array_rem_string(RtArena *arena, char **arr, long index) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (index < 0 || (size_t)index >= len) {
        fprintf(stderr, "rt_array_rem_string: index out of bounds\n");
        exit(1);
    }
    if (len == 1) {
        return NULL;
    }
    size_t new_len = len - 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rem_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    for (size_t i = (size_t)index; i < new_len; i++) {
        new_arr[i] = arr[i + 1] ? rt_arena_strdup(arena, arr[i + 1]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return a new array with the element inserted at the specified index.
 */
#define DEFINE_ARRAY_INS(suffix, elem_type)                                     \
elem_type *rt_array_ins_##suffix(RtArena *arena, elem_type *arr, elem_type elem, long index) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    if (index < 0) index = 0;                                                   \
    if ((size_t)index > len) index = (long)len;                                 \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_ins_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[index] = elem;                                                      \
    for (size_t i = (size_t)index + 1; i < new_len; i++) {                      \
        new_arr[i] = arr[i - 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array insert functions for each type */
DEFINE_ARRAY_INS(long, long long)
DEFINE_ARRAY_INS(double, double)
DEFINE_ARRAY_INS(char, char)
DEFINE_ARRAY_INS(bool, int)
DEFINE_ARRAY_INS(byte, unsigned char)
DEFINE_ARRAY_INS(int32, int32_t)
DEFINE_ARRAY_INS(uint32, uint32_t)
DEFINE_ARRAY_INS(uint, uint64_t)
DEFINE_ARRAY_INS(float, float)

/* String insert needs special handling to strdup elements */
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index) {
    size_t len = arr ? rt_array_length(arr) : 0;
    if (index < 0) index = 0;
    if ((size_t)index > len) index = (long)len;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_ins_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[index] = elem ? rt_arena_strdup(arena, elem) : NULL;
    for (size_t i = (size_t)index + 1; i < new_len; i++) {
        new_arr[i] = arr[i - 1] ? rt_arena_strdup(arena, arr[i - 1]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Push Copy Functions (Non-mutating)
 * ============================================================================
 * Create a NEW array with element appended - the original is not modified.
 */
#define DEFINE_ARRAY_PUSH_COPY(suffix, elem_type)                               \
elem_type *rt_array_push_copy_##suffix(RtArena *arena, elem_type *arr, elem_type elem) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_push_copy_" #suffix ": allocation failed\n"); \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[len] = elem;                                                        \
    return new_arr;                                                             \
}

/* Generate array push copy functions for each type */
DEFINE_ARRAY_PUSH_COPY(long, long long)
DEFINE_ARRAY_PUSH_COPY(double, double)
DEFINE_ARRAY_PUSH_COPY(char, char)
DEFINE_ARRAY_PUSH_COPY(bool, int)
DEFINE_ARRAY_PUSH_COPY(byte, unsigned char)
DEFINE_ARRAY_PUSH_COPY(int32, int32_t)
DEFINE_ARRAY_PUSH_COPY(uint32, uint32_t)
DEFINE_ARRAY_PUSH_COPY(uint, uint64_t)
DEFINE_ARRAY_PUSH_COPY(float, float)

/* String push copy needs special handling to strdup elements */
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem) {
    size_t len = arr ? rt_array_length(arr) : 0;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_push_copy_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[len] = elem ? rt_arena_strdup(arena, elem) : NULL;
    return new_arr;
}
