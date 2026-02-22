/*
 * Runtime Array V2 Any Conversions
 * =================================
 * Functions for converting between typed arrays and any[] arrays.
 */

#include "runtime_array_v2_internal.h"

/* ============================================================================
 * 1D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed V2 arrays to any[] handles by boxing each element.
 * Takes handle, extracts arena and data, returns V2 handle.
 */

RtHandleV2 *rt_array_to_any_long_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const long long *arr = (const long long *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        result = rt_array_push_any_v2(arena, result, rt_box_long(arr[i]));
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

RtHandleV2 *rt_array_to_any_double_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const double *arr = (const double *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        result = rt_array_push_any_v2(arena, result, rt_box_double(arr[i]));
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

RtHandleV2 *rt_array_to_any_char_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const char *arr = (const char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        result = rt_array_push_any_v2(arena, result, rt_box_char(arr[i]));
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

RtHandleV2 *rt_array_to_any_bool_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const int *arr = (const int *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        result = rt_array_push_any_v2(arena, result, rt_box_bool(arr[i] != 0));
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

RtHandleV2 *rt_array_to_any_byte_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const unsigned char *arr = (const unsigned char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        result = rt_array_push_any_v2(arena, result, rt_box_byte(arr[i]));
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

RtHandleV2 *rt_array_to_any_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(arr_h);
        const char *s;
        if (arr[i]) {
            rt_handle_begin_transaction(arr[i]);
            s = (const char *)arr[i]->ptr;
        } else {
            s = "";
        }
        result = rt_array_push_any_v2(arena, result, rt_box_string(s));
        if (arr[i]) rt_handle_end_transaction(arr[i]);
    }
    rt_handle_end_transaction(arr_h);
    return result;
}

/* ============================================================================
 * 2D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 2D V2 arrays to any[][] handles.
 * Takes outer handle, returns handle to array of any-array handles.
 */

RtHandleV2 *rt_array2_to_any_long_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_long_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array2_to_any_double_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_double_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array2_to_any_char_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_char_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array2_to_any_bool_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_bool_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array2_to_any_byte_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_byte_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array2_to_any_string_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_h = rt_array_to_any_string_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

/* ============================================================================
 * 3D Array to Any Conversion (V2)
 * ============================================================================
 * Convert typed 3D V2 arrays to any[][][] handles.
 */

RtHandleV2 *rt_array3_to_any_long_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_long_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array3_to_any_double_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_double_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array3_to_any_char_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_char_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array3_to_any_bool_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_bool_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array3_to_any_byte_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_byte_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

RtHandleV2 *rt_array3_to_any_string_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    rt_handle_begin_transaction(outer);
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) { rt_handle_end_transaction(outer); return NULL; }

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(outer);
        RtHandleV2 *any_2d_h = rt_array2_to_any_string_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    rt_handle_end_transaction(outer);
    return result;
}

/* ============================================================================
 * Convert any[] to Typed Array V2 Functions
 * ============================================================================ */

RtHandleV2 *rt_array_from_any_long_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(long long), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(long long));
    rt_handle_begin_transaction(data_h);
    long long *data = (long long *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_long(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(long long), data);
}

RtHandleV2 *rt_array_from_any_int32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(int32_t), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(int32_t));
    rt_handle_begin_transaction(data_h);
    int32_t *data = (int32_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_int32(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(int32_t), data);
}

RtHandleV2 *rt_array_from_any_uint_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(uint64_t), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(uint64_t));
    rt_handle_begin_transaction(data_h);
    uint64_t *data = (uint64_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_uint(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(uint64_t), data);
}

RtHandleV2 *rt_array_from_any_uint32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(uint32_t), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(uint32_t));
    rt_handle_begin_transaction(data_h);
    uint32_t *data = (uint32_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_uint32(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(uint32_t), data);
}

RtHandleV2 *rt_array_from_any_double_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(double), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(double));
    rt_handle_begin_transaction(data_h);
    double *data = (double *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_double(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(double), data);
}

RtHandleV2 *rt_array_from_any_float_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(float), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(float));
    rt_handle_begin_transaction(data_h);
    float *data = (float *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_float(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(float), data);
}

RtHandleV2 *rt_array_from_any_char_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(char), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(char));
    rt_handle_begin_transaction(data_h);
    char *data = (char *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = (char)rt_unbox_char(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(char), data);
}

RtHandleV2 *rt_array_from_any_bool_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(int), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(int));
    rt_handle_begin_transaction(data_h);
    int *data = (int *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_bool(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(int), data);
}

RtHandleV2 *rt_array_from_any_byte_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_generic_v2(arena, 0, sizeof(unsigned char), NULL); }

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(unsigned char));
    rt_handle_begin_transaction(data_h);
    unsigned char *data = (unsigned char *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(data_h);
        rt_handle_renew_transaction(arr_h);
        data[i] = rt_unbox_byte(arr[i]);
    }
    rt_handle_end_transaction(data_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_generic_v2(arena, len, sizeof(unsigned char), data);
}

RtHandleV2 *rt_array_from_any_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) { rt_handle_end_transaction(arr_h); return rt_array_create_string_v2(arena, 0, NULL); }

    /* Allocate array of string handles */
    RtHandleV2 *handles_alloc_h = rt_arena_v2_alloc(arena, len * sizeof(RtHandleV2 *));
    rt_handle_begin_transaction(handles_alloc_h);
    RtHandleV2 **handles = (RtHandleV2 **)handles_alloc_h->ptr;
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(handles_alloc_h);
        rt_handle_renew_transaction(arr_h);
        const char *str = rt_unbox_string(arr[i]);
        handles[i] = rt_arena_v2_strdup(arena, str);
    }
    rt_handle_end_transaction(handles_alloc_h);
    rt_handle_end_transaction(arr_h);
    return rt_array_create_ptr_v2(arena, len, handles);
}

/* ============================================================================
 * Any Array toString V2 Functions
 * ============================================================================ */

RtHandleV2 *rt_to_string_array_any_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    rt_handle_begin_transaction(arr_h);
    const RtAny *arr = (const RtAny *)((char *)arr_h->ptr + sizeof(RtArrayMetadataV2));
    size_t len = rt_array_length_v2(arr_h);

    if (len == 0) {
        rt_handle_end_transaction(arr_h);
        return rt_arena_v2_strdup(arena, "{}");
    }

    /* First pass: convert each element to string and calculate total length */
    RtHandleV2 *elem_strs_h = rt_arena_v2_alloc(arena, len * sizeof(char *));
    rt_handle_begin_transaction(elem_strs_h);
    char **elem_strs = (char **)elem_strs_h->ptr;
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(elem_strs_h);
        RtHandleV2 *_eh = rt_any_to_string((RtArenaV2 *)arena, arr[i]);
        rt_handle_begin_transaction(_eh);
        elem_strs[i] = (char *)_eh->ptr;
        if (i > 0) total_len += 2; /* ", " */
        total_len += strlen(elem_strs[i]);
        rt_handle_end_transaction(_eh);
    }
    rt_handle_end_transaction(arr_h);

    /* Build result string */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;

    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        rt_handle_renew_transaction(handle);
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(elem_strs_h);
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array2_any_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    rt_handle_begin_transaction(outer_h);
    RtHandleV2 **outer = (RtHandleV2 **)((char *)outer_h->ptr + sizeof(RtArrayMetadataV2));
    size_t outer_len = rt_array_length_v2(outer_h);

    if (outer_len == 0) {
        rt_handle_end_transaction(outer_h);
        return rt_arena_v2_strdup(arena, "{}");
    }

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        rt_handle_renew_transaction(inner_strs_h);
        RtHandleV2 *inner_h = rt_to_string_array_any_v2(outer[i]);
        rt_handle_begin_transaction(inner_h);
        inner_strs[i] = (char *)inner_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_h);
    }
    rt_handle_end_transaction(outer_h);

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        rt_handle_renew_transaction(handle);
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(inner_strs_h);
    rt_handle_end_transaction(handle);
    return handle;
}

RtHandleV2 *rt_to_string_array3_any_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return NULL;
    RtArenaV2 *arena = outer_h->arena;
    rt_handle_begin_transaction(outer_h);
    RtHandleV2 **outer = (RtHandleV2 **)((char *)outer_h->ptr + sizeof(RtArrayMetadataV2));
    size_t outer_len = rt_array_length_v2(outer_h);

    if (outer_len == 0) {
        rt_handle_end_transaction(outer_h);
        return rt_arena_v2_strdup(arena, "{}");
    }

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_begin_transaction(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        rt_handle_renew_transaction(inner_strs_h);
        RtHandleV2 *inner_h = rt_to_string_array2_any_v2(outer[i]);
        rt_handle_begin_transaction(inner_h);
        inner_strs[i] = (char *)inner_h->ptr;
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
        rt_handle_end_transaction(inner_h);
    }
    rt_handle_end_transaction(outer_h);

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_begin_transaction(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        rt_handle_renew_transaction(handle);
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    rt_handle_end_transaction(inner_strs_h);
    rt_handle_end_transaction(handle);
    return handle;
}

/* ============================================================================
 * Any Array GC Callbacks
 * ============================================================================ */

/* Copy callback for any[] arrays - deep copies string/array/struct elements */
void rt_array_any_copy_callback(RtArenaV2 *dest, void *ptr) {
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;
    meta->arena = dest;
    RtAny *arr = (RtAny *)((char *)ptr + sizeof(RtArrayMetadataV2));
    for (size_t i = 0; i < meta->size; i++) {
        rt_any_deep_copy(dest, &arr[i]);
    }
}

