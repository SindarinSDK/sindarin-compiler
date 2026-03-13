#ifndef SN_MINIMAL_H
#define SN_MINIMAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

/* ---- Threading ---- */

/* Platform-specific pthread include */
#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        #include <pthread.h>
    #else
        #include "compat_pthread.h"
    #endif
#else
    #include <pthread.h>
#endif

typedef struct {
    pthread_t thread;
    void *result;
    size_t result_size;
    int joined;
} SnThread;

static inline void sn_cleanup_thread(SnThread **p)
{
    if (*p) {
        if (!(*p)->joined) {
            pthread_join((*p)->thread, NULL);
        }
        free((*p)->result);
        free(*p);
    }
}

#define sn_auto_thread __attribute__((cleanup(sn_cleanup_thread)))

static inline SnThread *sn_thread_create(void)
{
    SnThread *t = calloc(1, sizeof(SnThread));
    return t;
}

static inline void sn_thread_join(SnThread *t)
{
    if (t && !t->joined) {
        pthread_join(t->thread, NULL);
        t->joined = 1;
    }
}

/* ---- Scope-based cleanup ---- */

static inline void sn_cleanup_str(char **p) { free(*p); }
static inline void sn_cleanup_ptr(void **p) { free(*p); }
static inline void sn_cleanup_fn(void **p) {
    if (*p) {
        void (**cleanup)(void *) = (void (**)(void *))((char *)*p + sizeof(void *) + sizeof(size_t));
        if (*cleanup)
            (*cleanup)(*p);
        else
            free(*p);
    }
    *p = NULL;
}

#define sn_auto_str __attribute__((cleanup(sn_cleanup_str)))
#define sn_auto_ptr __attribute__((cleanup(sn_cleanup_ptr)))
#define sn_auto_fn __attribute__((cleanup(sn_cleanup_fn)))
/* Generic cleanup for captured variables (any pointer type) */
static inline void sn_cleanup_capture(void *p) { free(*(void **)p); *(void **)p = NULL; }
#define sn_auto_capture __attribute__((cleanup(sn_cleanup_capture)))

/* ---- String operations ---- */

static inline char *sn_strdup(const char *s)
{
    return s ? strdup(s) : NULL;
}

static inline long long sn_str_length(const char *s)
{
    return s ? (long long)strlen(s) : 0;
}

char *sn_str_concat(const char *a, const char *b);
char *sn_str_concat_multi(int count, ...);

/* ---- Dynamic array ---- */

enum SnElemTag { SN_TAG_DEFAULT = 0, SN_TAG_INT, SN_TAG_DOUBLE, SN_TAG_STRING, SN_TAG_BOOL, SN_TAG_CHAR, SN_TAG_BYTE, SN_TAG_STRUCT, SN_TAG_ARRAY };

typedef struct {
    void *data;
    long long len;
    long long cap;
    size_t elem_size;
    void (*elem_release)(void *);                    /* per-element cleanup (NULL = no-op) */
    void (*elem_copy)(const void *src, void *dst);   /* per-element copy (NULL = memcpy) */
    enum SnElemTag elem_tag;                         /* element type tag for join/toString */
} SnArray;

static inline void sn_cleanup_array(SnArray **p)
{
    if (*p) {
        if ((*p)->elem_release) {
            for (long long i = 0; i < (*p)->len; i++) {
                void *elem = (char *)(*p)->data + (i * (*p)->elem_size);
                (*p)->elem_release(elem);
            }
        }
        free((*p)->data);
        free(*p);
    }
}

static inline void sn_array_free(SnArray *a) { if (a) { sn_cleanup_array(&a); } }

#define sn_auto_arr __attribute__((cleanup(sn_cleanup_array)))

SnArray *sn_array_new(size_t elem_size, long long initial_cap);
void sn_array_push(SnArray *arr, const void *elem);
void *sn_array_get(SnArray *arr, long long index);
long long sn_array_length(SnArray *arr);
SnArray *sn_array_range(long long start, long long end);
SnArray *sn_array_copy(const SnArray *src);
SnArray *sn_array_slice(const SnArray *arr, long long start, long long end);

/* Create a byte array from raw pointer memory: ptr[start..end] */
static inline SnArray *sn_array_from_ptr(const void *ptr, long long start, long long end) {
    if (end <= start) return sn_array_new(sizeof(unsigned char), 0);
    long long count = end - start;
    SnArray *arr = sn_array_new(sizeof(unsigned char), count);
    arr->elem_tag = SN_TAG_BYTE;
    if (ptr) {
        memcpy(arr->data, (const unsigned char *)ptr + start, (size_t)count);
    } else {
        memset(arr->data, 0, (size_t)count);
    }
    arr->len = count;
    return arr;
}

/* ---- Array equality ---- */

static inline bool sn_array_equals(const SnArray *a, const SnArray *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->len != b->len) return false;
    return memcmp(a->data, b->data, a->len * a->elem_size) == 0;
}

static inline bool sn_array_equals_string(const SnArray *a, const SnArray *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    if (a->len != b->len) return false;
    for (long long i = 0; i < a->len; i++) {
        const char *sa = ((const char **)a->data)[i];
        const char *sb = ((const char **)b->data)[i];
        if (sa == sb) continue;
        if (!sa || !sb) return false;
        if (strcmp(sa, sb) != 0) return false;
    }
    return true;
}

/* ---- String copy helper (for array elem_copy) ---- */

static inline void sn_copy_str(const void *src, void *dst)
{
    const char *s = *(const char **)src;
    *(char **)dst = s ? strdup(s) : NULL;
}

/* ---- Array accessors (generic + typed) ---- */

#define sn_array_get_as(arr, index, T) (((T *)(arr)->data)[(index)])

static inline long long      sn_array_get_long(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, long long); }
static inline double          sn_array_get_double(SnArray *arr, long long i)  { return sn_array_get_as(arr, i, double); }
static inline char *          sn_array_get_string(SnArray *arr, long long i)  { return sn_array_get_as(arr, i, char *); }
static inline bool            sn_array_get_bool(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, bool); }
static inline char            sn_array_get_char(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, char); }
static inline unsigned char   sn_array_get_byte(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, unsigned char); }
static inline void *          sn_array_get_generic(SnArray *arr, long long i) { return sn_array_get(arr, i); }

/* ---- Array built-in methods (type-generic via __typeof__) ---- */

/* push: arr.push(value) — template emits __sn___push(&arr, value)
 * For string arrays, the template must wrap with strdup() to ensure
 * the array owns the string (elem_release calls free). */
#define __sn___push(arr_ptr, ...) do { \
    __typeof__(__VA_ARGS__) __sn_push_tmp__ = (__VA_ARGS__); \
    sn_array_push(*(arr_ptr), &__sn_push_tmp__); \
} while(0)

/* pop: returns void* to the popped element (still in the buffer, past len).
 * Template emits the dereference cast: *(T *)__sn___pop(&arr) */
static inline void *sn_array_pop_ref(SnArray *arr)
{
    if (arr->len <= 0) return NULL;
    arr->len--;
    return (char *)arr->data + arr->elem_size * (size_t)arr->len;
}
#define __sn___pop(arr_ptr) sn_array_pop_ref(*(arr_ptr))

/* clear: arr.clear() */
#define __sn___clear(arr_ptr) do { \
    SnArray *__sn_a__ = *(arr_ptr); \
    if (__sn_a__->elem_release) { \
        for (long long __sn_i__ = 0; __sn_i__ < __sn_a__->len; __sn_i__++) { \
            __sn_a__->elem_release((char *)__sn_a__->data + __sn_i__ * __sn_a__->elem_size); \
        } \
    } \
    __sn_a__->len = 0; \
} while(0)

/* reverse: arr.reverse() */
static inline void sn_array_reverse(SnArray *arr)
{
    if (!arr || arr->len < 2) return;
    char tmp[arr->elem_size]; /* VLA */
    for (long long lo = 0, hi = arr->len - 1; lo < hi; lo++, hi--) {
        void *a = (char *)arr->data + (size_t)lo * arr->elem_size;
        void *b = (char *)arr->data + (size_t)hi * arr->elem_size;
        memcpy(tmp, a, arr->elem_size);
        memcpy(a, b, arr->elem_size);
        memcpy(b, tmp, arr->elem_size);
    }
}
#define __sn___reverse(arr_ptr) sn_array_reverse(*(arr_ptr))

/* remove: arr.remove(index) */
static inline void sn_array_remove(SnArray *arr, long long index)
{
    if (!arr || index < 0 || index >= arr->len) return;
    if (arr->elem_release) {
        arr->elem_release((char *)arr->data + (size_t)index * arr->elem_size);
    }
    if (index < arr->len - 1) {
        memmove((char *)arr->data + (size_t)index * arr->elem_size,
                (char *)arr->data + (size_t)(index + 1) * arr->elem_size,
                (size_t)(arr->len - index - 1) * arr->elem_size);
    }
    arr->len--;
}
#define __sn___remove(arr_ptr, idx) sn_array_remove(*(arr_ptr), (idx))

/* insert: arr.insert(value, index) — template emits __sn___insert(&arr, value, index) */
static inline void sn_array_insert(SnArray *arr, const void *elem, long long index)
{
    if (!arr || index < 0 || index > arr->len) return;
    if (arr->len >= arr->cap) {
        arr->cap *= 2;
        arr->data = realloc(arr->data, arr->elem_size * (size_t)arr->cap);
    }
    if (index < arr->len) {
        memmove((char *)arr->data + (size_t)(index + 1) * arr->elem_size,
                (char *)arr->data + (size_t)index * arr->elem_size,
                (size_t)(arr->len - index) * arr->elem_size);
    }
    memcpy((char *)arr->data + (size_t)index * arr->elem_size, elem, arr->elem_size);
    arr->len++;
}
/* insert: idx before val so val can be variadic (compound literals contain commas) */
#define __sn___insert(arr_ptr, idx, ...) do { \
    __typeof__(__VA_ARGS__) __sn_ins_tmp__ = (__VA_ARGS__); \
    sn_array_insert(*(arr_ptr), &__sn_ins_tmp__, (idx)); \
} while(0)

/* clone: var b = arr.clone() */
#define __sn___clone(arr_ptr) sn_array_copy(*(arr_ptr))

/* concat: var c = a.concat(b) */
SnArray *sn_array_concat(const SnArray *a, const SnArray *b);
#define __sn___concat(arr_ptr, other) sn_array_concat(*(arr_ptr), (other))

/* extend: append all elements of src into dst (for spread operator) */
void sn_array_extend(SnArray *dst, const SnArray *src);

/* ---- Array method aliases with __sn__arr_ prefix ---- */
/* With type.name="arr" for arrays, templates generate __sn__arr_methodName */
#define __sn__arr_push(arr_ptr, ...) __sn___push(arr_ptr, __VA_ARGS__)
#define __sn__arr_pop(arr_ptr) __sn___pop(arr_ptr)
#define __sn__arr_clear(arr_ptr) __sn___clear(arr_ptr)
#define __sn__arr_reverse(arr_ptr) __sn___reverse(arr_ptr)
#define __sn__arr_remove(arr_ptr, idx) __sn___remove(arr_ptr, idx)
#define __sn__arr_insert(arr_ptr, idx, ...) __sn___insert(arr_ptr, idx, __VA_ARGS__)
#define __sn__arr_clone(arr_ptr) __sn___clone(arr_ptr)
#define __sn__arr_concat(arr_ptr, other) __sn___concat(arr_ptr, other)
#define __sn__arr_join(arr_ptr, sep) __sn___join(arr_ptr, sep)
#define __sn__arr_toString(arr_ptr) __sn___toString(arr_ptr)

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

/* contains: arr.contains(value) or str.contains(substr) — dispatches on container type */
static inline bool sn_array_contains(const SnArray *arr, const void *elem)
{
    for (long long i = 0; i < arr->len; i++) {
        if (memcmp((char *)arr->data + (size_t)i * arr->elem_size, elem, arr->elem_size) == 0)
            return true;
    }
    return false;
}
static inline bool sn_array_contains_string(const SnArray *arr, const char *s)
{
    for (long long i = 0; i < arr->len; i++) {
        const char *elem = ((const char **)arr->data)[i];
        if (elem == s) return true;
        if (elem && s && strcmp(elem, s) == 0) return true;
    }
    return false;
}
/* String contains dispatches directly */
#define __sn___contains(container_ptr, val) sn_str_contains(*(container_ptr), (val))
/* Array contains: use void* dispatch to avoid _Generic cross-branch type errors.
 * For strings: pass pointer value directly via uintptr_t cast (always valid).
 * For non-strings: pass address of compound literal. */
static inline bool sn_array_contains_dispatch(const SnArray *arr, const void *val, int is_string)
{
    if (is_string) return sn_array_contains_string(arr, (const char *)val);
    return sn_array_contains(arr, val);
}
#define __sn__arr_contains(arr_ptr, val) \
    sn_array_contains_dispatch(*(arr_ptr), \
        _Generic((val), \
            char *:       (const void *)(uintptr_t)(val), \
            const char *: (const void *)(uintptr_t)(val), \
            default:      (const void *)&(__typeof__(val)){val}), \
        _Generic((val), char *: 1, const char *: 1, default: 0))

/* indexOf: arr.indexOf(value) or str.indexOf(substr) — dispatches on container type */
static inline long long sn_array_indexOf(const SnArray *arr, const void *elem)
{
    for (long long i = 0; i < arr->len; i++) {
        if (memcmp((char *)arr->data + (size_t)i * arr->elem_size, elem, arr->elem_size) == 0)
            return i;
    }
    return -1;
}
static inline long long sn_array_indexOf_string(const SnArray *arr, const char *s)
{
    for (long long i = 0; i < arr->len; i++) {
        const char *elem = ((const char **)arr->data)[i];
        if (elem == s) return i;
        if (elem && s && strcmp(elem, s) == 0) return i;
    }
    return -1;
}
/* String indexOf dispatches directly */
#define __sn___indexOf(container_ptr, val) sn_str_indexOf(*(container_ptr), (val))
/* Array indexOf: same void* dispatch pattern as contains */
static inline long long sn_array_indexOf_dispatch(const SnArray *arr, const void *val, int is_string)
{
    if (is_string) return sn_array_indexOf_string(arr, (const char *)val);
    return sn_array_indexOf(arr, val);
}
#define __sn__arr_indexOf(arr_ptr, val) \
    sn_array_indexOf_dispatch(*(arr_ptr), \
        _Generic((val), \
            char *:       (const void *)(uintptr_t)(val), \
            const char *: (const void *)(uintptr_t)(val), \
            default:      (const void *)&(__typeof__(val)){val}), \
        _Generic((val), char *: 1, const char *: 1, default: 0))

/* join: arr.join(sep) — returns heap-allocated string */
char *sn_array_join(const SnArray *arr, const char *sep);
#define __sn___join(arr_ptr, sep) sn_array_join(*(arr_ptr), (sep))

/* to_string: recursive array-to-string with {}-delimited format */
char *sn_array_to_string(const SnArray *arr);

/* toString: arr.toString() — returns heap-allocated string like "[elem1, elem2]"
 * For byte arrays: interprets bytes as UTF-8 string instead */
#define __sn___toString(arr_ptr) ({ SnArray *__ta__ = *(arr_ptr); \
    __ta__->elem_tag == SN_TAG_BYTE ? sn_byte_array_to_string(__ta__) : \
    ({ char *__j__ = sn_array_join(__ta__, ", "); \
    size_t __jl__ = __j__ ? strlen(__j__) : 0; \
    char *__s__ = malloc(__jl__ + 3); \
    __s__[0] = '['; if (__j__) memcpy(__s__ + 1, __j__, __jl__); \
    __s__[__jl__ + 1] = ']'; __s__[__jl__ + 2] = '\0'; free(__j__); __s__; }); })

/* ---- Byte array / string conversion methods ---- */

/* byte[].toString() — interpret bytes as UTF-8 string */
static inline char *sn_byte_array_to_string(const SnArray *arr) {
    if (!arr || arr->len == 0) return strdup("");
    char *s = malloc(arr->len + 1);
    memcpy(s, arr->data, arr->len);
    s[arr->len] = '\0';
    return s;
}

/* str.toBytes() — convert string to byte array */
static inline SnArray *sn_string_to_bytes(const char *s) {
    size_t len = s ? strlen(s) : 0;
    SnArray *arr = sn_array_new(sizeof(unsigned char), len);
    arr->elem_tag = SN_TAG_BYTE;
    for (size_t i = 0; i < len; i++)
        sn_array_push(arr, &(unsigned char){ (unsigned char)s[i] });
    return arr;
}
#define __sn___toBytes(str_ptr) sn_string_to_bytes(*(str_ptr))

/* byte[].toHex() — hex string representation */
char *sn_byte_array_to_hex(const SnArray *arr);
#define __sn__arr_toHex(arr_ptr) sn_byte_array_to_hex(*(arr_ptr))

/* byte[].toBase64() — base64 encoding */
char *sn_byte_array_to_base64(const SnArray *arr);
#define __sn__arr_toBase64(arr_ptr) sn_byte_array_to_base64(*(arr_ptr))

/* byte[].toStringLatin1() — Latin-1 to UTF-8 conversion */
static inline char *sn_byte_array_to_string_latin1(const SnArray *arr) {
    if (!arr || arr->len == 0) return strdup("");
    /* Worst case: every byte is >127 and needs 2 UTF-8 bytes */
    char *s = malloc(arr->len * 2 + 1);
    size_t out = 0;
    const unsigned char *data = (const unsigned char *)arr->data;
    for (size_t i = 0; i < arr->len; i++) {
        unsigned char b = data[i];
        if (b < 128) {
            s[out++] = (char)b;
        } else {
            s[out++] = (char)(0xC0 | (b >> 6));
            s[out++] = (char)(0x80 | (b & 0x3F));
        }
    }
    s[out] = '\0';
    return s;
}
#define __sn__arr_toStringLatin1(arr_ptr) sn_byte_array_to_string_latin1(*(arr_ptr))

/* append: str.append(suffix) — string concatenation, returns new string */
#define __sn___append(str_ptr, suffix) sn_str_concat(*(str_ptr), (suffix))

/* ---- String built-in methods ---- */
/* All take char ** (pointer to the string variable) as first arg. */

/* split: str.split(delimiter) or str.split(delimiter, limit) → SnArray* of char* */
SnArray *sn_str_split(const char *s, const char *delim);
SnArray *sn_str_split_limit(const char *s, const char *delim, long long limit);
#define __sn___split2(str_ptr, delim) sn_str_split(*(str_ptr), (delim))
#define __sn___split3(str_ptr, delim, lim) sn_str_split_limit(*(str_ptr), (delim), (lim))
#define __sn___split_pick(_1, _2, _3, NAME, ...) NAME
#define __sn___split(...) __sn___split_pick(__VA_ARGS__, __sn___split3, __sn___split2)(__VA_ARGS__)

/* splitLines: str.splitLines() → SnArray* of char* */
SnArray *sn_str_split_lines(const char *s);
#define __sn___splitLines(str_ptr) sn_str_split_lines(*(str_ptr))

/* splitWhitespace: str.splitWhitespace() → SnArray* of char* */
SnArray *sn_str_split_whitespace(const char *s);
#define __sn___splitWhitespace(str_ptr) sn_str_split_whitespace(*(str_ptr))

/* substring: str.substring(start, end) → new string */
char *sn_str_substring(const char *s, long long start, long long end);
#define __sn___substring(str_ptr, start, end) sn_str_substring(*(str_ptr), (start), (end))

/* replace: str.replace(old, new) → new string */
char *sn_str_replace(const char *s, const char *old_s, const char *new_s);
#define __sn___replace(str_ptr, old_s, new_s) sn_str_replace(*(str_ptr), (old_s), (new_s))

/* toUpper / toLower: str.toUpper() → new string */
char *sn_str_to_upper(const char *s);
char *sn_str_to_lower(const char *s);
#define __sn___toUpper(str_ptr) sn_str_to_upper(*(str_ptr))
#define __sn___toLower(str_ptr) sn_str_to_lower(*(str_ptr))

/* trim: str.trim() → new string */
char *sn_str_trim(const char *s);
#define __sn___trim(str_ptr) sn_str_trim(*(str_ptr))

/* startsWith / endsWith */
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

/* toString: value.toString() — for char, returns single-char string */
static inline char *sn_char_to_string(char c)
{
    char *s = malloc(2);
    s[0] = c; s[1] = '\0';
    return s;
}
/* char methods — prefixed with __sn__char_ to distinguish from string methods */
#define __sn__char_toString(ptr) sn_char_to_string(*(ptr))
#define __sn__char_toUpper(ptr) ((char)toupper((unsigned char)(*(ptr))))
#define __sn__char_toLower(ptr) ((char)tolower((unsigned char)(*(ptr))))
#define __sn__char_toInt(ptr) ((long long)(*(ptr)))
#define __sn__char_isDigit(ptr) ((bool)isdigit((unsigned char)(*(ptr))))
#define __sn__char_isAlpha(ptr) ((bool)isalpha((unsigned char)(*(ptr))))
#define __sn__char_isWhitespace(ptr) ((bool)isspace((unsigned char)(*(ptr))))
#define __sn__char_isAlnum(ptr) ((bool)isalnum((unsigned char)(*(ptr))))

/* int methods */
#define __sn__int_toDouble(ptr) ((double)(*(ptr)))
#define __sn__int_toLong(ptr) ((long long)(*(ptr)))
#define __sn__int_toUint(ptr) ((unsigned long long)(*(ptr)))
#define __sn__int_toByte(ptr) ((unsigned char)(*(ptr)))
#define __sn__int_toChar(ptr) ((char)(*(ptr)))

/* long methods */
#define __sn__long_toInt(ptr) ((long long)(*(ptr)))
#define __sn__long_toDouble(ptr) ((double)(*(ptr)))

/* double methods */
#define __sn__double_toInt(ptr) ((long long)(*(ptr)))
#define __sn__double_toLong(ptr) ((long long)(*(ptr)))

/* uint methods */
#define __sn__uint_toInt(ptr) ((long long)(*(ptr)))
#define __sn__uint_toLong(ptr) ((long long)(*(ptr)))
#define __sn__uint_toDouble(ptr) ((double)(*(ptr)))

/* byte methods */
#define __sn__byte_toInt(ptr) ((long long)(*(ptr)))
#define __sn__byte_toChar(ptr) ((char)(*(ptr)))

/* bool methods */
#define __sn__bool_toInt(ptr) ((long long)(*(ptr)))

/* ---- Arithmetic ---- */

/* String concat via + operator */
#define sn_add_string(a, b) sn_str_concat((a), (b))

/* Byte arithmetic (unsigned, wrapping) */
static inline unsigned char sn_add_byte(unsigned char a, unsigned char b) { return (unsigned char)(a + b); }
static inline unsigned char sn_sub_byte(unsigned char a, unsigned char b) { return (unsigned char)(a - b); }
static inline unsigned char sn_mul_byte(unsigned char a, unsigned char b) { return (unsigned char)(a * b); }

/* int32 arithmetic (wrapping) */
static inline int32_t sn_add_int32(int32_t a, int32_t b) { return a + b; }
static inline int32_t sn_sub_int32(int32_t a, int32_t b) { return a - b; }
static inline int32_t sn_mul_int32(int32_t a, int32_t b) { return a * b; }

/* uint arithmetic (wrapping) */
static inline uint64_t sn_add_uint(uint64_t a, uint64_t b) { return a + b; }
static inline uint64_t sn_sub_uint(uint64_t a, uint64_t b) { return a - b; }
static inline uint64_t sn_mul_uint(uint64_t a, uint64_t b) { return a * b; }

/* uint32 arithmetic (wrapping) */
static inline uint32_t sn_add_uint32(uint32_t a, uint32_t b) { return a + b; }
static inline uint32_t sn_sub_uint32(uint32_t a, uint32_t b) { return a - b; }
static inline uint32_t sn_mul_uint32(uint32_t a, uint32_t b) { return a * b; }

/* Float arithmetic */
static inline float sn_add_float(float a, float b) { return a + b; }
static inline float sn_sub_float(float a, float b) { return a - b; }
static inline float sn_mul_float(float a, float b) { return a * b; }
static inline float sn_div_float(float a, float b) { return a / b; }

/* ---- I/O ---- */

static inline void sn_print(const char *s)
{
    if (s) fputs(s, stdout);
}

static inline void sn_println(const char *s)
{
    if (s) puts(s);
    else puts("");
}

/* ---- Assertions ---- */

static inline void sn_assert(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message ? message : "Assertion failed");
        exit(1);
    }
}

/* ---- Checked arithmetic (long long) ---- */

static inline long long sn_add_long(long long a, long long b)
{
    long long r;
    if (__builtin_add_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return r;
}

static inline long long sn_sub_long(long long a, long long b)
{
    long long r;
    if (__builtin_sub_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return r;
}

static inline long long sn_mul_long(long long a, long long b)
{
    long long r;
    if (__builtin_mul_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return r;
}

static inline long long sn_div_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    return a / b;
}

static inline long long sn_mod_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    return a % b;
}

static inline long long sn_neg_long(long long a) { return -a; }

/* Double arithmetic */
static inline double sn_add_double(double a, double b) { return a + b; }
static inline double sn_sub_double(double a, double b) { return a - b; }
static inline double sn_mul_double(double a, double b) { return a * b; }
static inline double sn_div_double(double a, double b) { return a / b; }
static inline double sn_neg_double(double a) { return -a; }

/* ---- Comparison helpers ---- */

static inline bool sn_eq_long(long long a, long long b) { return a == b; }
static inline bool sn_lt_long(long long a, long long b) { return a < b; }
static inline bool sn_gt_long(long long a, long long b) { return a > b; }
static inline bool sn_le_long(long long a, long long b) { return a <= b; }
static inline bool sn_ge_long(long long a, long long b) { return a >= b; }

static inline bool sn_eq_double(double a, double b) { return a == b; }
static inline bool sn_lt_double(double a, double b) { return a < b; }
static inline bool sn_gt_double(double a, double b) { return a > b; }
static inline bool sn_le_double(double a, double b) { return a <= b; }
static inline bool sn_ge_double(double a, double b) { return a >= b; }

static inline bool sn_eq_float(float a, float b) { return a == b; }
static inline bool sn_lt_float(float a, float b) { return a < b; }
static inline bool sn_gt_float(float a, float b) { return a > b; }
static inline bool sn_le_float(float a, float b) { return a <= b; }
static inline bool sn_ge_float(float a, float b) { return a >= b; }

static inline bool sn_lt_char(char a, char b) { return a < b; }
static inline bool sn_gt_char(char a, char b) { return a > b; }
static inline bool sn_lt_byte(unsigned char a, unsigned char b) { return a < b; }
static inline bool sn_gt_byte(unsigned char a, unsigned char b) { return a > b; }
static inline bool sn_lt_int32(int a, int b) { return a < b; }
static inline bool sn_gt_int32(int a, int b) { return a > b; }
static inline bool sn_lt_uint(unsigned long long a, unsigned long long b) { return a < b; }
static inline bool sn_gt_uint(unsigned long long a, unsigned long long b) { return a > b; }
static inline bool sn_lt_uint32(unsigned int a, unsigned int b) { return a < b; }
static inline bool sn_gt_uint32(unsigned int a, unsigned int b) { return a > b; }

/* ---- Exit ---- */

static inline void sn_exit(int code)
{
    exit(code);
}

#endif
