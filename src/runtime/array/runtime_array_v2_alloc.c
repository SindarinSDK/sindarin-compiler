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
    rt_handle_begin_transaction(h);                                                         \
    void *raw = h->ptr;                                             \
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;                          \
    meta->arena = arena;                                                         \
    meta->size = count;                                                          \
    meta->capacity = count;                                                      \
    elem_type *arr = (elem_type *)((char *)raw + sizeof(RtArrayMetadataV2));     \
    for (size_t i = 0; i < count; i++) {                                         \
        rt_handle_renew_transaction(h);                                          \
        arr[i] = default_value;                                                  \
    }                                                                            \
    rt_handle_end_transaction(h);                                                       \
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
RtHandleV2 *rt_array_alloc_string_v2(RtArenaV2 *arena, size_t count, RtHandleV2 *default_value) {
    size_t alloc_size = sizeof(RtArrayMetadataV2) + count * sizeof(RtHandleV2 *);
    RtHandleV2 *h = rt_arena_v2_alloc(arena, alloc_size);
    if (!h) return NULL;

    /* Extract default string from handle */
    const char *def_str = "";
    if (default_value) {
        rt_handle_begin_transaction(default_value);
        const char *s = (const char *)default_value->ptr;
        if (s) def_str = s;
    }

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    meta->element_size = sizeof(RtHandleV2 *);

    RtHandleV2 **arr = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        rt_handle_renew_transaction(h);
        arr[i] = rt_arena_v2_strdup(arena, def_str);
    }

    rt_handle_end_transaction(h);
    if (default_value) rt_handle_end_transaction(default_value);
    rt_handle_set_copy_callback(h, rt_array_copy_callback);
    rt_handle_set_free_callback(h, rt_array_free_callback);
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

    rt_handle_begin_transaction(h);
    void *raw = h->ptr;
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long long *arr = (long long *)((char *)raw + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < count; i++) {
        rt_handle_renew_transaction(h);
        arr[i] = start + (long long)i;
    }

    rt_handle_end_transaction(h);
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

    rt_handle_begin_transaction(arr_h);
    void *raw = arr_h->ptr;
    if (raw == NULL) {
        rt_handle_end_transaction(arr_h);
        return NULL;
    }

    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)raw;
    size_t count = meta->size;

    /* Get pointer to the array of string handles */
    RtHandleV2 **handles = (RtHandleV2 **)((char *)raw + sizeof(RtArrayMetadataV2));

    /* Allocate a char** array from the arena WITH metadata prefix.
     * This ensures rt_v2_data_array_length() works on the returned pointer,
     * since native functions use it to get the array length. */
    RtArenaV2 *arena = rt_handle_v2_arena(arr_h);
    if (arena == NULL) {
        rt_handle_end_transaction(arr_h);
        return NULL;
    }

    /* Allocate space for metadata + char** result with null terminator */
    size_t alloc_size = sizeof(RtArrayMetadataV2) + (count + 1) * sizeof(char *);
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, alloc_size);
    if (result_h == NULL) {
        rt_handle_end_transaction(arr_h);
        return NULL;
    }
    rt_handle_begin_transaction(result_h);
    void *result_raw = result_h->ptr;

    /* Write metadata so rt_v2_data_array_length works */
    RtArrayMetadataV2 *result_meta = (RtArrayMetadataV2 *)result_raw;
    result_meta->arena = arena;
    result_meta->size = count;
    result_meta->capacity = count;

    char **result = (char **)((char *)result_raw + sizeof(RtArrayMetadataV2));

    /* Pin each string element to extract char* */
    for (size_t i = 0; i < count; i++) {
        rt_handle_renew_transaction(result_h);
        if (handles[i] != NULL) {
            rt_handle_begin_transaction(handles[i]);
            result[i] = (char *)handles[i]->ptr;
            rt_handle_end_transaction(handles[i]);
        } else {
            result[i] = NULL;
        }
    }
    result[count] = NULL; /* Null terminator */

    rt_handle_end_transaction(result_h);
    rt_handle_end_transaction(arr_h);

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

