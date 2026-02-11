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
    const long long *arr = (const long long *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_long(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_double_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const double *arr = (const double *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_double(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_char_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const char *arr = (const char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_char(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_bool_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const int *arr = (const int *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_bool(arr[i] != 0));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_byte_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const unsigned char *arr = (const unsigned char *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        result = rt_array_push_any_v2(arena, result, rt_box_byte(arr[i]));
    }
    return result;
}

RtHandleV2 *rt_array_to_any_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    RtHandleV2 **arr = (RtHandleV2 **)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        const char *s;
        if (arr[i]) {
            rt_handle_v2_pin(arr[i]);
            s = (const char *)arr[i]->ptr;
        } else {
            s = "";
        }
        result = rt_array_push_any_v2(arena, result, rt_box_string(s));
        if (arr[i]) rt_handle_v2_unpin(arr[i]);
    }
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
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_long_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_double_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_double_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_char_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_char_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_bool_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_bool_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_byte_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_byte_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
    return result;
}

RtHandleV2 *rt_array2_to_any_string_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_h = rt_array_to_any_string_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_h);
    }
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
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_long_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_double_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_double_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_char_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_char_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_bool_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_bool_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_byte_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_byte_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

RtHandleV2 *rt_array3_to_any_string_v2(RtHandleV2 *outer) {
    if (outer == NULL) return NULL;
    RtArenaV2 *arena = outer->arena;
    RtHandleV2 **inner_handles = (RtHandleV2 **)rt_array_data_v2(outer);
    size_t len = rt_array_length_v2(outer);
    if (len == 0) return NULL;

    RtHandleV2 *result = NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandleV2 *any_2d_h = rt_array2_to_any_string_v2(inner_handles[i]);
        result = rt_array_push_ptr_v2(arena, result, any_2d_h);
    }
    return result;
}

/* ============================================================================
 * Convert any[] to Typed Array V2 Functions
 * ============================================================================ */

RtHandleV2 *rt_array_from_any_long_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(long long), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(long long));
    rt_handle_v2_pin(data_h);
    long long *data = (long long *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_long(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(long long), data);
}

RtHandleV2 *rt_array_from_any_int32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(int32_t), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(int32_t));
    rt_handle_v2_pin(data_h);
    int32_t *data = (int32_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_int32(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(int32_t), data);
}

RtHandleV2 *rt_array_from_any_uint_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(uint64_t), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(uint64_t));
    rt_handle_v2_pin(data_h);
    uint64_t *data = (uint64_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_uint(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(uint64_t), data);
}

RtHandleV2 *rt_array_from_any_uint32_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(uint32_t), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(uint32_t));
    rt_handle_v2_pin(data_h);
    uint32_t *data = (uint32_t *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_uint32(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(uint32_t), data);
}

RtHandleV2 *rt_array_from_any_double_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(double), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(double));
    rt_handle_v2_pin(data_h);
    double *data = (double *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_double(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(double), data);
}

RtHandleV2 *rt_array_from_any_float_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(float), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(float));
    rt_handle_v2_pin(data_h);
    float *data = (float *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_float(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(float), data);
}

RtHandleV2 *rt_array_from_any_char_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(char), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(char));
    rt_handle_v2_pin(data_h);
    char *data = (char *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = (char)rt_unbox_char(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(char), data);
}

RtHandleV2 *rt_array_from_any_bool_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(int), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(int));
    rt_handle_v2_pin(data_h);
    int *data = (int *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_bool(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(int), data);
}

RtHandleV2 *rt_array_from_any_byte_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_generic_v2(arena, 0, sizeof(unsigned char), NULL);

    RtHandleV2 *data_h = rt_arena_v2_alloc(arena, len * sizeof(unsigned char));
    rt_handle_v2_pin(data_h);
    unsigned char *data = (unsigned char *)data_h->ptr;
    for (size_t i = 0; i < len; i++) {
        data[i] = rt_unbox_byte(arr[i]);
    }
    return rt_array_create_generic_v2(arena, len, sizeof(unsigned char), data);
}

RtHandleV2 *rt_array_from_any_string_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return NULL;
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);
    if (len == 0) return rt_array_create_string_v2(arena, 0, NULL);

    /* Allocate array of string handles */
    RtHandleV2 *handles_alloc_h = rt_arena_v2_alloc(arena, len * sizeof(RtHandleV2 *));
    rt_handle_v2_pin(handles_alloc_h);
    RtHandleV2 **handles = (RtHandleV2 **)handles_alloc_h->ptr;
    for (size_t i = 0; i < len; i++) {
        const char *str = rt_unbox_string(arr[i]);
        handles[i] = rt_arena_v2_strdup(arena, str);
    }
    return rt_array_create_ptr_v2(arena, len, handles);
}

/* ============================================================================
 * Any Array toString V2 Functions
 * ============================================================================ */

char *rt_to_string_array_any_v2(RtHandleV2 *arr_h) {
    if (arr_h == NULL) return "{}";
    RtArenaV2 *arena = arr_h->arena;
    const RtAny *arr = (const RtAny *)rt_array_data_v2(arr_h);
    size_t len = rt_array_length_v2(arr_h);

    if (len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        rt_handle_v2_pin(h);
        return (char *)h->ptr;
    }

    /* First pass: convert each element to string and calculate total length */
    RtHandleV2 *elem_strs_h = rt_arena_v2_alloc(arena, len * sizeof(char *));
    rt_handle_v2_pin(elem_strs_h);
    char **elem_strs = (char **)elem_strs_h->ptr;
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        elem_strs[i] = rt_any_to_string((RtArenaV2 *)arena, arr[i]);
        if (i > 0) total_len += 2; /* ", " */
        total_len += strlen(elem_strs[i]);
    }

    /* Build result string */
    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_v2_pin(handle);
    char *result = (char *)handle->ptr;

    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = elem_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array2_any_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return "{}";
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);

    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        rt_handle_v2_pin(h);
        return (char *)h->ptr;
    }

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_v2_pin(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array_any_v2(outer[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_v2_pin(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

char *rt_to_string_array3_any_v2(RtHandleV2 *outer_h) {
    if (outer_h == NULL) return "{}";
    RtArenaV2 *arena = outer_h->arena;
    RtHandleV2 **outer = (RtHandleV2 **)rt_array_data_v2(outer_h);
    size_t outer_len = rt_array_length_v2(outer_h);

    if (outer_len == 0) {
        RtHandleV2 *h = rt_arena_v2_strdup(arena, "{}");
        rt_handle_v2_pin(h);
        return (char *)h->ptr;
    }

    RtHandleV2 *inner_strs_h = rt_arena_v2_alloc(arena, outer_len * sizeof(char *));
    rt_handle_v2_pin(inner_strs_h);
    char **inner_strs = (char **)inner_strs_h->ptr;
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        inner_strs[i] = rt_to_string_array2_any_v2(outer[i]);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }

    RtHandleV2 *handle = rt_arena_v2_alloc(arena, total_len + 1);
    rt_handle_v2_pin(handle);
    char *result = (char *)handle->ptr;
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < outer_len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        const char *s = inner_strs[i];
        while (*s) *p++ = *s++;
    }
    *p++ = '}';
    *p = '\0';
    return result;
}
