#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "runtime_string.h"

/* ============================================================================
 * String Operations Implementation
 * ============================================================================
 * This module provides string manipulation functions for the Sn runtime:
 * - Immutable string concatenation (rt_str_concat)
 * - Mutable string operations (rt_string_with_capacity, rt_string_from, etc.)
 * - Type-to-string conversion (rt_to_string_*)
 * - Format functions (rt_format_*)
 * - Print functions (rt_print_*)
 * ============================================================================ */

/* ============================================================================
 * Immutable String Concatenation
 * ============================================================================
 * Creates a new immutable string from concatenating two strings.
 * Returns NULL on allocation failure or if result would exceed 1GB.
 *
 * This is the appropriate behavior for functional-style string operations.
 *
 * TODO: Future optimization - if left string is mutable and has enough
 * capacity, could append in-place using rt_string_append() instead. */
char *rt_str_concat(RtArena *arena, const char *left, const char *right) {
    const char *l = left ? left : "";
    const char *r = right ? right : "";
    size_t left_len = strlen(l);
    size_t right_len = strlen(r);
    size_t new_len = left_len + right_len;
    if (new_len > (1UL << 30) - 1) {
        return NULL;
    }
    char *new_str = rt_arena_alloc(arena, new_len + 1);
    if (new_str == NULL) {
        return NULL;
    }
    memcpy(new_str, l, left_len);
    memcpy(new_str + left_len, r, right_len + 1);
    return new_str;
}

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
char *rt_string_with_capacity(RtArena *arena, size_t capacity) {
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
    RtStringMeta *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_string_with_capacity: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->length = 0;
    meta->capacity = capacity;
    char *str = (char*)(meta + 1);
    str[0] = '\0';
    return str;
}

/* Create a mutable string from an immutable source string.
 * Copies the content into a new mutable string with metadata. */
char *rt_string_from(RtArena *arena, const char *src) {
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
char *rt_string_ensure_mutable(RtArena *arena, char *str) {
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

/* ============================================================================
 * Type Conversion Functions (rt_to_string_*)
 *
 * IMMUTABLE STRINGS: All rt_to_string_* functions create immutable strings
 * via rt_arena_strdup(). These functions can remain unchanged because:
 * 1. They create short, fixed-size strings that don't need appending
 * 2. The results are typically used in interpolation/concatenation
 * 3. No benefit to adding metadata for these small strings
 *
 * TODO: If profiling shows these are bottlenecks in hot loops, consider
 * caching common values (e.g., "true"/"false", small integers 0-99).
 * ============================================================================ */

static const char *null_str = "(null)";

char *rt_to_string_long(RtArena *arena, long long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_double(RtArena *arena, double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_char(RtArena *arena, char val)
{
    char buf[2] = {val, '\0'};
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_bool(RtArena *arena, int val)
{
    return rt_arena_strdup(arena, val ? "true" : "false");
}

char *rt_to_string_byte(RtArena *arena, unsigned char val)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_string(RtArena *arena, const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    return rt_arena_strdup(arena, val);
}

char *rt_to_string_void(RtArena *arena)
{
    return rt_arena_strdup(arena, "void");
}

char *rt_to_string_pointer(RtArena *arena, void *p)
{
    if (p == NULL) {
        return rt_arena_strdup(arena, "nil");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p);
    return rt_arena_strdup(arena, buf);
}

/* ============================================================================
 * Format Functions (rt_format_*)
 *
 * These functions support format specifiers in string interpolation.
 * Format syntax: [0][width][type] where type varies by function.
 * ============================================================================ */

char *rt_format_long(RtArena *arena, long long val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%lld", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][type] where type is d, x, X, o, b */
    int width = 0;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Determine type */
    char type = *p ? *p : 'd';

    switch (type) {
        case 'd':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dlld", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dlld", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%lld");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'x':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllx", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllx", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llx");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'X':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllX", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllX", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llX");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'o':
            if (zero_pad && width > 0) {
                snprintf(format_str, sizeof(format_str), "%%0%dllo", width);
            } else if (width > 0) {
                snprintf(format_str, sizeof(format_str), "%%%dllo", width);
            } else {
                snprintf(format_str, sizeof(format_str), "%%llo");
            }
            snprintf(buf, sizeof(buf), format_str, val);
            break;
        case 'b': {
            /* Binary format - custom implementation */
            char binbuf[65];
            int len = 0;
            unsigned long long uval = (unsigned long long)val;
            if (uval == 0) {
                binbuf[len++] = '0';
            } else {
                while (uval > 0) {
                    binbuf[len++] = (uval & 1) ? '1' : '0';
                    uval >>= 1;
                }
            }
            /* Reverse the string */
            for (int i = 0; i < len / 2; i++) {
                char tmp = binbuf[i];
                binbuf[i] = binbuf[len - 1 - i];
                binbuf[len - 1 - i] = tmp;
            }
            /* Pad if needed */
            if (width > len) {
                int pad = width - len;
                memmove(binbuf + pad, binbuf, len);
                for (int i = 0; i < pad; i++) {
                    binbuf[i] = zero_pad ? '0' : ' ';
                }
                len = width;
            }
            binbuf[len] = '\0';
            return rt_arena_strdup(arena, binbuf);
        }
        default:
            snprintf(buf, sizeof(buf), "%lld", val);
            break;
    }

    return rt_arena_strdup(arena, buf);
}

char *rt_format_double(RtArena *arena, double val, const char *fmt)
{
    char buf[128];
    char format_str[64];

    if (fmt == NULL || fmt[0] == '\0') {
        snprintf(buf, sizeof(buf), "%g", val);
        return rt_arena_strdup(arena, buf);
    }

    /* Parse format specifier: [width][.precision][type] where type is f, e, E, g, G, % */
    int width = 0;
    int precision = -1;
    int zero_pad = 0;
    const char *p = fmt;

    /* Check for zero padding */
    if (*p == '0') {
        zero_pad = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse precision */
    if (*p == '.') {
        p++;
        precision = 0;
        while (*p >= '0' && *p <= '9') {
            precision = precision * 10 + (*p - '0');
            p++;
        }
    }

    /* Determine type */
    char type = *p ? *p : 'f';

    /* Handle percentage format */
    if (type == '%') {
        val *= 100.0;
        if (precision >= 0) {
            snprintf(format_str, sizeof(format_str), "%%.%df%%%%", precision);
        } else {
            snprintf(format_str, sizeof(format_str), "%%f%%%%");
        }
        snprintf(buf, sizeof(buf), format_str, val);
        return rt_arena_strdup(arena, buf);
    }

    /* Build format string dynamically */
    int pos = 0;
    format_str[pos++] = '%';
    if (zero_pad) format_str[pos++] = '0';
    if (width > 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, "%d", width);
    }
    if (precision >= 0) {
        pos += snprintf(format_str + pos, sizeof(format_str) - pos, ".%d", precision);
    }

    switch (type) {
        case 'f':
            format_str[pos++] = 'f';
            break;
        case 'e':
            format_str[pos++] = 'e';
            break;
        case 'E':
            format_str[pos++] = 'E';
            break;
        case 'g':
            format_str[pos++] = 'g';
            break;
        case 'G':
            format_str[pos++] = 'G';
            break;
        default:
            format_str[pos++] = 'f';
            break;
    }
    format_str[pos] = '\0';

    snprintf(buf, sizeof(buf), format_str, val);
    return rt_arena_strdup(arena, buf);
}

char *rt_format_string(RtArena *arena, const char *val, const char *fmt)
{
    if (val == NULL) {
        val = "nil";
    }

    if (fmt == NULL || fmt[0] == '\0') {
        return rt_arena_strdup(arena, val);
    }

    /* Parse format specifier: [width][.maxlen]s */
    int width = 0;
    int maxlen = -1;
    int left_align = 0;
    const char *p = fmt;

    /* Check for left alignment */
    if (*p == '-') {
        left_align = 1;
        p++;
    }

    /* Parse width */
    while (*p >= '0' && *p <= '9') {
        width = width * 10 + (*p - '0');
        p++;
    }

    /* Parse max length */
    if (*p == '.') {
        p++;
        maxlen = 0;
        while (*p >= '0' && *p <= '9') {
            maxlen = maxlen * 10 + (*p - '0');
            p++;
        }
    }

    int len = strlen(val);

    /* Apply max length truncation */
    if (maxlen >= 0 && len > maxlen) {
        len = maxlen;
    }

    /* Calculate output size */
    int out_len = len;
    if (width > len) {
        out_len = width;
    }

    char *result = rt_arena_alloc(arena, out_len + 1);
    if (result == NULL) return NULL;

    if (width > len) {
        if (left_align) {
            memcpy(result, val, len);
            memset(result + len, ' ', width - len);
        } else {
            memset(result, ' ', width - len);
            memcpy(result + (width - len), val, len);
        }
        result[width] = '\0';
    } else {
        memcpy(result, val, len);
        result[len] = '\0';
    }

    return result;
}

/* ============================================================================
 * Print Functions
 * ============================================================================
 * These functions print values to stdout. Used by the print() built-in.
 * ============================================================================ */

void rt_print_long(long long val)
{
    printf("%lld", val);
}

void rt_print_double(double val)
{
    if (isnan(val))
    {
        printf("NaN");
    }
    else if (isinf(val))
    {
        if (val > 0)
        {
            printf("Inf");
        }
        else
        {
            printf("-Inf");
        }
    }
    else
    {
        printf("%.5f", val);
    }
}

void rt_print_char(long c)
{
    if (c < 0 || c > 255)
    {
        fprintf(stderr, "rt_print_char: invalid char value %ld (must be 0-255)\n", c);
        printf("?");
    }
    else
    {
        printf("%c", (int)c);
    }
}

void rt_print_string(const char *s)
{
    if (s == NULL)
    {
        printf("(null)");
    }
    else
    {
        printf("%s", s);
    }
}

void rt_print_bool(long b)
{
    printf("%s", b ? "true" : "false");
}

void rt_print_byte(unsigned char b)
{
    printf("0x%02X", b);
}

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

char *rt_str_substring(RtArena *arena, const char *str, long start, long end) {
    if (str == NULL) return rt_arena_strdup(arena, "");
    long len = (long)strlen(str);

    /* Handle negative indices */
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    /* Clamp to valid range */
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end || start >= len) return rt_arena_strdup(arena, "");

    long sub_len = end - start;
    char *result = rt_arena_alloc(arena, sub_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str + start, sub_len);
    result[sub_len] = '\0';
    return result;
}

char *rt_str_toUpper(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

    for (char *p = result; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }
    return result;
}

char *rt_str_toLower(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

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

char *rt_str_trim(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    /* Skip leading whitespace */
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }

    if (*str == '\0') return rt_arena_strdup(arena, "");

    /* Find end, skipping trailing whitespace */
    const char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    size_t len = end - str + 1;
    char *result = rt_arena_alloc(arena, len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str, len);
    result[len] = '\0';
    return result;
}

char *rt_str_replace(RtArena *arena, const char *str, const char *old, const char *new_str) {
    if (str == NULL || old == NULL || new_str == NULL) return rt_arena_strdup(arena, str ? str : "");

    size_t old_len = strlen(old);
    if (old_len == 0) return rt_arena_strdup(arena, str);

    size_t new_len = strlen(new_str);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) return rt_arena_strdup(arena, str);

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    char *result = rt_arena_alloc(arena, result_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, str);

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
        RtArrayMetadata *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + capacity * sizeof(char *));
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
    RtArrayMetadata *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + capacity * sizeof(char *));
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

/* ============================================================================
 * String Array Helpers
 * ============================================================================
 * These functions support creating and growing string arrays for split operations.
 * ============================================================================ */

char **rt_create_string_array(RtArena *arena, size_t initial_capacity)
{
    /* Allocate array with header storing length and capacity */
    size_t header_size = 2 * sizeof(size_t);
    size_t alloc_size = header_size + (initial_capacity + 1) * sizeof(char *);
    void *block = rt_arena_alloc(arena, alloc_size);
    if (block == NULL) return NULL;

    size_t *header = (size_t *)block;
    header[0] = 0;                  /* length */
    header[1] = initial_capacity;   /* capacity */

    char **arr = (char **)((char *)block + header_size);
    arr[0] = NULL;  /* NULL terminator */
    return arr;
}

char **rt_push_string_to_array(RtArena *arena, char **arr, char *str)
{
    size_t *header = (size_t *)((char *)arr - 2 * sizeof(size_t));
    size_t len = header[0];
    size_t cap = header[1];

    if (len >= cap) {
        /* Need to grow - allocate new array with double capacity */
        size_t new_cap = cap * 2;
        char **new_arr = rt_create_string_array(arena, new_cap);
        if (new_arr == NULL) return arr;

        /* Copy existing elements */
        for (size_t i = 0; i < len; i++) {
            new_arr[i] = arr[i];
        }
        size_t *new_header = (size_t *)((char *)new_arr - 2 * sizeof(size_t));
        new_header[0] = len;
        arr = new_arr;
        header = new_header;
    }

    arr[len] = str;
    arr[len + 1] = NULL;  /* NULL terminator */
    header[0] = len + 1;
    return arr;
}
