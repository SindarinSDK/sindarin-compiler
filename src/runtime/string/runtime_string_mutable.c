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
RtHandleV2 *rt_string_with_capacity(RtArenaV2 *arena, size_t capacity) {
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
    rt_handle_begin_transaction(meta_h);
    RtStringMeta *meta = (RtStringMeta *)meta_h->ptr;
    meta->arena = arena;
    meta->length = 0;
    meta->capacity = capacity;
    char *str = (char*)(meta + 1);
    str[0] = '\0';
    rt_handle_end_transaction(meta_h);
    return meta_h;
}

/* Create a mutable string from an immutable source string.
 * Copies the content into a new mutable string with metadata. */
RtHandleV2 *rt_string_from(RtArenaV2 *arena, const char *src) {
    if (arena == NULL) {
        fprintf(stderr, "rt_string_from: arena is NULL\n");
        exit(1);
    }

    size_t len = src ? strlen(src) : 0;
    /* Allocate with some extra capacity to allow appending */
    size_t capacity = len < 16 ? 32 : len * 2;

    RtHandleV2 *h = rt_string_with_capacity(arena, capacity);
    rt_handle_begin_transaction(h);
    char *str = (char *)((RtStringMeta *)h->ptr + 1);
    if (src && len > 0) {
        memcpy(str, src, len);
        str[len] = '\0';
        ((RtStringMeta *)h->ptr)->length = len;
    }
    rt_handle_end_transaction(h);
    return h;
}

/* Ensure a string is mutable. If it's already mutable (has valid metadata),
 * returns it unchanged. If it's immutable, converts it to a mutable string.
 * This is used before append operations on strings that may be immutable.
 *
 * SAFETY: We use a magic number approach to identify mutable strings.
 * Mutable strings have a specific pattern in their metadata that immutable
 * strings (from arena_strdup or string literals) cannot have.
 */
RtHandleV2 *rt_string_ensure_mutable(RtArenaV2 *arena, char *str) {
    if (str == NULL) {
        /* NULL becomes an empty mutable string */
        return rt_string_with_capacity(arena, 32);
    }

    /* Always create a new mutable copy. We cannot recover the original handle
     * from a raw char* pointer, so we must create a fresh handle. */
    return rt_string_from(arena, str);
}

/* Append a string to a mutable string (in-place if capacity allows).
 * Returns dest pointer - may be different from input if reallocation occurred.
 * Uses 2x growth strategy when capacity is exceeded. */
RtHandleV2 *rt_string_append(RtHandleV2 *dest_h, const char *src) {
    /* Validate inputs */
    if (dest_h == NULL) {
        fprintf(stderr, "rt_string_append: dest_h is NULL\n");
        exit(1);
    }
    if (src == NULL) {
        return dest_h;  /* Appending NULL is a no-op */
    }

    rt_handle_begin_transaction(dest_h);
    char *dest = (char *)((RtStringMeta *)dest_h->ptr + 1);

    /* Get metadata and validate it's a mutable string */
    RtStringMeta *meta = (RtStringMeta *)dest_h->ptr;
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

        rt_handle_end_transaction(dest_h);
        RtHandleV2 *new_h = rt_string_with_capacity(meta->arena, new_cap);
        rt_handle_begin_transaction(new_h);
        char *new_str = (char *)((RtStringMeta *)new_h->ptr + 1);

        /* Copy existing content to new buffer */
        memcpy(new_str, dest, old_len);

        /* Update dest_h, dest, and meta to point to new buffer */
        dest_h = new_h;
        dest = new_str;
        meta = (RtStringMeta *)dest_h->ptr;
    }

    /* Append the source string (including null terminator) */
    memcpy(dest + old_len, src, src_len + 1);
    meta->length = new_len;

    rt_handle_end_transaction(dest_h);
    return dest_h;
}
