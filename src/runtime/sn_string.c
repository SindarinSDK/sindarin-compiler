#include "sn_string.h"
#include "sn_core.h"
#include "sn_array.h"
#include <stdarg.h>

/* ---- String concat ---- */

char *sn_str_concat(const char *a, const char *b)
{
    size_t la = a ? strlen(a) : 0;
    size_t lb = b ? strlen(b) : 0;
    char *result = sn_malloc(la + lb + 1);
    if (la) memcpy(result, a, la);
    if (lb) memcpy(result + la, b, lb);
    result[la + lb] = '\0';
    return result;
}

char *sn_str_concat_multi(int count, ...)
{
    va_list args;
    size_t total = 0;

    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *s = va_arg(args, const char *);
        if (s) total += strlen(s);
    }
    va_end(args);

    char *result = sn_malloc(total + 1);
    char *p = result;

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

/* asprintf-style: snprintf into a malloc'd buffer sized to fit.
 * Returned pointer is owned by caller. Used by interpolated_string codegen
 * to format each typed part without a fixed buffer. */
char *sn_str_fmt(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    char *buf = sn_malloc((size_t)n + 1);
    va_start(ap, fmt);
    vsnprintf(buf, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return buf;
}

/* ---- String split ---- */

SnArray *sn_str_split(const char *s, const char *delim)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_tag = SN_TAG_STRING;
    arr->elem_copy = sn_copy_str;
    if (!s) return arr;
    if (!delim || delim[0] == '\0') {
        for (size_t i = 0; i < strlen(s); i++) {
            char *c = sn_malloc(2);
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
        char *part = sn_malloc(len + 1);
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

SnArray *sn_str_split_limit(const char *s, const char *delim, long long limit)
{
    SnArray *arr = sn_array_new(sizeof(char *), 4);
    arr->elem_release = (void (*)(void *))sn_cleanup_str;
    arr->elem_tag = SN_TAG_STRING;
    arr->elem_copy = sn_copy_str;
    if (!s) return arr;
    if (limit <= 0) return arr;
    if (!delim || delim[0] == '\0') {
        for (size_t i = 0; i < strlen(s) && (long long)arr->len < limit; i++) {
            char *c = sn_malloc(2);
            c[0] = s[i]; c[1] = '\0';
            sn_array_push(arr, &c);
        }
        return arr;
    }
    size_t dlen = strlen(delim);
    const char *p = s;
    while (*p) {
        if ((long long)arr->len >= limit - 1) {
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
        char *part = sn_malloc(len + 1);
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
        char *line = sn_malloc(len + 1);
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
        char *word = sn_malloc(len + 1);
        memcpy(word, start, len);
        word[len] = '\0';
        sn_array_push(arr, &word);
    }
    return arr;
}

/* ---- String operations ---- */

char *sn_str_substring(const char *s, long long start, long long end)
{
    if (!s) return strdup("");
    long long slen = (long long)strlen(s);
    if (start < 0) start = 0;
    if (end > slen) end = slen;
    if (start >= end) return strdup("");
    size_t len = (size_t)(end - start);
    char *result = sn_malloc(len + 1);
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

    /* Use signed arithmetic to avoid unsigned underflow when nlen < olen */
    size_t slen = strlen(s);
    long long diff = (long long)nlen - (long long)olen;
    size_t rlen = (size_t)((long long)slen + (long long)count * diff);
    char *result = sn_malloc(rlen + 1);
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
    char *result = sn_malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = (char)toupper((unsigned char)s[i]);
    result[len] = '\0';
    return result;
}

char *sn_str_to_lower(const char *s)
{
    if (!s) return strdup("");
    size_t len = strlen(s);
    char *result = sn_malloc(len + 1);
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
    char *result = sn_malloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}
