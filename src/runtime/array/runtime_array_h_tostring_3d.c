/* ============================================================================
 * Handle-Based Array Functions Implementation - ToString 3D
 * ============================================================================
 * Convert 3D nested arrays to string representation.
 * ============================================================================ */

char *rt_to_string_array3_long_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_long_h(arena, inner);
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

char *rt_to_string_array3_double_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_double_h(arena, inner);
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

char *rt_to_string_array3_char_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_char_h(arena, inner);
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

char *rt_to_string_array3_bool_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_bool_h(arena, inner);
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

char *rt_to_string_array3_byte_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_byte_h(arena, inner);
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

char *rt_to_string_array3_string_h(RtManagedArena *arena, RtHandle *outer) {
    if (outer == NULL || rt_array_length(outer) == 0) {
        return rt_arena_strdup((RtArena *)arena, "{}");
    }
    size_t outer_len = rt_array_length(outer);
    char **inner_strs = rt_arena_alloc((RtArena *)arena, outer_len * sizeof(char *));
    size_t total_len = 2;
    for (size_t i = 0; i < outer_len; i++) {
        RtHandle *inner = (RtHandle *)rt_managed_pin_array(arena, outer[i]);
        inner_strs[i] = rt_to_string_array2_string_h(arena, inner);
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
