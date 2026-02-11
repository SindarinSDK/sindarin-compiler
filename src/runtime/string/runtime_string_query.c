// src/runtime/runtime_string_query.c
// String Query Functions

/* ============================================================================
 * String Query Functions
 * ============================================================================
 * Functions for querying string properties and searching.
 * ============================================================================ */

long rt_str_length(const char *str) {
    if (str == NULL) return 0;
    return (long)strlen(str);
}

long rt_str_indexOf(const char *str, const char *search) {
    if (str == NULL || search == NULL) return -1;
    const char *pos = strstr(str, search);
    if (pos == NULL) return -1;
    return (long)(pos - str);
}

int rt_str_contains(const char *str, const char *search) {
    if (str == NULL || search == NULL) return 0;
    return strstr(str, search) != NULL;
}

long rt_str_charAt(const char *str, long index) {
    if (str == NULL) return 0;
    long len = (long)strlen(str);

    /* Handle negative index */
    if (index < 0) index = len + index;

    if (index < 0 || index >= len) return 0;
    return (long)(unsigned char)str[index];
}

char *rt_str_substring(RtArenaV2 *arena, const char *str, long start, long end) {
    if (str == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    long len = (long)strlen(str);

    /* Handle negative indices */
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    /* Clamp to valid range */
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end || start >= len) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    long sub_len = end - start;
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, sub_len + 1);
    if (result_h == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    memcpy(result, str + start, sub_len);
    result[sub_len] = '\0';
    return result;
}

char *rt_str_toUpper(RtArenaV2 *arena, const char *str) {
    if (str == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    RtHandleV2 *result_h = rt_arena_v2_strdup(arena, str);
    if (result_h == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    for (char *p = result; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }
    return result;
}

char *rt_str_toLower(RtArenaV2 *arena, const char *str) {
    if (str == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    RtHandleV2 *result_h = rt_arena_v2_strdup(arena, str);
    if (result_h == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    for (char *p = result; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = *p - 'A' + 'a';
        }
    }
    return result;
}

int rt_str_startsWith(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) return 0;
    size_t prefix_len = strlen(prefix);
    if (strlen(str) < prefix_len) return 0;
    return strncmp(str, prefix, prefix_len) == 0;
}

int rt_str_endsWith(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return 0;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char *rt_str_trim(RtArenaV2 *arena, const char *str) {
    if (str == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    /* Skip leading whitespace */
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }

    if (*str == '\0') { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    /* Find end, skipping trailing whitespace */
    const char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    size_t len = end - str + 1;
    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, len + 1);
    if (result_h == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    memcpy(result, str, len);
    result[len] = '\0';
    return result;
}

char *rt_str_replace(RtArenaV2 *arena, const char *str, const char *old, const char *new_str) {
    if (str == NULL || old == NULL || new_str == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, str ? str : ""); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    size_t old_len = strlen(old);
    if (old_len == 0) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, str); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    size_t new_len = strlen(new_str);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, str); rt_handle_v2_pin(_h); return (char *)_h->ptr; }

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    RtHandleV2 *result_h = rt_arena_v2_alloc(arena, result_len + 1);
    if (result_h == NULL) { RtHandleV2 *_h = rt_arena_v2_strdup(arena, str); rt_handle_v2_pin(_h); return (char *)_h->ptr; }
    rt_handle_v2_pin(result_h);
    char *result = (char *)result_h->ptr;

    /* Build result */
    char *dst = result;
    p = str;
    const char *found;
    while ((found = strstr(p, old)) != NULL) {
        size_t prefix_len = found - p;
        memcpy(dst, p, prefix_len);
        dst += prefix_len;
        memcpy(dst, new_str, new_len);
        dst += new_len;
        p = found + old_len;
    }
    /* Copy remainder */
    strcpy(dst, p);

    return result;
}
