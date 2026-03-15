#include "sn_array.h"
#include "sn_core.h"

/* ---- Core array operations ---- */

SnArray *sn_array_new(size_t elem_size, long long initial_cap)
{
    if (initial_cap < 4) initial_cap = 4;
    SnArray *arr = sn_calloc(1, sizeof(SnArray));
    arr->data = sn_malloc(elem_size * (size_t)initial_cap);
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
        arr->data = sn_realloc(arr->data, arr->elem_size * (size_t)arr->cap);
    }
    memcpy((char *)arr->data + arr->elem_size * (size_t)arr->len, elem, arr->elem_size);
    arr->len++;
}

void sn_array_push_safe(SnArray *arr, const void *elem, size_t value_size)
{
    if (arr->len >= arr->cap) {
        arr->cap *= 2;
        arr->data = sn_realloc(arr->data, arr->elem_size * (size_t)arr->cap);
    }
    char *dest = (char *)arr->data + arr->elem_size * (size_t)arr->len;
    size_t copy_size = value_size < (size_t)arr->elem_size ? value_size : (size_t)arr->elem_size;
    if (copy_size < (size_t)arr->elem_size)
        memset(dest, 0, arr->elem_size);
    memcpy(dest, elem, copy_size);
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
    sn_array_copy_elems(dst->data, 0, src->data, 0, src->len, src->elem_size, src->elem_copy);
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
    sn_array_copy_elems(dst->data, 0, a->data, 0, a->len, a->elem_size, a->elem_copy);
    sn_array_copy_elems(dst->data, a->len, b->data, 0, b->len, a->elem_size, a->elem_copy);
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
        dst->data = sn_realloc(dst->data, (size_t)new_cap * dst->elem_size);
        dst->cap = new_cap;
    }
    sn_array_copy_elems(dst->data, dst->len, src->data, 0, src->len, dst->elem_size, dst->elem_copy);
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
    sn_array_copy_elems(dst->data, 0, arr->data, start, count, arr->elem_size, arr->elem_copy);
    dst->len = count;
    return dst;
}

/* ---- Array join ---- */

char *sn_array_join(const SnArray *arr, const char *sep)
{
    if (!arr || arr->len == 0) return strdup("");
    size_t sep_len = sep ? strlen(sep) : 0;

    enum SnElemTag tag = arr->elem_tag;

    /* String arrays */
    if (tag == SN_TAG_STRING) {
        size_t total = 0;
        for (long long i = 0; i < arr->len; i++) {
            const char *s = ((const char **)arr->data)[i];
            if (s) total += strlen(s);
            if (i > 0) total += sep_len;
        }
        char *result = sn_malloc(total + 1);
        char *p = result;
        for (long long i = 0; i < arr->len; i++) {
            if (i > 0 && sep) { memcpy(p, sep, sep_len); p += sep_len; }
            const char *s = ((const char **)arr->data)[i];
            if (s) { size_t l = strlen(s); memcpy(p, s, l); p += l; }
        }
        *p = '\0';
        return result;
    }

    /* Other arrays: format each element using tag */
    size_t buf_size = 256;
    char *result = sn_malloc(buf_size);
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
            snprintf(elem_buf, sizeof(elem_buf), "%lld", ((long long *)arr->data)[i]);
        } else {
            snprintf(elem_buf, sizeof(elem_buf), "?");
        }
        size_t el = strlen(elem_buf);
        size_t need = off + el + sep_len + 1;
        if (need > buf_size) {
            buf_size = need * 2;
            result = sn_realloc(result, buf_size);
        }
        if (i > 0 && sep) { memcpy(result + off, sep, sep_len); off += sep_len; }
        memcpy(result + off, elem_buf, el);
        off += el;
    }
    result[off] = '\0';
    return result;
}

/* ---- Array to_string (recursive) ---- */

char *sn_array_to_string(const SnArray *arr)
{
    if (!arr || arr->len == 0) return strdup("[]");

    size_t buf_size = 256;
    char *result = sn_malloc(buf_size);
    size_t off = 0;
    result[off++] = '[';

    enum SnElemTag tag = arr->elem_tag;

    for (long long i = 0; i < arr->len; i++) {
        if (i > 0) {
            if (off + 2 >= buf_size) { buf_size *= 2; result = sn_realloc(result, buf_size); }
            result[off++] = ',';
            result[off++] = ' ';
        }

        char elem_buf[64];
        char *heap_str = NULL;

        if (tag == SN_TAG_ARRAY) {
            SnArray *sub = ((SnArray **)arr->data)[i];
            heap_str = sn_array_to_string(sub);
        } else if (tag == SN_TAG_STRING) {
            const char *s = ((const char **)arr->data)[i];
            size_t sl = s ? strlen(s) : 0;
            heap_str = sn_malloc(sl + 3);
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
            snprintf(elem_buf, sizeof(elem_buf), "%lld", ((long long *)arr->data)[i]);
        }

        const char *to_append = heap_str ? heap_str : elem_buf;
        size_t el = strlen(to_append);
        if (off + el + 2 >= buf_size) {
            buf_size = (off + el + 2) * 2;
            result = sn_realloc(result, buf_size);
        }
        memcpy(result + off, to_append, el);
        off += el;
        if (heap_str) free(heap_str);
    }

    if (off + 2 >= buf_size) { buf_size = off + 2; result = sn_realloc(result, buf_size); }
    result[off++] = ']';
    result[off] = '\0';
    return result;
}
