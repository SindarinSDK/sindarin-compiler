#include "sn_minimal.h"
#include <stdarg.h>

/* ---- String operations ---- */

char *sn_str_concat(const char *a, const char *b)
{
    size_t la = a ? strlen(a) : 0;
    size_t lb = b ? strlen(b) : 0;
    char *result = malloc(la + lb + 1);
    if (la) memcpy(result, a, la);
    if (lb) memcpy(result + la, b, lb);
    result[la + lb] = '\0';
    return result;
}

char *sn_str_concat_multi(int count, ...)
{
    va_list args;
    size_t total = 0;

    /* First pass: calculate total length */
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *s = va_arg(args, const char *);
        if (s) total += strlen(s);
    }
    va_end(args);

    char *result = malloc(total + 1);
    char *p = result;

    /* Second pass: copy strings */
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *s = va_arg(args, const char *);
        if (s) {
            size_t len = strlen(s);
            memcpy(p, s, len);
            p += len;
        }
    }
    va_end(args);

    *p = '\0';
    return result;
}

/* ---- Dynamic array ---- */

SnArray *sn_array_new(size_t elem_size, long long initial_cap)
{
    if (initial_cap < 4) initial_cap = 4;
    SnArray *arr = calloc(1, sizeof(SnArray));
    arr->data = malloc(elem_size * (size_t)initial_cap);
    arr->len = 0;
    arr->cap = initial_cap;
    arr->elem_size = elem_size;
    arr->elem_release = NULL;
    arr->elem_copy = NULL;
    return arr;
}

void sn_array_push(SnArray *arr, const void *elem)
{
    if (arr->len >= arr->cap) {
        arr->cap *= 2;
        arr->data = realloc(arr->data, arr->elem_size * (size_t)arr->cap);
    }
    memcpy((char *)arr->data + arr->elem_size * (size_t)arr->len, elem, arr->elem_size);
    arr->len++;
}

void *sn_array_get(SnArray *arr, long long index)
{
    return (char *)arr->data + arr->elem_size * (size_t)index;
}

long long sn_array_length(SnArray *arr)
{
    return arr->len;
}

SnArray *sn_array_range(long long start, long long end)
{
    long long count = end > start ? end - start : 0;
    SnArray *arr = sn_array_new(sizeof(long long), count > 4 ? count : 4);
    arr->elem_tag = SN_TAG_INT;
    for (long long i = start; i < end; i++) {
        sn_array_push(arr, &i);
    }
    return arr;
}

SnArray *sn_array_copy(const SnArray *src)
{
    if (!src) return NULL;
    SnArray *dst = sn_array_new(src->elem_size, src->cap);
    dst->elem_release = src->elem_release;
    dst->elem_copy = src->elem_copy;
    dst->elem_tag = src->elem_tag;
    dst->len = src->len;
    if (src->elem_copy) {
        for (long long i = 0; i < src->len; i++) {
            const void *s = (const char *)src->data + (i * src->elem_size);
            void *d = (char *)dst->data + (i * dst->elem_size);
            src->elem_copy(s, d);
        }
    } else {
        memcpy(dst->data, src->data, (size_t)src->len * src->elem_size);
    }
    return dst;
}

SnArray *sn_array_concat(const SnArray *a, const SnArray *b)
{
    if (!a && !b) return NULL;
    if (!a) return sn_array_copy(b);
    if (!b) return sn_array_copy(a);
    long long total = a->len + b->len;
    SnArray *dst = sn_array_new(a->elem_size, total > 4 ? total : 4);
    dst->elem_release = a->elem_release;
    dst->elem_copy = a->elem_copy;
    dst->elem_tag = a->elem_tag;
    if (a->elem_copy) {
        for (long long i = 0; i < a->len; i++) {
            const void *s = (const char *)a->data + (i * a->elem_size);
            void *d = (char *)dst->data + (i * dst->elem_size);
            a->elem_copy(s, d);
        }
        for (long long i = 0; i < b->len; i++) {
            const void *s = (const char *)b->data + (i * b->elem_size);
            void *d = (char *)dst->data + ((a->len + i) * dst->elem_size);
            a->elem_copy(s, d);
        }
    } else {
        memcpy(dst->data, a->data, (size_t)a->len * a->elem_size);
        memcpy((char *)dst->data + (size_t)a->len * a->elem_size,
               b->data, (size_t)b->len * b->elem_size);
    }
    dst->len = total;
    return dst;
}

void sn_array_extend(SnArray *dst, const SnArray *src)
{
    if (!src || src->len == 0) return;
    long long needed = dst->len + src->len;
    if (needed > dst->cap) {
        long long new_cap = dst->cap;
        while (new_cap < needed) new_cap = new_cap < 4 ? 4 : new_cap * 2;
        dst->data = realloc(dst->data, (size_t)new_cap * dst->elem_size);
        dst->cap = new_cap;
    }
    if (dst->elem_copy) {
        for (long long i = 0; i < src->len; i++) {
            const void *s = (const char *)src->data + (i * src->elem_size);
            void *d = (char *)dst->data + ((dst->len + i) * dst->elem_size);
            dst->elem_copy(s, d);
        }
    } else {
        memcpy((char *)dst->data + (size_t)dst->len * dst->elem_size,
               src->data, (size_t)src->len * src->elem_size);
    }
    dst->len = needed;
}

SnArray *sn_array_slice(const SnArray *arr, long long start, long long end)
{
    if (!arr) return sn_array_new(sizeof(long long), 4);
    if (start < 0) start = arr->len + start;
    if (end < 0) end = arr->len + end;
    if (start < 0) start = 0;
    if (end > arr->len) end = arr->len;
    if (start >= end) return sn_array_new(arr->elem_size, 4);
    long long count = end - start;
    SnArray *dst = sn_array_new(arr->elem_size, count > 4 ? count : 4);
    dst->elem_release = arr->elem_release;
    dst->elem_copy = arr->elem_copy;
    dst->elem_tag = arr->elem_tag;
    if (arr->elem_copy) {
        for (long long i = 0; i < count; i++) {
            const void *s = (const char *)arr->data + ((start + i) * arr->elem_size);
            void *d = (char *)dst->data + (i * dst->elem_size);
            arr->elem_copy(s, d);
        }
    } else {
        memcpy(dst->data, (const char *)arr->data + (size_t)start * arr->elem_size,
               (size_t)count * arr->elem_size);
    }
    dst->len = count;
    return dst;
}

/* join: convert array elements to strings and join with separator */
char *sn_array_join(const SnArray *arr, const char *sep)
{
    if (!arr || arr->len == 0) return strdup("");
    size_t sep_len = sep ? strlen(sep) : 0;

    /* Use elem_tag for type dispatch when available, fall back to size heuristics */
    enum SnElemTag tag = arr->elem_tag;

    /* String arrays: tag or legacy detection */
    if (tag == SN_TAG_STRING || (tag == SN_TAG_DEFAULT && arr->elem_release == (void (*)(void *))sn_cleanup_str)) {
        size_t total = 0;
        for (long long i = 0; i < arr->len; i++) {
            const char *s = ((const char **)arr->data)[i];
            if (s) total += strlen(s);
            if (i > 0) total += sep_len;
        }
        char *result = malloc(total + 1);
        char *p = result;
        for (long long i = 0; i < arr->len; i++) {
            if (i > 0 && sep) { memcpy(p, sep, sep_len); p += sep_len; }
            const char *s = ((const char **)arr->data)[i];
            if (s) { size_t l = strlen(s); memcpy(p, s, l); p += l; }
        }
        *p = '\0';
        return result;
    }

    /* Other arrays: format each element using tag or size heuristics */
    size_t buf_size = 256;
    char *result = malloc(buf_size);
    size_t off = 0;
    for (long long i = 0; i < arr->len; i++) {
        char elem_buf[64];
        if (tag == SN_TAG_DOUBLE) {
            snprintf(elem_buf, sizeof(elem_buf), "%.5f", ((double *)arr->data)[i]);
        } else if (tag == SN_TAG_BOOL) {
            snprintf(elem_buf, sizeof(elem_buf), "%s", ((bool *)arr->data)[i] ? "true" : "false");
        } else if (tag == SN_TAG_CHAR) {
            snprintf(elem_buf, sizeof(elem_buf), "%c", ((char *)arr->data)[i]);
        } else if (tag == SN_TAG_BYTE) {
            snprintf(elem_buf, sizeof(elem_buf), "0x%02X", (unsigned)((unsigned char *)arr->data)[i]);
        } else if (tag == SN_TAG_INT || tag == SN_TAG_DEFAULT) {
            /* Default: treat as long long (matches int, long, uint, etc.) */
            snprintf(elem_buf, sizeof(elem_buf), "%lld", ((long long *)arr->data)[i]);
        } else {
            snprintf(elem_buf, sizeof(elem_buf), "?");
        }
        size_t el = strlen(elem_buf);
        size_t need = off + el + sep_len + 1;
        if (need > buf_size) {
            buf_size = need * 2;
            result = realloc(result, buf_size);
        }
        if (i > 0 && sep) { memcpy(result + off, sep, sep_len); off += sep_len; }
        memcpy(result + off, elem_buf, el);
        off += el;
    }
    result[off] = '\0';
    return result;
}

/* recursive array-to-string: {elem, elem, ...} with nested array support */
char *sn_array_to_string(const SnArray *arr)
{
    if (!arr || arr->len == 0) return strdup("[]");

    size_t buf_size = 256;
    char *result = malloc(buf_size);
    size_t off = 0;
    result[off++] = '[';

    enum SnElemTag tag = arr->elem_tag;

    for (long long i = 0; i < arr->len; i++) {
        if (i > 0) {
            /* append ", " */
            if (off + 2 >= buf_size) { buf_size *= 2; result = realloc(result, buf_size); }
            result[off++] = ',';
            result[off++] = ' ';
        }

        char elem_buf[64];
        char *heap_str = NULL;

        if (tag == SN_TAG_ARRAY) {
            SnArray *sub = ((SnArray **)arr->data)[i];
            heap_str = sn_array_to_string(sub);
        } else if (tag == SN_TAG_STRING || (tag == SN_TAG_DEFAULT && arr->elem_release == (void (*)(void *))sn_cleanup_str)) {
            const char *s = ((const char **)arr->data)[i];
            size_t sl = s ? strlen(s) : 0;
            heap_str = malloc(sl + 3);
            heap_str[0] = '"';
            if (s) memcpy(heap_str + 1, s, sl);
            heap_str[sl + 1] = '"';
            heap_str[sl + 2] = '\0';
        } else if (tag == SN_TAG_CHAR) {
            snprintf(elem_buf, sizeof(elem_buf), "'%c'", ((char *)arr->data)[i]);
        } else if (tag == SN_TAG_BOOL) {
            snprintf(elem_buf, sizeof(elem_buf), "%s", ((bool *)arr->data)[i] ? "true" : "false");
        } else if (tag == SN_TAG_DOUBLE) {
            snprintf(elem_buf, sizeof(elem_buf), "%g", ((double *)arr->data)[i]);
        } else if (tag == SN_TAG_BYTE) {
            snprintf(elem_buf, sizeof(elem_buf), "0x%02X", (unsigned)((unsigned char *)arr->data)[i]);
        } else {
            /* Default: long long (int, long, etc.) */
            snprintf(elem_buf, sizeof(elem_buf), "%lld", ((long long *)arr->data)[i]);
        }

        const char *to_append = heap_str ? heap_str : elem_buf;
        size_t el = strlen(to_append);
        if (off + el + 2 >= buf_size) {
            buf_size = (off + el + 2) * 2;
            result = realloc(result, buf_size);
        }
        memcpy(result + off, to_append, el);
        off += el;
        if (heap_str) free(heap_str);
    }

    if (off + 2 >= buf_size) { buf_size = off + 2; result = realloc(result, buf_size); }
    result[off++] = ']';
    result[off] = '\0';
    return result;
}

/* ---- String methods ---- */

SnArray *sn_str_split(const char *s, const char *delim)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_tag = SN_TAG_STRING;
    arr->elem_copy = sn_copy_str;
    if (!s) return arr;
    if (!delim || delim[0] == '\0') {
        /* Split into individual characters */
        for (size_t i = 0; i < strlen(s); i++) {
            char *c = malloc(2);
            c[0] = s[i]; c[1] = '\0';
            sn_array_push(arr, &c);
        }
        return arr;
    }
    size_t dlen = strlen(delim);
    const char *p = s;
    while (*p) {
        const char *found = strstr(p, delim);
        if (!found) {
            char *part = strdup(p);
            sn_array_push(arr, &part);
            break;
        }
        size_t len = (size_t)(found - p);
        char *part = malloc(len + 1);
        memcpy(part, p, len);
        part[len] = '\0';
        sn_array_push(arr, &part);
        p = found + dlen;
        if (*p == '\0') {
            /* Trailing delimiter — add empty string */
            char *empty = strdup("");
            sn_array_push(arr, &empty);
        }
    }
    return arr;
}

SnArray *sn_str_split_limit(const char *s, const char *delim, long long limit)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_tag = SN_TAG_STRING;
    arr->elem_copy = sn_copy_str;
    if (!s) return arr;
    if (limit <= 0) return arr;
    if (!delim || delim[0] == '\0') {
        /* Split into individual characters up to limit */
        for (size_t i = 0; i < strlen(s) && (long long)arr->len < limit; i++) {
            char *c = malloc(2);
            c[0] = s[i]; c[1] = '\0';
            sn_array_push(arr, &c);
        }
        return arr;
    }
    size_t dlen = strlen(delim);
    const char *p = s;
    while (*p) {
        if ((long long)arr->len >= limit - 1) {
            /* Last slot: take the rest of the string */
            char *part = strdup(p);
            sn_array_push(arr, &part);
            return arr;
        }
        const char *found = strstr(p, delim);
        if (!found) {
            char *part = strdup(p);
            sn_array_push(arr, &part);
            break;
        }
        size_t len = (size_t)(found - p);
        char *part = malloc(len + 1);
        memcpy(part, p, len);
        part[len] = '\0';
        sn_array_push(arr, &part);
        p = found + dlen;
        if (*p == '\0') {
            char *empty = strdup("");
            sn_array_push(arr, &empty);
        }
    }
    return arr;
}

SnArray *sn_str_split_lines(const char *s)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_copy = sn_copy_str;
    arr->elem_tag = SN_TAG_STRING;
    if (!s) return arr;
    const char *p = s;
    while (*p) {
        const char *eol = p;
        while (*eol && *eol != '\n' && *eol != '\r') eol++;
        size_t len = (size_t)(eol - p);
        char *line = malloc(len + 1);
        memcpy(line, p, len);
        line[len] = '\0';
        sn_array_push(arr, &line);
        if (*eol == '\r' && *(eol + 1) == '\n') eol += 2;
        else if (*eol) eol++;
        p = eol;
    }
    return arr;
}

SnArray *sn_str_split_whitespace(const char *s)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_copy = sn_copy_str;
    arr->elem_tag = SN_TAG_STRING;
    if (!s) return arr;
    const char *p = s;
    while (*p) {
        while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
        if (*p == '\0') break;
        const char *start = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        size_t len = (size_t)(p - start);
        char *word = malloc(len + 1);
        memcpy(word, start, len);
        word[len] = '\0';
        sn_array_push(arr, &word);
    }
    return arr;
}

char *sn_str_substring(const char *s, long long start, long long end)
{
    if (!s) return strdup("");
    long long slen = (long long)strlen(s);
    if (start < 0) start = 0;
    if (end > slen) end = slen;
    if (start >= end) return strdup("");
    size_t len = (size_t)(end - start);
    char *result = malloc(len + 1);
    memcpy(result, s + start, len);
    result[len] = '\0';
    return result;
}

char *sn_str_replace(const char *s, const char *old_s, const char *new_s)
{
    if (!s) return strdup("");
    if (!old_s || old_s[0] == '\0') return strdup(s);
    if (!new_s) new_s = "";
    size_t olen = strlen(old_s);
    size_t nlen = strlen(new_s);

    /* Count occurrences */
    int count = 0;
    const char *p = s;
    while ((p = strstr(p, old_s)) != NULL) { count++; p += olen; }

    size_t slen = strlen(s);
    size_t rlen = slen + (size_t)count * (nlen - olen);
    char *result = malloc(rlen + 1);
    char *dst = result;
    p = s;
    while (*p) {
        const char *found = strstr(p, old_s);
        if (!found) {
            strcpy(dst, p);
            break;
        }
        size_t chunk = (size_t)(found - p);
        memcpy(dst, p, chunk);
        dst += chunk;
        memcpy(dst, new_s, nlen);
        dst += nlen;
        p = found + olen;
    }
    result[rlen] = '\0';
    return result;
}

char *sn_str_to_upper(const char *s)
{
    if (!s) return strdup("");
    size_t len = strlen(s);
    char *result = malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = (char)toupper((unsigned char)s[i]);
    result[len] = '\0';
    return result;
}

char *sn_str_to_lower(const char *s)
{
    if (!s) return strdup("");
    size_t len = strlen(s);
    char *result = malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = (char)tolower((unsigned char)s[i]);
    result[len] = '\0';
    return result;
}

char *sn_str_trim(const char *s)
{
    if (!s) return strdup("");
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return strdup("");
    const char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    size_t len = (size_t)(end - s + 1);
    char *result = malloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

/* ---- Byte array encoding methods ---- */

char *sn_byte_array_to_hex(const SnArray *arr)
{
    static const char hex_chars[] = "0123456789abcdef";
    if (!arr || arr->len == 0) return strdup("");
    size_t len = arr->len * 2;
    char *result = malloc(len + 1);
    const unsigned char *data = (const unsigned char *)arr->data;
    for (long long i = 0; i < arr->len; i++) {
        result[i * 2]     = hex_chars[data[i] >> 4];
        result[i * 2 + 1] = hex_chars[data[i] & 0x0F];
    }
    result[len] = '\0';
    return result;
}

char *sn_byte_array_to_base64(const SnArray *arr)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (!arr || arr->len == 0) return strdup("");
    const unsigned char *data = (const unsigned char *)arr->data;
    long long len = arr->len;
    size_t out_len = 4 * ((len + 2) / 3);
    char *result = malloc(out_len + 1);
    size_t j = 0;
    for (long long i = 0; i < len; i += 3) {
        unsigned int a = data[i];
        unsigned int b = (i + 1 < len) ? data[i + 1] : 0;
        unsigned int c = (i + 2 < len) ? data[i + 2] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;
        result[j++] = b64[(triple >> 18) & 0x3F];
        result[j++] = b64[(triple >> 12) & 0x3F];
        result[j++] = (i + 1 < len) ? b64[(triple >> 6) & 0x3F] : '=';
        result[j++] = (i + 2 < len) ? b64[triple & 0x3F] : '=';
    }
    result[j] = '\0';
    return result;
}
