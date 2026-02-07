// src/runtime/runtime_string_split.c
// String Split Functions

/* ============================================================
   String Split Functions
   ============================================================ */

/* Split a string by a delimiter */
char **rt_str_split(RtArena *arena, const char *str, const char *delimiter) {
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
        RtArrayMetadataV2 *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_str_split: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = len;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < len; i++) {
            char *ch = rt_arena_alloc(arena, 2);
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
    RtArrayMetadataV2 *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_str_split: allocation failed\n");
        exit(1);
    }
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
        char *part = rt_arena_alloc(arena, part_len + 1);
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part */
    result[idx] = rt_arena_strdup(arena, start);

    return result;
}

/* Split string with limit - returns at most 'limit' parts.
 * The last part contains the rest of the string (unsplit).
 * If limit <= 0, behaves like rt_str_split (no limit). */
char **rt_str_split_n(RtArena *arena, const char *str, const char *delimiter, int limit) {
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
        RtArrayMetadataV2 *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_str_split_n: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);
        result[0] = rt_arena_strdup(arena, str);
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
        RtArrayMetadataV2 *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_str_split_n: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = actual_count;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < actual_count - 1 && i < len; i++) {
            char *ch = rt_arena_alloc(arena, 2);
            ch[0] = str[i];
            ch[1] = '\0';
            result[i] = ch;
        }
        /* Last part is the remainder */
        if (actual_count > 0 && actual_count - 1 < len) {
            result[actual_count - 1] = rt_arena_strdup(arena, str + actual_count - 1);
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
    RtArrayMetadataV2 *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadataV2) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_str_split_n: allocation failed\n");
        exit(1);
    }
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
        char *part = rt_arena_alloc(arena, part_len + 1);
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part (remainder of string) */
    result[idx] = rt_arena_strdup(arena, start);

    return result;
}
