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
    SnArray *arr = malloc(sizeof(SnArray));
    arr->data = malloc(elem_size * (size_t)initial_cap);
    arr->len = 0;
    arr->cap = initial_cap;
    arr->elem_size = elem_size;
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
    for (long long i = start; i < end; i++) {
        sn_array_push(arr, &i);
    }
    return arr;
}
