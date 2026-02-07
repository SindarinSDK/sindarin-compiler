/*
 * Runtime Array V2 Alloc Implementation
 * ======================================
 * Array allocation, range, interop conversion, and promotion functions.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * Array Alloc Functions (with default value)
 * ============================================================================ */

#define DEFINE_ARRAY_ALLOC_V2(suffix, elem_type)                                 \
RtHandleV2 *rt_array_alloc_##suffix##_v2(RtArenaV2 *arena, size_t count,         \
                                          elem_type default_value) {             \
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(elem_type);   \
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);                        \
    if (!h) return NULL;                                                         \
    void *raw = rt_handle_v2_pin(h);                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = count;                                                          \
    meta->capacity = count;                                                      \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    for (size_t i = 0; i < count; i++) {                                         \
        arr[i] = default_value;                                                  \
    }                                                                            \
    rt_handle_v2_unpin(h);                                                       \
    return h;                                                                    \
}

/* Type-specific alloc functions - needed for sized array syntax */
DEFINE_ARRAY_ALLOC_V2(long, long long)
DEFINE_ARRAY_ALLOC_V2(double, double)
DEFINE_ARRAY_ALLOC_V2(char, char)
DEFINE_ARRAY_ALLOC_V2(bool, int)
DEFINE_ARRAY_ALLOC_V2(byte, unsigned char)
DEFINE_ARRAY_ALLOC_V2(int32, int32_t)
DEFINE_ARRAY_ALLOC_V2(uint32, uint32_t)
DEFINE_ARRAY_ALLOC_V2(uint, uint64_t)
DEFINE_ARRAY_ALLOC_V2(float, float)

/* String alloc */
RtHandleV2 *rt_array_alloc_string_v2(RtArenaV2 *arena, size_t count, const char *default_value) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        arr[i] = rt_arena_v2_strdup(arena, default_value ? default_value : "");
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array Range Function
 * ============================================================================ */

RtHandleV2 *rt_array_range_v2(RtArenaV2 *arena, long long start, long long end) {
    size_t count = 0;
    if (end > start) {
        count = (size_t)(end - start);
    }

    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(long long);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    void *raw = rt_handle_v2_pin(h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long long *arr = (long long *)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        arr[i] = start + (long long)i;
    }

    rt_handle_v2_unpin(h);
    return h;
}

/* ============================================================================
 * Array from raw strings
 * ============================================================================ */

RtHandleV2 *rt_array_from_raw_strings_v2(RtArenaV2 *arena, const char **src) {
    if (src == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t count = get_array_len_from_data((void *)src);
    return rt_array_create_string_v2(arena, count, src);
}

/* Convert legacy char** array (from native functions) to V2 handle-based string array */
RtHandleV2 *rt_array_from_legacy_string_v2(RtArenaV2 *arena, char **src) {
    if (src == NULL) return rt_array_create_string_v2(arena, 0, NULL);
    size_t count = get_array_len_from_data((void *)src);
    return rt_array_create_string_v2(arena, count, (const char **)src);
}

/* ============================================================================
 * String Array Pin for Native Interop
 * ============================================================================
 * Converts a V2 string array (RtHandleV2* with RtHandleV2* string elements)
 * to a legacy char** for use with native C functions.
 * Each element is pinned to extract the raw char*.
 * ============================================================================ */

char **rt_pin_string_array_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    void *raw = rt_handle_v2_pin(arr_h);
    if (raw == NULL) return NULL;

    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    size_t count = meta->size;

    /* Get pointer to the array of string handles */
    RtHandleV2 **handles = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    /* Allocate a char** array from the arena (for compatibility with native functions) */
    RtArenaV2 *arena = rt_handle_v2_arena(arr_h);
    if (arena == NULL) return NULL;

    /* Allocate space for the char** result with null terminator */
    size_t alloc_size = (count + 1) * sizeof(char *);
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, alloc_size);
    char **result = (char **)rt_handle_v2_ptr(result_h);

    /* Pin each string element to extract char* */
    for (size_t i = 0; i < count; i++) {
        if (handles[i] != NULL) {
            result[i] = (char *)rt_handle_v2_pin(handles[i]);
        } else {
            result[i] = NULL;
        }
    }
    result[count] = NULL; /* Null terminator */

    return result;
}

/* ============================================================================
 * Args Creation
 * ============================================================================ */

RtHandleV2 *rt_args_create_v2(RtArenaV2 *arena, int argc, char **argv) {
    if (argc <= 0 || argv == NULL) {
        return rt_array_create_string_v2(arena, 0, NULL);
    }
    return rt_array_create_string_v2(arena, (size_t)argc, (const char **)argv);
}

/* ============================================================================
 * Deep Array Promotion
 * ============================================================================
 * V2 promotion is simpler - handle carries its arena reference!
 * No source arena parameter needed.
 * ============================================================================ */

/* Promote str[] - promotes array AND all string elements */
RtHandleV2 *rt_promote_array_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    /* Get source arena from handle */
    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;  /* Already in dest arena */

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Promote each string element */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_arena_v2_promote(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);

    /* Mark old array as dead */
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote T[][] - promotes outer array AND all inner array handles */
RtHandleV2 *rt_promote_array_handle_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Promote each inner array handle */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_arena_v2_promote(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote T[][][] - promotes all three levels of handles */
RtHandleV2 *rt_promote_array_handle_3d_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each 2D inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array_handle_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote str[][] - promotes outer, inner arrays, AND strings */
RtHandleV2 *rt_promote_array2_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each str[] inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array_string_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}

/* Promote str[][][] - promotes all three levels AND strings */
RtHandleV2 *rt_promote_array3_string_v2(RtArenaV2 *dest, RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;

    RtArenaV2 *src = rt_handle_v2_arena(arr_h);
    if (src == dest) return arr_h;

    size_t len = rt_array_length_v2(arr_h);
    size_t alloc_size = sizeof(RtArrayMetadataV2) + len * sizeof(RtHandleV2 *);

    RtHandleV2 *new_h = rt_arena_v2_alloc(dest, alloc_size);
    if (!new_h) return NULL;

    void *old_raw = rt_handle_v2_pin(arr_h);
    void *new_raw = rt_handle_v2_pin(new_h);

    RtArrayMetadataV2 *new_meta = (RtArrayMetadataV2 *)new_raw;
    new_meta->arena = dest;
    new_meta->size = len;
    new_meta->capacity = len;

    RtHandleV2 **old_arr = (RtHandleV2 **)((char *)old_raw + sizeof(RtArrayMetadataV2));
    RtHandleV2 **new_arr = (RtHandleV2 **)((char *)new_raw + sizeof(RtArrayMetadataV2));

    /* Recursively promote each str[][] inner array */
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = rt_promote_array2_string_v2(dest, old_arr[i]);
    }

    rt_handle_v2_unpin(new_h);
    rt_handle_v2_unpin(arr_h);
    rt_arena_v2_free(arr_h);

    return new_h;
}
