#ifndef SN_STRING_H
#define SN_STRING_H

#include "sn_core.h"
#include "sn_array.h"

/* ---- String concat ---- */

char *sn_str_concat(const char *a, const char *b);
char *sn_str_concat_multi(int count, ...);

/* ---- String contains / indexOf ---- */

static inline bool sn_str_contains(const char *s, const char *substr)
{
    if (!s || !substr) return false;
    return strstr(s, substr) != NULL;
}
static inline long long sn_str_indexOf(const char *s, const char *substr)
{
    if (!s || !substr) return -1;
    const char *p = strstr(s, substr);
    return p ? (long long)(p - s) : -1;
}

/* String method dispatch macros */
#define __sn___contains(container_ptr, val) sn_str_contains(*(container_ptr), (val))
#define __sn___indexOf(container_ptr, val) sn_str_indexOf(*(container_ptr), (val))

/* ---- String split ---- */

SnArray *sn_str_split(const char *s, const char *delim);
SnArray *sn_str_split_limit(const char *s, const char *delim, long long limit);
SnArray *sn_str_split_lines(const char *s);
SnArray *sn_str_split_whitespace(const char *s);

#define __sn___split2(str_ptr, delim) sn_str_split(*(str_ptr), (delim))
#define __sn___split3(str_ptr, delim, lim) sn_str_split_limit(*(str_ptr), (delim), (lim))
#define __sn___split_pick(_1, _2, _3, NAME, ...) NAME
#define __sn___split(...) __sn___split_pick(__VA_ARGS__, __sn___split3, __sn___split2)(__VA_ARGS__)

#define __sn___splitLines(str_ptr) sn_str_split_lines(*(str_ptr))
#define __sn___splitWhitespace(str_ptr) sn_str_split_whitespace(*(str_ptr))

/* ---- String operations ---- */

char *sn_str_substring(const char *s, long long start, long long end);
char *sn_str_replace(const char *s, const char *old_s, const char *new_s);
char *sn_str_to_upper(const char *s);
char *sn_str_to_lower(const char *s);
char *sn_str_trim(const char *s);

#define __sn___substring(str_ptr, start, end) sn_str_substring(*(str_ptr), (start), (end))
#define __sn___replace(str_ptr, old_s, new_s) sn_str_replace(*(str_ptr), (old_s), (new_s))
#define __sn___toUpper(str_ptr) sn_str_to_upper(*(str_ptr))
#define __sn___toLower(str_ptr) sn_str_to_lower(*(str_ptr))
#define __sn___trim(str_ptr) sn_str_trim(*(str_ptr))

/* ---- String predicates ---- */

static inline bool sn_str_starts_with(const char *s, const char *prefix)
{
    if (!s || !prefix) return false;
    size_t pl = strlen(prefix);
    return strncmp(s, prefix, pl) == 0;
}
static inline bool sn_str_ends_with(const char *s, const char *suffix)
{
    if (!s || !suffix) return false;
    size_t sl = strlen(s), xl = strlen(suffix);
    if (xl > sl) return false;
    return strcmp(s + sl - xl, suffix) == 0;
}

#define __sn___startsWith(str_ptr, prefix) sn_str_starts_with(*(str_ptr), (prefix))
#define __sn___endsWith(str_ptr, suffix) sn_str_ends_with(*(str_ptr), (suffix))

/* charAt: str.charAt(index) → char */
static inline char sn_str_char_at(const char *s, long long index)
{
    if (!s || index < 0 || index >= (long long)strlen(s)) return '\0';
    return s[index];
}
#define __sn___charAt(str_ptr, index) sn_str_char_at(*(str_ptr), (index))

/* isBlank: str.isBlank() → bool */
static inline bool sn_str_is_blank(const char *s)
{
    if (!s) return true;
    while (*s) { if (!isspace((unsigned char)*s)) return false; s++; }
    return true;
}
#define __sn___isBlank(str_ptr) sn_str_is_blank(*(str_ptr))

/* append: str.append(suffix) — string concatenation, returns new string */
#define __sn___append(str_ptr, suffix) sn_str_concat(*(str_ptr), (suffix))

/* String concat via + operator */
#define sn_add_string(a, b) sn_str_concat((a), (b))

#endif
