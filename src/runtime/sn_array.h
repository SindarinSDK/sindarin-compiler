#ifndef SN_ARRAY_H
#define SN_ARRAY_H

#include "sn_core.h"

/* ---- Element type tags ---- */

enum SnElemTag { SN_TAG_DEFAULT = 0, SN_TAG_INT, SN_TAG_DOUBLE, SN_TAG_STRING, SN_TAG_BOOL, SN_TAG_CHAR, SN_TAG_BYTE, SN_TAG_STRUCT, SN_TAG_ARRAY };

/* ---- Dynamic array ---- */

typedef struct {
    void *data;
    long long len;
    long long cap;
    size_t elem_size;
    void (*elem_release)(void *);                    /* per-element cleanup (NULL = no-op) */
    void (*elem_copy)(const void *src, void *dst);   /* per-element copy (NULL = memcpy) */
    enum SnElemTag elem_tag;                         /* element type tag for join/toString */
} SnArray;

/* ---- Array cleanup ---- */

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

/* ---- String copy helper (for array elem_copy) ---- */

static inline void sn_copy_str(const void *src, void *dst)
{
    const char *s = *(const char **)src;
    *(char **)dst = s ? strdup(s) : NULL;
}

/* ---- Element copy helper (eliminates repeated if-elem_copy-else-memcpy) ---- */

static inline void sn_array_copy_elems(
    void *dst, long long dst_offset,
    const void *src, long long src_offset,
    long long count, size_t elem_size,
    void (*elem_copy)(const void *, void *))
{
    if (elem_copy) {
        for (long long i = 0; i < count; i++) {
            const void *s = (const char *)src + (src_offset + i) * (long long)elem_size;
            void *d = (char *)dst + (dst_offset + i) * (long long)elem_size;
            elem_copy(s, d);
        }
    } else {
        memcpy((char *)dst + dst_offset * (long long)elem_size,
               (const char *)src + src_offset * (long long)elem_size,
               (size_t)count * elem_size);
    }
}

/* ---- Core array operations (declared, defined in sn_array.c) ---- */

SnArray *sn_array_new(size_t elem_size, long long initial_cap);
void sn_array_push(SnArray *arr, const void *elem);
void sn_array_push_safe(SnArray *arr, const void *elem, size_t value_size);
void *sn_array_get(SnArray *arr, long long index);
long long sn_array_length(SnArray *arr);
SnArray *sn_array_range(long long start, long long end);
SnArray *sn_array_copy(const SnArray *src);
SnArray *sn_array_slice(const SnArray *arr, long long start, long long end);
SnArray *sn_array_concat(const SnArray *a, const SnArray *b);
void sn_array_extend(SnArray *dst, const SnArray *src);

/* ---- Byte array to string (needed by __sn___toString) ---- */

static inline char *sn_byte_array_to_string(const SnArray *arr) {
    if (!arr || arr->len == 0) return strdup("");
    char *s = sn_malloc((size_t)arr->len + 1);
    memcpy(s, arr->data, (size_t)arr->len);
    s[arr->len] = '\0';
    return s;
}

/* ---- Create a byte array from raw pointer memory: ptr[start..end] ---- */

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
    return memcmp(a->data, b->data, (size_t)a->len * a->elem_size) == 0;
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

/* ---- Array accessors (generic + typed) ---- */

#define sn_array_get_as(arr, index, T) (((T *)(arr)->data)[(index)])

static inline long long      sn_array_get_long(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, long long); }
static inline double          sn_array_get_double(SnArray *arr, long long i)  { return sn_array_get_as(arr, i, double); }
static inline char *          sn_array_get_string(SnArray *arr, long long i)  { return sn_array_get_as(arr, i, char *); }
static inline bool            sn_array_get_bool(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, bool); }
static inline char            sn_array_get_char(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, char); }
static inline unsigned char   sn_array_get_byte(SnArray *arr, long long i)    { return sn_array_get_as(arr, i, unsigned char); }
static inline void *          sn_array_get_generic(SnArray *arr, long long i) { return sn_array_get(arr, i); }

/* ---- Array built-in methods ---- */

/* push: arr.push(value)
 * Uses sizeof(value) for the copy to handle cases where the pushed value
 * is smaller than elem_size (e.g., thread handles pushed into struct arrays).
 * Zero-fills the slot first to avoid reading uninitialized memory. */
#define __sn___push(arr_ptr, ...) do { \
    __typeof__(__VA_ARGS__) __sn_push_tmp__ = (__VA_ARGS__); \
    sn_array_push_safe(*(arr_ptr), &__sn_push_tmp__, sizeof(__sn_push_tmp__)); \
} while(0)

/* pop: returns void* to the popped element */
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

/* reverse: arr.reverse() — uses fixed stack buffer, heap fallback for large elements */
static inline void sn_array_reverse(SnArray *arr)
{
    if (!arr || arr->len < 2) return;
    char stack_buf[256];
    char *tmp = arr->elem_size <= sizeof(stack_buf) ? stack_buf : (char *)sn_malloc(arr->elem_size);
    for (long long lo = 0, hi = arr->len - 1; lo < hi; lo++, hi--) {
        void *a = (char *)arr->data + (size_t)lo * arr->elem_size;
        void *b = (char *)arr->data + (size_t)hi * arr->elem_size;
        memcpy(tmp, a, arr->elem_size);
        memcpy(a, b, arr->elem_size);
        memcpy(b, tmp, arr->elem_size);
    }
    if (tmp != stack_buf) free(tmp);
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

/* insert: arr.insert(value, index) */
static inline void sn_array_insert(SnArray *arr, const void *elem, long long index)
{
    if (!arr || index < 0 || index > arr->len) return;
    if (arr->len >= arr->cap) {
        arr->cap *= 2;
        arr->data = sn_realloc(arr->data, arr->elem_size * (size_t)arr->cap);
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
#define __sn___concat(arr_ptr, other) sn_array_concat(*(arr_ptr), (other))

/* extend: append all elements of src into dst (for spread operator) — declared above */

/* ---- Array method aliases with __sn__arr_ prefix ---- */

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

/* ---- Array contains / indexOf ---- */

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

/* indexOf: arr.indexOf(value) */
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

/* ---- Array join / toString (declared, defined in sn_array.c) ---- */

char *sn_array_join(const SnArray *arr, const char *sep);
char *sn_array_to_string(const SnArray *arr);

#define __sn___join(arr_ptr, sep) sn_array_join(*(arr_ptr), (sep))

/* toString: arr.toString() — byte arrays return UTF-8 string, others return "[elem, elem]" */
#define __sn___toString(arr_ptr) ({ SnArray *__ta__ = *(arr_ptr); \
    __ta__->elem_tag == SN_TAG_BYTE ? sn_byte_array_to_string(__ta__) : \
    ({ char *__j__ = sn_array_join(__ta__, ", "); \
    size_t __jl__ = __j__ ? strlen(__j__) : 0; \
    char *__s__ = sn_malloc(__jl__ + 3); \
    __s__[0] = '['; if (__j__) memcpy(__s__ + 1, __j__, __jl__); \
    __s__[__jl__ + 1] = ']'; __s__[__jl__ + 2] = '\0'; free(__j__); __s__; }); })

#endif
