/* ============================================================================
 * Handle-Based Array Functions Implementation - ToString
 * ============================================================================
 * Convert nested arrays to string representation (1D string and 2D arrays).
 * ============================================================================ */

char *rt_to_string_array_string_h(RtManagedArena *arena, RtHandle *arr) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t len = rt_array_length(arr);
    size_t total_len = 2; /* {} */
    for (size_t i = 0; i < len; i++) {
        if (i > 0) total_len += 2; /* ", " */
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            total_len += strlen(s) + 2; /* "str" */
        } else {
            total_len += 4; /* null */
        }
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
    if (result == NULL) return "{}";
    char *p = result;
    *p++ = '{';
    for (size_t i = 0; i < len; i++) {
        if (i > 0) { *p++ = ','; *p++ = ' '; }
        if (arr[i] != RT_HANDLE_NULL) {
            *p++ = '"';
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            while (*s) *p++ = *s++;
            *p++ = '"';
        } else {
            const char *null_str = "null";
            while (*null_str) *p++ = *null_str++;
        }
    }
    *p++ = '}';
    *p = '\0';
    return result;
}

/* 2D array formatters */

char *rt_to_string_array2_long_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        long long *inner = (long long *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_long((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_double_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        double *inner = (double *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_double((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_char_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        char *inner = (char *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_char((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_bool_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        int *inner = (int *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_bool((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_byte_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        unsigned char *inner = (unsigned char *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_byte((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_string_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_string_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array2_any_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtAny *inner = (RtAny *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array_any((RtArena *)arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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

char *rt_to_string_array3_any_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_any_h(arena, inner);
        if (i > 0) total_len += 2;
        total_len += strlen(inner_strs[i]);
    }
    char *result = rt_arena_alloc((RtArena *)arena, total_len + 1);
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
