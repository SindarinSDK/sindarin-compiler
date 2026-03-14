#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Box (as ref — refcounted) */
typedef struct {
    int __rc__;
    long long __sn__value;
} __sn__Box;

static inline __sn__Box *__sn__Box_create(void) {
    __sn__Box *p = calloc(1, sizeof(__sn__Box));
    p->__rc__ = 1;
    return p;
}

static inline __sn__Box *__sn__Box_retain(__sn__Box *p) {
    if (p) p->__rc__++;
    return p;
}

static inline void __sn__Box_release(__sn__Box **p) {
    if (*p && --(*p)->__rc__ == 0) {
        free(*p);
    }
    *p = NULL;
}

static inline __sn__Box *__sn__Box_copy(const __sn__Box *src) {
    __sn__Box *dst = calloc(1, sizeof(__sn__Box));
    dst->__rc__ = 1;
    dst->__sn__value = src->__sn__value;
    return dst;
}

#define sn_auto_Box __attribute__((cleanup(__sn__Box_release)))

static inline void __sn__Box_release_elem(void *p) { __sn__Box_release((__sn__Box **)p); }
static inline void __sn__Box_retain_into(const void *src, void *dst) { *(__sn__Box **)dst = __sn__Box_retain(*(__sn__Box *const *)src); }

/* Auto-toString for string interpolation */
static inline char *__sn__Box_to_string(const __sn__Box *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Box { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



__sn__Box * __sn__make_box(long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


__sn__Box * __sn__make_box(long long __sn__v) {

    sn_auto_Box __sn__Box * __sn__b = ({
        __sn__Box *__tmp__ = __sn__Box_create();
        __tmp__->__sn__value = __sn__v;
        __tmp__;
    });

    {
        __sn__Box * __ret__ = __sn__b;
        __sn__b = NULL;
        return __ret__;
    }}


int main() {
    sn_auto_Box __sn__Box * __sn__b = __sn__make_box(42LL);
    sn_assert((__sn__b->__sn__value == 42LL), "box value should be 42");
    fflush(stdout);
    return 0;
}
