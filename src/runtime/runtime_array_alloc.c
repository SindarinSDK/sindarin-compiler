#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runtime_array.h"

/* ============================================================================
 * Array Alloc Functions Implementation
 * ============================================================================
 * Creates arrays of specified size filled with default value.
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

long long *rt_array_alloc_long(RtArena *arena, size_t count, long long default_value) {
    size_t data_size = count * sizeof(long long);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_long: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long long *data = (long long *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value) {
    size_t data_size = count * sizeof(double);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_double: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    double *data = (double *)(meta + 1);
    if (default_value == 0.0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value) {
    size_t data_size = count * sizeof(char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_char: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char *data = (char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value) {
    size_t data_size = count * sizeof(int);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_bool: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    int *data = (int *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value) {
    size_t data_size = count * sizeof(unsigned char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_byte: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    unsigned char *data = (unsigned char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        memset(data, default_value, data_size);  /* memset works perfectly for bytes */
    }
    return data;
}

char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value) {
    size_t data_size = count * sizeof(char *);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char **data = (char **)(meta + 1);
    if (default_value == NULL) {
        memset(data, 0, data_size);  /* Set all pointers to NULL */
    } else {
        for (size_t i = 0; i < count; i++) {
            data[i] = rt_arena_strdup(arena, default_value);
        }
    }
    return data;
}

int32_t *rt_array_alloc_int32(RtArena *arena, size_t count, int32_t default_value) {
    size_t data_size = count * sizeof(int32_t);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_int32: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    int32_t *data = (int32_t *)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        data[i] = default_value;
    }
    return data;
}

uint32_t *rt_array_alloc_uint32(RtArena *arena, size_t count, uint32_t default_value) {
    size_t data_size = count * sizeof(uint32_t);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_uint32: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    uint32_t *data = (uint32_t *)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        data[i] = default_value;
    }
    return data;
}

uint64_t *rt_array_alloc_uint(RtArena *arena, size_t count, uint64_t default_value) {
    size_t data_size = count * sizeof(uint64_t);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_uint: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    uint64_t *data = (uint64_t *)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        data[i] = default_value;
    }
    return data;
}

float *rt_array_alloc_float(RtArena *arena, size_t count, float default_value) {
    size_t data_size = count * sizeof(float);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_float: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    float *data = (float *)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        data[i] = default_value;
    }
    return data;
}
