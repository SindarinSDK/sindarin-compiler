// src/runtime/runtime_string_split.c
// String Split Functions

/* ============================================================
   String Split Functions
   ============================================================ */

/* Split a string by a delimiter */
char **rt_str_split(RtArenaV2 *arena, const char *str, const char *delimiter) {
    if (str == NULL || delimiter == NULL) {
        return NULL;
    }

    size_t delim_len = strlen(delimiter);
    if (delim_len == 0) {
        /* Empty delimiter: split into individual characters */
        size_t len = strlen(str);
        if (len == 0) return NULL;

        /* Allocate the result array directly */
        size_t capacity = len > 4 ? len : 4;
        RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta_h == NULL) {
            fprintf(stderr, "rt_str_split: allocation failed\n");
            exit(1);
        }
        rt_handle_begin_transaction(meta_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)meta_h->ptr;
        meta->arena = arena;
        meta->size = len;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < len; i++) {
            RtHandleV2 *ch_h = rt_arena_v2_alloc(arena, 2);
            rt_handle_begin_transaction(ch_h);
            char *ch = (char *)ch_h->ptr;
            ch[0] = str[i];
            ch[1] = '\0';
            result[i] = ch;
        }
        return result;
    }

    /* Count the number of parts */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        count++;
        p += delim_len;
    }

    /* Allocate the result array */
    size_t capacity = count > 4 ? count : 4;
    RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
    if (meta_h == NULL) {
        fprintf(stderr, "rt_str_split: allocation failed\n");
        exit(1);
    }
    rt_handle_begin_transaction(meta_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)meta_h->ptr;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);

    /* Split the string */
    const char *start = str;
    size_t idx = 0;
    p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        size_t part_len = p - start;
        RtHandleV2 *part_h = rt_arena_v2_alloc(arena, part_len + 1);
        rt_handle_begin_transaction(part_h);
        char *part = (char *)part_h->ptr;
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part */
    RtHandleV2 *final_h = rt_arena_v2_strdup(arena, start);
    rt_handle_begin_transaction(final_h);
    result[idx] = (char *)final_h->ptr;

    return result;
}

/* Split string with limit - returns at most 'limit' parts.
 * The last part contains the rest of the string (unsplit).
 * If limit <= 0, behaves like rt_str_split (no limit). */
char **rt_str_split_n(RtArenaV2 *arena, const char *str, const char *delimiter, int limit) {
    if (str == NULL || delimiter == NULL) {
        return NULL;
    }

    /* If limit is 0 or negative, behave like unlimited split */
    if (limit <= 0) {
        return rt_str_split(arena, str, delimiter);
    }

    /* If limit is 1, return the whole string as one part */
    if (limit == 1) {
        size_t capacity = 4;
        RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta_h == NULL) {
            fprintf(stderr, "rt_str_split_n: allocation failed\n");
            exit(1);
        }
        rt_handle_begin_transaction(meta_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)meta_h->ptr;
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);
        RtHandleV2 *str_h = rt_arena_v2_strdup(arena, str);
        rt_handle_begin_transaction(str_h);
        result[0] = (char *)str_h->ptr;
        return result;
    }

    size_t delim_len = strlen(delimiter);
    if (delim_len == 0) {
        /* Empty delimiter with limit: split into at most 'limit' characters */
        size_t len = strlen(str);
        if (len == 0) return NULL;

        size_t actual_count = (size_t)limit;
        if (actual_count > len) actual_count = len;

        size_t capacity = actual_count > 4 ? actual_count : 4;
        RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta_h == NULL) {
            fprintf(stderr, "rt_str_split_n: allocation failed\n");
            exit(1);
        }
        rt_handle_begin_transaction(meta_h);
        RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)meta_h->ptr;
        meta->arena = arena;
        meta->size = actual_count;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < actual_count - 1 && i < len; i++) {
            RtHandleV2 *ch_h = rt_arena_v2_alloc(arena, 2);
            rt_handle_begin_transaction(ch_h);
            char *ch = (char *)ch_h->ptr;
            ch[0] = str[i];
            ch[1] = '\0';
            result[i] = ch;
        }
        /* Last part is the remainder */
        if (actual_count > 0 && actual_count - 1 < len) {
            RtHandleV2 *rem_h = rt_arena_v2_strdup(arena, str + actual_count - 1);
            rt_handle_begin_transaction(rem_h);
            result[actual_count - 1] = (char *)rem_h->ptr;
        }
        return result;
    }

    /* Count the number of parts (up to limit) */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL && count < (size_t)limit) {
        count++;
        p += delim_len;
    }

    /* Allocate the result array */
    size_t capacity = count > 4 ? count : 4;
    RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
    if (meta_h == NULL) {
        fprintf(stderr, "rt_str_split_n: allocation failed\n");
        exit(1);
    }
    rt_handle_begin_transaction(meta_h);
    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)meta_h->ptr;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);

    /* Split the string (up to limit - 1 splits) */
    const char *start = str;
    size_t idx = 0;
    p = str;
    while ((p = strstr(p, delimiter)) != NULL && idx < count - 1) {
        size_t part_len = p - start;
        RtHandleV2 *part_h = rt_arena_v2_alloc(arena, part_len + 1);
        rt_handle_begin_transaction(part_h);
        char *part = (char *)part_h->ptr;
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part (remainder of string) */
    RtHandleV2 *final_h = rt_arena_v2_strdup(arena, start);
    rt_handle_begin_transaction(final_h);
    result[idx] = (char *)final_h->ptr;

    return result;
}
