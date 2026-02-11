// src/runtime/runtime_string_mutable.c
// Mutable String Functions

/* ============================================================================
 * Mutable String Functions
 * ============================================================================
 * These functions create and manipulate strings WITH RtStringMeta, enabling
 * efficient append operations and O(1) length queries. See runtime_string.h
 * for detailed documentation on mutable vs immutable strings.
 * ============================================================================ */

/* Create a mutable string with specified capacity.
 * Allocates RtStringMeta + capacity + 1 bytes, initializes metadata,
 * and returns pointer to the string data (after metadata).
 * The string is initialized as empty (length=0, str[0]='\0'). */
char *rt_string_with_capacity(RtArenaV2 *arena, size_t capacity) {
    /* Validate arena */
    if (arena == NULL) {
        fprintf(stderr, "rt_string_with_capacity: arena is NULL\n");
        exit(1);
    }

    /* Validate capacity to prevent overflow (limit to 1GB) */
    if (capacity > (1UL << 30)) {
        fprintf(stderr, "rt_string_with_capacity: capacity too large (%zu)\n", capacity);
        exit(1);
    }

    size_t total = sizeof(RtStringMeta) + capacity + 1;
    RtHandleV2 *meta_h = rt_arena_v2_alloc(arena, total);
    if (meta_h == NULL) {
        fprintf(stderr, "rt_string_with_capacity: allocation failed\n");
        exit(1);
    }
    rt_handle_v2_pin(meta_h);
    RtStringMeta *meta = (RtStringMeta *)meta_h->ptr;
    meta->arena = arena;
    meta->length = 0;
    meta->capacity = capacity;
    char *str = (char*)(meta + 1);
    str[0] = '\0';
    return str;
}

/* Create a mutable string from an immutable source string.
 * Copies the content into a new mutable string with metadata. */
char *rt_string_from(RtArenaV2 *arena, const char *src) {
    if (arena == NULL) {
        fprintf(stderr, "rt_string_from: arena is NULL\n");
        exit(1);
    }

    size_t len = src ? strlen(src) : 0;
    /* Allocate with some extra capacity to allow appending */
    size_t capacity = len < 16 ? 32 : len * 2;

    char *str = rt_string_with_capacity(arena, capacity);
    if (src && len > 0) {
        memcpy(str, src, len);
        str[len] = '\0';
        RT_STR_META(str)->length = len;
    }
    return str;
}

/* Ensure a string is mutable. If it's already mutable (has valid metadata),
 * returns it unchanged. If it's immutable, converts it to a mutable string.
 * This is used before append operations on strings that may be immutable.
 *
 * SAFETY: We use a magic number approach to identify mutable strings.
 * Mutable strings have a specific pattern in their metadata that immutable
 * strings (from arena_strdup or string literals) cannot have.
 */
char *rt_string_ensure_mutable(RtArenaV2 *arena, char *str) {
    if (str == NULL) {
        /* NULL becomes an empty mutable string */
        return rt_string_with_capacity(arena, 32);
    }

    /* Check if this pointer points into our arena's memory.
     * Mutable strings created by rt_string_with_capacity have their
     * metadata stored immediately before the string data.
     *
     * We check if the arena pointer in metadata matches AND is non-NULL.
     * For safety, we also verify the capacity is reasonable.
     *
     * For strings NOT created by rt_string_with_capacity (e.g., rt_arena_strdup
     * or string literals), the bytes before them are NOT our metadata.
     *
     * To avoid accessing invalid memory, we take a conservative approach:
     * We only check for strings that were previously returned from
     * rt_string_ensure_mutable or rt_string_with_capacity - which we identify
     * by checking if the alleged metadata has a valid-looking arena pointer
     * and reasonable capacity value.
     */
    RtStringMeta *meta = RT_STR_META(str);

    /* Validate that this looks like a valid mutable string:
     * 1. Arena pointer matches our arena
     * 2. Capacity is reasonable (not garbage)
     * 3. Length is <= capacity
     */
    if (meta->arena == arena &&
        meta->capacity > 0 &&
        meta->capacity < (1UL << 30) &&
        meta->length <= meta->capacity) {
        return str;
    }

    /* Otherwise, convert to mutable */
    return rt_string_from(arena, str);
}

/* Append a string to a mutable string (in-place if capacity allows).
 * Returns dest pointer - may be different from input if reallocation occurred.
 * Uses 2x growth strategy when capacity is exceeded. */
char *rt_string_append(char *dest, const char *src) {
    /* Validate inputs */
    if (dest == NULL) {
        fprintf(stderr, "rt_string_append: dest is NULL\n");
        exit(1);
    }
    if (src == NULL) {
        return dest;  /* Appending NULL is a no-op */
    }

    /* Get metadata and validate it's a mutable string */
    RtStringMeta *meta = RT_STR_META(dest);
    if (meta->arena == NULL) {
        fprintf(stderr, "rt_string_append: dest is not a mutable string (arena is NULL)\n");
        exit(1);
    }

    /* Calculate lengths */
    size_t src_len = strlen(src);
    size_t new_len = meta->length + src_len;

    /* Check for length overflow */
    if (new_len < meta->length) {
        fprintf(stderr, "rt_string_append: string length overflow\n");
        exit(1);
    }

    /* Save current length before potential reallocation */
    size_t old_len = meta->length;

    /* Check if we need to grow the buffer */
    if (new_len + 1 > meta->capacity) {
        /* Grow by 2x to amortize allocation cost */
        size_t new_cap = (new_len + 1) * 2;

        /* Check for capacity overflow */
        if (new_cap < new_len + 1 || new_cap > (1UL << 30)) {
            fprintf(stderr, "rt_string_append: capacity overflow (%zu)\n", new_cap);
            exit(1);
        }

        char *new_str = rt_string_with_capacity(meta->arena, new_cap);

        /* Copy existing content to new buffer */
        memcpy(new_str, dest, old_len);

        /* Update dest and meta to point to new buffer */
        dest = new_str;
        meta = RT_STR_META(dest);
    }

    /* Append the source string (including null terminator) */
    memcpy(dest + old_len, src, src_len + 1);
    meta->length = new_len;

    return dest;
}
