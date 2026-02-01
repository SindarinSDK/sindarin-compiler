/* ============================================================================
 * Handle-Based Array Functions Implementation - Type Conversion
 * ============================================================================
 * Convert typed arrays to any[][] arrays.
 * ============================================================================ */

/* 2D Array to any[][] Conversion */

RtHandle rt_array2_to_any_long_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        long long *inner_data = (long long *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_long((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_double_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        double *inner_data = (double *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_double((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_char_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        char *inner_data = (char *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_char((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_bool_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        int *inner_data = (int *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_bool((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_byte_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        unsigned char *inner_data = (unsigned char *)rt_managed_pin_array(arena, inner_handles[i]);
        RtAny *any_inner = rt_array_to_any_byte((RtArena *)arena, inner_data);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

RtHandle rt_array2_to_any_string_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtAny *any_inner = rt_array_to_any_string_h(arena, inner_handles[i]);
        RtHandle any_h = rt_array_clone_void_h(arena, RT_HANDLE_NULL, any_inner);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_h);
    }
    return result;
}

/* 3D Array to any[][][] Conversion */

RtHandle rt_array3_to_any_long_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_long_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_double_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_double_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_char_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_char_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_bool_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_bool_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_byte_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_byte_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}

RtHandle rt_array3_to_any_string_h(RtManagedArena *arena, RtHandle outer) {
    if (outer == RT_HANDLE_NULL) return RT_HANDLE_NULL;
    RtHandle *inner_handles = (RtHandle *)rt_managed_pin_array(arena, outer);
    size_t len = rt_array_length(inner_handles);
    if (len == 0) return RT_HANDLE_NULL;
    RtHandle result = RT_HANDLE_NULL;
    for (size_t i = 0; i < len; i++) {
        RtHandle any_2d_h = rt_array2_to_any_string_h(arena, inner_handles[i]);
        result = rt_array_push_ptr_h(arena, result, (void *)(uintptr_t)any_2d_h);
    }
    return result;
}
