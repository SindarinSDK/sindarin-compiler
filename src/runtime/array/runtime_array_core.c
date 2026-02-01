#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Core Operations Implementation
 * ============================================================================
 * This module provides core array manipulation functions:
 * - Array clear
 * - Push operations (append element to array)
 * - Pop operations (remove and return last element)
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array Clear
 * ============================================================================ */

/* Clear all elements from an array (sets size to 0, keeps capacity) */
void rt_array_clear(void *arr) {
    if (arr == NULL) return;
    RtArrayMetadata *meta = &((RtArrayMetadata *)arr)[-1];
    meta->size = 0;
}

/* ============================================================================
 * Array Push Operations
 * ============================================================================
 * Push element to end of array, growing capacity if needed.
 * Uses arena allocation - when capacity is exceeded, allocates new array and copies.
 * The arena parameter is used only for NEW arrays; existing arrays use their stored arena.
 *
 * Special case: If arena is NULL, uses malloc for allocation. This is used for
 * global arrays that need to persist beyond any function's lifetime.
 */

#define DEFINE_ARRAY_PUSH(suffix, elem_type, assign_expr)                      \
elem_type *rt_array_push_##suffix(RtArena *arena, elem_type *arr, elem_type element) { \
    ArrayMetadata *meta;                                                       \
    elem_type *new_arr;                                                        \
    size_t new_capacity;                                                       \
    RtArena *alloc_arena;                                                      \
                                                                               \
    if (arr == NULL) {                                                         \
        meta = rt_array_alloc_mem(arena, sizeof(ArrayMetadata) + 4 * sizeof(elem_type)); \
        if (meta == NULL) {                                                    \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        meta->arena = arena;                                                   \
        meta->size = 1;                                                        \
        meta->capacity = 4;                                                    \
        new_arr = (elem_type *)(meta + 1);                                     \
        new_arr[0] = assign_expr;                                              \
        return new_arr;                                                        \
    }                                                                          \
                                                                               \
    meta = ((ArrayMetadata *)arr) - 1;                                         \
    alloc_arena = meta->arena ? meta->arena : arena;                           \
                                                                               \
    if (meta->size >= meta->capacity) {                                        \
        new_capacity = meta->capacity == 0 ? 4 : meta->capacity * 2;           \
        if (new_capacity < meta->capacity) {                                   \
            fprintf(stderr, "rt_array_push_" #suffix ": capacity overflow\n"); \
            exit(1);                                                           \
        }                                                                      \
        ArrayMetadata *new_meta = rt_array_alloc_mem(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(elem_type)); \
        if (new_meta == NULL) {                                                \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        new_meta->arena = alloc_arena;                                         \
        new_meta->size = meta->size;                                           \
        new_meta->capacity = new_capacity;                                     \
        new_arr = (elem_type *)(new_meta + 1);                                 \
        memcpy(new_arr, arr, meta->size * sizeof(elem_type));                  \
        meta = new_meta;                                                       \
    } else {                                                                   \
        new_arr = arr;                                                         \
    }                                                                          \
                                                                               \
    new_arr[meta->size] = assign_expr;                                         \
    meta->size++;                                                              \
    return new_arr;                                                            \
}

/* Generate array push functions for each type */
DEFINE_ARRAY_PUSH(long, long long, element)
DEFINE_ARRAY_PUSH(double, double, element)
DEFINE_ARRAY_PUSH(char, char, element)
DEFINE_ARRAY_PUSH(bool, int, element)
DEFINE_ARRAY_PUSH(byte, unsigned char, element)
DEFINE_ARRAY_PUSH(ptr, void *, element)  /* For closures/function pointers and other pointer types */
DEFINE_ARRAY_PUSH(any, RtAny, element)   /* For any[] arrays */
DEFINE_ARRAY_PUSH(int32, int32_t, element)
DEFINE_ARRAY_PUSH(uint32, uint32_t, element)
DEFINE_ARRAY_PUSH(uint, uint64_t, element)
DEFINE_ARRAY_PUSH(float, float, element)

/* Generic struct push - copies element by value using memcpy */
void *rt_array_push_struct(RtArena *arena, void *arr, const void *element, size_t elem_size) {
    ArrayMetadata *meta;
    void *new_arr;
    size_t new_capacity;
    RtArena *alloc_arena;

    if (arr == NULL) {
        meta = rt_array_alloc_mem(arena, sizeof(ArrayMetadata) + 4 * elem_size);
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_struct: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (void *)(meta + 1);
        memcpy(new_arr, element, elem_size);
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;
    alloc_arena = meta->arena ? meta->arena : arena;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity == 0 ? 4 : meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_struct: capacity overflow\n");
            exit(1);
        }
        ArrayMetadata *new_meta = rt_array_alloc_mem(alloc_arena, sizeof(ArrayMetadata) + new_capacity * elem_size);
        if (new_meta == NULL) {
            fprintf(stderr, "rt_array_push_struct: reallocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        new_arr = (void *)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * elem_size);
        meta = new_meta;
    } else {
        new_arr = arr;
    }

    memcpy((char *)new_arr + meta->size * elem_size, element, elem_size);
    meta->size++;
    return new_arr;
}

/* String arrays need special handling for strdup */
char **rt_array_push_string(RtArena *arena, char **arr, const char *element) {
    ArrayMetadata *meta;
    char **new_arr;
    size_t new_capacity;
    RtArena *alloc_arena;

    if (arr == NULL) {
        meta = rt_array_alloc_mem(arena, sizeof(ArrayMetadata) + 4 * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (char **)(meta + 1);
        new_arr[0] = rt_array_strdup_mem(arena, element);
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;
    alloc_arena = meta->arena ? meta->arena : arena;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity == 0 ? 4 : meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_string: capacity overflow\n");
            exit(1);
        }
        ArrayMetadata *new_meta = rt_array_alloc_mem(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        meta = new_meta;
    } else {
        new_arr = arr;
    }

    new_arr[meta->size] = rt_array_strdup_mem(alloc_arena, element);
    meta->size++;
    return new_arr;
}

/* ============================================================================
 * Array Pop Operations
 * ============================================================================
 * Remove and return the last element of an array.
 * Decrements size but does not free memory.
 */
#define DEFINE_ARRAY_POP(suffix, elem_type, default_val)                       \
elem_type rt_array_pop_##suffix(elem_type *arr) {                              \
    if (arr == NULL) {                                                         \
        fprintf(stderr, "rt_array_pop_" #suffix ": NULL array\n");             \
        exit(1);                                                               \
    }                                                                          \
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;                          \
    if (meta->size == 0) {                                                     \
        fprintf(stderr, "rt_array_pop_" #suffix ": empty array\n");            \
        exit(1);                                                               \
    }                                                                          \
    meta->size--;                                                              \
    return arr[meta->size];                                                    \
}

/* Generate array pop functions for each type */
DEFINE_ARRAY_POP(long, long long, 0)
DEFINE_ARRAY_POP(double, double, 0.0)
DEFINE_ARRAY_POP(char, char, '\0')
DEFINE_ARRAY_POP(bool, int, 0)
DEFINE_ARRAY_POP(byte, unsigned char, 0)
DEFINE_ARRAY_POP(ptr, void *, NULL)  /* For closures/function pointers and other pointer types */
DEFINE_ARRAY_POP(int32, int32_t, 0)
DEFINE_ARRAY_POP(uint32, uint32_t, 0)
DEFINE_ARRAY_POP(uint, uint64_t, 0)
DEFINE_ARRAY_POP(float, float, 0.0f)

/* String arrays - same implementation, no special handling needed for pop */
char *rt_array_pop_string(char **arr) {
    if (arr == NULL) {
        fprintf(stderr, "rt_array_pop_string: NULL array\n");
        exit(1);
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    if (meta->size == 0) {
        fprintf(stderr, "rt_array_pop_string: empty array\n");
        exit(1);
    }
    meta->size--;
    return arr[meta->size];
}
