/* ============================================================================
 * Handle-Based Array Functions Implementation - String Operations
 * ============================================================================
 * String array utilities: join, print, indexOf, contains, conversions.
 * ============================================================================ */

char *rt_array_join_string_h(RtManagedArena *arena, RtHandle *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup((RtArena *)arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            total_len += strlen(s);
        }
    }
    total_len += (len - 1) * sep_len + 1;

    char *result = rt_arena_alloc((RtArena *)arena, total_len);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_string_h: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            size_t l = strlen(separator);
            memcpy(ptr, separator, l);
            ptr += l;
        }
        if (arr[i] != RT_HANDLE_NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            size_t l = strlen(s);
            memcpy(ptr, s, l);
            ptr += l;
        }
    }
    *ptr = '\0';
    return result;
}

void rt_print_array_string_h(RtManagedArena *arena, RtHandle *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            if (arr[i] != RT_HANDLE_NULL) {
                const char *s = (const char *)rt_managed_pin(arena, arr[i]);
                printf("\"%s\"", s);
            } else {
                printf("null");
            }
        }
    }
    printf("]");
}

long rt_array_indexOf_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem) {
    if (arr == NULL) {
        return -1L;
    }
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        if (arr[i] == RT_HANDLE_NULL && elem == NULL) {
            return (long)i;
        }
        if (arr[i] != RT_HANDLE_NULL && elem != NULL) {
            const char *s = (const char *)rt_managed_pin(arena, arr[i]);
            if (strcmp(s, elem) == 0) {
                return (long)i;
            }
        }
    }
    return -1L;
}

int rt_array_contains_string_h(RtManagedArena *arena, RtHandle *arr, const char *elem) {
    return rt_array_indexOf_string_h(arena, arr, elem) >= 0;
}

/* Convert a legacy char** string array to a handle-based string array */
RtHandle rt_array_from_legacy_string_h(RtManagedArena *arena, char **src) {
    if (src == NULL) {
        return array_create_h(arena, 0, sizeof(RtHandle), NULL);
    }
    size_t count = ((RtArrayMetadata *)src)[-1].size;
    RtHandle h = rt_managed_alloc(arena, RT_HANDLE_NULL, sizeof(RtArrayMetadata) + count * sizeof(RtHandle));
    void *raw = rt_managed_pin(arena, h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    meta->arena = (RtArena *)arena;
    meta->size = count;
    meta->capacity = count;
    RtHandle *dst = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    for (size_t i = 0; i < count; i++) {
        dst[i] = rt_managed_strdup(arena, RT_HANDLE_NULL, src[i] ? src[i] : "");
    }
    rt_managed_unpin(arena, h);
    return h;
}

/* Convert a handle-based string array to a legacy char** array */
char **rt_managed_pin_string_array(RtManagedArena *arena, RtHandle arr_h) {
    if (arr_h == RT_HANDLE_NULL) return NULL;
    RtHandle *handles = (RtHandle *)rt_managed_pin_array(arena, arr_h);
    if (handles == NULL) return NULL;
    size_t count = rt_array_length(handles);
    /* Allocate a legacy char** array from the arena */
    char **result = rt_array_create_string((RtArena *)arena, count, NULL);
    for (size_t i = 0; i < count; i++) {
        result[i] = (char *)rt_managed_pin(arena, handles[i]);
    }
    return result;
}

/* Convert a handle-based string array to a legacy RtAny* array */
RtAny *rt_array_to_any_string_h(RtManagedArena *arena, RtHandle arr_h) {
    if (arr_h == RT_HANDLE_NULL) return NULL;
    void *raw = rt_managed_pin(arena, arr_h);
    RtArrayMetadata *meta = (RtArrayMetadata *)raw;
    RtHandle *elements = (RtHandle *)((char *)raw + sizeof(RtArrayMetadata));
    size_t len = meta->size;
    if (len == 0) {
        rt_managed_unpin(arena, arr_h);
        return NULL;
    }
    RtAny *result = NULL;
    for (size_t i = 0; i < len; i++) {
        char *str = (char *)rt_managed_pin(arena, elements[i]);
        result = rt_array_push_any((RtArena *)arena, result, rt_box_string(str));
        rt_managed_unpin(arena, elements[i]);
    }
    rt_managed_unpin(arena, arr_h);
    return result;
}
