#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Create Functions Implementation
 * ============================================================================
 * Create runtime arrays from static C arrays, equality comparison, and range.
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create runtime array from static C array.
 */
#define DEFINE_ARRAY_CREATE(suffix, elem_type)                                  \
elem_type *rt_array_create_##suffix(RtArena *arena, size_t count, const elem_type *data) { \
    /* Always create array with metadata to track arena, even for empty arrays */ \
    size_t capacity = count > 4 ? count : 4;                                    \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_create_" #suffix ": allocation failed\n");    \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = count;                                                         \
    meta->capacity = capacity;                                                  \
    elem_type *arr = (elem_type *)(meta + 1);                                   \
    for (size_t i = 0; i < count; i++) {                                        \
        if (data != NULL) arr[i] = data[i];                                     \
    }                                                                           \
    return arr;                                                                 \
}

/* Generate array create functions for each type */
DEFINE_ARRAY_CREATE(long, long long)
DEFINE_ARRAY_CREATE(double, double)
DEFINE_ARRAY_CREATE(char, char)
DEFINE_ARRAY_CREATE(bool, int)
DEFINE_ARRAY_CREATE(byte, unsigned char)
DEFINE_ARRAY_CREATE(any, RtAny)
DEFINE_ARRAY_CREATE(int32, int32_t)
DEFINE_ARRAY_CREATE(uint32, uint32_t)
DEFINE_ARRAY_CREATE(uint, uint64_t)
DEFINE_ARRAY_CREATE(float, float)

/* Create an uninitialized byte array for filling in later (e.g., file reads) */
unsigned char *rt_array_create_byte_uninit(RtArena *arena, size_t count) {
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(unsigned char));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_byte_uninit: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    unsigned char *arr = (unsigned char *)(meta + 1);
    /* Zero-initialize for safety */
    memset(arr, 0, count);
    return arr;
}

/* String array create needs special handling for strdup */
char **rt_array_create_string(RtArena *arena, size_t count, const char **data) {
    /* Always create array with metadata to track arena, even for empty arrays */
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = (data && data[i]) ? rt_arena_strdup(arena, data[i]) : NULL;
    }
    return arr;
}

/* Pointer array create for nested arrays and function pointers */
void **rt_array_create_ptr(RtArena *arena, size_t count, void **data) {
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(void *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_ptr: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    void **arr = (void **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = data ? data[i] : NULL;
    }
    return arr;
}

/* Create array for arbitrary-sized elements (e.g., structs).
 * Allocates metadata + count * elem_size bytes and memcpy's data. */
void *rt_array_create_generic(RtArena *arena, size_t count, size_t elem_size, const void *data) {
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * elem_size);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_generic: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    void *arr = (void *)(meta + 1);
    if (count > 0 && data != NULL) {
        memcpy(arr, data, count * elem_size);
    }
    return arr;
}

/* Create str[] array from command-line arguments (argc/argv).
 * Copies all arguments into the arena and creates an array with metadata. */
char **rt_args_create(RtArena *arena, int argc, char **argv) {
    size_t count = (argc > 0) ? (size_t)argc : 0;
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_args_create: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = (argv && argv[i]) ? rt_arena_strdup(arena, argv[i]) : NULL;
    }
    return arr;
}

/* ============================================================================
 * Array Equality Functions
 * ============================================================================
 * Compare arrays element by element.
 */
#define DEFINE_ARRAY_EQ(suffix, elem_type, compare_expr)                        \
int rt_array_eq_##suffix(elem_type *a, elem_type *b) {                          \
    /* Both NULL means equal */                                                 \
    if (a == NULL && b == NULL) return 1;                                       \
    /* One NULL, one not means not equal */                                     \
    if (a == NULL || b == NULL) return 0;                                       \
    size_t len_a = rt_array_length(a);                                          \
    size_t len_b = rt_array_length(b);                                          \
    /* Different lengths means not equal */                                     \
    if (len_a != len_b) return 0;                                               \
    /* Compare element by element */                                            \
    for (size_t i = 0; i < len_a; i++) {                                        \
        if (!(compare_expr)) return 0;                                          \
    }                                                                           \
    return 1;                                                                   \
}

/* Generate array equality functions for each type */
DEFINE_ARRAY_EQ(long, long long, a[i] == b[i])
DEFINE_ARRAY_EQ(double, double, a[i] == b[i])
DEFINE_ARRAY_EQ(char, char, a[i] == b[i])
DEFINE_ARRAY_EQ(bool, int, a[i] == b[i])
DEFINE_ARRAY_EQ(byte, unsigned char, a[i] == b[i])
DEFINE_ARRAY_EQ(int32, int32_t, a[i] == b[i])
DEFINE_ARRAY_EQ(uint32, uint32_t, a[i] == b[i])
DEFINE_ARRAY_EQ(uint, uint64_t, a[i] == b[i])
DEFINE_ARRAY_EQ(float, float, a[i] == b[i])

/* String array equality needs strcmp */
int rt_array_eq_string(char **a, char **b) {
    /* Both NULL means equal */
    if (a == NULL && b == NULL) return 1;
    /* One NULL, one not means not equal */
    if (a == NULL || b == NULL) return 0;
    size_t len_a = rt_array_length(a);
    size_t len_b = rt_array_length(b);
    /* Different lengths means not equal */
    if (len_a != len_b) return 0;
    /* Compare element by element */
    for (size_t i = 0; i < len_a; i++) {
        /* Handle NULL strings */
        if (a[i] == NULL && b[i] == NULL) continue;
        if (a[i] == NULL || b[i] == NULL) return 0;
        if (strcmp(a[i], b[i]) != 0) return 0;
    }
    return 1;
}

/* ============================================================================
 * Array Range Function
 * ============================================================================
 * Creates long[] from start to end (exclusive).
 */
long long *rt_array_range(RtArena *arena, long long start, long long end) {
    /* Calculate count, handle both ascending and descending ranges */
    long long count;
    if (end >= start) {
        count = end - start;
    } else {
        /* Descending range: empty for now (future: could support negative step) */
        count = 0;
    }

    if (count <= 0) {
        /* Empty range */
        size_t capacity = 4;
        ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long long));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_range: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = capacity;
        return (long long *)(meta + 1);
    }

    size_t capacity = (size_t)count > 4 ? (size_t)count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long long));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_range: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = (size_t)count;
    meta->capacity = capacity;
    long long *arr = (long long *)(meta + 1);

    for (long long i = 0; i < count; i++) {
        arr[i] = start + i;
    }

    return arr;
}
